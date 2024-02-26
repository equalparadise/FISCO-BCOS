#pragma once

#include "RollbackableStorage.h"
#include "bcos-framework/protocol/BlockHeader.h"
#include "bcos-framework/protocol/TransactionReceiptFactory.h"
#include "bcos-framework/storage2/MemoryStorage.h"
#include "bcos-framework/storage2/Storage.h"
#include "bcos-framework/transaction-executor/TransactionExecutor.h"
#include "bcos-task/Generator.h"
#include "bcos-task/Trait.h"
#include "bcos-transaction-executor/vm/VMFactory.h"
#include "bcos-utilities/DataConvertUtility.h"
#include "precompiled/PrecompiledManager.h"
#include "vm/HostContext.h"
#include "vm/VMInstance.h"
#include <evmc/evmc.h>
#include <boost/algorithm/hex.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <gsl/util>
#include <iterator>
#include <type_traits>

namespace bcos::transaction_executor
{
#define TRANSACTION_EXECUTOR_LOG(LEVEL) BCOS_LOG(LEVEL) << LOG_BADGE("TRANSACTION_EXECUTOR")

struct InvalidArgumentsError : public bcos::Error
{
};
class TransactionExecutorImpl
{
public:
    TransactionExecutorImpl(
        protocol::TransactionReceiptFactory const& receiptFactory, crypto::Hash::Ptr hashImpl)
      : m_receiptFactory(receiptFactory),
        m_hashImpl(std::move(hashImpl)),
        m_precompiledManager(m_hashImpl)
    {}
    constexpr static bool defaultRetryFlag = false;

private:
    protocol::TransactionReceiptFactory const& m_receiptFactory;
    crypto::Hash::Ptr m_hashImpl;
    PrecompiledManager m_precompiledManager;

    static evmc_message newEVMCMessage(protocol::Transaction const& transaction, int64_t gasLimit)
    {
        auto toAddress = unhexAddress(transaction.to());
        evmc_message message = {.kind = transaction.to().empty() ? EVMC_CREATE : EVMC_CALL,
            .flags = 0,
            .depth = 0,
            .gas = gasLimit,
            .recipient = toAddress,
            .destination_ptr = nullptr,
            .destination_len = 0,
            .sender = (!transaction.sender().empty() &&
                          transaction.sender().size() == sizeof(evmc_address)) ?
                          *(evmc_address*)transaction.sender().data() :
                          evmc_address{},
            .sender_ptr = nullptr,
            .sender_len = 0,
            .input_data = transaction.input().data(),
            .input_size = transaction.input().size(),
            .value = {},
            .create2_salt = {},
            .code_address = toAddress};

        return message;
    }

    friend task::Generator<protocol::TransactionReceipt::Ptr> tag_invoke(
        tag_t<execute3Step> /*unused*/, TransactionExecutorImpl& executor, auto& storage,
        protocol::BlockHeader const& blockHeader, protocol::Transaction const& transaction,
        int contextID, ledger::LedgerConfig const& ledgerConfig, auto&& waitOperator)
    {
        protocol::TransactionReceipt::Ptr receipt;
        try
        {
            if (c_fileLogLevel <= LogLevel::TRACE)
            {
                TRANSACTION_EXECUTOR_LOG(TRACE)
                    << "Execte transaction: " << toHex(transaction.hash());
            }

            Rollbackable<std::decay_t<decltype(storage)>> rollbackableStorage(storage);
            auto gasLimit = static_cast<int64_t>(std::get<0>(ledgerConfig.gasLimit()));
            auto evmcMessage = newEVMCMessage(transaction, gasLimit);

            if (blockHeader.number() == 0 &&
                transaction.to() == precompiled::AUTH_COMMITTEE_ADDRESS)
            {
                evmcMessage.kind = EVMC_CREATE;
            }

            int64_t seq = 0;
            HostContext<decltype(rollbackableStorage)> hostContext(rollbackableStorage, blockHeader,
                evmcMessage, evmcMessage.sender, transaction.abi(), contextID, seq,
                executor.m_precompiledManager, ledgerConfig, *executor.m_hashImpl,
                std::forward<decltype(waitOperator)>(waitOperator));

            waitOperator(hostContext.prepare());
            co_yield receipt;  // 完成第一步 Complete the first step

            auto evmcResult = waitOperator(hostContext.execute());
            co_yield receipt;  // 完成第二步 Complete the second step

            bcos::bytesConstRef output;
            std::string newContractAddress;
            if (evmcMessage.kind == EVMC_CREATE)
            {
                newContractAddress.reserve(sizeof(evmcResult.create_address) * 2);
                boost::algorithm::hex_lower(evmcResult.create_address.bytes,
                    evmcResult.create_address.bytes + sizeof(evmcResult.create_address.bytes),
                    std::back_inserter(newContractAddress));
            }
            else
            {
                output = {evmcResult.output_data, evmcResult.output_size};
            }

            if (evmcResult.status_code != 0)
            {
                TRANSACTION_EXECUTOR_LOG(DEBUG) << "Transaction revert: " << evmcResult.status_code;
            }

            auto const& logEntries = hostContext.logs();
            switch (static_cast<bcos::protocol::TransactionVersion>(transaction.version()))
            {
            case bcos::protocol::TransactionVersion::V0_VERSION:
                receipt = executor.m_receiptFactory.createReceipt(gasLimit - evmcResult.gas_left,
                    newContractAddress, logEntries, evmcResult.status_code, output,
                    blockHeader.number());
                break;
            case bcos::protocol::TransactionVersion::V1_VERSION:
                receipt = executor.m_receiptFactory.createReceipt2(gasLimit - evmcResult.gas_left,
                    newContractAddress, logEntries, evmcResult.status_code, output,
                    blockHeader.number());
                break;
            default:
                BOOST_THROW_EXCEPTION(std::runtime_error(
                    "Invalid receipt version: " + std::to_string(transaction.version())));
            }
        }
        catch (NotFoundCodeError& e)
        {
            TRANSACTION_EXECUTOR_LOG(DEBUG)
                << "Not found code exception: " << boost::diagnostic_information(e);

            receipt = executor.m_receiptFactory.createReceipt(
                0, {}, {}, EVMC_REVERT, {}, blockHeader.number());
            receipt->setMessage(boost::diagnostic_information(e));
        }
        catch (std::exception& e)
        {
            TRANSACTION_EXECUTOR_LOG(DEBUG)
                << "Execute exception: " << boost::diagnostic_information(e);

            receipt = executor.m_receiptFactory.createReceipt(
                0, {}, {}, EVMC_INTERNAL_ERROR, {}, blockHeader.number());
            receipt->setMessage(boost::diagnostic_information(e));
        }

        if (c_fileLogLevel <= LogLevel::TRACE)
        {
            TRANSACTION_EXECUTOR_LOG(TRACE)
                << "Execte transaction finished: " << receipt->version() << " | "
                << receipt->status() << " | [" << receipt->message() << "] | "
                << receipt->logEntries().size() << " | " << receipt->gasUsed();
        }

        co_yield receipt;  // 完成第三步 Complete the third step
    }

    friend task::Task<protocol::TransactionReceipt::Ptr> tag_invoke(
        tag_t<executeTransaction> /*unused*/, TransactionExecutorImpl& executor, auto& storage,
        protocol::BlockHeader const& blockHeader, protocol::Transaction const& transaction,
        int contextID, ledger::LedgerConfig const& ledgerConfig, auto&& waitOperator)
    {
        for (auto receipt : execute3Step(executor, storage, blockHeader, transaction, contextID,
                 ledgerConfig, std::forward<decltype(waitOperator)>(waitOperator)))
        {
            if (receipt)
            {
                co_return receipt;
            }
        }
        co_return protocol::TransactionReceipt::Ptr{};
    }
};

}  // namespace bcos::transaction_executor
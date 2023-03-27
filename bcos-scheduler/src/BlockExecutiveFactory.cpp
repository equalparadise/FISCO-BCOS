/*
 *  Copyright (C) 2022 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief The block executive(context) for serial transaction execution
 * @file SerialBlockExecutive.h
 * @author: wenlinli
 * @date: 2022-07-13
 */

#include "BlockExecutiveFactory.h"
#include "BlockExecutive.h"
#include "SerialBlockExecutive.h"
#include "ShardingBlockExecutive.h"


using namespace std;
using namespace bcos::protocol;
using namespace bcos::scheduler;


std::shared_ptr<BlockExecutive> BlockExecutiveFactory::build(bcos::protocol::Block::Ptr block,
    SchedulerImpl* scheduler, size_t startContextID,
    bcos::protocol::TransactionSubmitResultFactory::Ptr transactionSubmitResultFactory,
    bool staticCall, bcos::protocol::BlockFactory::Ptr _blockFactory,
    bcos::txpool::TxPoolInterface::Ptr _txPool)
{
    if (m_isSerialExecute)
    {
        auto serialBlockExecutive = std::make_shared<SerialBlockExecutive>(block, scheduler,
            startContextID, transactionSubmitResultFactory, staticCall, _blockFactory, _txPool);
        return serialBlockExecutive;
    }
    else
    {
        auto blockExecutive = std::make_shared<BlockExecutive>(block, scheduler, startContextID,
            transactionSubmitResultFactory, staticCall, _blockFactory, _txPool);
        return blockExecutive;
    }
}

std::shared_ptr<BlockExecutive> BlockExecutiveFactory::build(bcos::protocol::Block::Ptr block,
    SchedulerImpl* scheduler, size_t startContextID,
    bcos::protocol::TransactionSubmitResultFactory::Ptr transactionSubmitResultFactory,
    bool staticCall, bcos::protocol::BlockFactory::Ptr _blockFactory,
    bcos::txpool::TxPoolInterface::Ptr _txPool, uint64_t _gasLimit, bool _syncBlock,
    bool _isTempForCall)
{
    if (block->blockHeaderConst()->version() >= (uint32_t)BlockVersion::V3_3_VERSION)
    {
        auto shardCache = decltype(m_contract2ShardCache)();
        if (_isTempForCall)
        {
            auto tempCache = *m_contract2ShardCache;  // Copy the cache for temp use
            shardCache = std::make_shared<decltype(tempCache)>(std::move(tempCache));
        }
        else
        {
            shardCache = m_contract2ShardCache;
        }

        // In 3.3.0, DMC and serial has been combined together: Sharding
        auto shardingBlockExecutive = std::make_shared<ShardingBlockExecutive>(block, scheduler,
            startContextID, transactionSubmitResultFactory, staticCall, _blockFactory, _txPool,
            shardCache, _gasLimit, _syncBlock, m_keyPageSize);
        return shardingBlockExecutive;
    }

    if (m_isSerialExecute)
    {
        auto serialBlockExecutive = std::make_shared<SerialBlockExecutive>(block, scheduler,
            startContextID, transactionSubmitResultFactory, staticCall, _blockFactory, _txPool,
            _gasLimit, _syncBlock);
        return serialBlockExecutive;
    }
    else
    {
        auto blockExecutive = std::make_shared<BlockExecutive>(block, scheduler, startContextID,
            transactionSubmitResultFactory, staticCall, _blockFactory, _txPool, _gasLimit,
            _syncBlock);
        return blockExecutive;
    }
}
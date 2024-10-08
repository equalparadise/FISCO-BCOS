/**
 *  Copyright (C) 2021 FISCO BCOS.
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
 * @brief factory to create BlockSync
 * @file BlockSyncFactory.h
 * @author: yujiechen
 * @date 2021-05-28
 */
#pragma once
#include "bcos-sync/BlockSync.h"
#include "bcos-sync/BlockSyncConfig.h"
#include "bcos-tool/NodeTimeMaintenance.h"

namespace bcos::sync
{
class BlockSyncFactory
{
public:
    using Ptr = std::shared_ptr<BlockSyncFactory>;
    BlockSyncFactory(bcos::crypto::PublicPtr _nodeId,
        bcos::protocol::BlockFactory::Ptr _blockFactory,
        bcos::protocol::TransactionSubmitResultFactory::Ptr _txResultFactory,
        bcos::ledger::LedgerInterface::Ptr _ledger, bcos::txpool::TxPoolInterface::Ptr _txpool,
        bcos::front::FrontServiceInterface::Ptr _frontService,
        bcos::scheduler::SchedulerInterface::Ptr _scheduler,
        bcos::consensus::ConsensusInterface::Ptr _consensus,
        bcos::tool::NodeTimeMaintenance::Ptr _nodeTimeMaintenance,
        bool enableSendBlockStatusByTree = false, std::uint32_t syncTreeWidth = 3,
        bool _syncArchivedBlockBody = false);
    virtual ~BlockSyncFactory() = default;

    virtual BlockSync::Ptr createBlockSync();

private:
    bcos::crypto::PublicPtr m_nodeId;
    bcos::protocol::BlockFactory::Ptr m_blockFactory;
    bcos::protocol::TransactionSubmitResultFactory::Ptr m_txResultFactory;
    bcos::ledger::LedgerInterface::Ptr m_ledger;
    bcos::txpool::TxPoolInterface::Ptr m_txpool;
    bcos::front::FrontServiceInterface::Ptr m_frontService;
    bcos::scheduler::SchedulerInterface::Ptr m_scheduler;
    bcos::consensus::ConsensusInterface::Ptr m_consensus;
    bcos::tool::NodeTimeMaintenance::Ptr m_nodeTimeMaintenance;
    bool m_enableSendBlockStatusByTree;
    std::int64_t m_syncTreeWidth;
    bool m_syncArchivedBlockBody = false;
};
}  // namespace bcos::sync
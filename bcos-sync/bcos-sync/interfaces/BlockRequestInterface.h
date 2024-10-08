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
 * @brief interfaces for block request packet
 * @file BlockRequestInterface.h
 * @author: yujiechen
 * @date 2021-05-24
 */

#pragma once
#include "bcos-sync/interfaces/BlockSyncMsgInterface.h"
namespace bcos
{
namespace sync
{
class BlockRequestInterface : virtual public BlockSyncMsgInterface
{
public:
    using Ptr = std::shared_ptr<BlockRequestInterface>;
    BlockRequestInterface() = default;
    virtual ~BlockRequestInterface() {}
    virtual int32_t blockDataFlag() const = 0;
    virtual void setBlockDataFlag(int32_t _flag) = 0;

    virtual size_t size() const = 0;
    virtual void setSize(size_t _size) = 0;
};
}  // namespace sync
}  // namespace bcos
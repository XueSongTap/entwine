/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/
/*
这段代码是一个命名空间 `entwine::heuristics` 中的常量定义。

以下是这些常量的解释：

- `sleepCount`：表示每个线程处理的点数达到多少时进行剪裁操作。剪裁操作包括对过去两个 `sleepCount` 窗口内未使用的块进行引用计数减少，从而触发它们的序列化。该常量的值为 `65536 * 32`。
- `cacheSize`：表示在块缓存中保持活跃的未引用块的数量。该常量的值为 `64`。
- `defaultWorkToClipRatio`：在构建过程中，给定了总线程数。由于序列化比实际进行树操作更昂贵，我们将分配更多的线程用于 "clip" 任务而不是 "work" 任务。该参数用于调整工作线程与剪裁线程的比例，默认值为 `0.33f`。
- `maxHierarchyNodesPerFile`：单个层次结构文件中存储的最大节点数。该常量的值为 `32768`。

这些常量提供了一些启发式参数，用于调整剪裁和缓存策略的行为。
*/
#pragma once

#include <cstdint>

namespace entwine
{
namespace heuristics
{

// After this many points (per thread), we'll clip - which involves reference-
// decrementing the chunks that haven't been used in the past two sleepCount
// windows, which will trigger their serialization.
const uint64_t sleepCount(65536 * 32);

// How many unreferenced chunks to keep alive in our chunk cache.
const uint64_t cacheSize(64);

// When building, we are given a total thread count.  Because serialization is
// more expensive than actually doing tree work, we'll allocate more threads to
// the "clip" task than to the "work" task.  This parameter tunes the ratio of
// work threads to clip threads.
const float defaultWorkToClipRatio(0.33f);

// Max number of nodes to store in a single hierarchy file.
const uint64_t maxHierarchyNodesPerFile(32768);

} // namespace heuristics
} // namespace entwine


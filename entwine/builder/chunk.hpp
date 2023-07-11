/******************************************************************************
* Copyright (c) 2019, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#pragma once

#include <cassert>
#include <cstddef>
#include <utility>

#include <entwine/builder/hierarchy.hpp>
#include <entwine/builder/overflow.hpp>
#include <entwine/third/arbiter/arbiter.hpp>
#include <entwine/types/endpoints.hpp>
#include <entwine/types/vector-point-table.hpp>
#include <entwine/util/spin-lock.hpp>

namespace entwine
{

class ChunkCache;
class Clipper;

struct VoxelTube
{
    SpinLock spin;
    std::map<uint32_t, Voxel> map;
};
/*
这是一个名为 `Chunk` 的类的定义。该类表示一个块，并提供插入、保存和加载等功能。

以下是该类的解释：

- `Chunk` 类具有以下公共成员函数和私有成员变量：
  - 构造函数 `Chunk`：接受元数据对象 `m`、块键对象 `ck` 和层级对象 `hierarchy`，并初始化成员变量。
  - `insert` 函数：将给定的 `Voxel` 对象插入到块中。如果块已满，则执行溢出操作。接受 `ChunkCache` 对象 `cache`、剪裁器对象 `clipper`、`Voxel` 对象 `voxel` 和键对象 `key`。
  - `save` 函数：将块保存到指定的终端点。接受终端点对象 `endpoints`，并返回保存的字节数。
  - `load` 函数：从指定的终端点加载块。接受 `ChunkCache` 对象 `cache`、剪裁器对象 `clipper`、终端点对象 `endpoints` 和要加载的块编号 `np`。
  - `chunkKey` 函数：返回块键对象的引用。
  - `childAt` 函数：返回指定方向的子块键对象的引用。接受方向枚举值 `dir`。
  - `spin` 函数：返回一个 `SpinLock` 对象的引用，用于实现并发访问的互斥锁。

- `Chunk` 类具有以下私有成员函数：
  - `insertOverflow` 函数：将给定的 `Voxel` 对象插入到溢出块中。接受 `ChunkCache` 对象 `cache`、剪裁器对象 `clipper`、`Voxel` 对象 `voxel` 和键对象 `key`。返回布尔值，表示是否成功插入。
  - `maybeOverflow` 函数：根据需要执行溢出操作。接受 `ChunkCache` 对象 `cache` 和剪裁器对象 `clipper`。
  - `doOverflow` 函数：执行指定方向的溢出操作。接受 `ChunkCache` 对象 `cache`、剪裁器对象 `clipper` 和方向值 `dir`。

- `Chunk` 类具有以下私有成员变量：
  - `m_metadata`：元数据对象的引用。
  - `m_span`：块的跨度。
  - `m_pointSize`：点的大小。
  - `m_chunkKey`：块键对象。
  - `m_childKeys`：子块键对象的数组。
  - `m_spin`：`SpinLock` 对象，用于实现并发访问的互斥锁。
  - `m_grid`：`VoxelTube` 对象的向量，表示块中的网格。
  - `m_gridBlock`：内存块对象，用于存储网格数据。
  - `m_overflowSpin`：`SpinLock` 对象，用于实现并发访问的互斥锁。
  - `m_overflows`：溢出对象的唯一指针数组。
  - `m_overflowCount`：溢出计数器，表示溢出对象的数量。
*/
class Chunk
{
public:
    Chunk(const Metadata& m, const ChunkKey& ck, const Hierarchy& hierarchy);

    bool insert(ChunkCache& cache, Clipper& clipper, Voxel& voxel, Key& key);
    uint64_t save(const Endpoints& endpoints) const;
    void load(
        ChunkCache& cache,
        Clipper& clipper,
        const Endpoints& endpoints,
        uint64_t np);

    const ChunkKey& chunkKey() const { return m_chunkKey; }
    const ChunkKey& childAt(Dir dir) const
    {
        return m_childKeys[toIntegral(dir)];
    }

    SpinLock& spin() { return m_spin; }

private:
    bool insertOverflow(
        ChunkCache& cache,
        Clipper& clipper,
        Voxel& voxel,
        Key& key);

    void maybeOverflow(ChunkCache& cache, Clipper& clipper);
    void doOverflow(ChunkCache& cache, Clipper& clipper, uint64_t dir);

    const Metadata& m_metadata;
    const uint64_t m_span;
    const uint64_t m_pointSize;
    const ChunkKey m_chunkKey;
    const std::array<ChunkKey, 8> m_childKeys;

    SpinLock m_spin;
    std::vector<VoxelTube> m_grid;
    MemBlock m_gridBlock;

    SpinLock m_overflowSpin;
    std::array<std::unique_ptr<Overflow>, 8> m_overflows;
    uint64_t m_overflowCount = 0;
};

} // namespace entwine


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

#include <array>

#include <entwine/builder/chunk.hpp>
#include <entwine/builder/hierarchy.hpp>
#include <entwine/types/defs.hpp>
#include <entwine/util/pool.hpp>
#include <entwine/util/spin-lock.hpp>

namespace entwine
{

class Clipper;
/*
这是一个名为 `ReffedChunk` 的类的定义。该类用于管理带有引用计数的数据块。

以下是该类的解释：

- `ReffedChunk` 类具有以下成员函数和成员变量：
  - `ReffedChunk` 构造函数：接受元数据对象 `m`、块键对象 `ck` 和层级对象 `h`，并使用它们创建一个 `Chunk` 对象，并将其存储在 `m_chunk` 成员变量中。
  - `spin` 函数：返回一个 `SpinLock` 对象的引用，用于实现并发访问的互斥锁。
  - `add` 函数：增加引用计数。
  - `del` 函数：减少引用计数，并返回计数器的值。如果计数器为零，则抛出运行时错误。
  - `count` 函数：返回当前引用计数的值。
  - `chunk` 函数：返回存储的 `Chunk` 对象的引用。如果对象不存在，则抛出运行时错误。
  - `reset` 函数：重置存储的 `Chunk` 对象。
  - `exists` 函数：检查存储的 `Chunk` 对象是否存在。
  - `assign` 函数：接受元数据对象 `m`、块键对象 `ck` 和层级对象 `h`，并使用它们创建一个新的 `Chunk` 对象，并将其存储在 `m_chunk` 成员变量中。在调用之前，必须确保存储的 `Chunk` 对象不存在。

- `ReffedChunk` 类具有以下成员变量：
  - `m_spin`：`SpinLock` 对象，用于实现并发访问的互斥锁。
  - `m_refs`：引用计数器，用于跟踪对 `Chunk` 对象的引用数。
  - `m_chunk`：存储的 `Chunk` 对象的指针。
*/
class ReffedChunk
{
public:
    ReffedChunk(const Metadata& m, const ChunkKey& ck, const Hierarchy& h)
        : m_chunk(makeUnique<Chunk>(m, ck, h))
    { }

    SpinLock& spin() { return m_spin; }

    void add() { ++m_refs; }
    uint64_t del()
    {
        if (!m_refs) throw std::runtime_error("Negative");
        return --m_refs;
    }
    uint64_t count() const { return m_refs; }

    Chunk& chunk()
    {
        if (!m_chunk) throw std::runtime_error("Missing chunk");
        return *m_chunk;
    }

    void reset() { m_chunk.reset(); }
    bool exists() { return !!m_chunk; }
    void assign(const Metadata& m, const ChunkKey& ck, const Hierarchy& h)
    {
        assert(!exists());
        m_chunk = makeUnique<Chunk>(m, ck, h);
    }

private:
    SpinLock m_spin;
    uint64_t m_refs = 0;
    std::unique_ptr<Chunk> m_chunk;
};
/*
这是一个名为 `ChunkCache` 的类的定义。该类用于管理块缓存，并提供插入、剪裁和清理等功能。

以下是该类的解释：

- `ChunkCache` 类具有以下公共成员函数和私有成员变量：
  - 构造函数 `ChunkCache`：接受终端点对象 `endpoints`、元数据对象 `Metadata`、层级对象 `hierarchy` 和线程数 `threads`，并初始化成员变量。
  - 析构函数 `~ChunkCache`：销毁 `ChunkCache` 对象。
  - `insert` 函数：将给定的 `Voxel` 对象插入到缓存中，并执行剪裁操作。接受 `Voxel` 对象、键对象 `key`、块键对象 `ck` 和剪裁器对象 `clipper`。
  - `clip` 函数：执行指定深度的剪裁操作，并传入已过期的块的映射。接受深度值 `depth` 和已过期块的映射 `stale`。
  - `clipped` 函数：执行可能的清理操作，以确保缓存的大小不超过指定的最大缓存大小。
  - `join` 函数：等待所有线程完成。
  - `latchInfo` 函数：静态函数，返回缓存的信息结构体 `Info`，包括已写入的数量、已读取的数量和当前存活的数量。

- `ChunkCache` 类具有以下私有成员函数：
  - `addRef` 函数：增加指定块键的引用，并返回对应的 `Chunk` 对象的引用。接受块键对象 `ck` 和剪裁器对象 `clipper`。
  - `maybeSerialize` 函数：根据给定的块键位置 `dxyz`，可能将块序列化到磁盘。
  - `maybeErase` 函数：根据给定的块键位置 `dxyz`，可能从缓存中删除块。
  - `maybePurge` 函数：根据给定的最大缓存大小 `maxCacheSize`，可能清理缓存中的块。

- `ChunkCache` 类具有以下私有成员变量：
  - `m_endpoints`：终端点对象的引用。
  - `m_metadata`：元数据对象的引用。
  - `m_hierarchy`：层级对象的引用。
  - `m_pool`：线程池对象。
  - `m_cacheSize`：缓存的最大大小。
  - `m_spins`：`SpinLock` 对象数组，用于实现并发访问的互斥锁。
  - `m_slices`：`ReffedChunk` 对象的 `Xyz` 键到 `ReffedChunk` 对象的映射的数组。
  - `m_ownedSpin`：`SpinLock` 对象，用于实现并发访问的互斥锁。
  - `m_owned`：`Dxyz` 对象的集合，表示已拥有的块的位置。
*/
class ChunkCache
{
public:
    ChunkCache(
        const Endpoints& endpoints,
        const Metadata& Metadata,
        Hierarchy& hierarchy,
        uint64_t threads);

    ~ChunkCache();

    void insert(Voxel& voxel, Key& key, const ChunkKey& ck, Clipper& clipper);
    void clip(uint64_t depth, const std::map<Xyz, Chunk*>& stale);
    void clipped() { maybePurge(m_cacheSize); }
    void join();

    struct Info
    {
        uint64_t written = 0;
        uint64_t read = 0;
        uint64_t alive = 0;
    };

    static Info latchInfo();

private:
    Chunk& addRef(const ChunkKey& ck, Clipper& clipper);
    void maybeSerialize(const Dxyz& dxyz);
    void maybeErase(const Dxyz& dxyz);
    void maybePurge(uint64_t maxCacheSize);

    const Endpoints& m_endpoints;
    const Metadata& m_metadata;
    Hierarchy& m_hierarchy;
    Pool m_pool;
    const uint64_t m_cacheSize = 64;

    std::array<SpinLock, maxDepth> m_spins;
    std::array<std::map<Xyz, ReffedChunk>, maxDepth> m_slices;

    SpinLock m_ownedSpin;
    std::set<Dxyz> m_owned;
};

} // namespace entwine


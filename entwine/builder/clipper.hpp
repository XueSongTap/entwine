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
#include <limits>

#include <entwine/types/key.hpp>

namespace entwine
{

class Chunk;
class ChunkCache;
/*
这是一个名为 `CachedChunk` 的结构体的定义。该结构体表示一个缓存的块。

以下是该结构体的解释：

- `CachedChunk` 结构体具有以下成员变量和构造函数：
  - 默认构造函数 `CachedChunk`：将 `xyz` 初始化为最大值。
  - 带参数的构造函数 `CachedChunk`：接受 `Xyz` 对象 `xyz`，并将其赋值给成员变量 `xyz`。
  - `xyz`：表示块的位置的 `Xyz` 对象。
  - `chunk`：指向 `Chunk` 对象的指针，默认为 `nullptr`。

- `operator<` 函数：定义了 `CachedChunk` 结构体之间的小于比较运算符。根据 `xyz` 的比较结果，返回布尔值，表示是否 `a` 小于 `b`。
*/
struct CachedChunk
{
    CachedChunk()
        : xyz(
            std::numeric_limits<uint64_t>::max(),
            std::numeric_limits<uint64_t>::max(),
            std::numeric_limits<uint64_t>::max())
    { }

    CachedChunk(const Xyz& xyz) : xyz(xyz) { }

    Xyz xyz;
    Chunk* chunk = nullptr;
};

inline bool operator<(const CachedChunk& a, const CachedChunk& b)
{
    return a.xyz < b.xyz;
}
/*
这是一个名为 `Clipper` 的类的定义。该类用于剪裁（Clip）和管理块（Chunk）的缓存。

以下是该类的解释：

- `Clipper` 类具有以下公共成员函数和私有成员变量：
  - 构造函数 `Clipper`：接受一个 `ChunkCache` 对象 `cache`，并将其赋值给成员变量 `m_cache`。还初始化成员变量 `m_fast` 为填充有默认构造的 `CachedChunk` 对象的数组。
  - 析构函数 `~Clipper`：在对象销毁时执行清理操作。
  - `get` 函数：根据给定的块键 `ck`，从缓存中获取对应的块指针。返回 `Chunk*` 类型的指针。
  - `set` 函数：将给定的块指针 `chunk` 关联到指定的块键 `ck`。将块放入缓存中。
  - `clip` 函数：执行剪裁操作，清理过时的块并调整缓存大小。

- `Clipper` 类具有以下私有成员变量：
  - `m_cache`：对 `ChunkCache` 对象的引用。
  - `m_fast`：一个大小为 `maxDepth` 的 `CachedChunk` 对象数组，用于快速访问最近使用的块。
  - `m_slow`：一个大小为 `maxDepth` 的 `std::map<Xyz, Chunk*>` 对象数组，用于存储较慢访问的块。
  - `m_aged`：一个大小为 `maxDepth` 的 `std::map<Xyz, Chunk*>` 对象数组，用于存储过时的块。
*/
class Clipper
{
public:
    Clipper(ChunkCache& cache)
        : m_cache(cache)
    {
        m_fast.fill(CachedChunk());
    }

    ~Clipper();

    Chunk* get(const ChunkKey& ck);
    void set(const ChunkKey& ck, Chunk* chunk);
    void clip();

private:
    ChunkCache& m_cache;

    using UsedMap = std::map<Xyz, Chunk*>;
    using AgedSet = std::set<Xyz>;

    std::array<CachedChunk, maxDepth> m_fast;
    std::array<std::map<Xyz, Chunk*>, maxDepth> m_slow;
    std::array<std::map<Xyz, Chunk*>, maxDepth> m_aged;
};

} // namespace entwine


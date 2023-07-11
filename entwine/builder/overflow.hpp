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

#include <entwine/types/key.hpp>
#include <entwine/types/vector-point-table.hpp>
#include <entwine/types/voxel.hpp>

namespace entwine
{
/*
这段代码定义了命名空间 `entwine` 中的结构体 `Overflow`。

以下是结构体 `Overflow` 的解释：

- `Overflow` 结构体具有以下成员变量和构造函数：
  - `chunkKey`：表示块的键（ChunkKey），用于标识溢出块所属的块。
  - `pointSize`：表示点的大小。
  - `block`：表示内存块（MemBlock），用于存储溢出数据。
  - `list`：表示溢出项列表，其中每个项都包含一个键（Key）和一个体素（Voxel）。

- `Overflow` 结构体具有以下成员函数：
  - `insert` 函数：将给定的体素（Voxel）和键（Key）插入到溢出项列表中。该函数会创建一个新的溢出项（Entry），并将其添加到列表末尾。溢出项的体素数据通过 `block.next()` 获取，并使用给定的体素点、数据和点大小进行初始化。

`Overflow` 结构体用于表示溢出块的数据结构，其中溢出块是指无法存储在主块中的额外数据。它提供了将体素和键插入溢出项列表的功能，并使用内存块来存储溢出数据。
*/
struct Overflow
{
    struct Entry
    {
        Entry(const Key& key) : key(key) { }
        Voxel voxel;
        Key key;
    };

    Overflow(const ChunkKey& chunkKey, uint64_t pointSize)
        : chunkKey(chunkKey)
        , pointSize(pointSize)
        , block(pointSize, 256)
    { }

    void insert(Voxel& voxel, Key& key)
    {
        Entry entry(key);
        entry.voxel.setData(block.next());
        entry.voxel.initDeep(voxel.point(), voxel.data(), pointSize);
        list.push_back(entry);
    }

    const ChunkKey chunkKey;
    const uint64_t pointSize = 0;

    MemBlock block;
    std::vector<Entry> list;
};

} // namespace entwine


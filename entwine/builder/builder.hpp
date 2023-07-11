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
这段代码是C++代码，使用了entwine命名空间定义了一些与构建器（Builder）相关的结构和函数。

以下是代码中的一些关键部分的解释：



这段代码主要用于构建和操作 entwine 数据库的构建器对象，它提供了一些方法来运行构建器、保存数据和执行其他相关操作。要完整理解和使用这段代码，还需要查看相关的头文件和其他代码文件，以了解更多关于 entwine 构建器的实现细节。
*/
#pragma once

#include <atomic>
#include <string>

#include <entwine/builder/chunk-cache.hpp>
#include <entwine/builder/hierarchy.hpp>
#include <entwine/types/endpoints.hpp>
#include <entwine/types/metadata.hpp>
#include <entwine/types/source.hpp>
#include <entwine/types/threads.hpp>
#include <entwine/util/json.hpp>

namespace entwine
{
// Builder 结构体：定义了一个构建器，包含了构建器的各种属性和方法。
struct Builder
{
    // Builder 结构体的构造函数：接受一些参数，如 Endpoints、Metadata、Manifest 和 Hierarchy，用于初始化构建器的属性。
    Builder(
        Endpoints endpoints,
        Metadata metadata,
        Manifest manifest,
        Hierarchy hierarchy = Hierarchy(),
        bool verbose = true);
    // run 方法：运行构建器，接受一些参数，如线程数 Threads、限制数 limit 和进度间隔秒数 progressIntervalSeconds，并返回一个 uint64_t 值。
    uint64_t run(
        Threads threads,
        uint64_t limit = 0,
        uint64_t progressIntervalSeconds = 10);
    //monitor 方法：监视构建器的进度，接受进度间隔秒数、计数器和完成标志等参数。
    void monitor(
        uint64_t progressIntervalSeconds,
        std::atomic_uint64_t& counter,
        std::atomic_bool& done);
    // runInserts 方法：运行插入操作，接受线程数、限制数和计数器等参数。
    void runInserts(
        Threads threads,
        uint64_t limit,
        std::atomic_uint64_t& counter);
    // tryInsert 方法：尝试插入操作，接受缓存、起始位置和计数器等参数。
    void tryInsert(
        ChunkCache& cache,
        uint64_t origin,
        std::atomic_uint64_t& counter);
    // insert 方法：执行插入操作，接受缓存、起始位置和计数器等参数。
    void insert(
        ChunkCache& cache,
        uint64_t origin,
        std::atomic_uint64_t& counter);
    //save 方法：保存数据，接受线程数作为参数。
    void save(unsigned threads);
    //saveHierarchy 方法：保存层次结构，接受线程数作为参数。
    void saveHierarchy(unsigned threads);
    // saveSources 方法：保存数据源，接受线程数作为参数。
    void saveSources(unsigned threads);
    //saveMetadata 方法：保存元数据。
    void saveMetadata();

    Endpoints endpoints;
    Metadata metadata;
    Manifest manifest;
    Hierarchy hierarchy;
    bool verbose = true;
};
//另外，代码还定义了一个名为 builder 的命名空间，其中包含了一些与构建器相关的函数，如 load、create、run、merge 和 mergeOne。
namespace builder
{

Builder load(
    Endpoints endpoints,
    unsigned threads,
    unsigned subsetId,
    bool verbose = true);
Builder create(json config);
uint64_t run(Builder& builder, json config);

void merge(json config);
void merge(
    Endpoints endpoints,
    unsigned threads,
    bool force = false,
    bool verbose = true);
void mergeOne(Builder& dst, const Builder& src, ChunkCache& cache);

} // namespace builder

} // namespace entwine

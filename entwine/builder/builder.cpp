/******************************************************************************
* Copyright (c) 2019, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/builder/builder.hpp>

#include <algorithm>
#include <cassert>

#include <pdal/PipelineManager.hpp>

#include <entwine/builder/clipper.hpp>
#include <entwine/builder/heuristics.hpp>
#include <entwine/types/dimension.hpp>
#include <entwine/types/point-counts.hpp>
#include <entwine/util/config.hpp>
#include <entwine/util/fs.hpp>
#include <entwine/util/info.hpp>
#include <entwine/util/io.hpp>
#include <entwine/util/pdal-mutex.hpp>
#include <entwine/util/pipeline.hpp>
#include <entwine/util/time.hpp>

namespace entwine
{

Builder::Builder(
    Endpoints endpoints,
    Metadata metadata,
    Manifest manifest,
    Hierarchy hierarchy,
    bool verbose)
    : endpoints(endpoints)
    , metadata(metadata)
    , manifest(manifest)
    , hierarchy(hierarchy)
    , verbose(verbose)
{ }
/*
这段代码是 Builder 结构体中的 run 方法的实现。该方法接受 Threads 对象、限制数 limit 和进度间隔 progressInterval 作为参数，并返回一个 uint64_t 类型的值。

以下是该方法的解释：
创建一个 Pool 对象 pool，并指定线程数为 2。Pool 是一个线程池，用于管理并发执行的任务。
创建一个 std::atomic_uint64_t 类型的计数器 counter，用于记录已处理的任务数量，初始值为 0。
创建一个 std::atomic_bool 类型的标志变量 done，用于表示任务是否完成，初始值为 false。
向线程池中添加两个任务：
第一个任务是调用 monitor 方法，传递进度间隔、计数器和完成标志等参数。该任务用于监视构建器的进度。
第二个任务是调用 runInserts 方法，传递线程数、限制数和计数器等参数。该任务用于运行插入操作，并在完成后将 done 标志设置为 true。
等待线程池中的所有任务执行完成。
返回计数器的值，表示已处理的任务数量。
这段代码使用了线程池来并发执行两个任务，一个用于监视进度，另一个用于运行插入操作。通过使用原子类型的计数器和标志变量，可以在多线程环境下安全地进行状态更新和同步操作。
*/
uint64_t Builder::run(
    const Threads threads,
    const uint64_t limit,
    const uint64_t progressInterval)
{
    Pool pool(2);

    std::atomic_uint64_t counter(0);
    std::atomic_bool done(false);
    pool.add([&]() { monitor(progressInterval, counter, done); });
    pool.add([&]() { runInserts(threads, limit, counter); done = true; });

    pool.join();

    return counter;
}
/*
这段代码是 Builder 结构体中的 runInserts 方法的实现。该方法接受 Threads 对象、限制数 limit 和计数器 counter 作为参数。

以下是该方法的解释：

根据元数据 metadata 中的子集信息和边界信息，计算出活动边界 active。如果存在子集信息，则将活动边界限制在子集边界内。
根据线程数 threads.work 和数据集大小 manifest.size()，计算出实际工作线程数 actualWorkThreads。如果工作线程数超过数据集大小，则将其限制为数据集大小。
计算被窃取的线程数 stolenThreads，即额外的工作线程数减去实际工作线程数。
根据剪裁线程数 threads.clip 和被窃取的线程数，计算出实际剪裁线程数 actualClipThreads。
创建一个 ChunkCache 对象 cache，用于缓存数据块，传递构建器的一些属性。
创建一个线程池 pool，线程数为实际工作线程数和数据集大小中的较小值。
初始化一个变量 filesInserted，用于记录已插入的文件数量，初始值为 0。
使用循环遍历数据集中的每个元素：
获取当前元素的信息，并检查是否已插入且具有点数据且与活动边界有重叠。
如果满足条件，则向线程池中添加一个任务，该任务调用 tryInsert 方法，传递缓存、起始位置和计数器等参数，并在插入完成后输出完成信息。
增加已插入文件数量。
如果设置了输出详细信息标志 verbose，则输出 "Joining"。
等待线程池中的所有任务执行完成。
等待缓存中的所有数据块写入磁盘。
调用 save 方法，传递总线程数作为参数，保存数据。
这段代码使用了线程池来并发执行插入操作。它遍历数据集中的每个元素，并根据条件决定是否插入数据。通过使用线程池和缓存对象，可以实现并发插入操作，并提高处理效率。最后，调用 save 方法保存数据。
*/
void Builder::runInserts(
    Threads threads,
    uint64_t limit,
    std::atomic_uint64_t& counter)
{
    const Bounds active = metadata.subset
        ? intersection(
            getBounds(metadata.bounds, *metadata.subset),
            metadata.boundsConforming
        )
        : metadata.boundsConforming;

    const uint64_t actualWorkThreads =
        std::min<uint64_t>(threads.work, manifest.size());
    const uint64_t stolenThreads = threads.work - actualWorkThreads;
    const uint64_t actualClipThreads = threads.clip + stolenThreads;

    ChunkCache cache(endpoints, metadata, hierarchy, actualClipThreads);
    Pool pool(std::min<uint64_t>(actualWorkThreads, manifest.size()));

    uint64_t filesInserted = 0;

    for (
        uint64_t origin = 0;
        origin < manifest.size() && (!limit || filesInserted < limit);
        ++origin)
    {
        const auto& item = manifest.at(origin);
        const auto& info = item.source.info;
        if (!item.inserted && info.points && active.overlaps(info.bounds))
        {
            if (verbose)
            {
                std::cout << "Adding " << origin << " - " << item.source.path <<
                    std::endl;
            }

            pool.add([this, &cache, origin, &counter]()
            {
                tryInsert(cache, origin, counter);
                if (verbose) std::cout << "\tDone " << origin << std::endl;
            });

            ++filesInserted;
        }
    }

    if (verbose) std::cout << "Joining" << std::endl;

    pool.join();
    cache.join();

    save(getTotal(threads));
}
/*
这段代码是 `Builder` 结构体中的 `monitor` 方法的实现。该方法用于监视构建器的进度，并在指定的进度间隔内输出进度信息。

以下是该方法的解释：

1. 如果进度间隔 `progressInterval` 为 0，则直接返回，不进行监视。
2. 定义一个别名 `ms`，表示 `std::chrono::milliseconds`。
3. 计算数据集中已插入点的数量 `already` 和总点数量 `total`。
4. 获取当前时间 `start`。
5. 初始化变量 `lastTick` 和 `lastInserted`，用于记录上次输出进度信息的时间和已插入的点数量。
6. 在循环中，当 `done` 标志为 false 时执行以下操作：
   - 计算距离上次输出进度信息的时间间隔 `tick`，并等待剩余的秒数。
   - 如果当前时间与上次输出进度信息的时间相同，或者时间间隔不是进度间隔的倍数，则继续下一次循环。
   - 更新 `lastTick` 为当前时间。
   - 计算当前已处理的点数量 `current` 和已插入的点数量 `inserted`。
   - 计算进度 `progress`，即已插入点数量占总点数量的比例。
   - 计算每小时的处理速度 `pace` 和进度间隔内的速度 `intervalPace`。
   - 更新 `lastInserted` 为当前已插入的点数量。
   - 获取缓存对象的信息 `info`。
   - 如果设置了输出详细信息标志 `verbose`，则输出当前时间、进度、已插入点数量、每小时速度、进度间隔速度以及缓存信息。

这段代码在循环中通过计算时间间隔和已处理的点数量来输出构建器的进度信息。通过使用睡眠函数 `std::this_thread::sleep_for` 和时间计算函数 `since`，可以控制输出的时间间隔。输出的信息包括已插入点的数量、处理速度以及缓存信息等。
*/
void Builder::monitor(
    const uint64_t progressInterval,
    std::atomic_uint64_t& atomicCurrent,
    std::atomic_bool& done)
{
    if (!progressInterval) return;

    using ms = std::chrono::milliseconds;
    const double mph = 3600.0 / 1000000.0;

    const double already = getInsertedPoints(manifest);
    const double total = getTotalPoints(manifest);
    const auto start = now();

    int64_t lastTick = 0;
    double lastInserted = 0;

    while (!done)
    {
        std::this_thread::sleep_for(ms(1000 - (since<ms>(start) % 1000)));
        const int64_t tick = since<std::chrono::seconds>(start);

        if (tick == lastTick || tick % progressInterval) continue;

        lastTick = tick;

        const double current = atomicCurrent;
        const double inserted = already + current;
        const double progress = inserted / total;

        const uint64_t pace = inserted / tick * mph;
        const uint64_t intervalPace =
            (inserted - lastInserted) / progressInterval * mph;

        lastInserted = inserted;

        const ChunkCache::Info info(ChunkCache::latchInfo());

        if (verbose)
        {
            std::cout << formatTime(tick) << " - " <<
                std::round(progress * 100) << "% - " <<
                commify(inserted) << " - " <<
                commify(pace) << " " <<
                "(" << commify(intervalPace) << ") M/h - " <<
                info.written << "W - " <<
                info.read << "R - " <<
                info.alive << "A" <<
                std::endl;
        }
    }
}
/*
这段代码是 `Builder` 结构体中的 `tryInsert` 方法的实现。该方法用于尝试插入数据，并在插入过程中捕获异常，记录错误信息，并将插入标志设置为 true。

以下是该方法的解释：

1. 从数据集中获取指定 `originId` 的元素信息，并存储在引用变量 `item` 中。
2. 尝试调用 `insert` 方法，传递缓存对象、`originId` 和计数器等参数，进行数据插入操作。
3. 如果捕获到 `std::exception` 类型的异常，将异常信息添加到元素的源信息的错误列表中。
4. 如果捕获到其他类型的异常，将默认的错误信息 "Unknown error during build" 添加到元素的源信息的错误列表中。
5. 将元素的插入标志设置为 true，表示已经插入完成。

这段代码通过调用 `insert` 方法来尝试插入数据，并捕获可能抛出的异常。如果捕获到异常，将错误信息添加到元素的源信息中。最后，将插入标志设置为 true，表示已经完成插入操作。
*/
void Builder::tryInsert(
    ChunkCache& cache,
    const Origin originId,
    std::atomic_uint64_t& counter)
{
    auto& item = manifest.at(originId);

    try
    {
        insert(cache, originId, counter);
    }
    catch (const std::exception& e)
    {
        item.source.info.errors.push_back(e.what());
    }
    catch (...)
    {
        item.source.info.errors.push_back("Unknown error during build");
    }

    item.inserted = true;
}
/*
这段代码是 `Builder` 结构体中的 `insert` 方法的实现。该方法用于将数据插入到 `ChunkCache` 中，并执行一系列的处理和过滤操作。

以下是该方法的解释：

1. 从数据集中获取指定 `originId` 的元素信息，并存储在引用变量 `item` 中。同时获取元素的源信息。
2. 调用 `ensureGetLocalHandle` 方法，传递仲裁器和元素的路径，获取本地处理句柄。
3. 获取本地路径 `localPath`。
4. 根据元数据的边界和起始深度创建 `ChunkKey` 对象 `ck`。
5. 创建 `Clipper` 对象 `clipper`，传递缓存对象。
6. 获取元数据的比例和偏移信息 `so`。
7. 根据元数据的子集信息，获取边界的子集 `boundsSubset`。
8. 初始化已插入的点数量 `inserted` 和点的唯一标识符 `pointId`。
9. 根据元数据的绝对模式创建点表的布局 `layout`。
10. 创建 `VectorPointTable` 对象 `table`，传递布局。
11. 设置点表的处理函数，用于处理每个点的操作。在处理过程中，更新已插入的点数量，并根据一定的规则执行 `Clipper` 的剪裁操作。
12. 创建 `Key` 对象 `key`，用于存储点的键信息。
13. 遍历点表中的每个点，设置点的原始标识符和点的唯一标识符，初始化 `Voxel` 对象并进行剪裁操作，然后根据点的位置初始化 `ChunkKey`，并将点插入到缓存对象中。
14. 更新计数器 `counter`。
15. 根据源信息中的管道配置，创建 JSON 对象 `pipeline`。
16. 如果需要统计信息（即源信息的模式中没有统计信息），则向管道中添加统计过滤器。
17. 创建 `PipelineManager` 对象 `pm`，并将管道配置加载到 `pm` 中。
18. 获取 `PdalMutex` 的互斥锁，以保证多线程环境下的安全操作。
19. 读取管道配置并验证阶段选项。
20. 获取管道的最后一个阶段，并为该阶段准备点表。
21. 执行管道，对点表进行处理和过滤操作。
22. 如果存在统计过滤器，将统计信息更新到元数据的模式中。

这段代码实现了将数据插入到缓存中，并执行了一系列的处理和过滤操作。它使用了 PDAL 库来处理点云数据，并根据配置的管道对数据进行处理和过滤。最后，将统计信息更新到元数据的模式中。
*/
void Builder::insert(
    ChunkCache& cache,
    const Origin originId,
    std::atomic_uint64_t& counter)
{
    auto& item = manifest.at(originId);
    auto& info(item.source.info);
    const auto handle =
        ensureGetLocalHandle(*endpoints.arbiter, item.source.path);

    const std::string localPath = handle.localPath();

    ChunkKey ck(metadata.bounds, getStartDepth(metadata));
    Clipper clipper(cache);

    optional<ScaleOffset> so = getScaleOffset(metadata.schema);
    const optional<Bounds> boundsSubset = metadata.subset
        ? getBounds(metadata.bounds, *metadata.subset)
        : optional<Bounds>();

    uint64_t inserted(0);
    uint64_t pointId(0);

    auto layout = toLayout(metadata.absoluteSchema);
    VectorPointTable table(layout);
    table.setProcess([&]()
    {
        inserted += table.numPoints();
        if (inserted > heuristics::sleepCount)
        {
            inserted = 0;
            clipper.clip();
        }

        Voxel voxel;
        PointCounts counts;

        Key key(metadata.bounds, getStartDepth(metadata));

        for (auto it = table.begin(); it != table.end(); ++it)
        {
            auto& pr = it.pointRef();
            pr.setField(DimId::OriginId, originId);
            pr.setField(DimId::PointId, pointId);
            ++pointId;

            voxel.initShallow(it.pointRef(), it.data());
            if (so) voxel.clip(*so);
            const Point& point(voxel.point());

            ck.reset();

            if (metadata.boundsConforming.contains(point))
            {
                if (!boundsSubset || boundsSubset->contains(point))
                {
                    key.init(point);
                    cache.insert(voxel, key, ck, clipper);
                    ++counts.inserts;
                }
            }
        }
        counter += counts.inserts;
    });

    json pipeline = info.pipeline.is_null()
        ? json::array({ json::object() })
        : info.pipeline;
    pipeline.at(0)["filename"] = localPath;

    // TODO: Allow this to be set via config.
    const bool needsStats = !hasStats(info.schema);
    if (needsStats)
    {
        json& statsFilter = findOrAppendStage(pipeline, "filters.stats");
        if (!statsFilter.count("enumerate"))
        {
            statsFilter.update({ { "enumerate", "Classification" } });
        }
    }

    pdal::PipelineManager pm;
    std::istringstream iss(pipeline.dump());

    std::unique_lock<std::mutex> lock(PdalMutex::get());

    pm.readPipeline(iss);
    pm.validateStageOptions();
    pdal::Stage& last = getStage(pm);
    last.prepare(table);

    lock.unlock();

    last.execute(table);

    // TODO:
    // - update point count information for this file's metadata.
    if (pdal::Stage* stage = findStage(last, "filters.stats"))
    {
        const pdal::StatsFilter& statsFilter(
            dynamic_cast<const pdal::StatsFilter&>(*stage));

        for (Dimension& d : info.schema)
        {
            const DimId id = layout.findDim(d.name);
            d.stats = DimensionStats(statsFilter.getStats(id));
        }
    }
}

void Builder::save(const unsigned threads)
{
    if (verbose) std::cout << "Saving" << std::endl;
    saveHierarchy(threads);
    saveSources(threads);
    saveMetadata();
}

void Builder::saveHierarchy(const unsigned threads)
{
    // If we are a) saving a subset or b) saving a partial build, then defer
    // choosing a hierarchy step and instead just write one monolothic file.
    const bool isStepped =
        !metadata.subset &&
        std::all_of(manifest.begin(), manifest.end(), isSettled);

    unsigned step = 0;
    if (isStepped)
    {
        if (metadata.internal.hierarchyStep)
        {
            step = metadata.internal.hierarchyStep;
        }
        else step = hierarchy::determineStep(hierarchy);
    }

    hierarchy::save(
        hierarchy,
        endpoints.hierarchy,
        step,
        threads,
        getPostfix(metadata));
}

void Builder::saveSources(const unsigned threads)
{
    const std::string postfix = getPostfix(metadata);
    const std::string manifestFilename = "manifest" + postfix + ".json";
    const bool pretty = manifest.size() <= 1000;

    if (metadata.subset)
    {
        // If we are a subset, write the whole detailed metadata as one giant
        // blob, since we know we're going to need to wake up the whole thing to
        // do the merge.
        ensurePut(
            endpoints.sources,
            manifestFilename,
            json(manifest).dump(getIndent(pretty)));
    }
    else
    {
        // Save individual per-file metadata.
        manifest = assignMetadataPaths(manifest);
        saveEach(manifest, endpoints.sources, threads, pretty);

        // And in this case, we'll only write an overview for the manifest
        // itself, which excludes things like detailed metadata.
        ensurePut(
            endpoints.sources,
            manifestFilename,
            toOverview(manifest).dump(getIndent(pretty)));
    }
}

void Builder::saveMetadata()
{
    // If we've gained dimension stats during our build, accumulate them and
    // add them to our main metadata.
    const auto pred = [](const BuildItem& b) { return hasStats(b); };
    if (!metadata.subset && std::all_of(manifest.begin(), manifest.end(), pred))
    {
        Schema schema = clearStats(metadata.schema);

        for (const BuildItem& item : manifest)
        {
            auto itemSchema = item.source.info.schema;
            if (auto so = getScaleOffset(metadata.schema))
            {
                itemSchema = setScaleOffset(itemSchema, *so);
            }
            schema = combine(schema, itemSchema, true);
        }

        metadata.schema = schema;
    }

    const std::string postfix = getPostfix(metadata);

    const std::string metaFilename = "ept" + postfix + ".json";
    json metaJson = metadata;
    metaJson["points"] = getInsertedPoints(manifest);
    ensurePut(endpoints.output, metaFilename, metaJson.dump(2));

    const std::string buildFilename = "ept-build" + postfix + ".json";
    ensurePut(endpoints.output, buildFilename, json(metadata.internal).dump(2));
}

namespace builder
{

Builder load(
    const Endpoints endpoints,
    const unsigned threads,
    const unsigned subsetId,
    const bool verbose)
{
    const std::string postfix = subsetId ? "-" + std::to_string(subsetId) : "";
    const json metadataJson = entwine::merge(
        json::parse(endpoints.output.get("ept-build" + postfix + ".json")),
        json::parse(endpoints.output.get("ept" + postfix + ".json")));

    const Metadata metadata = config::getMetadata(metadataJson);

    const Manifest manifest =
        manifest::load(endpoints.sources, threads, postfix, verbose);

    const Hierarchy hierarchy =
        hierarchy::load(endpoints.hierarchy, threads, postfix);

    return Builder(endpoints, metadata, manifest, hierarchy);
}

/*
这段代码是用于创建一个 Builder 对象的工厂方法。主要功能包括:

1. 从配置中解析参数,如 verbose、endpoints、threads 等

2. 检查输出目录是否已存在 build,如果存在则加载存在的 manifest 和 hierarchy

3. 解析输入数据源,移除 manifest 中已存在的,剩下的进行 analyze 

4. analyze 得到的新 source 信息添加到 manifest

5. 将 analyze 得到的 source 信息与配置进行 merge,配置优先级更高

6. 从配置中获取 metadata 信息

7. 使用解析得到的各种信息实例化一个 Builder 对象并返回

所以这段代码主要工作是:

- 解析配置
- 检查是否有存在的 build 信息并加载
- 分析输入数据源 
- 与存在信息合并
- 创建 Builder

它实现了增量建模的功能,可以对新增的数据进行分析和 merge,从而避免全量重建。
*/
Builder create(json j)
{
    const bool verbose = config::getVerbose(j);
    const Endpoints endpoints = config::getEndpoints(j);
    const unsigned threads = config::getThreads(j);

    Manifest manifest;
    Hierarchy hierarchy;

    // TODO: Handle subset postfixing during existence check - currently
    // continuations of subset builds will not work properly.
    // const optional<Subset> subset = config::getSubset(j);
    if (!config::getForce(j) && endpoints.output.tryGetSize("ept.json"))
    {
        // Merge in our metadata JSON, overriding any config settings.
        const json existingConfig = entwine::merge(
            json::parse(endpoints.output.get("ept-build.json")),
            json::parse(endpoints.output.get("ept.json"))
        );
        j = entwine::merge(j, existingConfig);

        // Awaken our existing manifest and hierarchy.
        manifest = manifest::load(endpoints.sources, threads, "", verbose);
        hierarchy = hierarchy::load(endpoints.hierarchy, threads);
    }

    // Now, analyze the incoming `input` if needed.
    StringList inputs = resolve(config::getInput(j), *endpoints.arbiter);
    const auto exists = [&manifest](std::string path)
    {
        return std::any_of(
            manifest.begin(),
            manifest.end(),
            [path](const BuildItem& b) { return b.source.path == path; });
    };
    // Remove any inputs we already have in our manifest prior to analysis.
    inputs.erase(
        std::remove_if(inputs.begin(), inputs.end(), exists),
        inputs.end());
    const SourceList sources = analyze(
        inputs,
        config::getPipeline(j),
        config::getDeep(j),
        config::getTmp(j),
        *endpoints.arbiter,
        threads,
        verbose);
    for (const auto& source : sources)
    {
        if (source.info.points) manifest.emplace_back(source);
    }

    // It's possible we've just analyzed some files, in which case we have
    // potentially new information like bounds, schema, and SRS.  Prioritize
    // values from the config, which may explicitly override these.
    const SourceInfo analysis = manifest::reduce(sources);
    j = merge(analysis, j);
    const Metadata metadata = config::getMetadata(j);

    return Builder(endpoints, metadata, manifest, hierarchy, verbose);
}

uint64_t run(Builder& builder, const json config)
{
    return builder.run(
        config::getCompoundThreads(config),
        config::getLimit(config),
        config::getProgressInterval(config));
}

void merge(const json config)
{
    merge(
        config::getEndpoints(config),
        config::getThreads(config),
        config::getForce(config),
        config::getVerbose(config));
}

void merge(
    const Endpoints endpoints,
    const unsigned threads,
    const bool force,
    const bool verbose)
{
    if (!force && endpoints.output.tryGetSize("ept.json"))
    {
        throw std::runtime_error(
            "Completed dataset already exists here: "
            "re-run with '--force' to overwrite it");
    }

    if (!endpoints.output.tryGetSize("ept-1.json"))
    {
        throw std::runtime_error("Failed to find first subset");
    }

    if (verbose) std::cout << "Initializing" << std::endl;
    const Builder base = builder::load(endpoints, threads, 1, verbose);

    // Grab the total number of subsets, then clear the subsetting from our
    // metadata aggregator which will represent our merged output.
    Metadata metadata = base.metadata;
    const unsigned of = metadata.subset.value().of;
    metadata.subset = { };

    Manifest manifest = base.manifest;

    Builder builder(endpoints, metadata, manifest, Hierarchy(), verbose);
    ChunkCache cache(endpoints, builder.metadata, builder.hierarchy, threads);

    if (verbose) std::cout << "Merging" << std::endl;

    Pool pool(threads);
    std::mutex mutex;

    for (unsigned id = 1; id <= of; ++id)
    {
        if (verbose) std::cout << "\t" << id << "/" << of << ": ";
        if (endpoints.output.tryGetSize("ept-" + std::to_string(id) + ".json"))
        {
            if (verbose) std::cout << "merging" << std::endl;
            pool.add([
                &endpoints,
                threads,
                verbose,
                id,
                &builder,
                &cache,
                &mutex]()
            {
                Builder current = builder::load(
                    endpoints,
                    threads,
                    id,
                    verbose);
                builder::mergeOne(builder, current, cache);

                std::lock_guard<std::mutex> lock(mutex);
                builder.manifest = manifest::merge(
                    builder.manifest,
                    current.manifest);
            });
        }
        else if (verbose) std::cout << "skipping" << std::endl;
    }

    pool.join();
    cache.join();

    builder.save(threads);
    if (verbose) std::cout << "Done" << std::endl;
}

void mergeOne(Builder& dst, const Builder& src, ChunkCache& cache)
{
    // TODO: Should make sure that the src/dst metadata match.  For now we're
    // relying on the user not to have done anything weird.
    const auto& endpoints = dst.endpoints;
    const auto& metadata = dst.metadata;

    Clipper clipper(cache);
    const auto sharedDepth = getSharedDepth(src.metadata);
    for (const auto& node : src.hierarchy.map)
    {
        const Dxyz& key = node.first;
        const uint64_t count = node.second;
        if (!count) continue;

        if (key.d >= sharedDepth)
        {
            assert(!hierarchy::get(dst.hierarchy, key));
            hierarchy::set(dst.hierarchy, key, count);
        }
        else
        {
            auto layout = toLayout(metadata.absoluteSchema);
            VectorPointTable table(layout, count);
            table.setProcess([&]()
            {
                Voxel voxel;
                Key pk(metadata.bounds, getStartDepth(metadata));
                ChunkKey ck(metadata.bounds, getStartDepth(metadata));

                for (auto it(table.begin()); it != table.end(); ++it)
                {
                    voxel.initShallow(it.pointRef(), it.data());
                    const Point point(voxel.point());
                    pk.init(point, key.d);
                    ck.init(point, key.d);

                    assert(ck.dxyz() == key);

                    cache.insert(voxel, pk, ck, clipper);
                }
            });

            const auto stem = key.toString() + getPostfix(src.metadata);
            io::read(metadata.dataType, metadata, endpoints, stem, table);
        }
    }
}

} // namespace builder
} // namespace entwine

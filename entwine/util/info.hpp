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

#include <entwine/third/arbiter/arbiter.hpp>
#include <entwine/types/reprojection.hpp>
#include <entwine/types/source.hpp>
#include <entwine/util/optional.hpp>
/*
这两个文件实现了 Entwine 中获取点云信息的逻辑,主要功能如下:

info.hpp:

- 定义了获取点云浅层信息、深层信息、解析信息的函数

info.cpp:

- getShallowInfo:通过 PDAL 流式管道获取点云的基本信息,如边界、点数等

- getDeepInfo:通过 PDAL 统计管道获取点云字段的统计信息 

- execute:执行 PDAL 管道的帮助函数

- analyzeOne:分析单个文件,调用 getShallowInfo 或 getDeepInfo

- parseOne:解析已存在的 JSON 信息文件

- localize:获取文件本地副本的帮助函数 

- analyze:并行分析多个文件信息

主要功能是利用 PDAL 获取点云数据的元信息,包括:

- 浅层信息:点数、边界、坐标系等

- 深层信息:字段的统计信息,支持数据裁剪

- 利用 JSON 缓存信息,避免重复分析

- 支持并行分析多个文件

- 获取本地文件副本加速读取

这为后续的点云处理和索引提供了基础的数据集信息。
*/
namespace entwine
{

arbiter::LocalHandle localize(
    std::string path,
    bool deep,
    std::string tmp,
    const arbiter::Arbiter& a);

SourceInfo analyzeOne(std::string path, bool deep, json pipelineTemplate);
Source parseOne(std::string path, const arbiter::Arbiter& a = { });

SourceList analyze(
    const StringList& inputs,
    const json& pipelineTemplate = json::array({ json::object() }),
    bool deep = false,
    std::string tmp = arbiter::getTempPath(),
    const arbiter::Arbiter& a = { },
    unsigned threads = 8,
    bool verbose = true);

} // namespace entwine

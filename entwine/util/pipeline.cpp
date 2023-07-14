/******************************************************************************
* Copyright (c) 2019, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/util/pipeline.hpp>

#include <algorithm>

#include <pdal/io/LasReader.hpp>
#include <pdal/io/LasHeader.hpp>
/*
这两个文件定义了 Entwine 中与 PDAL 管道处理相关的工具函数,主要功能:

pipeline.cpp:

- findStage:在 JSON 管道中查找指定 stage 

- findOrAppendStage:查找或追加指定 stage

- omitStage:从管道中省略指定 stage

- getStage:从 PDAL 管道中获取 stage

- getReader:获取管道第一个 reader 

- getFirst:获取管道第一个 stage

- getMetadata:获取 reader 的元数据

- getScaleOffset:获取 reader 的缩放偏移信息

pipeline.hpp:

- 定义相关函数

主要功能是封装了管道处理的常用功能:

- 在 JSON 管道中查找和修改 stage

- 从 PDAL 管道获取特定 stage 

- 获取 reader 信息

- 元数据和缩放偏移信息

这样可以方便地对管道进行查询和修改,减少重复代码。

getMetadata 和 getScaleOffset 也提供了获取重要管道信息的接口。

所以这些函数简化了管道处理流程,避免各处重复提取管道信息的逻辑。
*/
namespace entwine
{

json::const_iterator findStage(const json& pipeline, const std::string type)
{
    return std::find_if(
        pipeline.begin(),
        pipeline.end(),
        [type](const json& stage) { return stage.value("type", "") == type; });
}

json::iterator findStage(json& pipeline, const std::string type)
{
    return std::find_if(
        pipeline.begin(),
        pipeline.end(),
        [type](const json& stage) { return stage.value("type", "") == type; });
}

json& findOrAppendStage(json& pipeline, const std::string type)
{
    auto it = findStage(pipeline, type);
    if (it != pipeline.end()) return *it;

    pipeline.push_back({ { "type", type } });
    return pipeline.back();
}

json omitStage(json pipeline, const std::string type)
{
    auto it = findStage(pipeline, type);
    if (it == pipeline.end()) return pipeline;

    pipeline.erase(it);
    return pipeline;
}

pdal::Stage* findStage(pdal::Stage& last, const std::string type)
{
    pdal::Stage* current(&last);

    do
    {
        if (current->getName() == type) return current;

        if (current->getInputs().size() > 1)
        {
            throw std::runtime_error("Invalid pipeline - must be linear");
        }

        current = current->getInputs().size()
            ? current->getInputs().at(0)
            : nullptr;
    }
    while (current);

    return nullptr;
}

pdal::Stage& getStage(pdal::PipelineManager& pm)
{
    if (pdal::Stage* s = pm.getStage()) return *s;
    throw std::runtime_error("Invalid pipeline - no stages");
}

pdal::Reader& getReader(pdal::Stage& last)
{
    pdal::Stage& first(getFirst(last));

    if (pdal::Reader* reader = dynamic_cast<pdal::Reader*>(&first))
    {
        return *reader;
    }

    throw std::runtime_error("Invalid pipeline - must start with reader");
}

pdal::Stage& getFirst(pdal::Stage& last)
{
    pdal::Stage* current(&last);

    while (current->getInputs().size())
    {
        if (current->getInputs().size() > 1)
        {
            throw std::runtime_error("Invalid pipeline - must be linear");
        }

        current = current->getInputs().at(0);
    }

    return *current;
}

json getMetadata(pdal::Reader& reader)
{
    return json::parse(pdal::Utils::toJSON(reader.getMetadata()));
}

optional<ScaleOffset> getScaleOffset(const pdal::Reader& reader)
{
    if (const auto* las = dynamic_cast<const pdal::LasReader*>(&reader))
    {
        const auto& h(las->header());
        return ScaleOffset(
            Scale(h.scaleX(), h.scaleY(), h.scaleZ()),
            Offset(h.offsetX(), h.offsetY(), h.offsetZ())
        );
    }
    return { };
}

} // namespace entwine

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
/*
这两个文件定义了 Entwine 的 IO 相关工具函数,主要功能:

io.cpp:

- putWithRetry:尝试多次 PUT 远程数据

- ensurePut:确保 PUT 成功,否则抛错

- getWithRetry:尝试多次 GET 远程数据 

- ensureGet:确保 GET 成功,否则抛错

- getPointlessLasFile:获取 LAS 文件 header,用来加速读取

io.hpp:

- 定义了 IO 类型枚举:Binary、Laszip、Zstandard

- write:根据 IO 类型选择对应写入函数

- read:根据 IO 类型选择对应读取函数

主要功能有:

- 重试机制,确保 IO 成功

- 获取无点 LAS 文件加速读取

- 封装不同格式的读写函数

- 提供 IO 类型枚举的编解码

这些 IO 工具函数为 Entwine 的远程数据传输提供了便利与可靠性保证。

重试机制可以提高容错能力,获取无点 LAS 文件可以减少不必要的数据传输。IO 类型枚举使得格式选择及扩展更简单方便。

所以这些 IO 工具函数实现了可靠、便利、可扩展的远程数据访问。
*/
#include <stdexcept>
#include <string>
#include <vector>

#include <entwine/third/arbiter/arbiter.hpp>
#include <entwine/types/exceptions.hpp>
#include <entwine/util/optional.hpp>

namespace entwine
{

static constexpr int defaultTries = 8;

bool putWithRetry(
    const arbiter::Endpoint& ep,
    const std::string& path,
    const std::vector<char>& data,
    int tries = defaultTries);
bool putWithRetry(
    const arbiter::Endpoint& ep,
    const std::string& path,
    const std::string& s,
    int tries = defaultTries);

void ensurePut(
    const arbiter::Endpoint& ep,
    const std::string& path,
    const std::vector<char>& data,
    int tries = defaultTries);
void ensurePut(
    const arbiter::Endpoint& ep,
    const std::string& path,
    const std::string& s,
    int tries = defaultTries);

optional<std::vector<char>> getBinaryWithRetry(
    const arbiter::Endpoint& ep,
    const std::string& path,
    int tries = defaultTries);
optional<std::string> getWithRetry(
    const arbiter::Endpoint& ep,
    const std::string& path,
    int tries = defaultTries);
optional<std::string> getWithRetry(
    const arbiter::Arbiter& a,
    const std::string& path,
    int tries = defaultTries);

std::vector<char> ensureGetBinary(
    const arbiter::Endpoint& ep,
    const std::string& path,
    int tries = defaultTries);
std::string ensureGet(
    const arbiter::Endpoint& ep,
    const std::string& path,
    int tries = defaultTries);
std::string ensureGet(
    const arbiter::Arbiter& a,
    const std::string& path,
    int tries = defaultTries);

arbiter::LocalHandle ensureGetLocalHandle(
    const arbiter::Arbiter& a,
    const std::string& path,
    int tries = defaultTries);

arbiter::LocalHandle getPointlessLasFile(
    const std::string& path,
    const std::string& tmp,
    const arbiter::Arbiter& a);

} // namespace entwine

/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/
/**
这段代码看起来是C++代码，它定义了一个名为App的类，该类是一个应用程序的基类。这个类提供了一些方法和成员变量，用于处理命令行参数、执行应用程序逻辑以及打印信息。


该类是一个抽象基类，因此不能直接实例化。您可以创建一个继承自App的子类，并在子类中实现addArgs()和run()方法，以定义特定应用程序的行为。
*/
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "arg-parser.hpp"

#include <entwine/third/arbiter/arbiter.hpp>
#include <entwine/types/dimension.hpp>
#include <entwine/types/reprojection.hpp>
#include <entwine/types/srs.hpp>
#include <entwine/util/json.hpp>
#include <entwine/util/optional.hpp>

namespace entwine
{
namespace app
{

class App
{
public:
    App() : m_json(json::object()) { }
    virtual ~App() { }

    void go(Args args)
    {
        addArgs();
        if (!m_ap.handle(args)) return;
        run();
    }

protected:
    // addArgs()：一个虚函数，用于在子类中添加特定应用程序的命令行参数。
    virtual void addArgs() = 0;
    // run()：一个虚函数，用于在子类中实现应用程序的逻辑。

    virtual void run() = 0;
    // m_json：一个json对象，用于存储应用程序的配置信息。
    json m_json;
    // m_ap：一个ArgParser对象，用于解析和处理命令行参数。
    ArgParser m_ap;
    // addInput()、addOutput()、addConfig() 等方法：用于向命令行参数中添加特定的选项和描述。
    void addInput(std::string description, bool asDefault = false);
    void addOutput(std::string description, bool asDefault = false);
    void addConfig();
    void addTmp();
    void addSimpleThreads();
    void addReprojection();
    void addNoTrustHeaders();
    void addDeep();
    void addAbsolute();
    void addArbiter();
    // checkEmpty()、extract()、yesNo() 等方法：用于检查和处理配置信息。
    void checkEmpty(json j) const
    {
        if (!j.is_null()) throw std::runtime_error("Invalid specification");
    }

    uint64_t extract(json j) const
    {
        return json::parse(j.get<std::string>()).get<uint64_t>();
    }

    std::string yesNo(bool b) const { return b ? "yes" : "no"; }
    // getReprojectionString()、getDimensionString() 等方法：用于获取字符串表示的投影和维度信息。
    std::string getReprojectionString(optional<Reprojection> r);
    std::string getDimensionString(const Schema& schema) const;
    // printProblems()、printInfo() 等方法：用于打印警告和错误信息
    void printProblems(const StringList& warnings, const StringList& errors);
    void printInfo(
        const Schema& schema,
        const Bounds& bounds,
        const Srs& srs,
        uint64_t points,
        const StringList& warnings,
        const StringList& errors);
};

} // namespace app
} // namespace entwine


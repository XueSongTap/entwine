cmake_minimum_required(VERSION 3.5)
# 设置项目名称为"entwine"
project(entwine)
# 设置CMake模块路径，用于查找自定义的CMake模块
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})
# 设置Entwine的版本号，包括主版本、次版本和修订版本，并生成完整的版本字符串。
set(ENTWINE_VERSION_MAJOR 2 CACHE STRING "Entwine major version" FORCE)
set(ENTWINE_VERSION_MINOR 3 CACHE STRING "Entwine minor version" FORCE)
set(ENTWINE_VERSION_PATCH 0 CACHE STRING "Entwine patch version" FORCE)
set(ENTWINE_VERSION_STRING
    "${ENTWINE_VERSION_MAJOR}.${ENTWINE_VERSION_MINOR}.${ENTWINE_VERSION_PATCH}"
    CACHE STRING "Entwine version string" FORCE)
# 设置C++标准为C++17，并要求编译器使用C++17标准。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 设置Entwine的API版本和构建版本
set(ENTWINE_API_VERSION "3")
set(ENTWINE_BUILD_VERSION ${ENTWINE_VERSION_STRING})
# 如果未设置构建类型（例如Debug或Release），则将构建类型默认设置为Release，并输出一条消息。
if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
    MESSAGE("Defaulting to release mode")
    set(CMAKE_BUILD_TYPE "Release")
endif()
# 包含FeatureSummary模块，用于生成特性摘要报告。
include(FeatureSummary)
# 设置ROOT_DIR为项目源代码目录，设置CMAKE_DIR为cmake目录的路径，并链接额外的目录。
set(ROOT_DIR ${PROJECT_SOURCE_DIR})
set(CMAKE_DIR ${ROOT_DIR}/cmake)
link_directories(${ADDITIONAL_LINK_DIRECTORIES_PATHS})
# 设置entwine_defs_hpp_in为defs.hpp.in文件的路径，设置entwine_defs_hpp为生成的defs.hpp文件的路径，并通过configure_file命令将defs.hpp.in复制为defs.hpp。这个命令将在构建过程中生成一个已配置的头文件
set(entwine_defs_hpp_in
    "${ROOT_DIR}/entwine/types/defs.hpp.in")
set(entwine_defs_hpp
    ${PROJECT_BINARY_DIR}/include/entwine/types/defs.hpp)
configure_file(${entwine_defs_hpp_in} ${entwine_defs_hpp})
# 定义了一个选项WITH_ZSTD，用于决定是否构建对Zstd压缩/解压缩的支持。如果WITH_ZSTD为假，则添加预处理定义-DNO_ZSTD。
option(WITH_ZSTD
    "Build support for compression/decompression with Zstd." TRUE)
if (NOT WITH_ZSTD)
    add_definitions(-DNO_ZSTD)
endif()
# 包含一些CMake模块文件，如threads.cmake、backtrace.cmake、curl.cmake、openssl.cmake和pdal.cmake。
include(${CMAKE_DIR}/threads.cmake)
include(${CMAKE_DIR}/backtrace.cmake)
include(${CMAKE_DIR}/curl.cmake)
include(${CMAKE_DIR}/openssl.cmake)
include(${CMAKE_DIR}/pdal.cmake)
#
# Must come last.  Depends on vars set in other include files.
# 包含system.cmake模块文件，这个模块文件必须在其他包含文件中的变量设置之后包含。
include(${CMAKE_DIR}/system.cmake)
# 添加子目录"entwine"和"app"，分别构建"entwine"和"app"目标。
add_subdirectory(entwine)
add_subdirectory(app)

#
# Each subdirectory is built as an object library, which is a collection
# of files which act as a target.  No actual file is built.
# 设置一个变量OBJS，它包含了多个目标对象（object library）。这些目标对象是io、third、builder、types和util的目标对象。
set(OBJS
    $<TARGET_OBJECTS:io>
    $<TARGET_OBJECTS:third>
    $<TARGET_OBJECTS:builder>
    $<TARGET_OBJECTS:types>
    $<TARGET_OBJECTS:util>
)
# 根据是否为MSVC平台，构建一个静态库（STATIC）或共享库（SHARED）的entwine库，并将OBJS作为库的源文件。
if (MSVC)
    add_library(entwine STATIC ${OBJS})
    target_link_libraries(entwine PRIVATE wsock32 ws2_32)
else()
    add_library(entwine SHARED ${OBJS})
endif()
# 设置entwine库的链接库依赖关系，其中PUBLIC部分是公共链接库，PRIVATE部分是私有链接库。
target_link_libraries(entwine
    PUBLIC
        ${LZMA_LIBRARY}
        ${CMAKE_DL_LIBS}
    PRIVATE
        pdalcpp
        ${CURL_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${SHLWAPI}
)
# 设置entwine库的属性，包括版本号、SO版本号和CLEAN_DIRECT_OUTPUT。
set_target_properties(
    entwine
    PROPERTIES
    VERSION ${ENTWINE_BUILD_VERSION}
    SOVERSION ${ENTWINE_API_VERSION}
    CLEAN_DIRECT_OUTPUT 1)

#
# Test
# 定义一个选项WITH_TESTS，用于决定是否构建Entwine的单元测试。如果WITH_TESTS为真，则输出一条消息并启用测试，然后添加Google Test的子目录和test子目录。否则，输出一条消息表示不构建单元测试。
option(WITH_TESTS "Choose if Entwine unit tests should be built" TRUE)
if (WITH_TESTS)
    message("Configuring with unit tests")
    enable_testing()
    set(GOOGLETEST_VERSION 1.13.0)
    add_subdirectory("${PROJECT_SOURCE_DIR}/test/googletest-1.13.0/googletest")

    add_subdirectory(test)
else()
    message("Configuring with NO unit tests")
endif()

#
# Installation
# 安装entwine库到lib目录，并导出entwine-targets。同时，安装entwine_defs_hpp文件到include/entwine/types目录。
install(TARGETS entwine DESTINATION lib EXPORT entwine-targets)
install(FILES ${entwine_defs_hpp} DESTINATION "include/entwine/types")
#export(
#    EXPORT entwine-targets
#    FILE "${PROJECT_BINARY_DIR}/entwine-targets.cmake")
# 通过configure_file命令生成entwine-config.cmake文件，并安装到lib/cmake/entwine目录。同时，安装entwine-targets.cmake文件到lib/cmake/entwine目录。
configure_file(
    cmake/entwine-config.cmake.in "${PROJECT_BINARY_DIR}/entwine-config.cmake"
    @ONLY)
install(
    EXPORT entwine-targets
    FILE entwine-targets.cmake
    NAMESPACE Entwine:: DESTINATION lib/cmake/entwine)
install(
    FILES "${PROJECT_BINARY_DIR}/entwine-config.cmake"
    DESTINATION lib/cmake/entwine)

#
# Cpack
# 包含cpack.cmake模块文件，并添加一个名为dist的自定义目标，用于创建源代码包。
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack.cmake")
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
# feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)
feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)

# 根据您提供的代码，这是一个CMakeLists.txt文件，用于构建名为"entwine"的项目。它定义了项目的一些属性、依赖项和构建规则。
# 
# 以下是代码的主要部分：
# 
# 设置项目的最低CMake版本为3.5。
# 设置项目的名称为"entwine"。
# 定义了项目的版本号，包括主版本、次版本和修订版本。
# 设置C++标准为C++17。
# 定义了一些选项和变量，如WITH_ZSTD、ROOT_DIR和CMAKE_DIR。
# 包含了一些CMake模块文件，如threads.cmake、backtrace.cmake、curl.cmake等。
# 添加了子目录"entwine"和"app"，分别用于构建"entwine"和"app"目标。
# 如果是Windows平台（MSVC），则构建一个静态库"entwine"，否则构建一个共享库。
# 链接了一些库，如pdalcpp、CURL、OpenSSL等。
# 设置"entwine"目标的一些属性，如版本号、SO版本号和输出目录。
# 如果启用了WITH_TESTS选项，则添加一个名为"test"的子目录用于构建测试。
# 最后，代码包含了一些安装和打包相关的配置。
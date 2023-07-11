# 这段代码看起来是使用CMake构建一个名为app的可执行文件的脚本。它定义了一些变量和目标，用于指定源代码文件、链接库和编译选项。

# 以下是代码中的一些关键部分的解释：

# set(CMAKE_THREAD_PREFER_PTHREAD TRUE)：设置CMake变量CMAKE_THREAD_PREFER_PTHREAD为TRUE，表示首选使用pthread线程库。
# find_package(Threads REQUIRED)：查找并引入Threads库，该库用于多线程支持。
# add_executable(app ${SOURCES})：将名为app的可执行文件添加到构建中，并指定源代码文件的路径。
# compiler_options(app)：调用自定义函数compiler_options()，该函数用于设置编译选项。
# add_dependencies(app entwine)：指定app目标依赖于entwine目标，确保在构建app之前先构建entwine。
# target_link_libraries(app ...)：指定app目标的链接库。这里包括entwine库、PDAL_LIBRARIES库、CMAKE_DL_LIBS库、线程库以及JSONCPP_LIBRARY库。
# if (DEFINED BACKTRACE_DEFS) ... endif (DEFINED BACKTRACE_DEFS)：如果定义了BACKTRACE_DEFS变量，则将Backtrace_LIBRARIES库链接到app目标。
# set_target_properties(app PROPERTIES OUTPUT_NAME entwine)：将app目标的输出名称设置为entwine。
# install(TARGETS app DESTINATION bin)：在构建完成后，将app目标安装到bin目录中。
# 这段代码是一个CMake脚本的一部分，用于构建一个名为app的可执行文件。要完整理解和使用这段代码，还需要查看其他CMake脚本文件以及配置文件中定义的变量和库的定义。

# set(BASE "${CMAKE_CURRENT_SOURCE_DIR}")：将BASE变量设置为当前源代码目录的路径。
set(BASE "${CMAKE_CURRENT_SOURCE_DIR}")
# set(SOURCES ...)：将SOURCES变量设置为一个包含源代码文件路径的列表。这些源代码文件包括build.cpp、entwine.cpp、info.cpp和merge.cpp。注释掉的scan.cpp文件没有包含在构建中。
set(
    SOURCES
    "${BASE}/build.cpp"
    "${BASE}/entwine.cpp"
    "${BASE}/info.cpp"
    "${BASE}/merge.cpp"
    # "${BASE}/scan.cpp"
)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

add_executable(app ${SOURCES})
compiler_options(app)
add_dependencies(app entwine)

target_link_libraries(app
    PRIVATE
        entwine
        ${PDAL_LIBRARIES}
        ${CMAKE_DL_LIBS}
        ${CMAKE_THREAD_LIBS_INIT}
        ${JSONCPP_LIBRARY}
)

if (DEFINED BACKTRACE_DEFS)
  target_link_libraries(app PRIVATE ${Backtrace_LIBRARIES})
endif (DEFINED BACKTRACE_DEFS)

set_target_properties(app PROPERTIES OUTPUT_NAME entwine)

install(TARGETS app DESTINATION bin)

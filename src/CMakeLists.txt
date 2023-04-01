#set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")
find_package(Protobuf 3 REQUIRED)

#设置输出路径
SET(MESSAGE_DIR ${CMAKE_BINARY_DIR}/message)
if(EXISTS "${CMAKE_BINARY_DIR}/message" AND IS_DIRECTORY "${CMAKE_BINARY_DIR}/message")
        SET(PROTO_META_BASE_DIR ${MESSAGE_DIR})
else()
        file(MAKE_DIRECTORY ${MESSAGE_DIR})
        SET(PROTO_META_BASE_DIR ${MESSAGE_DIR})
endif()

#设置protoc的搜索路径
LIST(APPEND PROTO_FLAGS -I${CMAKE_SOURCE_DIR}/proto)
#获取需要编译的proto文件
file(GLOB_RECURSE MSG_PROTOS ${CMAKE_SOURCE_DIR}/proto/*.proto)
set(MESSAGE_SRC "")
set(MESSAGE_HDRS "")

foreach(msg ${MSG_PROTOS})
        get_filename_component(FIL_WE ${msg} NAME_WE)

        list(APPEND MESSAGE_SRC "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.cc")
        list(APPEND MESSAGE_HDRS "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.h")

        # 使用自定义命令
        add_custom_command(
          OUTPUT "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.cc"
                 "${PROJECT_BINARY_DIR}/message/${FIL_WE}.pb.h"
          COMMAND  protoc
          ARGS --cpp_out  ${PROTO_META_BASE_DIR}
            -I ${CMAKE_SOURCE_DIR}/proto
            ${msg}
          DEPENDS ${msg}
          COMMENT "Running C++ protocol buffer compiler on ${msg}"
          VERBATIM
        )
endforeach()

# 设置文件属性为 GENERATED
set_source_files_properties(${MESSAGE_SRC} ${MESSAGE_HDRS} PROPERTIES GENERATED TRUE)

# 添加自定义target
add_custom_target(generate_message ALL
                DEPENDS ${MESSAGE_SRC} ${MESSAGE_HDRS}
                COMMENT "generate message target"
                VERBATIM
                )

# end protobuf


set(BASE_HEADERS
    "include/common/db/db.hpp"
    "include/common/redis/redis.hpp"
    "include/common/ChatProtocol.hpp"
    "include/server/ChatTcpConn.hpp"
    "include/model/userModel.hpp"
    "include/model/friendModel.hpp"
    "include/model/offlineMsgModel.hpp"
    "include/model/offlineGroupMsgModel.hpp"
    "include/model/groupModel.hpp"
    "include/server/ChatServer.hpp"
    "include/server/ChatService.hpp"
)

set(BASE_SRCS
    "db/db.cpp"
    "redis/redis.cpp"
    "server/ChatTcpConn.cpp"
    "server/ChatServer.cpp"
    "server/ChatService.cpp"
    "server/model/userModel.cpp"
    "server/model/friendModel.cpp"
    "server/model/groupModel.cpp"
    "server/model/offlineMsgModel.cpp"
    "server/model/offlineGroupMsgModel.cpp"
)

add_library(utillib STATIC ${BASE_HEADERS} ${BASE_SRCS} ${MESSAGE_SRC})

target_include_directories(utillib
    PUBLIC
        ./include
)

target_include_directories(utillib
    PRIVATE
        $<BUILD_INTERFACE:${MESSAGE_DIR}>
)

target_compile_features(utillib PUBLIC cxx_std_20)

target_compile_options(utillib PRIVATE
            ${CXX_FLAGS}
            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND IS_ENABLE_SANITIZE)
    target_link_options(utillib PRIVATE ${SANITIZE_FLAG})
endif()
#target_compile_options(TinyNet PRIVATE "-fsanitize=address" "-fsanitize=undefined")
target_link_libraries(utillib PRIVATE
    ${Protobuf_LIBRARIES}
    ${Boost_LIBRARIES}
    fmt
    mysqlclient
    nlohmann_json::nlohmann_json
    hiredis
)


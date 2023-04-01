#set(Protobuf_PROTOC_EXECUTABLE "/usr/bin/protoc")
find_package(Protobuf 3 REQUIRED)

#设置输出路径
SET(MESSAGE_DIR ${CMAKE_BINARY_DIR}/message_client)
if(EXISTS "${CMAKE_BINARY_DIR}/message_client" AND IS_DIRECTORY "${CMAKE_BINARY_DIR}/message_client")
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

        list(APPEND MESSAGE_SRC "${PROJECT_BINARY_DIR}/message_client/${FIL_WE}.pb.cc")
        list(APPEND MESSAGE_HDRS "${PROJECT_BINARY_DIR}/message_client/${FIL_WE}.pb.h")

        # 使用自定义命令
        add_custom_command(
          OUTPUT "${PROJECT_BINARY_DIR}/message_client/${FIL_WE}.pb.cc"
                 "${PROJECT_BINARY_DIR}/message_client/${FIL_WE}.pb.h"
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
add_custom_target(generate_message_for_client ALL
                DEPENDS ${MESSAGE_SRC} ${MESSAGE_HDRS}
                COMMENT "generate message for client target"
                VERBATIM
                )

# end protobuf

set(ClientHeader
        ChatClient.hpp
        ChatService.hpp
)

set(ClientSource
        ChatService.cpp
)

add_executable(ChatClient main.cpp ${ClientHeader} ${ClientSource} ${MESSAGE_SRC})

target_compile_features(ChatClient PUBLIC cxx_std_20)

target_include_directories(ChatClient
    PRIVATE
        $<BUILD_INTERFACE:${MESSAGE_DIR}>
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND IS_ENABLE_SANITIZE)
    target_link_options(ChatClient PRIVATE ${SANITIZE_FLAG})
endif()

target_link_libraries(ChatClient PRIVATE
    ${Protobuf_LIBRARIES}
    ${Boost_LIBRARIES}
    fmt
)
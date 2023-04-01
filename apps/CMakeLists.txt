add_subdirectory(client)
add_subdirectory(imgui_client)

add_executable(server app.cpp)

target_link_libraries(server
        utillib
        fmt
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND IS_ENABLE_SANITIZE)
    target_link_options(server PRIVATE ${SANITIZE_FLAG})
endif()
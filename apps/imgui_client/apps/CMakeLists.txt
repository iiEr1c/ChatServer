set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/apps/imgui_client/imgui)
set(phdrs ${IMGUI_DIR}/imgui.h)
set(hdrs
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/imstb_truetype.h
    )

set(srcs
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    )

set(sdl2_opengl3_hdrs
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.hpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.hpp
    )
set(sdl2_opengl3_srcs
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

add_executable(imgui_client main.cpp login.hpp ${srcs} ${sdl2_opengl3_srcs})

target_include_directories(imgui_client PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SDL2_INCLUDE_DIRS}
    )
target_link_libraries(imgui_client
    PRIVATE
        ${SDL2_LIBRARIES}
        ${OPENGL_LIBRARY}
    )
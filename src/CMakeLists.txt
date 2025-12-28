if(WIN32)
add_library(imguilib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp

    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
else()
add_library(imguilib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp

    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
endif()

target_include_directories(imguilib PUBLIC ${imgui_SOURCE_DIR} ${imguizmo_SOURCE_DIR})

add_library(
    ufpslib STATIC
)

add_executable(
    ufps
    main.cpp
)

install(TARGETS ufps DESTINATION bin COMPONENT application)
install(FILES ${PROJECT_BINARY_DIR}/resources DESTINATION bin COMPONENT application)

add_subdirectory(core)
add_subdirectory(events)
add_subdirectory(graphics)
add_subdirectory(math)
# add_subdirectory(resources)
add_subdirectory(utils)

configure_file(${PROJECT_SOURCE_DIR}/include/config.h.in ${PROJECT_SOURCE_DIR}/include/config.h)

if(WIN32)
    target_sources(
        ufpslib
        PRIVATE win32/window.cpp win32/debug_ui.cpp win32/file_resource_loader.cpp
        # PRIVATE win32/window.cpp win32/debug_ui.cpp win32/file.cpp win32/sound_routine.cpp
    )
    # target_sources(
    #     imguilib
    #     PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    # )
elseif(UNIX)
    target_sources(
        ufpslib
        PRIVATE posix/window.cpp posix/debug_ui.cpp posix/file.cpp posix/file_resource_loader.cpp
        # PRIVATE posix/window.cpp posix/debug_ui.cpp posix/file.cpp posix/sound_routine.cpp
    )
    # target_sources(
    #     imguilib
    #     PRIVATE ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    # )
endif()

# target_include_directories(ufpslib PUBLIC ${stb_lib_SOURCE_DIR} ${imgui_SOURCE_DIR} ${imguizmo_SOURCE_DIR} ${JoltPhysics_SOURCE_DIR} ${lua_SOURCE_DIR})
target_include_directories(ufpslib PUBLIC ${stb_lib_SOURCE_DIR})

# if(${Freetype_FOUND})
# target_include_directories(ufpslib PUBLIC ${FREETYPE_INCLUDE_DIRS})
# else()
# target_include_directories(ufpslib PUBLIC ${freetype_SOURCE_DIR}/include)
# endif()

target_compile_features(ufpslib PUBLIC cxx_std_23)
target_compile_features(ufps PUBLIC cxx_std_23)

if(MSVC)
target_include_directories(ufpslib PUBLIC third_party)
target_compile_options(ufpslib PUBLIC /W4 /WX /Debug /Od)
target_compile_definitions(ufpslib PRIVATE -DWIN32 -D_WIN32 -DNOMINMAX)
target_compile_options(ufps PUBLIC  /W4 /WX /Debug /Od)
target_compile_definitions(ufps PRIVATE -DWIN32 -D_WIN32 -DNOMINMAX)

# target_include_directories(ufpslib PRIVATE ${zstd_SOURCE_DIR}/lib)
# target_include_directories(ufps PRIVATE ${zstd_SOURCE_DIR}/lib)
else()
target_compile_options(ufpslib PUBLIC -Wall -Wextra -pedantic -Werror   -Wcast-qual -Wconversion-null -Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings)
target_compile_options(ufps PUBLIC -Wall -Wextra -pedantic -Werror   -Wcast-qual -Wconversion-null -Wmissing-declarations -Woverlength-strings -Wpointer-arith -Wunused-local-typedefs -Wunused-result -Wvarargs -Wvla -Wwrite-strings)
endif()

# if(NOT ${Freetype_FOUND})
# target_include_directories(ufpslib PRIVATE ${freetype_SOURCE_DIR}/include)
# endif()

if(WIN32)
target_link_libraries(
    ufpslib
    PUBLIC imguilib     
    # PUBLIC imguilib Jolt opengl32 lua
    PRIVATE dwmapi
    # PRIVATE libzstd_static ${FREETYPE_LIBRARY_NAME}
)
target_link_libraries(
    ufps
    PUBLIC ufpslib
    PRIVATE opengl32
)
elseif(UNIX)
target_link_libraries(
    ufpslib
    PUBLIC imguilib
    # PUBLIC imguilib Jolt lua
    PRIVATE GL glfw Backtrace::Backtrace stdc++exp
    # PRIVATE GL glfw Backtrace::Backtrace stdc++exp libzstd_static ${FREETYPE_LIBRARY_NAME}
)
target_link_libraries(
    ufps
    PUBLIC ufpslib 
    PRIVATE Backtrace::Backtrace stdc++exp
)
endif()

# add_custom_command(
#     OUTPUT ${PROJECT_BINARY_DIR}/resources
#     COMMAND $<TARGET_FILE:resource_packer> ${PROJECT_SOURCE_DIR}/assets ${PROJECT_BINARY_DIR}/resources
#     COMMENT "Building resources"
# )
# add_custom_target(resources DEPENDS ${PROJECT_BINARY_DIR}/resources COMMENT "Building resources")
# add_dependencies(resources resource_packer ufpslib)

# add_dependencies(ufps resources)

cmake_minimum_required(VERSION 3.30)

project(
    ufps
    VERSION 0.0.1
    DESCRIPTION "A simple C++/OpenGL FPS following Nathan's stream"
    LANGUAGES C CXX
)

set(CMAKE_CONFIGURATION_TYPES Debug;Release)

include(FetchContent)

set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)

if(MSVC)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)

FetchContent_GetProperties(googletest)

if (MSVC)
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "" FORCE)
	if (USE_STATIC_MSVC_RUNTIME_LIBRARY)
		set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif()
    set(USE_STATIC_MSVC_RUNTIME_LIBRARY ON CACHE BOOL "" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(CPP_RTTI_ENABLED ON CACHE BOOL "" FORCE)
set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)

# FetchContent_Declare(
#     JoltPhysics
#     GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
#     GIT_TAG v5.3.0
#     SOURCE_SUBDIR "Build"
# )

# FetchContent_MakeAvailable(JoltPhysics)


if(UNIX)
find_package(glfw3 REQUIRED)
find_package(Backtrace REQUIRED)
endif()

# find_package(Freetype)
# if(${Freetype_FOUND})
#     set(FREETYPE_LIBRARY_NAME Freetype::Freetype)
# else()
#     FetchContent_Declare(
#         freetype
#         GIT_REPOSITORY https://github.com/freetype/freetype.git
#         GIT_TAG VER-2-14-1    
#     )
#     FetchContent_MakeAvailable(freetype)
#     set(FREETYPE_LIBRARY_NAME freetype)
# endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(FETCHCONTENT_QUIET FALSE)

include_directories(include)
if(WIN32)
include_directories(third_party)
endif()


set(LOG_TO_FILE true)
set(LOG_ENABLE_DEBUG true)

## TODO ##
# foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
#     string(TOUPPER ${CONFIG_TYPE} CONFIG_TYPE_UPPER)

#     set(IS_DEBUG FALSE)
#     if(CONFIG_TYPE_UPPER STREQUAL "DEBUG")
#         set(IS_DEBUG TRUE)
#     endif()

#     set(CONFIG_FILE_INPUT  "${CMAKE_CURRENT_SOURCE_DIR}/build_config.h.in")
#     set(CONFIG_FILE_OUTPUT "${GENERATED_SOURCE_DIR}/build_config_${CONFIG_TYPE}.h")

#     configure_file(${CONFIG_FILE_INPUT} ${CONFIG_FILE_OUTPUT} @ONLY)
# endforeach()

# # Add include directory so the generated headers can be found
# target_include_directories(MyTarget PRIVATE "${GENERATED_SOURCE_DIR}")

# # Use generator expression to include correct header for each config


add_subdirectory(src)
# add_subdirectory(tools)

enable_testing()
include(CTest)
add_subdirectory(tests)

set(CPACK_PACKAGE_NAME "ufps")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP")
set(CPACK_ARCHIVE_COMPONENT_INSTALL OFF)
set(CPACK_SOURCE_IGNORE_FILES "build")
set(CPACK_COMPONENTS_ALL application)
include(CPack)


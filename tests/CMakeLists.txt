FetchContent_MakeAvailable(googletest)

include(GoogleTest)

mark_as_advanced(BUILD_GMOCK BUILD_GTEST gtest_hide_internal_symbols)

add_executable(unit_tests
    ensure_tests.cpp
)

if(MSVC)
target_include_directories(unit_tests PRIVATE ${zstd_SOURCE_DIR}/lib)
endif()

if(${Freetype_FOUND})
target_include_directories(ufpslib PUBLIC ${FREETYPE_INCLUDE_DIRS})
else()
target_include_directories(ufpslib PUBLIC ${freetype_SOURCE_DIR}/include)
endif()

if(MSVC)
target_compile_options(unit_tests PUBLIC /W4 /WX /Debug /Od)
target_compile_definitions(unit_tests PRIVATE -DWIN32 -D_WIN32 -DNOMINMAX)
endif()

target_link_libraries(unit_tests gmock_main ufpslib)
gtest_discover_tests(unit_tests DISCOVERY_MODE PRE_TEST)

add_dependencies(unit_tests ufpslib)

# add_custom_command(
#     TARGET unit_tests
#     POST_BUILD
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     COMMAND ${CMAKE_CTEST_COMMAND} -C $<CONFIGURATION> -R "^${UNIT_TEST}$" --output-on-failure -j
# )
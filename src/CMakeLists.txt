cmake_minimum_required(VERSION 3.13)

set(srcs main.h main.cpp xxhash.c xxhash.h)

file(GLOB_RECURSE ts_srcs "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.ui")
qt6_create_translation(QM_FILES ${ts_srcs} main_en.ts)

add_library(plugin_sanitycheck SHARED ${srcs} ${QM_FILES})

# simply link uibase
target_link_libraries(plugin_sanitycheck uibase)

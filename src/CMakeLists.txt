cmake_minimum_required(VERSION 3.16)

find_package(mo2-cmake CONFIG REQUIRED)
find_package(mo2-uibase CONFIG REQUIRED)

add_library(plugin_sanitycheck SHARED
	main.h main.cpp xxhash.h xxhash.c
)
mo2_configure_plugin(plugin_sanitycheck NO_SOURCES WARNINGS 4)
target_link_libraries(plugin_sanitycheck PRIVATE mo2::uibase)
mo2_install_plugin(plugin_sanitycheck)

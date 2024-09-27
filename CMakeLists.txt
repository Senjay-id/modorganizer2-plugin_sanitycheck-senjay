cmake_minimum_required(VERSION 3.13)

include(FetchContent)

# name of your project
project(plugin_sanitycheck)

set(MO2_VERSION "2.5.2")

# MO2 uibase settings
# WARNING: Do not modify this, or at your own risk.

FetchContent_Declare(
  uibase
  URL      "https://github.com/ModOrganizer2/modorganizer/releases/download/v${MO2_VERSION}/Mod.Organizer-${MO2_VERSION}-uibase.7z"
  DOWNLOAD_EXTRACT_TIMESTAMP true
)
FetchContent_MakeAvailable(uibase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE RelWithDebInfo)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets LinguistTools REQUIRED)

add_library(uibase STATIC IMPORTED)
set_target_properties(uibase PROPERTIES
    IMPORTED_LOCATION "${uibase_SOURCE_DIR}/uibase.lib")
target_link_libraries(uibase INTERFACE Qt6::Widgets)
target_include_directories(uibase INTERFACE "${uibase_SOURCE_DIR}")

# END: uibase settings

add_subdirectory(src)

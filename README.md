# Sanity Check C++ Plugin

**Note:** This currently targets development MO2, and will only work with a nightly
build of MO2.

## Requirements

You will need:

- A proper MO2 installation - You **do not** need to build ModOrganizer2 itself, but you
  will need a version of MO2 that matches the latest version of
  [`uibase`](https://github.com/ModOrganizer2/modorganizer-uibase) available.
- Qt with a version matching the one used in `uibase` (currently 6.7.0).
- CMake and VCPKG.

## Quick build
* Run buildvcpkg.bat
* Run build.bat

## How to build?

To build and install the plugin, simply run:

```pwsh
# set CMAKE_PREFIX_PATH to the appropriate location and CMAKE_INSTALL_PREFIX to the
# folder containing your MO2 installation (the one with ModOrganizer.exe)
cmake --preset vs2022-windows-standalone "-DCMAKE_PREFIX_PATH=C:\Qt\6.7.3\msvc2022_64\" "-DCMAKE_INSTALL_PREFIX=C:\Modding\MO2"

# this will build the project and install the plugins under plugins/ in your MO2
# installation, making it available after the next restart of MO2
cmake --build vsbuild --config RelWithDebInfo --target INSTALL
```

Unless you build MO2 itself in debug mode, you can only build in `Release` or
`RelWithDebInfo` mode.

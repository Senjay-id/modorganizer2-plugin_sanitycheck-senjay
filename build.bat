# set CMAKE_PREFIX_PATH to the appropriate location and CMAKE_INSTALL_PREFIX to the
# folder containing your MO2 installation (the one with ModOrganizer.exe)
cmake --preset vs2022-windows-standalone "-DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake" "-DCMAKE_PREFIX_PATH=C:\\Qt\\6.7.3\\msvc2022_64\\" "-DCMAKE_INSTALL_PREFIX=C:\\modding\\mo2"

# this will build the project and install the plugins under plugins/ in your MO2
# installation, making it available after the next restart of MO2
# cmake --build vsbuild --config RelWithDebInfo --target INSTALL

PAUSE
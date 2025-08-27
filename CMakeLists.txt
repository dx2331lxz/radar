cmake_minimum_required(VERSION 3.21)
project(radar VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Qt automatic processing of MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6 installed via Homebrew or official installer
find_package(Qt6 6.2 COMPONENTS Widgets Network REQUIRED)

add_executable(radar
    src/main.cpp
    src/RadarConfigWidget.cpp
    src/NetworkManager.cpp
)

target_link_libraries(radar PRIVATE Qt6::Widgets Qt6::Network)

# On macOS, make sure app can run from build dir
if(APPLE)
    # Avoid forcing bundle for easy terminal run
    set_target_properties(radar PROPERTIES MACOSX_BUNDLE FALSE)
endif()

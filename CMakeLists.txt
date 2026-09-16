cmake_minimum_required(VERSION 3.21)
project(TransactionPhotoPro LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui PrintSupport)

set(APP_SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    resources/resources.qrc
)

if(WIN32)
    list(APPEND APP_SOURCES resources/app.rc)
endif()

add_executable(TransactionPhotoPro ${APP_SOURCES})
target_link_libraries(TransactionPhotoPro PRIVATE Qt6::Widgets Qt6::Gui Qt6::PrintSupport)
if(WIN32)
  set_target_properties(TransactionPhotoPro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

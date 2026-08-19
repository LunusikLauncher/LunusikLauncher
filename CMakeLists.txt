cmake_minimum_required(VERSION 3.16)
project(LunusikLauncher)

set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg Network Gui)

qt_standard_project_setup()

find_package(LibLZMA REQUIRED)

add_executable(LunusikLauncher WIN32 main.cpp 
                mainwindow.cpp mainwindow.h 
                manager.cpp manager.h 
                filescontrol.h filescontrol.cpp 
                minecraftmanager.h minecraftmanager.cpp 
                downloadmanager.h downloadmanager.cpp 
                versionsmanager.h versionsmanager.cpp 
                libs.h 
                resources.qrc resources.rc)

target_link_libraries(LunusikLauncher PRIVATE 
    Qt6::Widgets 
    Qt6::Svg 
    Qt6::Network 
    Qt6::GuiPrivate 
    LibLZMA::LibLZMA)
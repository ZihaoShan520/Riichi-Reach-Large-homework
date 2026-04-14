cmake_minimum_required(VERSION 3.19)
project(Riichi_Reach LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Gui)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt_add_executable(Riichi_Reach
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    core/tile.h
    core/hand_manager.h
    manager/gamemanager.h
    manager/gamemanager.cpp
)

target_link_libraries(Riichi_Reach
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
)

target_compile_options(Riichi_Reach PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra -pedantic>
)

target_compile_definitions(Riichi_Reach PRIVATE
    PROJECT_NAME="Riichi_Reach"
    $<$<CONFIG:Debug>:DEBUG_BUILD>
)

include(GNUInstallDirs)

install(TARGETS Riichi_Reach
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Riichi_Reach
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

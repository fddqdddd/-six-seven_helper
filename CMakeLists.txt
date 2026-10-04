cmake_minimum_required(VERSION 3.16)
project(Six_Seven VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

configure_file(src/six_seven.rc.in ${CMAKE_BINARY_DIR}/six_seven.rc @ONLY)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -municode")
endif()

set(SIX_SEVEN_SOURCES
    src/main.cpp
    src/Application.cpp
    src/Util.cpp
    src/ConfigTables.cpp
    src/SpriteEngine.cpp
    src/ActionRunner.cpp
    src/SpeechBubble.cpp
    src/SpeechEngine.cpp
    src/AudioEngine.cpp
    src/Phrases.cpp
    src/MovementEngine.cpp
    src/Settings.cpp
    src/UserInformation.cpp
    src/NameValidator.cpp
    src/Songs.cpp
    src/SongCatalog.cpp
    src/SongMelody.cpp
    src/RussianSyllables.cpp
    src/TrayIcon.cpp
    src/MiniGameRecords.cpp
    src/MiniGameManager.cpp
    src/MiniGameMemory.cpp
    src/BootSplash.cpp
    src/BootLogoInstaller.cpp
    src/ProfileCustomizer.cpp
    src/TerminalGuard.cpp
    src/TerminalCommands.cpp
    src/DeepSeekClient.cpp
)

add_executable(Six_Seven_BootLogo WIN32
    src/boot_logo_main.cpp
    src/BootLogoInstaller.cpp
    src/Util.cpp
)

add_executable(Six_Seven_Profile WIN32
    src/profile_main.cpp
    src/ProfileCustomizer.cpp
    src/Util.cpp
)

add_executable(Six_Seven_VideoBoot WIN32
    src/video_boot_main.cpp
    src/VideoBootSplash.cpp
    src/Util.cpp
)

add_executable(Six_Seven WIN32 ${SIX_SEVEN_SOURCES} ${CMAKE_BINARY_DIR}/six_seven.rc)

target_include_directories(Six_Seven PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_include_directories(Six_Seven_BootLogo PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Six_Seven_BootLogo PRIVATE
    user32
    gdi32
    shell32
    gdiplus
)

target_include_directories(Six_Seven_Profile PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Six_Seven_Profile PRIVATE
    user32
    gdi32
    shell32
    gdiplus
    ole32
    advapi32
    secur32
)

target_include_directories(Six_Seven_VideoBoot PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Six_Seven_VideoBoot PRIVATE
    user32
    gdi32
    shell32
    winmm
    advapi32
)

target_link_libraries(Six_Seven PRIVATE
    user32
    gdi32
    shell32
    ole32
    uuid
    gdiplus
    winmm
    sapi
    comctl32
    advapi32
    secur32
    winhttp
)

add_custom_command(TARGET Six_Seven POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:Six_Seven>/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/six_seven.ini.example
        $<TARGET_FILE_DIR:Six_Seven>/six_seven.ini.example
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/mini_games_records.ini.example
        $<TARGET_FILE_DIR:Six_Seven>/mini_games_records.ini.example
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/tools/HackBGRT
        $<TARGET_FILE_DIR:Six_Seven>/HackBGRT
    COMMENT "Copy assets"
)

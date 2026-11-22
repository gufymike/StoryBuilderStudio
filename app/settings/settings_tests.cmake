#    if(MINGW)
#        target_link_options(${TEST_TARGET} PRIVATE -mconsole)
#    elseif(MSVC)
#	target_link_options(${TEST_TARGET} PRIVATE /SUBSYSTEM:CONSOLE)
#    endif()
add_executable(test_SettingsManager
        tests/test_SettingsManager.cpp
        app/settings/settingsmanager.cpp
        app/settings/settingsmanager.h
    )

target_include_directories(test_SettingsManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

target_link_libraries(test_SettingsManager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_SettingsManager COMMAND test_SettingsManager)


# --- test_LocalModelManifestCard ---
add_executable(test_LocalModelManifestCard
    tests/test_LocalModalManifestCard.cpp
)

target_compile_definitions(test_LocalModelManifestCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_LocalModelManifestCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
)

if(WIN32)
    set_target_properties(test_LocalModelManifestCard PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_LocalModelManifestCard PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_LocalModelManifestCard PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_LocalModelManifestCard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_LocalModelManifestCard COMMAND test_LocalModelManifestCard)


# --- test_SettingsControlPanel ---
add_executable(test_SettingsControlPanel
    tests/test_SettingsControlPanel.cpp
)

target_compile_definitions(test_SettingsControlPanel PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_SettingsControlPanel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
)

if(WIN32)
    set_target_properties(test_SettingsControlPanel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_SettingsControlPanel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_SettingsControlPanel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_SettingsControlPanel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_SettingsControlPanel COMMAND test_SettingsControlPanel)

# --- test_RemoteApiConfigCard ---
add_executable(test_RemoteApiConfigCard
    tests/test_RemoteApiConfigCard.cpp
)

target_compile_definitions(test_RemoteApiConfigCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_RemoteApiConfigCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
)

if(WIN32)
    set_target_properties(test_RemoteApiConfigCard PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_RemoteApiConfigCard PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_RemoteApiConfigCard PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_RemoteApiConfigCard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_RemoteApiConfigCard COMMAND test_RemoteApiConfigCard)

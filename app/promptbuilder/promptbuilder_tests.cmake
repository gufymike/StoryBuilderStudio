# Copyright (c) 2026 Mike Ramirez
# SPDX-License-Identifier: MIT

# --- test_PromptHistoryStore ---
add_executable(test_PromptHistoryStore
    tests/test_PromptHistoryStore.cpp
    app/promptbuilder/prompthistorystore.h
    app/promptbuilder/prompthistorystore.cpp
)

target_include_directories(test_PromptHistoryStore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptHistoryStore PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptHistoryStore PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptHistoryStore PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptHistoryStore PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_PromptHistoryStore COMMAND test_PromptHistoryStore)

# --- test_PromptManager ---
add_executable(test_PromptManager
    tests/test_PromptManager.cpp
    app/promptbuilder/promptmanager.h
    app/promptbuilder/promptmanager.cpp
)

target_include_directories(test_PromptManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptManager PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptManager PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptManager PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptManager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_PromptManager COMMAND test_PromptManager)

# --- test_PromptHistoryViewerDialog ---
add_executable(test_PromptHistoryViewerDialog
    tests/test_PromptHistoryViewerDialog.cpp
)

target_compile_definitions(test_PromptHistoryViewerDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_PromptHistoryViewerDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptHistoryViewerDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptHistoryViewerDialog PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptHistoryViewerDialog PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptHistoryViewerDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_PromptHistoryViewerDialog COMMAND test_PromptHistoryViewerDialog)

# --- test_PromptEditorDialog ---
add_executable(test_PromptEditorDialog
    tests/test_PromptEditorDialog.cpp
)

target_compile_definitions(test_PromptEditorDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_PromptEditorDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptEditorDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptEditorDialog PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptEditorDialog PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptEditorDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_PromptEditorDialog COMMAND test_PromptEditorDialog)

# --- test_PromptOptionsPanel ---
add_executable(test_PromptOptionsPanel
    tests/test_PromptOptionsPanel.cpp
)

target_compile_definitions(test_PromptOptionsPanel PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_PromptOptionsPanel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptOptionsPanel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptOptionsPanel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptOptionsPanel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptOptionsPanel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_PromptOptionsPanel COMMAND test_PromptOptionsPanel)

add_executable(test_PromptBuilder
    tests/test_PromptBuilder.cpp
)

target_compile_definitions(test_PromptBuilder PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_PromptBuilder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
)

if(WIN32)
    set_target_properties(test_PromptBuilder PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_PromptBuilder PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_PromptBuilder PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_PromptBuilder PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_PromptBuilder COMMAND test_PromptBuilder)


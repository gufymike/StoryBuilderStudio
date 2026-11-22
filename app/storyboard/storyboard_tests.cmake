
# --- test_StoryDocument ---
add_executable(test_StoryDocument
    tests/test_StoryDocument.cpp
    app/storyboard/storydocument.h
    app/storyboard/storydocument.cpp
)

target_include_directories(test_StoryDocument PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)

if(WIN32)
    set_target_properties(test_StoryDocument PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryDocument PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryDocument PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_StoryDocument PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_StoryDocument COMMAND test_StoryDocument)

# --- test_StoryBoardManager ---
add_executable(test_StoryBoardManager
    tests/test_StoryBoardManager.cpp
    app/storyboard/storyboardmanager.h
    app/storyboard/storyboardmanager.cpp
)

target_include_directories(test_StoryBoardManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)

if(WIN32)
    set_target_properties(test_StoryBoardManager PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryBoardManager PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardManager PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_StoryBoardManager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_StoryBoardManager COMMAND test_StoryBoardManager)

add_executable(test_StoryBoardAssetList
    tests/test_StoryBoardAssetList.cpp
)
target_compile_definitions(test_StoryBoardAssetList PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_StoryBoardAssetList PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)
if(WIN32)
    set_target_properties(test_StoryBoardAssetList PROPERTIES WIN32_EXECUTABLE FALSE)
        if(MINGW)
        target_link_options(test_StoryBoardAssetList PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardAssetList PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_StoryBoardAssetList PRIVATE
    Qt6::Core 
    Qt6::Gui
    Qt6::Qml 
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_StoryBoardAssetList COMMAND test_StoryBoardAssetList)

add_executable(test_StoryBoardAssetPickerModal
    tests/test_StoryBoardAssetPickerModal.cpp
)
target_compile_definitions(test_StoryBoardAssetPickerModal PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_StoryBoardAssetPickerModal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
)
if(WIN32)
    set_target_properties(test_StoryBoardAssetPickerModal PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryBoardAssetPickerModal PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardAssetPickerModal PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_StoryBoardAssetPickerModal PRIVATE
    Qt6::Core 
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_StoryBoardAssetPickerModal COMMAND test_StoryBoardAssetPickerModal)

add_executable(test_StoryBoardShotDetailDialog
    tests/test_StoryBoardShotDetailDialog.cpp
)
target_compile_definitions(test_StoryBoardShotDetailDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_StoryBoardShotDetailDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
)
if(WIN32)
    set_target_properties(test_StoryBoardShotDetailDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryBoardShotDetailDialog PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardShotDetailDialog PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_StoryBoardShotDetailDialog PRIVATE
    Qt6::Core 
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test 
    StoryBuilderStudioLib
)
add_test(NAME test_StoryBoardShotDetailDialog COMMAND test_StoryBoardShotDetailDialog)

add_executable(test_StoryBoardCard
    tests/test_StoryBoardCard.cpp
)
target_compile_definitions(test_StoryBoardCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_StoryBoardCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)
if(WIN32)
    set_target_properties(test_StoryBoardCard PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryBoardCard PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardCard PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_StoryBoardCard PRIVATE
    Qt6::Core 
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_StoryBoardCard COMMAND test_StoryBoardCard)

add_executable(test_StoryBoardWorkspaceView
    tests/test_StoryBoardWorkspaceView.cpp
)
target_compile_definitions(test_StoryBoardWorkspaceView PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_StoryBoardWorkspaceView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
)
if(WIN32)
    set_target_properties(test_StoryBoardWorkspaceView PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_StoryBoardWorkspaceView PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_StoryBoardWorkspaceView PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_StoryBoardWorkspaceView PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_StoryBoardWorkspaceView COMMAND test_StoryBoardWorkspaceView)

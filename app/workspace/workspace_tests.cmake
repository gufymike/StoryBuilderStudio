
# --- test_ScriptsDirectoryModel ---
add_executable(test_ScriptsDirectoryModel
    tests/test_ScriptsDirectoryModel.cpp
    app/workspace/abstractassetdirectorymodel.h
    app/workspace/abstractassetdirectorymodel.cpp
    app/workspace/scriptsdirectorymodel.h
)

target_include_directories(test_ScriptsDirectoryModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_ScriptsDirectoryModel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_ScriptsDirectoryModel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_ScriptsDirectoryModel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_ScriptsDirectoryModel PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_ScriptsDirectoryModel COMMAND test_ScriptsDirectoryModel)

# --- test_AudioDirectoryModel ---
add_executable(test_AudioDirectoryModel
    tests/test_AudioDirectoryModel.cpp
    app/workspace/abstractassetdirectorymodel.h
    app/workspace/abstractassetdirectorymodel.cpp
    app/workspace/audiodirectorymodel.h
)

target_include_directories(test_AudioDirectoryModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_AudioDirectoryModel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AudioDirectoryModel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AudioDirectoryModel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AudioDirectoryModel PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_AudioDirectoryModel COMMAND test_AudioDirectoryModel)

# --- test_ImageDirectoryModel ---
add_executable(test_ImageDirectoryModel
    tests/test_ImageDirectoryModel.cpp
    app/workspace/abstractassetdirectorymodel.h
    app/workspace/abstractassetdirectorymodel.cpp
    app/workspace/imagedirectorymodel.h
    app/workspace/imagedirectorymodel.cpp
)

target_include_directories(test_ImageDirectoryModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_ImageDirectoryModel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_ImageDirectoryModel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_ImageDirectoryModel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_ImageDirectoryModel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_ImageDirectoryModel COMMAND test_ImageDirectoryModel)

# --- test_VideoDirectoryModel ---
add_executable(test_VideoDirectoryModel
    tests/test_VideoDirectoryModel.cpp
    app/workspace/abstractassetdirectorymodel.h
    app/workspace/abstractassetdirectorymodel.cpp
    app/workspace/videodirectorymodel.h
)

target_include_directories(test_VideoDirectoryModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_VideoDirectoryModel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_VideoDirectoryModel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_VideoDirectoryModel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_VideoDirectoryModel PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_VideoDirectoryModel COMMAND test_VideoDirectoryModel)

add_executable(test_AssetContextMenu
    tests/test_AssetContextMenu.cpp
)

target_compile_definitions(test_AssetContextMenu PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_AssetContextMenu PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_AssetContextMenu PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AssetContextMenu PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AssetContextMenu PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AssetContextMenu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_AssetContextMenu COMMAND test_AssetContextMenu)

add_executable(test_AssetImportAction
    tests/test_AssetImportAction.cpp
)

target_compile_definitions(test_AssetImportAction PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_AssetImportAction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_AssetImportAction PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AssetImportAction PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AssetImportAction PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AssetImportAction PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_AssetImportAction COMMAND test_AssetImportAction)

add_executable(test_AssetWorkspaceSplitView
    tests/test_AssetWorkspaceSplitView.cpp
)

target_compile_definitions(test_AssetWorkspaceSplitView PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_AssetWorkspaceSplitView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
)

if(WIN32)
    set_target_properties(test_AssetWorkspaceSplitView PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AssetWorkspaceSplitView PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AssetWorkspaceSplitView PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AssetWorkspaceSplitView PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_AssetWorkspaceSplitView COMMAND test_AssetWorkspaceSplitView)

add_executable(test_AudioWaveformModel
    tests/test_AudioWaveformModel.cpp
    app/workspace/audio/audiowaveformmodel.h
    app/workspace/audio/audiowaveformmodel.cpp
)

target_compile_definitions(test_AudioWaveformModel PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_AudioWaveformModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
)

if(WIN32)
    set_target_properties(test_AudioWaveformModel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AudioWaveformModel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AudioWaveformModel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AudioWaveformModel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_AudioWaveformModel COMMAND test_AudioWaveformModel)

add_executable(test_AudioWorkspaceWidget
    tests/test_AudioWorkspaceWidget.cpp
)


target_compile_definitions(test_AudioWorkspaceWidget PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_AudioWorkspaceWidget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
)

if(WIN32)
    set_target_properties(test_AudioWorkspaceWidget PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_AudioWorkspaceWidget PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_AudioWorkspaceWidget PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_AudioWorkspaceWidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_AudioWorkspaceWidget COMMAND test_AudioWorkspaceWidget)

add_executable(test_EditorPane
    tests/test_EditorPane.cpp
)

target_compile_definitions(test_EditorPane PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_EditorPane PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/text
)

if(WIN32)
    set_target_properties(test_EditorPane PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_EditorPane PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_EditorPane PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_EditorPane PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_EditorPane COMMAND test_EditorPane)

add_executable(test_ImageCandidateModal
    tests/test_ImageCandidateModal.cpp
)
target_compile_definitions(test_ImageCandidateModal PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_ImageCandidateModal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
)
if(WIN32)
    set_target_properties(test_ImageCandidateModal PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_ImageCandidateModal PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_ImageCandidateModal PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_ImageCandidateModal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_ImageCandidateModal COMMAND test_ImageCandidateModal)

add_executable(test_ImageGalleryWidget
    tests/test_ImageGalleryWidget.cpp
)
target_compile_definitions(test_ImageGalleryWidget PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_ImageGalleryWidget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
)
if(WIN32)
    set_target_properties(test_ImageGalleryWidget PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_ImageGalleryWidget PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_ImageGalleryWidget PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_ImageGalleryWidget PRIVATE
    Qt6::Core 
    Qt6::Gui 
    Qt6::Qml
    Qt6::Quick 
    Qt6::Test 
    StoryBuilderStudioLib
)
add_test(NAME test_ImageGalleryWidget COMMAND test_ImageGalleryWidget)

add_executable(test_VideoWorkspaceWidget
    tests/test_VideoWorkspaceWidget.cpp
)

target_compile_definitions(test_VideoWorkspaceWidget PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_VideoWorkspaceWidget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
)

if(WIN32)
    set_target_properties(test_VideoWorkspaceWidget PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_VideoWorkspaceWidget PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_VideoWorkspaceWidget PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_VideoWorkspaceWidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_VideoWorkspaceWidget COMMAND test_VideoWorkspaceWidget)

add_executable(test_WorkspaceView
    tests/test_WorkspaceView.cpp
)
target_compile_definitions(test_WorkspaceView PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)
target_include_directories(test_WorkspaceView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/text
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)
if(WIN32)
    set_target_properties(test_WorkspaceView PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_WorkspaceView PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_WorkspaceView PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()
target_link_libraries(test_WorkspaceView PRIVATE
    Qt6::Core
    Qt6::Gui 
    Qt6::Qml 
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)
add_test(NAME test_WorkspaceView COMMAND test_WorkspaceView)


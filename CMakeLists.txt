# Copyright (c) 2026 Mike Ramirez
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.16)

project(StoryBuilderStudio VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)

if(UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    set(CMAKE_BUILD_RPATH "$ORIGIN:$ORIGIN/../lib")
endif()

# ============================================================================
# 1. Qt Dependencies & Standard Setup
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS
    Quick Qml Gui Concurrent Multimedia Network QuickControls2 Test QuickTest
)
qt_standard_project_setup(REQUIRES 6.8)

# ============================================================================
# 2. Local Presets & Binary Search Paths
# ============================================================================
set(LOCAL_INCLUDE_PATHS "" CACHE STRING "Semicolon-separated include paths (like /usr/include)")
set(LOCAL_DLL_PATHS     "" CACHE STRING "Semicolon-separated library paths (like /usr/lib)")

if(LOCAL_INCLUDE_PATHS)
    separate_arguments(CLEAN_INC_PATHS NATIVE_COMMAND "${LOCAL_INCLUDE_PATHS}")
    if(NOT CLEAN_INC_PATHS)
        set(CLEAN_INC_PATHS ${LOCAL_INCLUDE_PATHS})
    endif()

    include_directories(SYSTEM ${CLEAN_INC_PATHS})
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-I$<JOIN:${CLEAN_INC_PATHS},;-I>>")
endif()

if(LOCAL_DLL_PATHS)
    separate_arguments(CLEAN_LIB_PATHS NATIVE_COMMAND "${LOCAL_DLL_PATHS}")
    if(NOT CLEAN_LIB_PATHS)
        set(CLEAN_LIB_PATHS ${LOCAL_DLL_PATHS})
    endif()

    link_directories(${CLEAN_LIB_PATHS})
endif()

if(WIN32)
    enable_language(RC)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
    set_source_files_properties("${APP_ICON_RESOURCE}" PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/app.ico"
    )
else()
    set(APP_ICON_RESOURCE "")
endif()

# ============================================================================
# 3. Resolve Native Inference Libraries
# ============================================================================
if(WIN32)
    find_file(LLAMA_DLL               NAMES llama.dll libllama.dll                 PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_DLL                NAMES ggml.dll libggml.dll                   PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_BASE_DLL           NAMES ggml-base.dll libggml-base.dll         PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_CPU_DLL            NAMES ggml-cpu.dll libggml-cpu.dll           PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(SD_LINK_LIB             NAMES stable-diffusion.lib libstable-diffusion.dll.a stable-diffusion.dll libstable-diffusion.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(AUDIOCPP_DLL            NAMES audiocpp.lib libaudiocpp.dll.a audiocpp.dll libaudiocpp.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(SHERPA_ONNX_BIN         NAMES sherpa-onnx-c-api.lib libsherpa-onnx-c-api.dll.a sherpa-onnx-c-api.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(ONNXRUNTIME_BIN         NAMES onnxruntime.lib libonnxruntime.dll.a onnxruntime.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(SHERPA_ONNX_RUNTIME_DLL NAMES sherpa-onnx-c-api.dll                  PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(ONNXRUNTIME_RUNTIME_DLL NAMES onnxruntime.dll                        PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
else()
    find_library(LLAMA_DLL       NAMES llama             HINTS ${CLEAN_LIB_PATHS})
    find_library(GGML_DLL        NAMES ggml              HINTS ${CLEAN_LIB_PATHS})
    find_library(GGML_BASE_DLL   NAMES ggml-base         HINTS ${CLEAN_LIB_PATHS})
    find_library(GGML_CPU_DLL    NAMES ggml-cpu          HINTS ${CLEAN_LIB_PATHS})
    find_library(SD_LINK_LIB     NAMES stable-diffusion  HINTS ${CLEAN_LIB_PATHS})
    find_library(AUDIOCPP_DLL    NAMES audiocpp          HINTS ${CLEAN_LIB_PATHS})
    find_library(SHERPA_ONNX_BIN NAMES sherpa-onnx-c-api HINTS ${CLEAN_LIB_PATHS})
    find_library(ONNXRUNTIME_BIN NAMES onnxruntime       HINTS ${CLEAN_LIB_PATHS})
endif()

set(FOUND_ENGINE_DLLS
    "${LLAMA_DLL}"
    "${GGML_DLL}"
    "${GGML_BASE_DLL}"
    "${GGML_CPU_DLL}"
    "${SD_LINK_LIB}"
    "${AUDIOCPP_DLL}"
    "${SHERPA_ONNX_BIN}"
    "${ONNXRUNTIME_BIN}"
    "${SHERPA_ONNX_RUNTIME_DLL}"
    "${ONNXRUNTIME_RUNTIME_DLL}"
)

message(STATUS "--> Resolved LLAMA_DLL:       ${LLAMA_DLL}")
message(STATUS "--> Resolved GGML_DLL:        ${GGML_DLL}")
message(STATUS "--> Resolved GGML_BASE_DLL:   ${GGML_BASE_DLL}")
message(STATUS "--> Resolved GGML_CPU_DLL:    ${GGML_CPU_DLL}")
message(STATUS "--> Resolved SD_LINK_LIB:     ${SD_LINK_LIB}")
message(STATUS "--> Resolved AUDIOCPP_DLL:    ${AUDIOCPP_DLL}")
message(STATUS "--> Resolved SHERPA_ONNX_BIN: ${SHERPA_ONNX_BIN}")
message(STATUS "--> Resolved ONNXRUNTIME_BIN: ${ONNXRUNTIME_BIN}")

# ============================================================================
# 4. Core Backing Library & QML Module
# ============================================================================
qt_add_library(StoryBuilderStudioLib STATIC)

qt_add_resources(StoryBuilderStudioLib "default_prompts"
    PREFIX "/prompts"
    BASE "prompts"
    FILES
        prompts/audio/conversational.prompt
        prompts/audio/dramatic_reading.prompt
        prompts/audio/neutral_narration.prompt
        prompts/image/4k_uhd.prompt
        prompts/image/cinematic_widescreen.prompt
        prompts/image/close_up.prompt
        prompts/image/daylight.prompt
        prompts/image/neon_glow.prompt
        prompts/image/night.prompt
        prompts/image/square.prompt
        prompts/image/wide_pan.prompt
        prompts/text/anime.prompt
        prompts/text/cyberpunk.prompt
        prompts/text/film_noir.prompt
        prompts/text/realistic.prompt
        prompts/video/4k_uhd.prompt
        prompts/video/cinematic_widescreen.prompt
        prompts/video/close_up.prompt
        prompts/video/daylight.prompt
        prompts/video/neon_glow.prompt
        prompts/video/night.prompt
        prompts/video/square.prompt
        prompts/video/tracking_shot.prompt
        prompts/video/wide_pan.prompt
)

qt_add_resources(StoryBuilderStudioLib "resources"
    PREFIX "/"
    FILES
        app.png
        app.ico
)

# Must precede qt_add_qml_module so qmltyperegistrar inherits include paths
target_include_directories(StoryBuilderStudioLib PUBLIC
    ${CLEAN_INC_PATHS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/remote
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
    ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/text
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
)

qt_add_qml_module(StoryBuilderStudioLib
    URI StoryBuilderStudio
    RESOURCE_PREFIX "/qt/qml"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/StoryBuilderStudio"
    NO_PLUGIN
    QML_FILES
        app/Main.qml
        app/core/EditableFieldPair.qml
        app/core/EpisodeListCard.qml
        app/core/FirstRunWizardModal.qml
        app/core/HelpManualDialog.qml
        app/core/NewProjectDialog.qml
        app/core/NewSeriesDialog.qml
        app/core/OpenProjectDialog.qml
        app/core/ProjectDetailsCard.qml
        app/core/ProjectMenuBar.qml
        app/core/SeriesHeaderCard.qml
        app/core/SeriesLauncherModal.qml
        app/engine/GenerationProgressDialog.qml
        app/engine/InferenceEstimateDialog.qml
        app/engine/InferenceSettingsPanel.qml
        app/engine/ProviderSelectorWidget.qml
        app/promptbuilder/PromptBuilder.qml
        app/promptbuilder/PromptEditorDialog.qml
        app/promptbuilder/PromptHistoryViewerDialog.qml
        app/promptbuilder/PromptOptionsPanel.qml
        app/settings/LocalModalManifestCard.qml
        app/settings/RemoteApiConfigCard.qml
        app/settings/SettingsControlPanel.qml
        app/storyboard/StoryBoardAssetList.qml
        app/storyboard/StoryBoardAssetPickerModal.qml
        app/storyboard/StoryBoardCard.qml
        app/storyboard/StoryBoardShotDetailDialog.qml
        app/storyboard/StoryBoardWorkspaceView.qml
        app/workspace/AssetContextMenu.qml
        app/workspace/AssetImportAction.qml
        app/workspace/AssetWorkspaceSplitView.qml
        app/workspace/WorkspaceView.qml
        app/workspace/audio/AudioWorkspaceWidget.qml
        app/workspace/image/ImageCandidateModal.qml
        app/workspace/image/ImageGalleryWidget.qml
        app/workspace/text/EditorPane.qml
        app/workspace/video/VideoWorkspaceWidget.qml
    SOURCES
        app/core/applogger.h app/core/applogger.cpp
        app/core/audittrailstore.h app/core/audittrailstore.cpp
        app/core/projectdirectorymodel.h app/core/projectdirectorymodel.cpp
        app/core/projectmanager.h app/core/projectmanager.cpp
        app/core/seriescontainer.h app/core/seriescontainer.cpp
        app/core/ui_constants.h
        app/engine/abstractinferencedriver.h
        app/engine/engineregistry.h app/engine/engineregistry.cpp
        app/engine/inferenceestimator.h app/engine/inferenceestimator.cpp
        app/engine/remote/genericrestdriver.h app/engine/remote/genericrestdriver.cpp
        app/promptbuilder/prompthistorystore.h app/promptbuilder/prompthistorystore.cpp
        app/promptbuilder/promptmanager.h app/promptbuilder/promptmanager.cpp
        app/settings/settingsmanager.h app/settings/settingsmanager.cpp
        app/storyboard/storyboardmanager.h app/storyboard/storyboardmanager.cpp
        app/storyboard/storydocument.h app/storyboard/storydocument.cpp
        app/workspace/abstractassetdirectorymodel.h app/workspace/abstractassetdirectorymodel.cpp
        app/workspace/audiodirectorymodel.h
        app/workspace/audio/audiowaveformmodel.h app/workspace/audio/audiowaveformmodel.cpp
        app/workspace/imagedirectorymodel.h app/workspace/imagedirectorymodel.cpp
        app/workspace/scriptsdirectorymodel.h
        app/workspace/videodirectorymodel.h
)

target_link_libraries(StoryBuilderStudioLib PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Concurrent
)

target_compile_definitions(StoryBuilderStudioLib PUBLIC QT_NO_QML_DEBUGGER)

# Expose non-compiled docs, scripts, and packaging files in IDE project trees without bloating QRC
add_custom_target(StoryBuilderStudioProjectFiles SOURCES
    TODO.md
    README.md
    LICENSE
    THIRD-PARTY-NOTICES.txt
    app.rc
    app.ico
    app.icns
    app.png
    resources.qrc
    docs/index.html
    scripts/build.sh
    scripts/build.ps1
    scripts/install.sh
    scripts/install.ps1
    scripts/uninstall.sh
    scripts/uninstall.ps1
    scripts/storybuilderstudio.sh
    scripts/storybuilderstudio.bat
    packaging/rpm/storybuilderstudio.spec
    packaging/debian/control
    packaging/debian/rules
    packaging/debian/changelog
    packaging/linux/StoryBuilderStudio.desktop
    app/core/core_tests.cmake
    app/engine/engine_tests.cmake
    app/promptbuilder/promptbuilder_tests.cmake
    app/settings/settings_tests.cmake
    app/storyboard/storyboard_tests.cmake
    app/workspace/workspace_tests.cmake
)

# ============================================================================
# 5. Dynamic Inference Plugins (DLLs / SOs)
# ============================================================================
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins/drivers")

macro(add_inference_plugin PLUGIN_NAME CPP_FILE H_FILE EXTRA_LIBS)
    add_library(${PLUGIN_NAME} MODULE ${CPP_FILE} ${H_FILE})

    target_link_libraries(${PLUGIN_NAME} PRIVATE
        Qt6::Core Qt6::Qml Qt6::Network StoryBuilderStudioLib ${EXTRA_LIBS}
    )

    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
        PREFIX ""
    )

    install(TARGETS ${PLUGIN_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/drivers"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/drivers"
    )
endmacro()

if(LLAMA_DLL)
    add_inference_plugin(LocalLlmEngine "app/engine/local/localllmengine.cpp" "app/engine/local/localllmengine.h" "${LLAMA_DLL}")
    target_sources(LocalLlmEngine PRIVATE app/engine/local/llama_compat.h)
else()
    message(STATUS "Skipping LocalLlmEngine plugin (llama library not found)")
endif()

if(SHERPA_ONNX_BIN)
    add_inference_plugin(KokoroTtsEngine "app/engine/local/kokorottsengine.cpp" "app/engine/local/kokorottsengine.h" "${SHERPA_ONNX_BIN}")
else()
    message(STATUS "Skipping KokoroTtsEngine plugin (sherpa-onnx library not found)")
endif()

if(AUDIOCPP_DLL)
    add_inference_plugin(LocalAudioEngine "app/engine/local/localaudioengine.cpp" "app/engine/local/localaudioengine.h" "${AUDIOCPP_DLL}")
else()
    message(STATUS "Skipping LocalAudioEngine plugin (audiocpp library not found)")
endif()

if(SD_LINK_LIB)
    add_inference_plugin(LocalSdImagePlugin "app/engine/local/localsdengine.cpp" "app/engine/local/localsdengine.h" "${SD_LINK_LIB}")
    target_compile_definitions(LocalSdImagePlugin PRIVATE SD_MODE_IMAGE)

    add_inference_plugin(LocalSdVideoPlugin "app/engine/local/localsdengine.cpp" "app/engine/local/localsdengine.h" "${SD_LINK_LIB}")
    target_compile_definitions(LocalSdVideoPlugin PRIVATE SD_MODE_VIDEO)
else()
    message(STATUS "Skipping LocalSdImagePlugin & LocalSdVideoPlugin (stable-diffusion library not found)")
endif()

# ============================================================================
# 6. Main Executable & Installation Rules
# ============================================================================
qt_add_executable(appStoryBuilderStudio
    app/main.cpp
    ${APP_ICON_RESOURCE}
)

target_link_libraries(appStoryBuilderStudio PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::QuickControls2
    StoryBuilderStudioLib
)

if(WIN32)
    foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
        if(EXISTS "${DLL_PATH}")
            add_custom_command(TARGET appStoryBuilderStudio POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:appStoryBuilderStudio>
            )
        endif()
    endforeach()

    if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(appStoryBuilderStudio PRIVATE "-Wl,--enable-auto-import")
    endif()
endif()

install(TARGETS appStoryBuilderStudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
    set(MACOS_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/app.icns")
    if(NOT EXISTS "${MACOS_APP_ICON}")
        set(MACOS_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.icns")
    endif()

    set_source_files_properties(${MACOS_APP_ICON} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(appStoryBuilderStudio PRIVATE ${MACOS_APP_ICON})

    install(PROGRAMS "${CMAKE_SOURCE_DIR}/scripts/uninstall.sh"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/StoryBuilderStudio")

    install(FILES
        "${CMAKE_SOURCE_DIR}/README.md"
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${CMAKE_SOURCE_DIR}/docs/index.html"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )

    set_target_properties(appStoryBuilderStudio PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "app.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "Story Builder Studio"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.storybuilderstudio.app"
    )
elseif(UNIX)
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/scripts/storybuilderstudio.sh"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    install(PROGRAMS "${CMAKE_SOURCE_DIR}/scripts/uninstall.sh"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/StoryBuilderStudio")

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/StoryBuilderStudio.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

    set(LINUX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/app.png")
    if(NOT EXISTS "${LINUX_APP_ICON}")
        set(LINUX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.png")
    endif()

    install(FILES "${LINUX_APP_ICON}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
            RENAME storybuilderstudio.png)

    install(FILES
        "${CMAKE_SOURCE_DIR}/README.md"
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt"
        "${CMAKE_SOURCE_DIR}/docs/index.html"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
elseif(WIN32)
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/app.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/app.png"
        "${CMAKE_SOURCE_DIR}/README.md"
        "${CMAKE_SOURCE_DIR}/LICENSE"
        "${CMAKE_SOURCE_DIR}/THIRD-PARTY-NOTICES.txt"
        "${CMAKE_SOURCE_DIR}/scripts/storybuilderstudio.bat"
        "${CMAKE_SOURCE_DIR}/scripts/uninstall.ps1"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    install(DIRECTORY "${CMAKE_SOURCE_DIR}/docs"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(LLVM_OPENMP_DLL NAMES libomp.dll    PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(LLVM_UNWIND_DLL NAMES libunwind.dll PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(LLVM_CXX_DLL    NAMES libc++.dll    PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)

    foreach(WIN_RUNTIME_DLL
        "${LLVM_OPENMP_DLL}"
        "${LLVM_UNWIND_DLL}"
        "${LLVM_CXX_DLL}"
        "${ONNXRUNTIME_RUNTIME_DLL}"
    )
        if(WIN_RUNTIME_DLL AND EXISTS "${WIN_RUNTIME_DLL}")
            install(FILES "${WIN_RUNTIME_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endforeach()
endif()

# ============================================================================
# 7. Test Suite Infrastructure
# ============================================================================
option(BUILD_TESTS "Build and enable automated test suite" ON)

if(BUILD_TESTS)
    enable_testing()
    include(CTest)

    set(ENGINE_DLL_DIRS "")
    foreach(DLL_FILE ${FOUND_ENGINE_DLLS})
        if(EXISTS "${DLL_FILE}")
            get_filename_component(D_DIR "${DLL_FILE}" DIRECTORY)
            list(APPEND ENGINE_DLL_DIRS "${D_DIR}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES ENGINE_DLL_DIRS)

    if(WIN32)
        list(JOIN ENGINE_DLL_DIRS "\\;" FORMATTED_ENGINE_DLL_DIRS)
        list(JOIN LOCAL_DLL_PATHS "\\;" FORMATTED_LOCAL_DLL_PATHS)

        set(FULL_TEST_PATH "${QT6_INSTALL_PREFIX}/bin")
        if(FORMATTED_LOCAL_DLL_PATHS)
            string(APPEND FULL_TEST_PATH "\\;${FORMATTED_LOCAL_DLL_PATHS}")
        endif()
        if(FORMATTED_ENGINE_DLL_DIRS)
            string(APPEND FULL_TEST_PATH "\\;${FORMATTED_ENGINE_DLL_DIRS}")
        endif()
        string(APPEND FULL_TEST_PATH "\\;$ENV{PATH}")

        set(CTEST_RUN_ENV
            "PATH=${FULL_TEST_PATH}"
            "QT_QPA_PLATFORM=offscreen"
            "QT_QUICK_CONTROLS_STYLE=Basic"
        )
    else()
        set(CTEST_RUN_ENV
            "QT_QPA_PLATFORM=offscreen"
            "QT_QUICK_CONTROLS_STYLE=Basic"
            "QT_LOGGING_RULES=*.debug=true"
        )
    endif()

    include(app/main_tests.cmake)
    include(app/core/core_tests.cmake)
    include(app/engine/engine_tests.cmake)
    include(app/promptbuilder/promptbuilder_tests.cmake)
    include(app/settings/settings_tests.cmake)
    include(app/storyboard/storyboard_tests.cmake)
    include(app/workspace/workspace_tests.cmake)

    get_property(ALL_TESTS DIRECTORY PROPERTY TESTS)
    if(ALL_TESTS)
        set_tests_properties(${ALL_TESTS} PROPERTIES
            ENVIRONMENT "${CTEST_RUN_ENV}"
        )
    endif()
endif()

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
find_package(Qt6 REQUIRED COMPONENTS Quick Qml Gui Concurrent Multimedia Network QuickControls2 Test QuickTest )
qt_standard_project_setup(REQUIRES 6.8)

# ============================================================================
# 2. Local Presets & Binary Search Paths
# Developer System Roots (Acts like /usr/include and /usr/lib on Windows)
# ============================================================================
set(LOCAL_INCLUDE_PATHS "" CACHE STRING "Semicolon-separated include paths (like /usr/include)")
set(LOCAL_DLL_PATHS     "" CACHE STRING "Semicolon-separated library paths (like /usr/lib)")

# Convert to clean CMake lists and propagate globally
if(LOCAL_INCLUDE_PATHS)
    # Ensure any escaped backslashes or list formatting are parsed cleanly
    separate_arguments(CLEAN_INC_PATHS NATIVE_COMMAND "${LOCAL_INCLUDE_PATHS}")
    if(NOT CLEAN_INC_PATHS)
        set(CLEAN_INC_PATHS ${LOCAL_INCLUDE_PATHS})
    endif()

    # 1. Global preprocessor include for standard compilation units
    include_directories(SYSTEM ${CLEAN_INC_PATHS})

    # 2. Tell CMake to pass these paths to AUTOMOC / AUTORCC pre-scanners
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-I$<JOIN:${CLEAN_INC_PATHS},;-I>>")
endif()

if(LOCAL_DLL_PATHS)
    separate_arguments(CLEAN_LIB_PATHS NATIVE_COMMAND "${LOCAL_DLL_PATHS}")
    if(NOT CLEAN_LIB_PATHS)
        set(CLEAN_LIB_PATHS ${LOCAL_DLL_PATHS})
    endif()

    # Global linker search paths (like -L/usr/lib)
    link_directories(${CLEAN_LIB_PATHS})
endif()

# Add icon
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
# 3. Resolve Binary Library Dependencies from LOCAL_DLL_PATHS
# (MUST BE BEFORE target_link_libraries!)
# ============================================================================
if(WIN32)
    find_file(LLAMA_DLL       NAMES llama.dll libllama.dll         PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_DLL        NAMES ggml.dll libggml.dll           PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_BASE_DLL   NAMES ggml-base.dll libggml-base.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(GGML_CPU_DLL    NAMES ggml-cpu.dll libggml-cpu.dll   PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(SD_LINK_LIB     NAMES stable-diffusion.lib libstable-diffusion.dll.a stable-diffusion.dll libstable-diffusion.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(AUDIOCPP_DLL    NAMES audiocpp.lib libaudiocpp.dll.a audiocpp.dll libaudiocpp.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(SHERPA_ONNX_BIN NAMES sherpa-onnx-c-api.lib libsherpa-onnx-c-api.dll.a sherpa-onnx-c-api.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(ONNXRUNTIME_BIN NAMES onnxruntime.lib libonnxruntime.dll.a onnxruntime.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
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
if(WIN32)
    find_file(SHERPA_ONNX_RUNTIME_DLL NAMES sherpa-onnx-c-api.dll PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
    find_file(ONNXRUNTIME_RUNTIME_DLL NAMES onnxruntime.dll       PATHS ${LOCAL_DLL_PATHS} NO_DEFAULT_PATH)
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

# Diagnostic verification
message(STATUS "--> Resolved LLAMA_DLL:     ${LLAMA_DLL}")
message(STATUS "--> Resolved GGML_DLL:      ${GGML_DLL}")
message(STATUS "--> Resolved GGML_BASE_DLL ${GGML_BASE_DLL}")
message(STATUS "--> Resolved GGML_CPU_DLL ${GGML_CPU_DLL}")
message(STATUS "--> Resolved SD_LINK_LIB:   ${SD_LINK_LIB}")
message(STATUS "--> Resolved AUDIOCPP_DLL:  ${AUDIOCPP_DLL}")
message(STATUS "--> Resolved SHERPA_ONNX_BIN: ${SHERPA_ONNX_BIN}")
message(STATUS "--> Resolved ONNXRUNTIME_BIN: ${ONNXRUNTIME_BIN}")
# ============================================================================
# 3. Main Executable & QML Module Definition
# ============================================================================



# 1. Define the core static backing library
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

# 2. MUST BE BEFORE qt_add_qml_module so the QML type registrar generator inherits these paths!
target_include_directories(StoryBuilderStudioLib PUBLIC
    ${LOCAL_INCLUDE_PATHS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/remote
    ${CMAKE_CURRENT_SOURCE_DIR}/app/promptbuilder
    ${CMAKE_CURRENT_SOURCE_DIR}/app/settings
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/text
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)

# 3. Now run the QML module code generator
qt_add_qml_module(StoryBuilderStudioLib
    URI StoryBuilderStudio
    RESOURCE_PREFIX "/qt/qml"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/StoryBuilderStudio"
    NO_PLUGIN
    QML_FILES app/Main.qml
    QML_FILES app/core/EditableFieldPair.qml
    QML_FILES app/core/NewProjectDialog.qml
    QML_FILES app/core/NewSeriesDialog.qml
    QML_FILES app/core/OpenProjectDialog.qml
    QML_FILES app/core/EpisodeListCard.qml
    QML_FILES app/core/ProjectDetailsCard.qml
    QML_FILES app/core/SeriesHeaderCard.qml
    QML_FILES app/core/SeriesLauncherModal.qml
    QML_FILES app/core/FirstRunWizardModal.qml
    QML_FILES app/core/HelpManualDialog.qml
    QML_FILES app/settings/LocalModalManifestCard.qml
    QML_FILES app/settings/SettingsControlPanel.qml
    QML_FILES app/settings/RemoteApiConfigCard.qml
    QML_FILES app/engine/InferenceEstimateDialog.qml
    QML_FILES app/engine/ProviderSelectorWidget.qml
    QML_FILES app/core/ProjectMenuBar.qml
    QML_FILES app/promptbuilder/PromptHistoryViewerDialog.qml
    QML_FILES app/promptbuilder/PromptOptionsPanel.qml
    QML_FILES app/promptbuilder/PromptEditorDialog.qml
    QML_FILES app/engine/InferenceSettingsPanel.qml
    QML_FILES app/promptbuilder/PromptBuilder.qml
    QML_FILES app/workspace/AssetContextMenu.qml
    QML_FILES app/workspace/AssetImportAction.qml
    QML_FILES app/workspace/AssetWorkspaceSplitView.qml
    QML_FILES app/workspace/audio/AudioWorkspaceWidget.qml
    QML_FILES app/workspace/text/EditorPane.qml
    QML_FILES app/workspace/image/ImageCandidateModal.qml
    QML_FILES app/workspace/image/ImageGalleryWidget.qml
    QML_FILES app/workspace/video/VideoWorkspaceWidget.qml
    QML_FILES app/storyboard/StoryBoardAssetList.qml
    QML_FILES app/storyboard/StoryBoardAssetPickerModal.qml
    QML_FILES app/storyboard/StoryBoardShotDetailDialog.qml
    QML_FILES app/storyboard/StoryBoardCard.qml
    QML_FILES app/storyboard/StoryBoardWorkspaceView.qml
    QML_FILES app/workspace/WorkspaceView.qml

    SOURCES app/engine/abstractinferencedriver.h
    SOURCES app/core/applogger.h app/core/applogger.cpp
    SOURCES app/core/audittrailstore.h app/core/audittrailstore.cpp
    SOURCES app/settings/settingsmanager.h app/settings/settingsmanager.cpp
    SOURCES app/core/seriescontainer.h app/core/seriescontainer.cpp
    SOURCES app/core/projectmanager.h app/core/projectmanager.cpp
    SOURCES app/engine/engineregistry.h app/engine/engineregistry.cpp
    SOURCES app/engine/remote/genericrestdriver.h app/engine/remote/genericrestdriver.cpp
    SOURCES app/core/projectdirectorymodel.h app/core/projectdirectorymodel.cpp
    SOURCES app/engine/inferenceestimator.h app/engine/inferenceestimator.cpp
    SOURCES app/workspace/audiodirectorymodel.h
    SOURCES app/workspace/abstractassetdirectorymodel.h app/workspace/abstractassetdirectorymodel.cpp
    SOURCES app/workspace/imagedirectorymodel.h app/workspace/imagedirectorymodel.cpp
    SOURCES app/workspace/videodirectorymodel.h
    SOURCES app/promptbuilder/prompthistorystore.h app/promptbuilder/prompthistorystore.cpp
    SOURCES app/promptbuilder/promptmanager.h app/promptbuilder/promptmanager.cpp
    SOURCES app/storyboard/storydocument.h app/storyboard/storydocument.cpp
    SOURCES app/storyboard/storyboardmanager.h app/storyboard/storyboardmanager.cpp
    SOURCES app/workspace/audio/audiowaveformmodel.h app/workspace/audio/audiowaveformmodel.cpp
    SOURCES app/workspace/scriptsdirectorymodel.h
    SOURCES app/core/ui_constants.h
    RESOURCES app/core/core_tests.cmake
    RESOURCES app/engine/engine_tests.cmake
    RESOURCES app/promptbuilder/promptbuilder_tests.cmake
    RESOURCES app/settings/settings_tests.cmake
    RESOURCES app/storyboard/storyboard_tests.cmake
    RESOURCES app/workspace/workspace_tests.cmake
    RESOURCES TODO.md
    RESOURCES README.md
    RESOURCES LICENSE
    RESOURCES scripts/storybuilderstudio.bat
    RESOURCES scripts/storybuilderstudio.sh
    RESOURCES resources.qrc
    RESOURCES app.rc
    RESOURCES scripts/build.sh
    RESOURCES scripts/build.ps1
    RESOURCES packaging/rpm/storybuilderstudio.spec
    RESOURCES packaging/debian/control
    RESOURCES packaging/debian/rules
    RESOURCES packaging/debian/changelog
    RESOURCES packaging/linux/StoryBuilderStudio.desktop
    RESOURCES app.ico
    RESOURCES app.icns
    RESOURCES app.png
    RESOURCES docs/index.html
    RESOURCES scripts/install.sh
    RESOURCES scripts/uninstall.sh
    RESOURCES scripts/uninstall.ps1
    RESOURCES scripts/install.ps1
)

# ============================================================================
# 4. Dynamic Inference Plugins (DLLs / SOs)
# ============================================================================
set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins/drivers")

# Macro to quickly generate driver plugins
macro(add_inference_plugin PLUGIN_NAME CPP_FILE H_FILE EXTRA_LIBS)
    add_library(${PLUGIN_NAME} MODULE ${CPP_FILE} ${H_FILE})

    # Link against the core app library so it knows about AbstractInferenceDriver
    target_link_libraries(${PLUGIN_NAME} PRIVATE
        Qt6::Core Qt6::Qml Qt6::Network StoryBuilderStudioLib ${EXTRA_LIBS}
    )

    # Force output to the plugins folder
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
        PREFIX "" # Removes 'lib' prefix on Linux for cleaner loading
    )

    # Automatically copy the built DLL/SO to the install directory
    install(TARGETS ${PLUGIN_NAME}
        LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/drivers"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins/drivers"
    )
endmacro()

# Build the Plugins

# Build the Plugins (optional: skip any plugin whose native library is not installed)
if(LLAMA_DLL)
    add_inference_plugin(LocalLlmEngine "app/engine/local/localllmengine.cpp" "app/engine/local/localllmengine.h" "${LLAMA_DLL}")

target_sources(LocalLlmEngine
  PRIVATE
    app/engine/local/llama_compat.h
)
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

# 4. Link dependencies
target_link_libraries(StoryBuilderStudioLib PUBLIC
    Qt6::Core
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Gui
    Qt6::Multimedia
    Qt6::Concurrent
)

# Ensure qml debugging isn't polluting stdout on test binaries
target_compile_definitions(StoryBuilderStudioLib PRIVATE QT_NO_QML_DEBUGGER)



# Main application executable links to the backing library
qt_add_executable(appStoryBuilderStudio app/main.cpp
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

# Copy runtime DLLs next to executable
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
endif()

# Tells LLVM's lld-link / ld.lld to generate runtime pseudo-relocations
# and import tables for functions in DLLs that lack .lib / .dll.a export files.
if(WIN32 AND (MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    target_link_options(appStoryBuilderStudio PRIVATE "-Wl,--enable-auto-import")
endif()

# Ensure qml debugging isn't polluting stdout on test binaries
target_compile_definitions(appStoryBuilderStudio PRIVATE QT_NO_QML_DEBUGGER)

# ============================================================================
# 5. Installation Target (Now appStoryBuilderStudio exists)
# ============================================================================
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

    set_target_properties(appStoryBuilderStudio PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_ICON_FILE "app.icns"
        MACOSX_BUNDLE_BUNDLE_NAME "Story Builder Studio"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.storybuilderstudio.app"
    )
elseif(UNIX)
    install(PROGRAMS "${CMAKE_SOURCE_DIR}/scripts/storybuilderstudio.sh"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/linux/StoryBuilderStudio.desktop"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")

    set(LINUX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/app.png")
    if(NOT EXISTS "${LINUX_APP_ICON}")
        set(LINUX_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/app.png")
    endif()
    install(FILES "${LINUX_APP_ICON}"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
            RENAME storybuilderstudio.png)
endif()

install(FILES "${CMAKE_SOURCE_DIR}/README.md"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(FILES "${CMAKE_SOURCE_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_BINDIR}")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/docs/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/docs")

# ==============================================================================
# Windows Deployment Components: NonQtLibs & QtRuntime
# ==============================================================================
if(WIN32)
    install(FILES "${CMAKE_SOURCE_DIR}/scripts/storybuilderstudio.bat"
            DESTINATION "${CMAKE_INSTALL_BINDIR}")

    # --------------------------------------------------------------------------
    # Component: NonQtLibs (Compiler Runtimes + 3rd-Party Engine DLLs)
    # --------------------------------------------------------------------------
    get_filename_component(TOOLCHAIN_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(LLVM_OPENMP_DLL NAMES libomp.dll    PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(LLVM_UNWIND_DLL NAMES libunwind.dll PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)
    find_file(LLVM_CXX_DLL    NAMES libc++.dll    PATHS "${TOOLCHAIN_BIN_DIR}" NO_DEFAULT_PATH)

    if(LLVM_OPENMP_DLL)
        install(FILES "${LLVM_OPENMP_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    if(LLVM_UNWIND_DLL)
        install(FILES "${LLVM_UNWIND_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    if(LLVM_CXX_DLL)
        install(FILES "${LLVM_CXX_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
    if(ONNXRUNTIME_BIN AND ONNXRUNTIME_RUNTIME_DLL)
        install(FILES "${ONNXRUNTIME_RUNTIME_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()

# ============================================================================
# 6. Test Suite Infrastructure
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
        # Escape semicolons so CMake does not split the PATH string into list items
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
            "QT_LOGGING_RULES=*.debug=true"
        )
    else()
        set(CTEST_RUN_ENV "QT_QPA_PLATFORM=offscreen")
    endif()



    include(app/main_tests.cmake)
    include(app/core/core_tests.cmake)
    include(app/engine/engine_tests.cmake)
    include(app/promptbuilder/promptbuilder_tests.cmake)
    include(app/settings/settings_tests.cmake)
    include(app/storyboard/storyboard_tests.cmake)
    include(app/workspace/workspace_tests.cmake)

    # Apply runtime PATH to all tests registered in this directory scope
    get_property(ALL_TESTS DIRECTORY PROPERTY TESTS)
    if(ALL_TESTS)
        set_tests_properties(${ALL_TESTS} PROPERTIES
            ENVIRONMENT "${CTEST_RUN_ENV}"
        )
    endif()
endif()

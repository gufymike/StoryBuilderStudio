
add_executable(test_EngineRegistry
    tests/test_EngineRegistry.cpp
    app/engine/engineregistry.cpp
    app/engine/engineregistry.h
    app/engine/abstractinferencedriver.h
    app/engine/remote/genericrestdriver.h
    app/engine/remote/genericrestdriver.cpp
)

target_include_directories(test_EngineRegistry PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
       ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/remote
)

target_link_libraries(test_EngineRegistry PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
    Qt6::Network
)

add_test(NAME test_EngineRegistry COMMAND test_EngineRegistry)

if(LLAMA_DLL)
    add_executable(test_LocalLlmEngine
        tests/test_LocalLlmEngine.cpp
        app/engine/abstractinferencedriver.h
        app/settings/settingsmanager.h
        app/settings/settingsmanager.cpp
        app/engine/local/localllmengine.h
        app/engine/local/localllmengine.cpp
    )

    target_include_directories(test_LocalLlmEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app/core
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    )

    target_link_libraries(test_LocalLlmEngine PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qt6::Concurrent
        ${LLAMA_DLL}
        ${GGML_DLL}
        ${GGML_BASE_DLL}
        ${GGML_CPU_DLL}
    )

    if(WIN32)
        foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
            add_custom_command(TARGET test_LocalLlmEngine POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_PATH}"
                $<TARGET_FILE_DIR:test_LocalLlmEngine>
            )
        endforeach()
    endif()

    add_test(NAME test_LocalLlmEngine COMMAND test_LocalLlmEngine)
endif()

if(SD_LINK_LIB)
    # --- test_LocalSdEngine ---
    add_executable(test_LocalSdEngine
        tests/test_LocalSdEngine.cpp
        app/engine/local/localsdengine.h
        app/engine/local/localsdengine.cpp
        app/engine/abstractinferencedriver.h
    )

    target_include_directories(test_LocalSdEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    )

    if(WIN32)
        set_target_properties(test_LocalSdEngine PROPERTIES WIN32_EXECUTABLE FALSE)
	if(MINGW)
	    target_link_options(test_LocalSdEngine PRIVATE -mconsole)
	elseif(MSVC)
	    target_link_options(test_LocalSdEngine PRIVATE /SUBSYSTEM:CONSOLE)
	endif()
    endif()

    target_link_libraries(test_LocalSdEngine PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Gui
        Qt6::Test
        Qt6::Concurrent
        ${SD_LINK_LIB}
    )

    if(WIN32)
        foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
            if(EXISTS "${DLL_PATH}")
                add_custom_command(TARGET test_LocalSdEngine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_PATH}"
                    $<TARGET_FILE_DIR:test_LocalSdEngine>
                )
            endif()
        endforeach()
    endif()

    add_test(NAME test_LocalSdEngine COMMAND test_LocalSdEngine)


    # --- test_LocalSdEngine ---
    add_executable(test_LocalSdEngineVideo
        tests/test_LocalSdEngine.cpp
        app/engine/local/localsdengine.h
        app/engine/local/localsdengine.cpp
        app/engine/abstractinferencedriver.h
    )

    target_include_directories(test_LocalSdEngineVideo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    )

    if(WIN32)
        set_target_properties(test_LocalSdEngineVideo PROPERTIES WIN32_EXECUTABLE FALSE)
        if(MINGW)
	    target_link_options(test_LocalSdEngineVideo PRIVATE -mconsole)
	elseif(MSVC)
	    target_link_options(test_LocalSdEngineVideo PRIVATE /SUBSYSTEM:CONSOLE)
	endif()

    endif()

    target_link_libraries(test_LocalSdEngineVideo PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Gui
        Qt6::Test
        Qt6::Concurrent
        ${SD_LINK_LIB}
    )

    if(WIN32)
        foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
            if(EXISTS "${DLL_PATH}")
                add_custom_command(TARGET test_LocalSdEngineVideo POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_PATH}"
                    $<TARGET_FILE_DIR:test_LocalSdEngineVideo>
                )
            endif()
        endforeach()
    endif()

    target_compile_definitions(test_LocalSdEngineVideo PRIVATE SD_MODE_VIDEO)

    add_test(NAME test_LocalSdEngineVideo COMMAND test_LocalSdEngineVideo)
endif()

if(AUDIOCPP_DLL)
    # --- test_LocalAudioEngine ---
    add_executable(test_LocalAudioEngine
        tests/test_LocalAudioEngine.cpp
        app/engine/local/localaudioengine.h
        app/engine/local/localaudioengine.cpp
        app/engine/abstractinferencedriver.h
        app/settings/settingsmanager.h
        app/settings/settingsmanager.cpp
    )

    target_include_directories(test_LocalAudioEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/core
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    )

    if(WIN32)
        set_target_properties(test_LocalAudioEngine PROPERTIES WIN32_EXECUTABLE FALSE)
        if(MINGW)
	    target_link_options(test_LocalAudioEngine PRIVATE -mconsole)
	elseif(MSVC)
	    target_link_options(test_LocalAudioEngine PRIVATE /SUBSYSTEM:CONSOLE)
	endif()

    endif()

    target_link_libraries(test_LocalAudioEngine PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qt6::Concurrent
        ${AUDIOCPP_DLL}
    )

    if(WIN32)
        foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
            if(EXISTS "${DLL_PATH}")
                add_custom_command(TARGET test_LocalAudioEngine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_PATH}"
                    $<TARGET_FILE_DIR:test_LocalAudioEngine>
                )
            endif()
        endforeach()
    endif()

    add_test(NAME test_LocalAudioEngine COMMAND test_LocalAudioEngine)
endif()

if(SHERPA_ONNX_BIN AND ONNXRUNTIME_BIN)
    # --- test_KokoroTtsEngine ---
    add_executable(test_KokoroTtsEngine
        tests/test_KokoroTtsEngine.cpp
        app/engine/local/kokorottsengine.h
        app/engine/local/kokorottsengine.cpp
        app/engine/abstractinferencedriver.h
    )

    target_include_directories(test_KokoroTtsEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/local
    )

    if(WIN32)
        set_target_properties(test_KokoroTtsEngine PROPERTIES WIN32_EXECUTABLE FALSE)
        if(MINGW)
	    target_link_options(test_KokoroTtsEngine PRIVATE -mconsole)
	elseif(MSVC)
	    target_link_options(test_KokoroTtsEngine PRIVATE /SUBSYSTEM:CONSOLE)
	endif()
    endif()

    target_link_libraries(test_KokoroTtsEngine PRIVATE
        Qt6::Core
        Qt6::Qml
        Qt6::Test
        Qt6::Concurrent
        ${SHERPA_ONNX_BIN}
        ${ONNXRUNTIME_BIN}
    )

    if(WIN32)
        foreach(DLL_PATH ${FOUND_ENGINE_DLLS})
            if(EXISTS "${DLL_PATH}")
                add_custom_command(TARGET test_KokoroTtsEngine POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_PATH}"
                    $<TARGET_FILE_DIR:test_KokoroTtsEngine>
                )
            endif()
        endforeach()
    endif()

    add_test(NAME test_KokoroTtsEngine COMMAND test_KokoroTtsEngine)
endif()

# --- test_GenericRestDriver ---
add_executable(test_GenericRestDriver
    tests/test_GenericRestDriver.cpp
    app/engine/remote/genericrestdriver.h
    app/engine/remote/genericrestdriver.cpp
    app/engine/abstractinferencedriver.h
)

target_include_directories(test_GenericRestDriver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine/remote
)

if(WIN32)
    set_target_properties(test_GenericRestDriver PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
	    target_link_options(test_GenericRestDriver PRIVATE -mconsole)
	elseif(MSVC)
	    target_link_options(test_GenericRestDriver PRIVATE /SUBSYSTEM:CONSOLE)
	endif()

endif()

target_link_libraries(test_GenericRestDriver PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Network
    Qt6::Test
)

add_test(NAME test_GenericRestDriver COMMAND test_GenericRestDriver)

# --- test_InferenceEstimator ---
add_executable(test_InferenceEstimator
    tests/test_InferenceEstimator.cpp
    app/engine/inferenceestimator.h
    app/engine/inferenceestimator.cpp
)

target_include_directories(test_InferenceEstimator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
)

if(WIN32)
    set_target_properties(test_InferenceEstimator PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_InferenceEstimator PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_InferenceEstimator PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_InferenceEstimator PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_InferenceEstimator COMMAND test_InferenceEstimator)

# --- test_InferenceEstimateDialog ---
add_executable(test_InferenceEstimateDialog
    tests/test_InferenceEstimateDialog.cpp
)

target_compile_definitions(test_InferenceEstimateDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_InferenceEstimateDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
)

if(WIN32)
    set_target_properties(test_InferenceEstimateDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_InferenceEstimateDialog PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_InferenceEstimateDialog PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_InferenceEstimateDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_InferenceEstimateDialog COMMAND test_InferenceEstimateDialog)

# --- test_ProviderSelectorWidget ---
add_executable(test_ProviderSelectorWidget
    tests/test_ProviderSelectorWidget.cpp
)

target_compile_definitions(test_ProviderSelectorWidget PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_ProviderSelectorWidget PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
)

if(WIN32)
    set_target_properties(test_ProviderSelectorWidget PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_ProviderSelectorWidget PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_ProviderSelectorWidget PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_ProviderSelectorWidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_ProviderSelectorWidget COMMAND test_ProviderSelectorWidget)

add_executable(test_InferenceSettingsPanel
    tests/test_InferenceSettingsPanel.cpp
)

target_compile_definitions(test_InferenceSettingsPanel PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_InferenceSettingsPanel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/engine
)

if(WIN32)
    set_target_properties(test_InferenceSettingsPanel PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
        target_link_options(test_InferenceSettingsPanel PRIVATE -mconsole)
    elseif(MSVC)
	target_link_options(test_InferenceSettingsPanel PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_InferenceSettingsPanel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_InferenceSettingsPanel COMMAND test_InferenceSettingsPanel)

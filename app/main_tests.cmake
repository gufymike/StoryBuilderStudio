enable_testing()
# MainView Test
add_executable(test_MainView
    tests/test_MainView.cpp
)

target_compile_definitions(test_MainView PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_MainView PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/image
    ${CMAKE_CURRENT_SOURCE_DIR}/app/workspace/video
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storyboard
)

if(WIN32)
    set_target_properties(test_MainView PROPERTIES WIN32_EXECUTABLE FALSE)
    if(MINGW)
    	target_link_options(test_MainView PRIVATE -mconsole)
    elseif(MSVC)
    	target_link_options(test_MainView PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
endif()

target_link_libraries(test_MainView PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Multimedia
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_MainView COMMAND test_MainView)

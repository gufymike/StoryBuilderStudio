
add_executable(test_AppLogger
    tests/test_AppLogger.cpp
    app/core/applogger.cpp
    app/core/applogger.h
)

target_include_directories(test_AppLogger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

target_link_libraries(test_AppLogger PRIVATE
    Qt6::Core
    Qt6::Test
    Qt6::Concurrent
)

add_test(NAME test_AppLogger COMMAND test_AppLogger)

add_executable(test_AuditTrailStore
    tests/test_AuditTrailStore.cpp
    app/core/audittrailstore.cpp
    app/core/audittrailstore.h
)

target_include_directories(test_AuditTrailStore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

target_link_libraries(test_AuditTrailStore PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_AuditTrailStore COMMAND test_AuditTrailStore)
# Supply the Qt and MinGW bin directories to the test runner environment
if(WIN32)
    set_tests_properties(test_AppLogger PROPERTIES
        ENVIRONMENT "PATH=${QT6_INSTALL_PREFIX}/bin;$ENV{PATH}"
    )
endif()

add_executable(test_SeriesContainer
    tests/test_SeriesContainer.cpp
    app/core/seriescontainer.cpp
    app/core/seriescontainer.h
)

target_include_directories(test_SeriesContainer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

target_link_libraries(test_SeriesContainer PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_SeriesContainer COMMAND test_SeriesContainer)

add_executable(test_ProjectManager
    tests/test_ProjectManager.cpp
    app/core/projectmanager.cpp
    app/core/projectmanager.h
)

target_include_directories(test_ProjectManager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

target_link_libraries(test_ProjectManager PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_ProjectManager COMMAND test_ProjectManager)

# --- test_ProjectDirectoryModel ---
add_executable(test_ProjectDirectoryModel
    tests/test_ProjectDirectoryModel.cpp
    app/core/projectdirectorymodel.h
    app/core/projectdirectorymodel.cpp
)

target_include_directories(test_ProjectDirectoryModel PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_ProjectDirectoryModel PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_ProjectDirectoryModel PRIVATE -mconsole)
endif()

target_link_libraries(test_ProjectDirectoryModel PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_test(NAME test_ProjectDirectoryModel COMMAND test_ProjectDirectoryModel)

# --- test_EditableFieldPair ---
add_executable(test_EditableFieldPair
    tests/test_EditableFieldPair.cpp
)

target_compile_definitions(test_EditableFieldPair PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(test_EditableFieldPair PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_EditableFieldPair PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_EditableFieldPair PRIVATE -mconsole)
endif()

target_link_libraries(test_EditableFieldPair PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

add_test(NAME test_EditableFieldPair COMMAND test_EditableFieldPair)

# --- test_NewProjectDialog ---
add_executable(test_NewProjectDialog
    tests/test_NewProjectDialog.cpp
)

target_compile_definitions(test_NewProjectDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(test_NewProjectDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_NewProjectDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_NewProjectDialog PRIVATE -mconsole)
endif()

target_link_libraries(test_NewProjectDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

add_test(NAME test_NewProjectDialog COMMAND test_NewProjectDialog)

# --- test_NewSeriesDialog ---
add_executable(test_NewSeriesDialog
    tests/test_NewSeriesDialog.cpp
)

target_compile_definitions(test_NewSeriesDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(test_NewSeriesDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_NewSeriesDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_NewSeriesDialog PRIVATE -mconsole)
endif()

target_link_libraries(test_NewSeriesDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

add_test(NAME test_NewSeriesDialog COMMAND test_NewSeriesDialog)

# --- test_OpenProjectDialog ---
add_executable(test_OpenProjectDialog
    tests/test_OpenProjectDialog.cpp
)

target_compile_definitions(test_OpenProjectDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(test_OpenProjectDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_OpenProjectDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_OpenProjectDialog PRIVATE -mconsole)
endif()

target_link_libraries(test_OpenProjectDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

add_test(NAME test_OpenProjectDialog COMMAND test_OpenProjectDialog)

# --- test_EpisodeListCard ---
add_executable(test_EpisodeListCard
    tests/test_EpisodeListCard.cpp
)

target_compile_definitions(test_EpisodeListCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_include_directories(test_EpisodeListCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_EpisodeListCard PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_EpisodeListCard PRIVATE -mconsole)
endif()

target_link_libraries(test_EpisodeListCard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
)

add_test(NAME test_EpisodeListCard COMMAND test_EpisodeListCard)

# --- test_ProjectDetailsCard ---
add_executable(test_ProjectDetailsCard
    tests/test_ProjectDetailsCard.cpp
)

target_compile_definitions(test_ProjectDetailsCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_ProjectDetailsCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_ProjectDetailsCard PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_ProjectDetailsCard PRIVATE -mconsole)
endif()

target_link_libraries(test_ProjectDetailsCard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_ProjectDetailsCard COMMAND test_ProjectDetailsCard)

# --- test_SeriesHeaderCard ---
add_executable(test_SeriesHeaderCard
    tests/test_SeriesHeaderCard.cpp
)

target_compile_definitions(test_SeriesHeaderCard PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_SeriesHeaderCard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_SeriesHeaderCard PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_SeriesHeaderCard PRIVATE -mconsole)
endif()

target_link_libraries(test_SeriesHeaderCard PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_SeriesHeaderCard COMMAND test_SeriesHeaderCard)


# --- test_SeriesLauncherModal ---
add_executable(test_SeriesLauncherModal
    tests/test_SeriesLauncherModal.cpp
)

target_compile_definitions(test_SeriesLauncherModal PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_SeriesLauncherModal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_SeriesLauncherModal PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_SeriesLauncherModal PRIVATE -mconsole)
endif()

target_link_libraries(test_SeriesLauncherModal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_SeriesLauncherModal COMMAND test_SeriesLauncherModal)

# --- test_FirstRunWizardModal ---
add_executable(test_FirstRunWizardModal
    tests/test_FirstRunWizardModal.cpp
)

target_compile_definitions(test_FirstRunWizardModal PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_FirstRunWizardModal PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_FirstRunWizardModal PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_FirstRunWizardModal PRIVATE -mconsole)
endif()

target_link_libraries(test_FirstRunWizardModal PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_FirstRunWizardModal COMMAND test_FirstRunWizardModal)

# --- test_HelpManualDialog ---
add_executable(test_HelpManualDialog
    tests/test_HelpManualDialog.cpp
)

target_compile_definitions(test_HelpManualDialog PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_HelpManualDialog PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_HelpManualDialog PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_HelpManualDialog PRIVATE -mconsole)
endif()

target_link_libraries(test_HelpManualDialog PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_HelpManualDialog COMMAND test_HelpManualDialog)

# --- test_ProjectMenuBar ---
add_executable(test_ProjectMenuBar
    tests/test_ProjectMenuBar.cpp
)

target_compile_definitions(test_ProjectMenuBar PRIVATE
    CMAKE_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    CMAKE_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}"
)

target_include_directories(test_ProjectMenuBar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
)

if(WIN32)
    set_target_properties(test_ProjectMenuBar PROPERTIES WIN32_EXECUTABLE FALSE)
    target_link_options(test_ProjectMenuBar PRIVATE -mconsole)
endif()

target_link_libraries(test_ProjectMenuBar PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Test
    StoryBuilderStudioLib
)

add_test(NAME test_ProjectMenuBar COMMAND test_ProjectMenuBar)

// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

ApplicationWindow {
    id: rootWindow
    objectName: "rootApplicationWindow"
    width: 1150
    height: 800
    minimumWidth: 1084
    minimumHeight: 800
    visible: true
    color: "#08121A"

    title: (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen)
           ? qsTr("Story Builder Studio - ") + projectManager.projectName
           : qsTr("Story Builder Studio - No Project Open")

    property string currentFilePath: ""
    property bool isDocumentLoaded: false

    // Testing / Inspection aliases
    property alias workspaceView: workspaceView
    property alias statusPopup: statusBanner
    property alias statusBannerMessage: statusBannerText.text

    function resolveEpisodeScriptsFolderUrl() {
        if (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen) {
            let base = projectManager.currentProjectPath
            if (base && base.length > 0) {
                let fullPath = (base + "/scripts").replace(/\\/g, "/")
                return (Qt.platform.os === "windows" ? "file:///" : "file://") + fullPath
            }
        }
        return StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
    }

    function openScriptFile(nativePath) {
        if (!nativePath || nativePath.length === 0) return false
        if (storyDocument.loadLocalFile(nativePath)) {
            rootWindow.currentFilePath = nativePath
            rootWindow.isDocumentLoaded = true
            if (workspaceView && workspaceView.mainEditorPane) {
                workspaceView.mainEditorPane.isDocumentLoaded = true
                workspaceView.mainEditorPane.paneTitle = nativePath
                workspaceView.mainEditorPane.textContent = storyDocument.rawText
            }
            rootWindow.title = "Story Builder Studio - " + nativePath
            if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                settingsManager.addRecentFile(nativePath)
            }
            return true
        }
        return false
    }

    // Operational Dialogs
    OpenProjectDialog { id: openProjectDialog; objectName: "openProjectDialog" }
    NewProjectDialog  { id: newProjectDialog;  objectName: "newProjectDialog" }
    NewSeriesDialog   { id: newSeriesDialog;   objectName: "newSeriesDialog" }

    FirstRunWizardModal {
        id: firstRunWizard
        objectName: "firstRunWizard"
    }

    SeriesLauncherModal {
        id: seriesLauncherModal
        objectName: "seriesLauncherModal"
    }

    StoryDocument {
        id: storyDocument
        objectName: "storyDocument"
        onFileOperationSuccess: (message) => {
            statusBannerText.text = message
            statusBanner.open()
        }
        onFileOperationFailure: (errorMessage) => {
            statusBannerText.text = "⚠️ " + errorMessage
            statusBanner.open()
        }
    }

    // --- Native Operational File Dialogs ---
    FileDialog {
        id: localFileLoader
        objectName: "localFileLoader"
        title: qsTr("Open Story Manuscript")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["All Script Files (*.md *.txt *.html *.fountain)", "Markdown files (*.md)", "Text files (*.txt)", "HTML files (*.html)", "All Files (*)"]
        onAccepted: {
            let nativePath = urlToLocalPath(selectedFile)
            rootWindow.openScriptFile(nativePath)
        }
    }

    FileDialog {
        id: localFileSaver
        objectName: "localFileSaver"
        title: qsTr("Save Story Manuscript As")
        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["Markdown files (*.md)", "Text files (*.txt)", "HTML files (*.html)"]
        onAccepted: {
            let nativePath = urlToLocalPath(selectedFile)
            rootWindow.currentFilePath = nativePath
            rootWindow.executeSaveSequence()
            if (nativePath !== "" && typeof settingsManager !== "undefined" && settingsManager !== null) {
                settingsManager.addRecentFile(nativePath)
            }
        }
    }

    FolderDialog {
        id: openSeriesFolderDialog
        objectName: "openSeriesFolderDialog"
        title: qsTr("Select Series Directory")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            let nativePath = urlToLocalPath(selectedFolder)
            if (typeof seriesContainer !== "undefined" && seriesContainer !== null) {
                if (!seriesContainer.loadFromDirectory(nativePath)) {
                    statusBannerText.text = "⚠️ Failed to load series manifest."
                    statusBanner.open()
                }
            }
        }
    }

    // --- Workspace Settings Overlay ---
    SettingsControlPanel {
        id: settingsControlPanel
        objectName: "settingsControlPanel"
        onThemeRestartRequested: themeRestartNotice.open()
        onRerunWizardRequested: {
            firstRunWizard.currentStep = 0
            firstRunWizard.open()
        }
    }

    Popup {
        id: themeRestartNotice
        objectName: "themeRestartNotice"
        anchors.centerIn: parent
        width: 380
        height: 100
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#0B1B26"
            border.color: "#00F0FF"
            border.width: 1
            radius: 6
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14

            Text {
                text: qsTr("Workspace Configuration Locked")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 13
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("Theme updated on disk track! Please restart\nStory Builder Studio to re-render the graphics wrapper.")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.family: "Courier New, monospace"
                font.pixelSize: 11
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    // --- ApplicationWindow MenuBar Binding ---
    menuBar: ProjectMenuBar {
        id: appMenuBar
        objectName: "appMenuBar"
        isSeriesOpen: typeof seriesContainer !== "undefined" && seriesContainer !== null &&
                      seriesContainer.rootDirectory.length > 0
        isEpisodeOpen: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
        isDocumentLoaded: rootWindow.isDocumentLoaded

        onNewSeriesTriggered: newSeriesDialog.open()
        onOpenSeriesTriggered: openSeriesFolderDialog.open()
        onCloseSeriesTriggered: {
            if (typeof projectManager !== "undefined" && projectManager !== null) projectManager.closeProject()
            if (typeof seriesContainer !== "undefined" && seriesContainer !== null) seriesContainer.loadFromDirectory("")
        }
        onExitTriggered: Qt.quit()

        onNewEpisodeTriggered: newProjectDialog.open()
        onCloseEpisodeTriggered: {
            if (typeof projectManager !== "undefined" && projectManager !== null) projectManager.closeProject()
        }

        onRecentDocumentTriggered: (filePath) => {
            rootWindow.openScriptFile(filePath)
        }
        onSettingsTriggered: settingsControlPanel.open()
        onDocumentationTriggered: helpDialogInstance.open()
        onOpenManualBrowserTriggered: {
            let manualUrl = "file:///" + (typeof applicationDirPath !== "undefined" ? applicationDirPath.replace(/\\/g, "/") : "") + "/docs/index.html"
            Qt.openUrlExternally(manualUrl)
        }
        onAboutTriggered: aboutDialog.open()
        onPromptHistoryTriggered: historyInspectorDialog.showCentered(rootWindow)
    }

    // --- Central Workspace Layout ---
    WorkspaceView {
        id: workspaceView
        objectName: "centralWorkspaceView"
        anchors.fill: parent
        enabled: (typeof seriesContainer !== "undefined" && seriesContainer !== null &&
                  seriesContainer.rootDirectory.length > 0)
        opacity: enabled ? 1.0 : 0.25

        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }

        onNewScriptTriggered: {
            rootWindow.forceBufferPurge()
            rootWindow.isDocumentLoaded = true
            if (workspaceView && workspaceView.mainEditorPane) {
                workspaceView.mainEditorPane.isDocumentLoaded = true
            }
        }
        onOpenScriptTriggered: {
            localFileLoader.currentFolder = rootWindow.resolveEpisodeScriptsFolderUrl()
            localFileLoader.open()
        }
        onSaveScriptTriggered: rootWindow.executeSaveSequence()
        onCloseScriptTriggered: rootWindow.executeCloseSequence()
        onRecentDocumentTriggered: (filePath) => {
            rootWindow.openScriptFile(filePath)
        }
    }

    // --- Pipeline Operations ---
    function executeSaveSequence() {
        if (workspaceView && workspaceView.mainEditorPane) {
            storyDocument.rawText = workspaceView.mainEditorPane.textContent
        }

        if (rootWindow.currentFilePath === "") {
            localFileSaver.currentFolder = rootWindow.resolveEpisodeScriptsFolderUrl()
            localFileSaver.open()
            return true
        }

        if (workspaceView && workspaceView.mainEditorPane) {
            workspaceView.mainEditorPane.paneTitle = rootWindow.currentFilePath
        }

        let fmt = "markdown"
        if (rootWindow.currentFilePath.endsWith(".html") || rootWindow.currentFilePath.endsWith(".htm")) {
            fmt = "html"
        } else if (rootWindow.currentFilePath.endsWith(".txt")) {
            fmt = "text"
        }

        storyDocument.saveLocalFile(rootWindow.currentFilePath, fmt)
        rootWindow.title = "Story Builder Studio - " + rootWindow.currentFilePath
        return true
    }

    function executeCloseSequence() {
        if (!rootWindow.isDocumentLoaded) return
        rootWindow.executeSaveSequence()
        if (rootWindow.currentFilePath !== "") {
            rootWindow.forceBufferPurge()
        }
    }

    function forceBufferPurge() {
        storyDocument.rawText = ""
        rootWindow.currentFilePath = ""
        rootWindow.isDocumentLoaded = false
        rootWindow.title = "Story Builder Studio - Workspace Engine"
        if (workspaceView && workspaceView.mainEditorPane) {
            workspaceView.mainEditorPane.textContent = ""
            workspaceView.mainEditorPane.isDocumentLoaded = false
            workspaceView.mainEditorPane.paneTitle = "Editor Pane"
        }
    }

    function urlToLocalPath(url) {
        if (!url) return ""
        let path = url.toString().replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && path.startsWith("/")) {
            path = path.substring(1)
        }
        return decodeURIComponent(path)
    }

    Popup {
        id: statusBanner
        objectName: "statusBanner"
        y: parent.height - 56
        x: (parent.width - width) / 2
        width: 400
        height: 36
        modal: false
        focus: false
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: "#0B1B26"
            border.color: "#00F0FF"
            border.width: 1
            radius: 4
        }
        Text {
            id: statusBannerText
            objectName: "statusBannerText"
            anchors.centerIn: parent
            color: "#00F0FF"
            style: Text.Outline
            styleColor: "#000000"
            font.family: "Courier New, monospace"
            font.pixelSize: 11
        }
        Timer {
            interval: 2500
            running: statusBanner.opened
            repeat: false
            onTriggered: statusBanner.close()
        }
    }
    // ==========================================
    // DIALOGS & WINDOWS
    // ==========================================
    Dialog {
        id: aboutDialog
        objectName: "aboutDialog"
        title: qsTr("About Story Builder Studio")
        modal: true
        anchors.centerIn: Overlay.overlay
        width: 540
        height: 520
        standardButtons: Dialog.Close

        background: Rectangle {
            color: "#08121A" // Dark Well
            border.color: "#1A4A68" // Accent Border
            border.width: 1
            radius: 8
        }

        contentItem: ColumnLayout {
            spacing: 10
            anchors.margins: 6

            Text {
                id: aboutTitleText
                objectName: "aboutTitleText"
                text: qsTr("Story Builder Studio")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 18
            }

            Text {
                text: qsTr("A unified, offline-first production workspace designed to sequence scripts, image plates, dialogue tracks, and video clips into cohesive narrative storyboards.")
                color: "#FFFFFF"
                font.pixelSize: 12
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1A4A68"
            }

            // --- Local Inference Plugins Status ---
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("Local Inference Plugins:")
                    color: "#00F0FF"
                    font.bold: true
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
                Text {
                    id: aboutPluginModeText
                    objectName: "aboutPluginModeText"
                    text: (typeof engineRegistry !== "undefined" && engineRegistry && engineRegistry.hasLocalPlugins)
                          ? qsTr("Local + Remote Mode")
                          : qsTr("Remote / REST Only")
                    color: (typeof engineRegistry !== "undefined" && engineRegistry && engineRegistry.hasLocalPlugins)
                           ? "#00FF88" : "#FFAA00"
                    font.bold: true
                    font.pixelSize: 10
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: pluginStatusCol.implicitHeight + 12
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                ColumnLayout {
                    id: pluginStatusCol
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    Repeater {
                        id: aboutPluginRepeater
                        objectName: "aboutPluginRepeater"
                        model: (typeof engineRegistry !== "undefined" && engineRegistry)
                               ? engineRegistry.pluginStatuses : []

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Text {
                                text: modelData.available ? "✔" : "✖"
                                color: modelData.available ? "#00FF88" : "#FF5555"
                                font.bold: true
                                font.pixelSize: 11
                            }

                            Text {
                                text: modelData.label
                                color: modelData.available ? "#FFFFFF" : "#888888"
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }

                            Text {
                                text: modelData.available ? qsTr("Installed") : qsTr("Not Installed")
                                color: modelData.available ? "#00F0FF" : "#888888"
                                font.pixelSize: 10
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1A4A68"
            }

            Text {
                text: qsTr("Mission:")
                color: "#1A4A68"
                font.bold: true
                font.pixelSize: 11
            }

            Text {
                text: qsTr("This project was built to empower creators to make original multimedia stories, series, and films who might not otherwise have the resources, equipment, or traditional industry access to do so.")
                color: "#CCCCCC"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1A4A68"
            }

            Text {
                text: qsTr("License & Credits:")
                color: "#1A4A68"
                font.bold: true
                font.pixelSize: 11
            }

            Text {
                text: qsTr("Licensed under the GNU General Public License v3.0 (GPL-3.0-or-later).\nDeveloped by Mike Ramirez and Gemini.")
                color: "#888888"
                font.pixelSize: 11
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }
    HelpManualDialog {
        id: helpDialogInstance
        objectName: "helpDialogInstance"
    }

    PromptHistoryViewerDialog {
        id: historyInspectorDialog
        objectName: "historyInspectorDialog"
    }
    Connections {
        target: (typeof seriesContainer !== "undefined") ? seriesContainer : null
        ignoreUnknownSignals: true

        function onRootDirectoryChanged() {
            if (!seriesContainer.rootDirectory || seriesContainer.rootDirectory.length === 0) {
                seriesLauncherModal.open()
            } else {
                seriesLauncherModal.close()
            }
        }
        function onManifestError(errorMessage) {
            console.warn("[SeriesContainer] " + errorMessage)
            statusBannerText.text = "⚠️ " + errorMessage
            statusBanner.open()
        }
    }

    Connections {
        target: (typeof projectManager !== "undefined") ? projectManager : null
        ignoreUnknownSignals: true

        function onIsProjectOpenChanged() {
            if (!projectManager.isProjectOpen) {
                rootWindow.currentFilePath = ""
                rootWindow.isDocumentLoaded = false
                rootWindow.title = "Story Builder Studio - No Project Open"

                if (typeof storyDocument !== "undefined" && storyDocument !== null) {
                    storyDocument.rawText = ""
                }
            }
        }
    }

    Component.onCompleted: {
        Qt.callLater(() => {
            if (typeof settingsManager !== "undefined" && settingsManager !== null && settingsManager.isFirstRun) {
                firstRunWizard.open()
            } else if (typeof seriesContainer !== "undefined" && seriesContainer !== null) {
                if (!seriesContainer.rootDirectory || seriesContainer.rootDirectory.length === 0) {
                    seriesLauncherModal.open()
                }
            }
        })
    }
}

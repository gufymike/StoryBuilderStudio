// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Item {
    id: workspaceRoot
    objectName: "workspaceViewRoot"

    // Centralized Readiness Indicators derived directly from engineRegistry
    readonly property bool isScriptReady: typeof engineRegistry !== "undefined" && engineRegistry !== null &&
                                          engineRegistry.activeScriptDriver !== null && engineRegistry.activeScriptDriver.isModelLoaded
    readonly property bool isImageReady:  typeof engineRegistry !== "undefined" && engineRegistry !== null &&
                                          engineRegistry.activeImageDriver !== null && engineRegistry.activeImageDriver.isModelLoaded
    readonly property bool isAudioReady:  typeof engineRegistry !== "undefined" && engineRegistry !== null &&
                                          engineRegistry.activeAudioDriver !== null && engineRegistry.activeAudioDriver.isModelLoaded
    readonly property bool isVideoReady:  typeof engineRegistry !== "undefined" && engineRegistry !== null &&
                                          engineRegistry.activeVideoDriver !== null && engineRegistry.activeVideoDriver.isModelLoaded

    property alias mainEditorPane: mainEditorPane
    property alias tabIndex: workspaceTabBar.currentIndex
    property alias candidateSelector: candidateModal
    property string lastImageSentTimestamp: ""
    property var pendingSavedImageFiles: []

    signal newScriptTriggered()
    signal openScriptTriggered()
    signal saveScriptTriggered()
    signal closeScriptTriggered()
    signal recentDocumentTriggered(string filePath)

    function extractFileName(pathOrUrl) {
        if (!pathOrUrl || pathOrUrl.length === 0) return ""
        let normalized = String(pathOrUrl).replace(/\\/g, "/")
        let idx = normalized.lastIndexOf("/")
        return idx >= 0 ? normalized.substring(idx + 1) : normalized
    }

    function snapshotImageModelFiles() {
        let names = []
        const model = imageGalleryWidget.directoryModel
        if (!model || typeof model.rowCount !== "function") return names
        const count = model.rowCount()
        for (let i = 0; i < count; ++i) {
            let idx = model.index(i, 0)
            let val = model.data(idx, Qt.DisplayRole)
            if (val) names.push(String(val))
        }
        return names
    }

    function refreshAudioWorkspace(filePath) {
        if (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen) {
            audioWorkspaceWidget.updateProjectPath(projectManager.currentProjectPath)
        }
        if (audioWorkspaceWidget.directoryModel && typeof audioWorkspaceWidget.directoryModel.refresh === "function") {
            audioWorkspaceWidget.directoryModel.refresh()
        }
        if (filePath && filePath.length > 0 && typeof audioWorkspaceWidget.loadTrack === "function") {
            audioWorkspaceWidget.loadTrack(filePath)
        }
    }

    NewProjectDialog {
        id: newProjectDialog
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ==========================================
        // 1. TOP HEADER & METADATA BAR
        // ==========================================
        RowLayout {
            id: headerStrip
            objectName: "headerStrip"
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 12

            Text {
                id: appTitleText
                objectName: "appTitleText"
                text: qsTr("Story Builder Studio")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 14
                font.bold: true
            }

            Rectangle { width: 1; height: 16; color: "#1A4A68" }

            Text {
                id: episodeStatusText
                objectName: "episodeStatusText"
                text: (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen)
                    ? qsTr("Episode: %1").arg(projectManager.projectName)
                    : qsTr("No Production Loaded")
                color: (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen)
                    ? "#FFFFFF" : "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }

            Text {
                text: {
                    if (typeof engineRegistry === "undefined" || !engineRegistry.activeScriptDriver) {
                        return qsTr("Offline Runtime")
                    }
                    let name = engineRegistry.activeScriptDriver.displayName
                    return qsTr("API: %1 | Tokens: ~0").arg(name)
                }
                color: "#00F0FF"
                font.pixelSize: 11
                font.bold: true
            }
        }

        // ==========================================
        // 2. MASTER WORKSPACE TAB BAR
        // ==========================================
        TabBar {
            id: workspaceTabBar
            objectName: "workspaceTabBar"
            Layout.fillWidth: true
            currentIndex: workspaceStack.currentIndex
            background: Rectangle {
                color: "#08121A"
                border.color: "#1A4A68"
                border.width: 1
            }

            // Tab 0: Project Details
            TabButton {
                id: projectTabBtn
                objectName: "projectTabBtn"
                implicitHeight: 32
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Manage project configuration, episode metadata, and directory structure")
                background: Rectangle {
                    color: projectTabBtn.checked ? "#1A4A68" : (projectTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: Text {
                    text: qsTr("📁 Project Details")
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: projectTabBtn.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            // Tab 1: Storyboard
            TabButton {
                id: storyboardTabBtn
                objectName: "storyboardTabBtn"
                implicitHeight: 32
                enabled: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
                opacity: enabled ? 1.0 : 0.4
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Sequence shots, assign keyframes, and structure timeline beats")
                background: Rectangle {
                    color: storyboardTabBtn.checked ? "#1A4A68" : (storyboardTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: RowLayout {
                    spacing: 6
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen)
                               ? "#00F0FF" : "#666666"
                    }
                    Text {
                        text: qsTr("🎬 Storyboard")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: storyboardTabBtn.checked
                    }
                }
            }

            // Tab 2: Script
            TabButton {
                id: scriptTabBtn
                objectName: "scriptTabBtn"
                implicitHeight: 32
                enabled: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
                opacity: enabled ? 1.0 : 0.4
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Screenplay, beats editor, markdown preview, and prompt dispatcher")
                background: Rectangle {
                    color: scriptTabBtn.checked ? "#1A4A68" : (scriptTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: RowLayout {
                    spacing: 6
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: workspaceRoot.isScriptReady ? "#00F0FF" : "#FF4444"
                    }
                    Text {
                        text: qsTr("📝 Script")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: scriptTabBtn.checked
                    }
                }
            }

            // Tab 3: Images
            TabButton {
                id: imageTabBtn
                objectName: "imageTabBtn"
                implicitHeight: 32
                enabled: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
                opacity: enabled ? 1.0 : 0.4
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Image gallery, conditioning frames, and FLUX batch candidate selector")
                background: Rectangle {
                    color: imageTabBtn.checked ? "#1A4A68" : (imageTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: RowLayout {
                    spacing: 6
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: workspaceRoot.isImageReady ? "#00F0FF" : "#FF4444"
                    }
                    Text {
                        text: qsTr("🖼️ Images")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: imageTabBtn.checked
                    }
                }
            }

            // Tab 4: Audio
            TabButton {
                id: audioTabBtn
                objectName: "audioTabBtn"
                implicitHeight: 32
                enabled: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
                opacity: enabled ? 1.0 : 0.4
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Audio library, synthetic PCM waveform inspector, and speech playback")
                background: Rectangle {
                    color: audioTabBtn.checked ? "#1A4A68" : (audioTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: RowLayout {
                    spacing: 6
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: workspaceRoot.isAudioReady ? "#00F0FF" : "#FF4444"
                    }
                    Text {
                        text: qsTr("🔊 Audio")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: audioTabBtn.checked
                    }
                }
            }

            // Tab 5: Video
            TabButton {
                id: videoTabBtn
                objectName: "videoTabBtn"
                implicitHeight: 32
                enabled: typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen
                opacity: enabled ? 1.0 : 0.4
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Video viewport, scrubbing timeline, and clip library")
                background: Rectangle {
                    color: videoTabBtn.checked ? "#1A4A68" : (videoTabBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                }
                contentItem: RowLayout {
                    spacing: 6
                    Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: workspaceRoot.isVideoReady ? "#00F0FF" : "#FF4444"
                    }
                    Text {
                        text: qsTr("🎞️ Video")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: videoTabBtn.checked
                    }
                }
            }
        }

        // ==========================================
        // 3. MASTER STACK OF ACTIVE MODALITIES
        // ==========================================
        StackLayout {
            id: workspaceStack
            objectName: "workspaceStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: workspaceTabBar.currentIndex

            // TAB 0: PROJECT DETAILS
            Item {
                id: projectDetailsTab
                objectName: "projectDetailsTab"
                Rectangle {
                    anchors.fill: parent
                    color: "#08121A"
                    ScrollView {
                        anchors.fill: parent
                        anchors.margins: 14
                        clip: true
                        ColumnLayout {
                            width: parent.width - 24
                            spacing: 12

                            SeriesHeaderCard {
                                id: seriesHeaderCard
                                objectName: "seriesHeaderCard"
                                Layout.fillWidth: true
                                onNewSeriesRequested: newProjectDialog.open()
                            }

                            ProjectDetailsCard {
                                id: projectDetailsCard
                                objectName: "projectDetailsCard"
                                Layout.fillWidth: true
                            }

                            EpisodeListCard {
                                id: episodeListCard
                                objectName: "episodeListCard"
                                Layout.fillWidth: true

                                onNewEpisodeRequested: newProjectDialog.open()
                            }
                        }
                    }
                }
            }

            // TAB 1: STORYBOARD
            StoryBoardWorkspaceView {
                id: storyBoardView
                objectName: "storyBoardView"
                episodeProjectPath: (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen)
                                     ? projectManager.currentProjectPath : ""
            }

            // TAB 2: SCRIPT WORKSPACE
            AssetWorkspaceSplitView {
                id: scriptWorkspace
                objectName: "scriptWorkspace"
                targetAssetType: "script"

                EditorPane {
                    id: mainEditorPane
                    objectName: "mainEditorPane"
                    anchors.fill: parent
                    anchors.margins: 4

                    onNewScriptTriggered: workspaceRoot.newScriptTriggered()
                    onOpenScriptTriggered: workspaceRoot.openScriptTriggered()
                    onSaveScriptTriggered: workspaceRoot.saveScriptTriggered()
                    onCloseScriptTriggered: workspaceRoot.closeScriptTriggered()
                    onRecentDocumentTriggered: (path) => workspaceRoot.recentDocumentTriggered(path)

                    onSendToImagePromptTriggered: (payload) => {
                        workspaceTabBar.currentIndex = 3
                        if (imageWorkspace.promptBuilder) imageWorkspace.promptBuilder.promptText = payload
                    }
                    onSendToAudioPromptTriggered: (payload) => {
                        workspaceTabBar.currentIndex = 4
                        if (audioWorkspace.promptBuilder) audioWorkspace.promptBuilder.promptText = payload
                    }
                    onSendToVideoPromptTriggered: (payload) => {
                        workspaceTabBar.currentIndex = 5
                        if (videoWorkspace.promptBuilder) videoWorkspace.promptBuilder.promptText = payload
                    }
                }
            }

            // TAB 3: IMAGES WORKSPACE
            AssetWorkspaceSplitView {
                id: imageWorkspace
                objectName: "imageWorkspace"
                targetAssetType: "image"

                ImageGalleryWidget {
                    id: imageGalleryWidget
                    objectName: "imageGalleryWidget"
                    anchors.fill: parent
                    anchors.margins: 4
                }
            }

            // TAB 4: AUDIO WORKSPACE
            AssetWorkspaceSplitView {
                id: audioWorkspace
                objectName: "audioWorkspace"
                targetAssetType: "audio"

                AudioWorkspaceWidget {
                    id: audioWorkspaceWidget
                    objectName: "audioWorkspaceWidget"
                    anchors.fill: parent
                    anchors.margins: 4
                }
            }

            // TAB 5: VIDEO WORKSPACE
            AssetWorkspaceSplitView {
                id: videoWorkspace
                objectName: "videoWorkspace"
                targetAssetType: "video"

                VideoWorkspaceWidget {
                    id: videoWorkspaceWidget
                    objectName: "videoWorkspaceWidget"
                    anchors.fill: parent
                    anchors.margins: 4
                }
            }
        }
    }

    // ==========================================
    // 4. IMAGE CANDIDATE MODAL
    // ==========================================
    ImageCandidateModal {
        id: candidateModal
        objectName: "candidateModal"

        onImageSelected: (base64Data) => {
            if (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen) {
                let beforeFiles = workspaceRoot.snapshotImageModelFiles()
                let result = projectManager.saveCandidateImage(base64Data)
                if (result) {
                    imageGalleryWidget.updateProjectPath(projectManager.currentProjectPath)

                    let savedName = (typeof result === "string" && result.length > 0)
                                    ? workspaceRoot.extractFileName(result)
                                    : ""
                    if (savedName.length === 0) {
                        let afterFiles = workspaceRoot.snapshotImageModelFiles()
                        for (let i = 0; i < afterFiles.length; ++i) {
                            if (beforeFiles.indexOf(afterFiles[i]) < 0) {
                                savedName = workspaceRoot.extractFileName(afterFiles[i])
                                break
                            }
                        }
                    }

                    if (savedName.length > 0 && workspaceRoot.pendingSavedImageFiles.indexOf(savedName) < 0) {
                        let nextFiles = workspaceRoot.pendingSavedImageFiles.slice()
                        nextFiles.push(savedName)
                        workspaceRoot.pendingSavedImageFiles = nextFiles
                    }

                    if (typeof auditTrailStore !== "undefined" && auditTrailStore !== null && workspaceRoot.pendingSavedImageFiles.length > 0) {
                        auditTrailStore.updateTransactionFile(
                            workspaceRoot.lastImageSentTimestamp,
                            workspaceRoot.pendingSavedImageFiles.join(", "),
                            "saved",
                            "image"
                        )
                    }

                    if (imageWorkspace.promptBuilder) {
                        imageWorkspace.promptBuilder.appendResponseText(
                            savedName.length > 0
                                ? qsTr("Image saved to project assets: %1").arg(savedName)
                                : qsTr("Image saved successfully to project assets.")
                        )
                    }
                }
            }
        }

        onAllDiscarded: {
            if (typeof auditTrailStore !== "undefined" && auditTrailStore !== null) {
                auditTrailStore.updateTransactionFile(
                    workspaceRoot.lastImageSentTimestamp,
                    "",
                    "discarded",
                    "image"
                )
            }
            if (imageWorkspace.promptBuilder) {
                imageWorkspace.promptBuilder.appendResponseText("Image generation candidates discarded by user.")
            }
        }
    }

    // Connect to engineRegistry activeImageDriver for generated candidate popups
    Connections {
        target: (typeof engineRegistry !== "undefined" && engineRegistry !== null) ? engineRegistry.activeImageDriver : null
        ignoreUnknownSignals: true
        function onGenerationFinished(payload) {
            if (payload && payload.candidates && payload.candidates.length > 0) {
                workspaceRoot.pendingSavedImageFiles = []
                workspaceRoot.lastImageSentTimestamp = payload.sentTimestamp || ""
                candidateModal.openWithCandidates(payload.candidates)
            }
        }
    }

    // Auto-refresh Audio gallery & waveform player when audio.cpp or Kokoro finishes
    Connections {
        target: (typeof engineRegistry !== "undefined" && engineRegistry !== null) ? engineRegistry.activeAudioDriver : null
        ignoreUnknownSignals: true
        function onGenerationFinished(payload) {
            if (!payload) return
            workspaceRoot.refreshAudioWorkspace(payload.filePath || "")
            if (typeof auditTrailStore !== "undefined" && auditTrailStore !== null && payload.filePath && payload.filePath.length > 0) {
                auditTrailStore.updateTransactionFile(
                    payload.sentTimestamp || "",
                    payload.filePath,
                    "saved",
                    "audio"
                )
            }
        }
    }

    // Project state synchronization
    Connections {
        target: typeof projectManager !== "undefined" ? projectManager : null
        ignoreUnknownSignals: true

        function syncProjectState() {
            let path = (projectManager && projectManager.isProjectOpen) ? projectManager.currentProjectPath : ""

            if (typeof storyBoardView !== "undefined") storyBoardView.episodeProjectPath = path

            imageGalleryWidget.updateProjectPath(path)
            audioWorkspaceWidget.updateProjectPath(path)
            videoWorkspaceWidget.updateProjectPath(path)

            if (!projectManager || !projectManager.isProjectOpen) {
                workspaceTabBar.currentIndex = 0
            }
        }

        function onCurrentProjectPathChanged() { syncProjectState() }
        function onIsProjectOpenChanged() { syncProjectState() }
    }

    // Refresh video gallery
    Connections {
        target: (typeof engineRegistry !== "undefined" && engineRegistry !== null) ? engineRegistry.activeVideoDriver : null
        ignoreUnknownSignals: true
        function onGenerationFinished(payload) {
            if (!payload) return

            if (payload.filePath && payload.filePath.length > 0) {
                videoWorkspaceWidget.previewVideoFile(payload.filePath)
                if (typeof auditTrailStore !== "undefined" && auditTrailStore !== null) {
                    auditTrailStore.updateTransactionFile(
                        payload.sentTimestamp || "",
                        payload.filePath,
                        "saved",
                        "video"
                    )
                }
                if (videoWorkspace.promptBuilder) {
                    videoWorkspace.promptBuilder.appendResponseText(
                        qsTr("Video saved and loaded: %1").arg(payload.filePath)
                    )
                }
            } else {
                videoWorkspaceWidget.directoryModel.refresh()
            }
        }
    }

    Component.onCompleted: {
        if (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen) {
            let path = projectManager.currentProjectPath
            imageGalleryWidget.updateProjectPath(path)
            audioWorkspaceWidget.updateProjectPath(path)
            videoWorkspaceWidget.updateProjectPath(path)
        }
    }
}

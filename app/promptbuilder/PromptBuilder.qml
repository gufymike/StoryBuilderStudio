/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: PromptBuilder.qml
**
****************************************************************************/
import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

Item {
    id: promptBuilderRoot
    objectName: "promptBuilderRoot"
    readonly property string promptCategory: targetAssetType === "script" ? "text" : targetAssetType
    property string targetAssetType: "script" // "script", "image", "audio", "video"
    property alias promptText: promptTextArea.text
    property bool isGenerating: false
    property string lastDispatchedPrompt: ""
    property string lastSentTimestamp: ""

    // Tracks unadulterated user input and active checkbox options
    property string lastRawPrompt: ""
    property var selectedOptions: []

    // Tracks user-attached files
    property var attachedFileList: []

    // Test inspection hooks
    property var mockEngineRegistry: null
    property string mockDriverId: ""
    property var mockAvailableModels: null

    property alias currentDriverText: driverSelectorBox.currentText
    property alias currentModelText: modelSelectorBox.currentText

    readonly property var activeDriver: {
        if (mockEngineRegistry) return mockEngineRegistry.activeDriver
        if (typeof engineRegistry === "undefined" || engineRegistry === null) return null
        if (targetAssetType === "script") return engineRegistry.activeScriptDriver
        if (targetAssetType === "image")  return engineRegistry.activeImageDriver
        if (targetAssetType === "audio")  return engineRegistry.activeAudioDriver
        if (targetAssetType === "video")  return engineRegistry.activeVideoDriver
        return null
    }

    readonly property string engineDisplayName: {
        if (mockDriverId.length > 0) return mockDriverId
        if (activeDriver) {
            let base = (activeDriver.loadedModelName && activeDriver.loadedModelName.length > 0)
                       ? activeDriver.loadedModelName
                       : (activeDriver.driverId || "Engine")
            if (modelSelectorBox.visible && modelSelectorBox.currentText.length > 0) {
                return modelSelectorBox.currentText
            }
            return base
        }
        return qsTr("No Engine Selected")
    }

    readonly property var availableModelsForActiveDriver: {
        if (mockAvailableModels !== null) return mockAvailableModels
        if (!activeDriver || typeof settingsManager === "undefined" || settingsManager === null) {
            return []
        }

        let driverId = activeDriver.driverId
        if (driverId === "llama_cpp") {
            let list = []
            let providers = settingsManager.scriptProviders || []
            for (let i = 0; i < providers.length; ++i) {
                let name = providers[i].name || ""
                if (name.startsWith("Local: ")) {
                    list.push(name.replace(/^Local:\s*/, ""))
                }
            }
            return list
        } else if (driverId === "sd_image") {
            let list = []
            let providers = settingsManager.imageProviders || []
            for (let i = 0; i < providers.length; ++i) {
                let name = providers[i].name || ""
                list.push(name.replace(/^Local:\s*/, ""))
            }
            return list
        } else if (driverId === "sd_video") {
            let list = []
            let providers = settingsManager.videoProviders || []
            for (let i = 0; i < providers.length; ++i) {
                let name = providers[i].name || ""
                list.push(name.replace(/^Local:\s*/, ""))
            }
            return list
        } else if (driverId === "audio_cpp") {
            return settingsManager.availableAudioCppModels || []
        } else if (driverId === "kokoro_tts") {
            return ["model.onnx"]
        }
        return []
    }
    readonly property bool isLocalDriver: {
        if (!activeDriver || !activeDriver.driverId) return false
        const id = activeDriver.driverId
        return id === "llama_cpp" || id === "sd_image" || id === "sd_video" ||
               id === "kokoro_tts" || id === "audio_cpp" || id === "mock_engine"
    }

    readonly property string resolvedEstimatorEngine: {
        if (activeDriver && !isLocalDriver) return "remote"
        if (activeDriver && activeDriver.driverId === "audio_cpp") return "audio_cpp"
        if (activeDriver && activeDriver.driverId === "kokoro_tts") return "audio_kokoro"
        return targetAssetType
    }
    signal promptSubmitted(string finalPrompt, string targetAssetType)
    signal audioGenerated(string filePath)
    // Outgoing notifications for parent workspace view orchestration
    signal scriptGenerated(string scriptText, string category)
    signal imageCandidatesReady(var candidateList)
    signal videoRenderFinished(string outputDir, int frameCount, double fps)
    signal engineErrorOccurred(string driverId, string errorMessage)

    function resolveAttachFolderUrl() {
        if (typeof projectManager === "undefined" || !projectManager || !projectManager.isProjectOpen) {
            return ""
        }
        let base = projectManager.currentProjectPath
        if (!base || base.length === 0) return ""

        let sub = ""
        if (targetAssetType === "image") sub = "/assets/images"
        else if (targetAssetType === "audio") sub = "/assets/audio"
        else if (targetAssetType === "video") sub = "/assets/video"
        else if (targetAssetType === "script") sub = "/scripts"

        let fullPath = (base + sub).replace(/\\/g, "/")
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + fullPath
    }

    function addAttachedFiles(fileUrls) {
        let list = attachedFileList.slice()
        for (let i = 0; i < fileUrls.length; ++i) {
            let path = fileUrls[i].toString().replace(/^file:\/\//i, "")
            if (Qt.platform.os === "windows" && path.startsWith("/")) {
                path = path.substring(1)
            }
            if (list.indexOf(path) === -1) {
                list.push(path)
            }
        }
        attachedFileList = list
    }

    function removeAttachedFile(index) {
        let list = attachedFileList.slice()
        list.splice(index, 1)
        attachedFileList = list
    }

    function getFirstAttachedImage() {
        for (let i = 0; i < attachedFileList.length; ++i) {
            let path = attachedFileList[i].toString().toLowerCase()
            if (path.endsWith(".png") || path.endsWith(".jpg") ||
                path.endsWith(".jpeg") || path.endsWith(".webp") ||
                path.endsWith(".bmp")) {
                return attachedFileList[i]
            }
        }
        return ""
    }

    function getCombinedPromptText() {
        let raw = promptTextArea.text.trim()
        let optionsPart = selectedOptions.join(", ")
        if (optionsPart.length === 0) return raw
        if (raw.length === 0) return optionsPart
        return raw + ", " + optionsPart
    }

    function toggleOption(optionText, checked) {
        let list = selectedOptions.slice()
        let index = list.indexOf(optionText)
        if (checked && index === -1) {
            list.push(optionText)
        } else if (!checked && index !== -1) {
            list.splice(index, 1)
        }
        selectedOptions = list
    }

    function appendResponseText(text) {
        isGenerating = false
        responseDisplayArea.appendIncomingText(text)
        recordAuditTransaction(targetAssetType, text, false)
    }

    function recordAuditTransaction(assetType, payloadText, isThinkText) {
        if (typeof auditTrailStore !== "undefined" && auditTrailStore !== null &&
            typeof projectManager !== "undefined" && projectManager !== null) {

            let receivedTimestamp = new Date().toISOString()
            auditTrailStore.recordTransaction(
                projectManager.projectName,
                assetType,
                lastRawPrompt.length > 0 ? lastRawPrompt : lastDispatchedPrompt,
                payloadText,
                lastSentTimestamp,
                receivedTimestamp,
                selectedOptions,
                Boolean(isThinkText)
            )
        }
    }

    FileDialog {
        id: attachFileDialog
        title: targetAssetType === "video"
               ? qsTr("Select Conditioning Image or Context Files")
               : qsTr("Select Files to Attach to Prompt")
        fileMode: FileDialog.OpenFiles
        nameFilters: targetAssetType === "video"
                     ? ["Image Files (*.png *.jpg *.jpeg *.webp *.bmp)", "Text & Context (*.txt *.json *.md)", "All Files (*)"]
                     : ["All Supported Files (*.png *.jpg *.jpeg *.txt *.md *.json)", "All Files (*)"]
        onAccepted: promptBuilderRoot.addAttachedFiles(selectedFiles)
    }

    InferenceEstimateDialog {
        id: standaloneEstimateDialog
        engineType: promptBuilderRoot.resolvedEstimatorEngine
        promptText: promptTextArea.text
        steps: (inferenceSettingsPanel.currentOptions && inferenceSettingsPanel.currentOptions.steps !== undefined)
               ? inferenceSettingsPanel.currentOptions.steps : 4
        widthVal: (inferenceSettingsPanel.currentOptions && inferenceSettingsPanel.currentOptions.width !== undefined)
                  ? inferenceSettingsPanel.currentOptions.width : 512
        heightVal: (inferenceSettingsPanel.currentOptions && inferenceSettingsPanel.currentOptions.height !== undefined)
                   ? inferenceSettingsPanel.currentOptions.height : 512
    }

    implicitWidth: 600
    implicitHeight: 520
    // ==========================================
    // SPLIT WORKSPACE: UPPER (RESPONSE) & LOWER (INPUT/OPTIONS)
    // ==========================================
    SplitView {
        id: promptBuilderSplitter
        objectName: "promptBuilderSplitter"
        anchors.fill: parent
        anchors.margins: 10
        orientation: Qt.Vertical

        // Custom splitter handle with visual grab indicator
        handle: Rectangle {
            id: splitHandle
            implicitHeight: 8
            color: SplitHandle.hovered || SplitHandle.pressed ? "#00F0FF" : "#1A4A68"
            radius: 2

            // Visual grab bar in center
            Rectangle {
                anchors.centerIn: parent
                width: 36
                height: 2
                color: SplitHandle.pressed ? "#FFFFFF" : "#08121A"
                radius: 1
            }
        }

        // ----------------------------------------------------
        // PANE 1: UPPER (ENGINE CONTROLS + RESPONSE DISPLAY)
        // ----------------------------------------------------
        Item {
            id: upperResponsePane
            objectName: "upperResponsePane"
            SplitView.minimumHeight: 90
            SplitView.preferredHeight: 180
            SplitView.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                // Engine Selector Row (Engine, ComboBoxes, Load Button, Profiler)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        text: qsTr("Engine:")
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 12
                        style: Text.Outline
                        styleColor: "#000000"
                        verticalAlignment: Text.AlignVCenter
                    }

                    ComboBox {
                        id: driverSelectorBox
                        objectName: "driverSelectorBox"
                        implicitWidth: 140
                        implicitHeight: 26
                        model: (typeof engineRegistry !== "undefined" && engineRegistry !== null && typeof engineRegistry.availableDriversForAsset === "function")
                               ? (engineRegistry.availableDriversForAsset(promptBuilderRoot.targetAssetType) || [])
                               : []

                        currentIndex: (model && model.length > 0 && typeof engineRegistry.getActiveDriverId === "function")
                                      ? Math.max(0, model.indexOf(engineRegistry.getActiveDriverId(promptBuilderRoot.targetAssetType)))
                                      : 0

                        onActivated: (index) => {
                            let selectedDriverId = model[index]
                            if (typeof engineRegistry !== "undefined" && engineRegistry && typeof engineRegistry.setActiveDriver === "function") {
                                engineRegistry.setActiveDriver(promptBuilderRoot.targetAssetType, selectedDriverId)
                            }
                        }
                    }

                    ComboBox {
                        id: modelSelectorBox
                        objectName: "modelSelectorBox"
                        implicitWidth: 175
                        implicitHeight: 26
                        visible: model && model.length > 0 && activeDriver && activeDriver.driverId !== "kokoro_tts"
                        model: promptBuilderRoot.availableModelsForActiveDriver
                    }

                    Button {
                        id: driverLoadBtn
                        objectName: "driverLoadBtn"
                        implicitHeight: 26
                        implicitWidth: 70
                        enabled: activeDriver !== null && !activeDriver.isBusy

                        text: activeDriver && activeDriver.isBusy
                              ? qsTr("...")
                              : (activeDriver && activeDriver.isModelLoaded ? qsTr("Unload") : qsTr("Load"))

                        background: Rectangle {
                            color: driverLoadBtn.down
                                   ? "#0077A8"
                                   : (activeDriver && activeDriver.isModelLoaded ? "#331515" : "#0B1B26")
                            border.color: activeDriver && activeDriver.isModelLoaded ? "#FF4444" : "#00F0FF"
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: driverLoadBtn.text
                            color: activeDriver && activeDriver.isModelLoaded ? "#FF6666" : "#00F0FF"
                            font.bold: true
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            if (!activeDriver) return
                            let driverId = activeDriver.driverId
                            if (activeDriver.isModelLoaded) {
                                responseDisplayArea.appendIncomingText(">>> Unloading " + driverId + " from memory...")
                                activeDriver.unloadModel()
                                if (!activeDriver.isModelLoaded) {
                                    responseDisplayArea.appendIncomingText(">>> " + driverId + " unloaded.")
                                }
                            } else {
                                let selectedModel = modelSelectorBox.visible ? modelSelectorBox.currentText : ""
                                responseDisplayArea.appendIncomingText(">>> Initializing " + driverId + " with model: " + (selectedModel.length > 0 ? selectedModel : "(default)") + "...")
                                let config = (typeof settingsManager !== "undefined" && settingsManager !== null)
                                           ? settingsManager.getDriverLoadConfig(driverId, selectedModel)
                                           : ({})
                                let loaded = activeDriver.loadModel(config)
                                if (loaded) {
                                    let readyName = (activeDriver.loadedModelName && activeDriver.loadedModelName.length > 0)
                                                    ? activeDriver.loadedModelName
                                                    : (selectedModel.length > 0 ? selectedModel : driverId)
                                    responseDisplayArea.appendIncomingText(">>> " + driverId + " ready (" + readyName + ").")
                                }
                            }
                        }
                    }

                    Item { Layout.fillWidth: true }

                    ToolButton {
                        id: profilerBtn
                        objectName: "profilerBtn"
                        text: qsTr("⚡ Specs & Profiler")
                        hoverEnabled: true
                        contentItem: Text {
                            text: profilerBtn.text
                            color: profilerBtn.hovered ? "#00F0FF" : "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                            style: Text.Outline
                            styleColor: "#000000"
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: profilerBtn.down ? "#0077A8" : (profilerBtn.hovered ? "#0B1B26" : "transparent")
                            border.color: profilerBtn.hovered ? "#1A4A68" : "#08121A"
                            border.width: 1
                            radius: 4
                        }
                        onClicked: {
                            standaloneEstimateDialog.setReferences(promptBuilderRoot.attachedFileList)
                            standaloneEstimateDialog.recalculate()
                            standaloneEstimateDialog.open()
                        }
                    }
                }

                // Response Output Well
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#0B1B26"
                    border.color: promptBuilderRoot.isGenerating ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                    clip: true

                    ScrollView {
                        anchors.fill: parent
                        clip: true

                        TextArea {
                            id: responseDisplayArea
                            objectName: "responseDisplayArea"
                            readOnly: true
                            selectByMouse: true
                            placeholderText: qsTr("Returned output from %1 will appear here...").arg(promptBuilderRoot.engineDisplayName)
                            color: "#00F0FF"
                            font.family: "Courier New, monospace"
                            font.pixelSize: 12
                            wrapMode: TextArea.Wrap
                            background: null

                            function appendIncomingText(newText) {
                                let cleaned = String(newText).replace(/^\n+/, "")
                                if (cleaned.length === 0) return
                                if (responseDisplayArea.text.length === 0) {
                                    responseDisplayArea.text = cleaned
                                } else {
                                    responseDisplayArea.append(cleaned)
                                }
                                responseDisplayArea.cursorPosition = responseDisplayArea.length
                            }
                        }

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: promptBuilderRoot.isGenerating
                            visible: promptBuilderRoot.isGenerating
                        }
                    }
                }
            }
        }

        // ----------------------------------------------------
        // PANE 2: LOWER (TOOLBAR, DIRECTIVES, INPUT, DISPATCH)
        // ----------------------------------------------------
        Item {
            id: lowerInputPane
            objectName: "lowerInputPane"
            SplitView.minimumHeight: 180
            SplitView.fillHeight: true
            SplitView.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 8

                // Workspace Tab Bar & Options Drawer Toggle
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RowLayout {
                        spacing: 2

                        Button {
                            id: promptTabBtn
                            objectName: "promptTabBtn"
                            text: qsTr("Prompt Text")
                            implicitHeight: 26
                            background: Rectangle {
                                color: workspaceStack.currentIndex === 0 ? "#1A4A68" : "#0B1B26"
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: workspaceStack.currentIndex === 0
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: workspaceStack.currentIndex = 0
                        }

                        Button {
                            id: settingsTabBtn
                            objectName: "settingsTabBtn"
                            text: qsTr("⚙ Model Options")
                            implicitHeight: 26
                            background: Rectangle {
                                color: workspaceStack.currentIndex === 1 ? "#1A4A68" : "#0B1B26"
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: parent.text
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: workspaceStack.currentIndex === 1
                                font.pixelSize: 11
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: workspaceStack.currentIndex = 1
                        }
                    }

                    Button {
                        id: toggleDrawerBtn
                        objectName: "toggleDrawerBtn"
                        text: promptOptionsPanel.visible ? qsTr("▲ Hide Prompts") : qsTr("▼ Prompts")
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: toggleDrawerBtn.hovered ? "#00B4F0" : "#0B1B26"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            promptOptionsPanel.visible = !promptOptionsPanel.visible
                            if (promptOptionsPanel.visible) {
                                if (typeof promptManager !== "undefined" && promptManager) {
                                    promptManager.activeCategory = promptBuilderRoot.promptCategory
                                    if (typeof projectManager !== "undefined" && projectManager && projectManager.isProjectOpen) {
                                        let epPath = projectManager.currentProjectPath
                                        let seriesPath = (typeof seriesContainer !== "undefined" && seriesContainer) ? seriesContainer.rootDirectory : ""
                                        promptManager.setPaths(seriesPath, epPath)
                                    }
                                }
                            }
                        }
                    }

                    Button {
                        id: editDefaultsBtn
                        objectName: "editDefaultsBtn"
                        text: qsTr("✏ Edit Prompts")
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: editDefaultsBtn.hovered ? "#00B4F0" : "#0B1B26"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            editorWindow.showCentered(promptBuilderRoot.Window.window)
                        }
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        id: promptHistoryBtn
                        objectName: "promptHistoryBtn"
                        text: qsTr("📜 History")
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: promptHistoryBtn.hovered ? "#00B4F0" : "#0B1B26"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            historyListModel.clear()
                            if (typeof promptHistoryStore !== "undefined") {
                                let entries = promptHistoryStore.getHistoryEntries()
                                for (let i = 0; i < entries.length; i++) {
                                    if (entries[i]["category"] === targetAssetType || targetAssetType === "") {
                                        historyListModel.append(entries[i])
                                    }
                                }
                            }
                            historyPopup.open()
                        }
                    }

                    Button {
                        id: clearInputBtn
                        objectName: "clearInputBtn"
                        text: qsTr("Clear Input")
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: clearInputBtn.down ? "#0077A8" : (clearInputBtn.hovered ? "#00B4F0" : "#0B1B26")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            promptTextArea.text = ""
                            promptBuilderRoot.attachedFileList = []
                        }
                    }
                }

                // Directives Drawer Panel
                PromptOptionsPanel {
                    id: promptOptionsPanel
                    objectName: "promptOptionsPanel"
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 130 : 0
                    visible: false
                    activeSelectedList: promptBuilderRoot.selectedOptions
                    targetCategory: promptBuilderRoot.promptCategory
                    onPromptOptionToggled: (content, isChecked) => promptBuilderRoot.toggleOption(content, isChecked)
                }

                // Workspace Stack (Prompt Text vs Inference Settings)
                StackLayout {
                    id: workspaceStack
                    objectName: "workspaceStack"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: 0

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 6

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            TextArea {
                                id: promptTextArea
                                objectName: "promptTextArea"
                                placeholderText: qsTr("Type your prompt here or select directives from above...")
                                color: "#FFFFFF"
                                font.pixelSize: 13
                                wrapMode: TextArea.Wrap
                                background: Rectangle {
                                    color: "#0B1B26"
                                    border.color: promptTextArea.activeFocus ? "#00F0FF" : "#1A4A68"
                                    border.width: 1
                                    radius: 4
                                }
                            }
                        }

                        // Attached Files Tray
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 32
                            color: "#08121A"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 4

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 4
                                spacing: 6

                                Button {
                                    id: attachFileBtn
                                    objectName: "attachFileBtn"
                                    text: qsTr("📎 Attach File(s)")
                                    implicitHeight: 24
                                    font.pixelSize: 10
                                    background: Rectangle {
                                        color: attachFileBtn.hovered ? "#00B4F0" : "#0B1B26"
                                        border.color: "#1A4A68"
                                        border.width: 1
                                        radius: 3
                                    }
                                    contentItem: Text {
                                        text: parent.text
                                        color: "#00F0FF"
                                        font.bold: true
                                        style: Text.Outline
                                        styleColor: "#000000"
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    onClicked: {
                                        let folderUrl = promptBuilderRoot.resolveAttachFolderUrl()
                                        if (folderUrl.length > 0) {
                                            attachFileDialog.currentFolder = folderUrl
                                        }
                                        attachFileDialog.open()
                                    }
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                                    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
                                    clip: true

                                    Row {
                                        spacing: 6

                                        Repeater {
                                            model: promptBuilderRoot.attachedFileList

                                            delegate: Rectangle {
                                                height: 22
                                                width: chipRow.implicitWidth + 12
                                                color: "#0B1B26"
                                                border.color: "#00F0FF"
                                                radius: 3

                                                RowLayout {
                                                    id: chipRow
                                                    anchors.fill: parent
                                                    anchors.margins: 4
                                                    spacing: 4

                                                    Text {
                                                        text: {
                                                            let parts = modelData.split(/[\\/]/)
                                                            return parts[parts.length - 1]
                                                        }
                                                        color: "#FFFFFF"
                                                        font.pixelSize: 10
                                                        elide: Text.ElideMiddle
                                                        Layout.maximumWidth: 140
                                                    }

                                                    Text {
                                                        text: "✕"
                                                        color: "#FF6666"
                                                        font.pixelSize: 10
                                                        font.bold: true
                                                        MouseArea {
                                                            anchors.fill: parent
                                                            cursorShape: Qt.PointingHandCursor
                                                            onClicked: promptBuilderRoot.removeAttachedFile(index)
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        Text {
                                            visible: promptBuilderRoot.attachedFileList.length === 0
                                            text: qsTr("No conditioning files attached (Optional: attach lore or reference images)")
                                            color: "#888888"
                                            font.italic: true
                                            font.pixelSize: 10
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }
                                }
                            }
                        }
                    }

                    InferenceSettingsPanel {
                        id: inferenceSettingsPanel
                        objectName: "inferenceSettingsPanel"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        targetAssetType: promptBuilderRoot.targetAssetType
                    }
                }

                // Dispatch Button
                Button {
                    id: submitPromptBtn
                    objectName: "submitPromptBtn"
                    text: {
                        if (!activeDriver) return qsTr("No Active Driver Available")
                        if (!activeDriver.isModelLoaded) return qsTr("%1 Not Loaded — Click 'Load' Above").arg(activeDriver.driverId)
                        return qsTr("Submit Prompt to %1").arg(promptBuilderRoot.engineDisplayName)
                    }
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    enabled: (promptTextArea.text.trim().length > 0 || promptBuilderRoot.attachedFileList.length > 0) &&
                             !promptBuilderRoot.isGenerating &&
                             activeDriver !== null &&
                             activeDriver.isModelLoaded

                    background: Rectangle {
                        color: !submitPromptBtn.enabled ? "#1B3B4F" : (submitPromptBtn.down ? "#0077A8" : (submitPromptBtn.hovered ? "#00B4F0" : "#1A4A68"))
                        border.color: "#000000"
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: submitPromptBtn.text
                        color: submitPromptBtn.enabled ? "#FFFFFF" : "#888888"
                        style: submitPromptBtn.enabled ? Text.Outline : Text.Normal
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 13
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        previewTextArea.text = promptBuilderRoot.getCombinedPromptText()
                        submitPreviewPopup.open()
                    }
                }
            }
        }
    }
    PromptEditorDialog {
        id: editorWindow
        objectName: "editorWindow"
    }

    // ==========================================
    // 6. CONFIRMATION POPUP MODAL
    // ==========================================
    Popup {
        id: submitPreviewPopup
        objectName: "submitPreviewPopup"
        anchors.centerIn: parent
        width: 520
        height: 380
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 6
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 8

            Text {
                text: qsTr("Confirm Outgoing Dispatch Preview")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 14
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                TextArea {
                    id: previewTextArea
                    objectName: "previewTextArea"
                    readOnly: true
                    color: "#FFFFFF"
                    font.family: "Courier New, monospace"
                    font.pixelSize: 12
                    wrapMode: TextArea.Wrap
                    background: Rectangle {
                        color: "#0B1B26"
                        border.color: "#1A4A68"
                        border.width: 1
                        radius: 4
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Button {
                    id: cancelDispatchBtn
                    objectName: "cancelDispatchBtn"
                    text: qsTr("Cancel")
                    Layout.preferredWidth: 90
                    onClicked: submitPreviewPopup.close()
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: confirmDispatchBtn
                    objectName: "confirmDispatchBtn"
                    text: qsTr("Confirm & Dispatch")
                    Layout.preferredWidth: 150
                    background: Rectangle {
                        color: confirmDispatchBtn.down ? "#0077A8" : (confirmDispatchBtn.hovered ? "#00B4F0" : "#1A4A68")
                        border.color: "#000000"
                        border.width: 1
                        radius: 4
                    }
                    contentItem: Text {
                        text: parent.text
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        submitPreviewPopup.close()
                        if (!activeDriver) return

                        promptBuilderRoot.isGenerating = true
                        promptBuilderRoot.lastDispatchedPrompt = previewTextArea.text
                        promptBuilderRoot.lastRawPrompt = promptTextArea.text.trim()
                        promptBuilderRoot.lastSentTimestamp = new Date().toISOString()

                        responseDisplayArea.appendIncomingText(">>> Dispatching job to " + activeDriver.driverId + "...")

                        let targetDir = ""
                        if (typeof projectManager !== "undefined" && projectManager && projectManager.isProjectOpen) {
                            targetDir = projectManager.currentProjectPath + "/assets/" + promptBuilderRoot.targetAssetType
                        }

                        let authorStyle = (typeof settingsManager !== "undefined" && settingsManager !== null)
                                        ? settingsManager.scriptAuthorStyle
                                        : ""

                        activeDriver.generate(
                            previewTextArea.text,
                            authorStyle,
                            promptBuilderRoot.attachedFileList,
                            targetDir,
                            inferenceSettingsPanel.getOptionsPayload()
                        )

                        promptTextArea.text = ""
                    }
                }
            }
        }
    }
    // ==========================================
    // PROMPT HISTORY POPUP MODAL
    // ==========================================
    Popup {
        id: historyPopup
        objectName: "historyPopup"
        anchors.centerIn: parent
        width: 480
        height: 380
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle { color: "#1A1A1A"; border.color: "#333333"; radius: 6 }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            Text {
                text: qsTr("Prompt History Log")
                color: "#00A3E0"; font.bold: true; font.pixelSize: 16
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#252525" }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    id: historyListView
                    width: parent.width
                    model: ListModel { id: historyListModel }

                    delegate: ItemDelegate {
                        id: historyDelegate
                        width: historyListView.width
                        height: 40
                        contentItem: RowLayout {
                            anchors.fill: parent
                            spacing: 10

                            Text {
                                text: model.timestamp ? model.timestamp.replace("T", " ").substring(0, 19) : ""
                                color: "#888888"; font.pixelSize: 11; font.family: "Courier"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: model.promptText ? (model.promptText.length > 45 ? model.promptText.substring(0, 45) + "..." : model.promptText) : ""
                                color: "#FFFFFF"; font.pixelSize: 12; elide: Text.ElideRight
                            }
                        }

                        background: Rectangle {
                            color: historyDelegate.hovered ? "#2A2A2A" : "transparent"
                            radius: 3
                        }

                        onClicked: {
                            promptTextArea.text = model.promptText || ""
                            historyPopup.close()
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Button {
                    text: qsTr("Clear History")
                    font.pixelSize: 11
                    onClicked: {
                        if (typeof promptHistoryStore !== "undefined") {
                            promptHistoryStore.clearHistory()
                            historyListModel.clear()
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Clears the history file and the list of all entries.")
                }

                Item { Layout.fillWidth: true }

                Button {
                    text: qsTr("Close")
                    onClicked: historyPopup.close()
                }
            }
        }
    }

    // ==========================================
    // UNIFIED CONNECTIONS TO ACTIVE DRIVER
    // ==========================================
    Connections {
        target: promptBuilderRoot.activeDriver
        ignoreUnknownSignals: true

        function onTokenGenerated(token) {
            responseDisplayArea.insert(responseDisplayArea.length, token)
        }

        function onGenerationFinished(payload) {
            promptBuilderRoot.isGenerating = false

            let category = payload.category || promptBuilderRoot.targetAssetType
            let summaryText = ""
            let thinkText = ""

            // 1. Script generation
            if (payload.text !== undefined) {
                summaryText = payload.text
                thinkText = payload.think
                if (typeof promptHistoryStore !== "undefined" && promptHistoryStore) {
                    console.log("[QML] Calling addHistoryEntry...");
                    promptHistoryStore.addHistoryEntry(
                        category,
                        promptBuilderRoot.lastDispatchedPrompt,
                        payload.text,
                        payload.timestamp || new Date().toISOString()
                    )
                    if (payload.think && payload.think.length > 0) {
                        promptHistoryStore.addThinkEntry(
                            category,
                            promptBuilderRoot.lastDispatchedPrompt,
                            payload.think,
                            payload.timestamp || new Date().toISOString()
                        )
                    }

                } else {
                    console.error("[QML] promptHistoryStore is undefined in QML context!");
                }
                promptBuilderRoot.scriptGenerated(payload.text, category)
            }
            // 2. Image generation
            else if (payload.candidates !== undefined) {
                summaryText = "Image generation finished: " + (payload.count || payload.candidates.length) + " candidate variations ready."
                responseDisplayArea.appendIncomingText(">>> " + summaryText)

                // Bubble candidates array up to the workspace view
                promptBuilderRoot.imageCandidatesReady(payload.candidates)
            }
            // 3. Audio generation
            else if (payload.filePath !== undefined) {
                summaryText = "Audio saved to: " + payload.filePath
                responseDisplayArea.appendIncomingText(">>> " + summaryText)
                promptBuilderRoot.audioGenerated(payload.filePath)
            }
            // 4. Video generation
            else if (payload.outputDir !== undefined) {
                summaryText = "Video render finished: " + (payload.frameCount || 0) + " frames @ " + (payload.fps || 24) + " FPS saved to " + payload.outputDir
                responseDisplayArea.appendIncomingText(">>> " + summaryText)
                promptBuilderRoot.videoRenderFinished(payload.outputDir, payload.frameCount || 0, payload.fps || 24.0)
            }

            promptBuilderRoot.recordAuditTransaction(category, summaryText, false)
            if(thinkText != "") {
                promptBuilderRoot.recordAuditTransaction(category, thinkText, true)
            }
        }

        function onEngineError(errorMessage) {
            promptBuilderRoot.isGenerating = false
            let driverName = promptBuilderRoot.activeDriver ? promptBuilderRoot.activeDriver.driverId : "Engine"
            responseDisplayArea.appendIncomingText("[ERROR " + driverName + "]: " + errorMessage)
            promptBuilderRoot.engineErrorOccurred(driverName, errorMessage)
        }
    }
}

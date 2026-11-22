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
** File: SettingsControlPanel.qml
** Unified settings drawer for runtime providers, drivers, and manifests.
**
****************************************************************************/
import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

Popup {
    id: controlPanelRoot
    objectName: "settingsControlPanelRoot"
    anchors.centerIn: parent
    width: Math.min(860, parent ? parent.width - 40 : 820)
    height: Math.min(680, parent ? parent.height - 40 : 640)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    signal themeRestartRequested()
    signal rerunWizardRequested()

    // Unified local-vs-remote visibility state per modality
    readonly property bool isLocalScript: (typeof engineRegistry === "undefined")
                                          ? true : (engineRegistry.activeScriptDriver !== null && engineRegistry.activeScriptDriver.driverId === "llama_cpp")
    readonly property bool isLocalImage:  (typeof engineRegistry === "undefined")
                                          ? true : (engineRegistry.activeImageDriver !== null && engineRegistry.activeImageDriver.driverId === "sd_image")
    readonly property bool isLocalVideo:  (typeof engineRegistry === "undefined")
                                          ? true : (engineRegistry.activeVideoDriver !== null && engineRegistry.activeVideoDriver.driverId === "sd_video")
    readonly property bool isKokoroAudio: (typeof engineRegistry === "undefined")
                                          ? true : (engineRegistry.activeAudioDriver !== null && engineRegistry.activeAudioDriver.driverId === "kokoro_tts")
    readonly property bool isAudioCpp:    (typeof engineRegistry !== "undefined" && engineRegistry.activeAudioDriver !== null)
                                          ? (engineRegistry.activeAudioDriver.driverId === "audio_cpp") : false
    readonly property bool isRemoteAudio: !isKokoroAudio && !isAudioCpp

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 8
    }

    FolderDialog {
        id: generalFolderDialog
        objectName: "generalFolderDialog"
        property string targetSettingKey: ""
        title: qsTr("Select Directory")
        onAccepted: {
            let cleanPath = selectedFolder.toString().replace(/^file:\/\//i, "")
            if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) cleanPath = cleanPath.substring(1)

            if (typeof settingsManager !== "undefined") {
                if (targetSettingKey === "script") settingsManager.scriptModelsDir = cleanPath
                else if (targetSettingKey === "images") settingsManager.imagesRootDir = cleanPath
                else if (targetSettingKey === "video") settingsManager.videoRootDir = cleanPath
                else if (targetSettingKey === "audio") settingsManager.audioRootDir = cleanPath
                else if (targetSettingKey === "save") settingsManager.defaultSavePath = cleanPath
            }
        }
    }

    FileDialog {
        id: t5FileDialog
        objectName: "t5FileDialog"
        title: qsTr("Select Universal T5-XXL Encoder (.gguf / .safetensors)")
        nameFilters: ["Model Weights (*.gguf *.safetensors *.bin)", "All Files (*)"]
        onAccepted: {
            let cleanPath = selectedFile.toString().replace(/^file:\/\//i, "")
            if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) cleanPath = cleanPath.substring(1)
            if (typeof settingsManager !== "undefined") {
                settingsManager.globalT5Path = cleanPath
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: panelTitleText
                objectName: "panelTitleText"
                text: qsTr("⚙ Studio Configuration & Driver Engine Settings")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 15
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                id: closePanelToolBtn
                objectName: "closePanelToolBtn"
                text: "✕"
                font.pixelSize: 14
                contentItem: Text {
                    text: parent.text
                    color: parent.hovered ? "#00F0FF" : "#888888"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                }
                onClicked: controlPanelRoot.close()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Close settings panel")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Modality Tabs Bar
        TabBar {
            id: settingsTabBar
            objectName: "settingsTabBar"
            Layout.fillWidth: true
            implicitHeight: 32

            background: Rectangle {
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
            }

            TabButton {
                id: tabScript
                objectName: "tabScript"
                text: qsTr("📝 Script & LLM")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
            }

            TabButton {
                id: tabImage
                objectName: "tabImage"
                text: qsTr("🖼 Image Gen")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
            }

            TabButton {
                id: tabVideo
                objectName: "tabVideo"
                text: qsTr("🎥 Video Gen")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
            }

            TabButton {
                id: tabAudio
                objectName: "tabAudio"
                text: qsTr("🔊 Audio & Speech")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
            }

            TabButton {
                id: tabGeneral
                objectName: "tabGeneral"
                text: qsTr("🛠 General & Paths")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
            }
        }

        // Stack of Settings Content
        StackLayout {
            id: settingsStack
            objectName: "settingsStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: settingsTabBar.currentIndex

            // ==========================================
            // 1. SCRIPT / TEXT MODALITY
            // ==========================================
            ScrollView {
                clip: true
                ColumnLayout {
                    width: settingsStack.width - 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: qsTr("Active Driver:")
                            color: "#888888"
                            font.pixelSize: 11
                            Layout.preferredWidth: 90
                        }

                        ComboBox {
                            id: scriptDriverBox
                            objectName: "scriptDriverBox"
                            Layout.preferredWidth: 200
                            model: (typeof engineRegistry !== "undefined") ? engineRegistry.availableDriversForAsset("script") : ["llama_cpp", "gemini_bridge"]
                            currentIndex: (typeof engineRegistry !== "undefined") ? Math.max(0, model.indexOf(engineRegistry.getActiveDriverId("script"))) : 0
                            onActivated: (index) => {
                                if (typeof engineRegistry !== "undefined") {
                                    engineRegistry.setActiveDriver("script", model[index])
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            id: rescanWeightsBtn
                            objectName: "rescanWeightsBtn"
                            visible: controlPanelRoot.isLocalScript
                            text: qsTr("Rescan Weights")
                            font.pixelSize: 11
                            background: Rectangle {
                                color: rescanWeightsBtn.down ? "#0077A8" : (rescanWeightsBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
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
                                if (typeof settingsManager !== "undefined") {
                                    settingsManager.refreshScannedDirectories()
                                }
                            }
                        }
                    }

                    // Local Script Controls (Hidden when remote API is active)
                    ColumnLayout {
                        id: localScriptSection
                        Layout.fillWidth: true
                        spacing: 10
                        visible: controlPanelRoot.isLocalScript

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text { text: qsTr("Model Folder:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                            ComboBox {
                                id: scriptModelSelect
                                objectName: "scriptModelSelect"
                                Layout.fillWidth: true
                                model: (typeof settingsManager !== "undefined") ? settingsManager.scriptProviders : []
                                textRole: "name"
                                currentIndex: {
                                    if (typeof settingsManager === "undefined") return 0
                                    for (let i = 0; i < count; ++i) {
                                        if (model[i].name === settingsManager.activeScriptProvider) return i
                                    }
                                    return 0
                                }
                                onActivated: (idx) => {
                                    if (typeof settingsManager !== "undefined") {
                                        settingsManager.activeScriptProvider = model[idx].name
                                    }
                                }
                            }
                        }

                        LocalModalManifestCard {
                            id: scriptManifestCard
                            objectName: "scriptManifestCard"
                            category: "script"
                            modelName: (typeof settingsManager !== "undefined") ? settingsManager.activeScriptProvider : ""
                        }
                    }

                    // Remote Script API Card
                    RemoteApiConfigCard {
                        id: remoteScriptConfigCard
                        objectName: "remoteScriptConfigCard"
                        category: "script"
                        visible: !controlPanelRoot.isLocalScript
                        driverId: (typeof engineRegistry !== "undefined" && engineRegistry.activeScriptDriver) ? engineRegistry.activeScriptDriver.driverId : "gemini_bridge"
                    }

                    // Author Style Prompt
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            text: qsTr("Author System Directive:")
                            color: "#888888"
                            font.pixelSize: 11
                        }
                        TextArea {
                            id: scriptAuthorStyleArea
                            objectName: "scriptAuthorStyleArea"
                            Layout.fillWidth: true
                            implicitHeight: 70
                            wrapMode: TextArea.Wrap
                            color: "#FFFFFF"
                            font.pixelSize: 12
                            text: (typeof settingsManager !== "undefined") ? settingsManager.scriptAuthorStyle : ""
                            onTextChanged: {
                                if (typeof settingsManager !== "undefined") {
                                    settingsManager.scriptAuthorStyle = text
                                }
                            }
                            background: Rectangle {
                                color: "#08121A"
                                border.color: "#1A4A68"
                                radius: 4
                            }
                        }
                    }
                }
            }

            // ==========================================
            // 2. IMAGE MODALITY
            // ==========================================
            ScrollView {
                clip: true
                ColumnLayout {
                    width: settingsStack.width - 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text { text: qsTr("Active Driver:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                        ComboBox {
                            id: imageDriverBox
                            objectName: "imageDriverBox"
                            Layout.preferredWidth: 200
                            model: (typeof engineRegistry !== "undefined") ? engineRegistry.availableDriversForAsset("image") : ["sd_image"]
                            currentIndex: (typeof engineRegistry !== "undefined") ? Math.max(0, model.indexOf(engineRegistry.getActiveDriverId("image"))) : 0
                            onActivated: (idx) => {
                                if (typeof engineRegistry !== "undefined") {
                                    engineRegistry.setActiveDriver("image", model[idx])
                                }
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            id: browseImagesDirBtn
                            objectName: "browseImagesDirBtn"
                            visible: controlPanelRoot.isLocalImage
                            text: qsTr("Browse Images Dir")
                            font.pixelSize: 11
                            background: Rectangle {
                                color: browseImagesDirBtn.down ? "#0077A8" : (browseImagesDirBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
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
                                generalFolderDialog.targetSettingKey = "images"
                                generalFolderDialog.open()
                            }
                        }
                    }

                    // Local Image Controls (Hidden when remote API is active)
                    ColumnLayout {
                        id: localImageSection
                        Layout.fillWidth: true
                        spacing: 10
                        visible: controlPanelRoot.isLocalImage

                        // Universal T5 Selector
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text { text: qsTr("Global T5-XXL:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                            TextField {
                                id: globalT5Field
                                objectName: "globalT5Field"
                                Layout.fillWidth: true
                                readOnly: true
                                text: (typeof settingsManager !== "undefined") ? settingsManager.globalT5Path : ""
                                color: "#00F0FF"
                                font.pixelSize: 10
                                font.family: "Courier New, monospace"
                                background: Rectangle { color: "#08121A"; border.color: "#1A4A68"; radius: 3 }
                            }
                            Button {
                                id: changeT5Btn
                                objectName: "changeT5Btn"
                                text: qsTr("Change...")
                                font.pixelSize: 10
                                background: Rectangle {
                                    color: changeT5Btn.down ? "#0077A8" : (changeT5Btn.hovered ? "#00B4F0" : "#0B1B26")
                                    border.color: "#1A4A68"
                                    border.width: 1
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#FFFFFF"
                                    style: Text.Outline
                                    styleColor: "#000000"
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: t5FileDialog.open()
                            }
                        }

                        // Active Image Model Selector
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text { text: qsTr("Model Folder:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                            ComboBox {
                                id: imageModelSelect
                                objectName: "imageModelSelect"
                                Layout.fillWidth: true
                                model: (typeof settingsManager !== "undefined") ? settingsManager.imageProviders : []
                                textRole: "name"
                                currentIndex: {
                                    if (typeof settingsManager === "undefined") return 0
                                    for (let i = 0; i < count; ++i) {
                                        if (model[i].name === settingsManager.activeImageProvider) return i
                                    }
                                    return 0
                                }
                                onActivated: (idx) => {
                                    if (typeof settingsManager !== "undefined") {
                                        settingsManager.activeImageProvider = model[idx].name
                                    }
                                }
                            }
                        }

                        LocalModalManifestCard {
                            id: imageManifestCard
                            objectName: "imageManifestCard"
                            category: "images"
                            modelName: (typeof settingsManager !== "undefined") ? settingsManager.activeImageProvider : ""
                        }
                    }

                    // Remote Image API Card
                    RemoteApiConfigCard {
                        id: remoteImageConfigCard
                        objectName: "remoteImageConfigCard"
                        category: "images"
                        visible: !controlPanelRoot.isLocalImage
                        driverId: (typeof engineRegistry !== "undefined" && engineRegistry.activeImageDriver) ? engineRegistry.activeImageDriver.driverId : "gemini_image"
                    }
                }
            }

            // ==========================================
            // 3. VIDEO MODALITY
            // ==========================================
            ScrollView {
                clip: true
                ColumnLayout {
                    width: settingsStack.width - 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text { text: qsTr("Active Driver:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                        ComboBox {
                            id: videoDriverBox
                            objectName: "videoDriverBox"
                            Layout.preferredWidth: 200
                            model: (typeof engineRegistry !== "undefined") ? engineRegistry.availableDriversForAsset("video") : ["sd_video"]
                            currentIndex: (typeof engineRegistry !== "undefined") ? Math.max(0, model.indexOf(engineRegistry.getActiveDriverId("video"))) : 0
                            onActivated: (idx) => {
                                if (typeof engineRegistry !== "undefined") {
                                    engineRegistry.setActiveDriver("video", model[idx])
                                }
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            id: browseVideoDirBtn
                            objectName: "browseVideoDirBtn"
                            visible: controlPanelRoot.isLocalVideo
                            text: qsTr("Browse Video Dir")
                            font.pixelSize: 11
                            background: Rectangle {
                                color: browseVideoDirBtn.down ? "#0077A8" : (browseVideoDirBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
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
                                generalFolderDialog.targetSettingKey = "video"
                                generalFolderDialog.open()
                            }
                        }
                    }

                    // Local Video Controls (Hidden when remote API is active)
                    ColumnLayout {
                        id: localVideoSection
                        Layout.fillWidth: true
                        spacing: 10
                        visible: controlPanelRoot.isLocalVideo

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text { text: qsTr("Active Model:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }
                            ComboBox {
                                id: videoModelSelect
                                objectName: "videoModelSelect"
                                Layout.fillWidth: true
                                model: (typeof settingsManager !== "undefined") ? settingsManager.videoProviders : []
                                textRole: "name"
                                currentIndex: {
                                    if (typeof settingsManager === "undefined") return 0
                                    for (let i = 0; i < count; ++i) {
                                        if (model[i].name === settingsManager.activeVideoProvider) return i
                                    }
                                    return 0
                                }
                                onActivated: (idx) => {
                                    if (typeof settingsManager !== "undefined") {
                                        settingsManager.activeVideoProvider = model[idx].name
                                    }
                                }
                            }
                        }

                        LocalModalManifestCard {
                            id: videoManifestCard
                            objectName: "videoManifestCard"
                            category: "video"
                            modelName: (typeof settingsManager !== "undefined") ? settingsManager.activeVideoProvider : ""
                        }
                    }

                    // Remote Video API Card
                    RemoteApiConfigCard {
                        id: remoteVideoConfigCard
                        objectName: "remoteVideoConfigCard"
                        category: "video"
                        visible: !controlPanelRoot.isLocalVideo
                        driverId: (typeof engineRegistry !== "undefined" && engineRegistry.activeVideoDriver) ? engineRegistry.activeVideoDriver.driverId : "gemini_video"
                    }
                }
            }

            // ==========================================
            // 4. AUDIO MODALITY (Kokoro vs Audio.cpp vs Remote)
            // ==========================================
            ScrollView {
                clip: true
                ColumnLayout {
                    width: settingsStack.width - 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text { text: qsTr("Active Driver:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }

                        ComboBox {
                            id: audioDriverBox
                            objectName: "audioDriverBox"
                            Layout.preferredWidth: 200
                            model: (typeof engineRegistry !== "undefined") ? engineRegistry.availableDriversForAsset("audio") : ["kokoro_tts", "audio_cpp"]
                            currentIndex: (typeof engineRegistry !== "undefined") ? Math.max(0, model.indexOf(engineRegistry.getActiveDriverId("audio"))) : 0
                            onActivated: (idx) => {
                                if (typeof engineRegistry !== "undefined") {
                                    engineRegistry.setActiveDriver("audio", model[idx])
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            id: browseAudioDirBtn
                            objectName: "browseAudioDirBtn"
                            visible: !controlPanelRoot.isRemoteAudio
                            text: qsTr("Browse Audio Dir")
                            font.pixelSize: 11
                            background: Rectangle {
                                color: browseAudioDirBtn.down ? "#0077A8" : (browseAudioDirBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
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
                                generalFolderDialog.targetSettingKey = "audio"
                                generalFolderDialog.open()
                            }
                        }
                    }

                    // Local Audio Sub-Panel (Kokoro or audio.cpp)
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: audioSubCol.implicitHeight + 20
                        visible: !controlPanelRoot.isRemoteAudio
                        color: "#0B1B26"
                        border.color: "#1A4A68"
                        border.width: 1
                        radius: 6

                        ColumnLayout {
                            id: audioSubCol
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 10

                            // Kokoro Speech Controls
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: controlPanelRoot.isKokoroAudio

                                Text {
                                    text: qsTr("Kokoro Multi-Speaker TTS Pipeline (Sherpa-ONNX Runtime)")
                                    color: "#00F0FF"
                                    style: Text.Outline
                                    styleColor: "#000000"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                LocalModalManifestCard {
                                    id: kokoroManifestCard
                                    objectName: "kokoroManifestCard"
                                    category: "audio"
                                    subCategory: "speech"
                                    modelName: "Kokoro"
                                }
                            }

                            // Audio.cpp SFX / Music Controls
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                visible: controlPanelRoot.isAudioCpp

                                Text {
                                    text: qsTr("audio.cpp Native Inference Engine (Direct GGUF SFX & Music)")
                                    color: "#00F0FF"
                                    style: Text.Outline
                                    styleColor: "#000000"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text { text: qsTr("Loaded GGUF:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 90 }

                                    ComboBox {
                                        id: audioCppSelect
                                        objectName: "audioCppSelect"
                                        Layout.fillWidth: true
                                        model: (typeof settingsManager !== "undefined") ? settingsManager.availableAudioCppModels : []
                                        onActivated: (idx) => {
                                            if (typeof settingsManager !== "undefined") {
                                                settingsManager.activeAudioProvider = model[idx]
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Remote Audio API Card (Sibling to local sub-panel)
                    RemoteApiConfigCard {
                        id: remoteAudioConfigCard
                        objectName: "remoteAudioConfigCard"
                        category: "audio"
                        visible: controlPanelRoot.isRemoteAudio
                        driverId: (typeof engineRegistry !== "undefined" && engineRegistry.activeAudioDriver) ? engineRegistry.activeAudioDriver.driverId : "gemini_audio"
                    }
                }
            }

            // ==========================================
            // 5. GENERAL PREFERENCES & DIRECTORIES
            // ==========================================
            ScrollView {
                clip: true
                ColumnLayout {
                    width: settingsStack.width - 20
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: qsTr("UI Theme:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 100 }
                        ComboBox {
                            id: themeSelectBox
                            objectName: "themeSelectBox"
                            model: ["Fusion", "Basic", "Material"]
                            currentIndex: (typeof settingsManager !== "undefined") ? model.indexOf(settingsManager.activeTheme) : 0
                            onActivated: (idx) => {
                                if (typeof settingsManager !== "undefined") {
                                    settingsManager.activeTheme = model[idx]
                                }
                                controlPanelRoot.themeRestartRequested()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { text: qsTr("Default Save Path:"); color: "#888888"; font.pixelSize: 11; Layout.preferredWidth: 100 }
                        TextField {
                            id: defaultSavePathField
                            objectName: "defaultSavePathField"
                            Layout.fillWidth: true
                            readOnly: true
                            text: (typeof settingsManager !== "undefined") ? settingsManager.defaultSavePath : ""
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            background: Rectangle { color: "#08121A"; border.color: "#1A4A68"; radius: 3 }
                        }
                        Button {
                            id: browseSavePathBtn
                            objectName: "browseSavePathBtn"
                            text: qsTr("Browse...")
                            font.pixelSize: 10
                            background: Rectangle {
                                color: browseSavePathBtn.down ? "#0077A8" : (browseSavePathBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
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
                                generalFolderDialog.targetSettingKey = "save"
                                generalFolderDialog.open()
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: qsTr("Initial Setup Wizard:")
                            color: "#CCCCCC"
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }

                        Button {
                            id: rerunWizardBtn
                            objectName: "rerunWizardBtn"
                            text: qsTr("🔄 Re-run Setup Wizard...")
                            font.pixelSize: 11
                            background: Rectangle {
                                color: rerunWizardBtn.down ? "#0077A8" : (rerunWizardBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
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
                            onClicked: controlPanelRoot.rerunWizardRequested()
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

        RowLayout {
            Layout.fillWidth: true
            Button {
                id: resetDefaultsBtn
                objectName: "resetDefaultsBtn"
                text: qsTr("Reset Defaults")
                font.pixelSize: 11
                background: Rectangle {
                    color: resetDefaultsBtn.down ? "#0077A8" : (resetDefaultsBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
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
                    if (typeof settingsManager !== "undefined") {
                        settingsManager.resetDefaults()
                    }
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: closeBtn
                objectName: "closeBtn"
                text: qsTr("Close")
                font.pixelSize: 11
                font.bold: true
                Layout.preferredWidth: 90
                background: Rectangle {
                    color: closeBtn.down ? "#0077A8" : (closeBtn.hovered ? "#00B4F0" : "#1A4A68")
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
                onClicked: controlPanelRoot.close()
            }
        }
    }
}

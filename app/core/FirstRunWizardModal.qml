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
** File: FirstRunWizardModal.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtCore
import StoryBuilderStudio

Dialog {
    id: wizardDialog
    objectName: "firstRunWizardModalRoot"
    title: qsTr("Story Builder Studio — Quick Setup Wizard")
    width: 700
    height: 620
    anchors.centerIn: parent
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 8
    }

    property int currentStep: 0 // 0: AI Setup (Local/Remote), 1: Projects & Series, 2: Ready


    property var scanResults: ({ textCount: 0, imageCount: 0, audioCount: 0, videoCount: 0, hasT5: false })

    property string initialModelsDir: (typeof settingsManager !== "undefined" && settingsManager && settingsManager.baseModelsDir)
                                      ? settingsManager.baseModelsDir : ""
    property string initialT5Path: (typeof settingsManager !== "undefined" && settingsManager && settingsManager.globalT5Path)
                                   ? settingsManager.globalT5Path : ""
    property string initialSavePath: (typeof settingsManager !== "undefined" && settingsManager && settingsManager.defaultSavePath)
                                     ? settingsManager.defaultSavePath : ""
    property string activeSeriesName: (typeof seriesContainer !== "undefined" && seriesContainer && seriesContainer.name)
                                      ? seriesContainer.name : ""
    property string activeSeriesRoot: (typeof seriesContainer !== "undefined" && seriesContainer && seriesContainer.rootDirectory)
                                      ? seriesContainer.rootDirectory : ""

    property bool hasLocalPlugins: (typeof engineRegistry !== "undefined" && engineRegistry)
                                       ? engineRegistry.hasLocalPlugins : true
    property bool useRemoteSetup: !hasLocalPlugins

    function nextStep() {
        if (currentStep < 2) currentStep++
    }

    function prevStep() {
        if (currentStep > 0) currentStep--
    }

    function goToStep(stepIndex) {
        currentStep = stepIndex
    }

    function triggerScan() {
        if (typeof settingsManager !== "undefined" && settingsManager !== null) {
            scanResults = settingsManager.scanModelDirectory(modelsDirField.text, t5Field.text)
        }
    }

    function cleanUrlPath(rawUrl) {
        let clean = rawUrl.toString().replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && clean.startsWith("/")) {
            clean = clean.substring(1)
        }
        return clean
    }

    // Keep wizard synced when a new series is created via newSeriesDialog
    Connections {
        target: (typeof seriesContainer !== "undefined" && seriesContainer !== null) ? seriesContainer : null
        ignoreUnknownSignals: true
        function onNameChanged() {
            wizardDialog.activeSeriesName = seriesContainer.name
        }
        function onRootDirectoryChanged() {
            wizardDialog.activeSeriesRoot = seriesContainer.rootDirectory
        }
    }

    FolderDialog {
        id: modelsFolderPicker
        objectName: "modelsFolderPicker"
        title: qsTr("Select Base Models Directory")
        currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        onAccepted: {
            modelsDirField.text = wizardDialog.cleanUrlPath(selectedFolder)
            wizardDialog.triggerScan()
        }
    }

    FileDialog {
        id: t5FilePicker
        objectName: "t5FilePicker"
        title: qsTr("Select T5XXL Text Encoder File")
        nameFilters: ["T5XXL Checkpoints (*.safetensors *.gguf *.bin)", "All Files (*)"]
        onAccepted: {
            t5Field.text = wizardDialog.cleanUrlPath(selectedFile)
            wizardDialog.triggerScan()
        }
    }

    FolderDialog {
        id: defaultSaveFolderPicker
        objectName: "defaultSaveFolderPicker"
        title: qsTr("Select Default Projects Directory")
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: {
            defaultSavePathField.text = wizardDialog.cleanUrlPath(selectedFolder)
        }
    }

    FolderDialog {
        id: openSeriesFolderDialog
        objectName: "openSeriesFolderDialog"
        title: qsTr("Select Existing Series Folder")
        currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        onAccepted: {
            let path = wizardDialog.cleanUrlPath(selectedFolder)
            if (typeof seriesContainer !== "undefined" && seriesContainer !== null) {
                seriesContainer.loadFromDirectory(path)
                wizardDialog.activeSeriesName = seriesContainer.name
                wizardDialog.activeSeriesRoot = seriesContainer.rootDirectory
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 14
        anchors.margins: 16

        // Stepper Breadcrumbs
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Repeater {
                model: [qsTr("1. AI Setup (Optional)"), qsTr("2. Series & Projects"), qsTr("3. Workspace Ready")]
                Rectangle {
                    objectName: "stepBreadcrumb_" + index
                    Layout.fillWidth: true
                    height: 28
                    radius: 4
                    color: wizardDialog.currentStep === index ? "#132B3A" : (wizardDialog.currentStep > index ? "#0B1B26" : "#08121A")
                    border.color: wizardDialog.currentStep === index ? "#00F0FF" : "#1A4A68"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: wizardDialog.currentStep === index ? "#00F0FF" : (wizardDialog.currentStep > index ? "#1A4A68" : "#888888")
                        style: wizardDialog.currentStep === index ? Text.Outline : Text.Normal
                        styleColor: "#000000"
                        font.bold: wizardDialog.currentStep === index
                        font.pixelSize: 11
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Stacked Steps
        StackLayout {
            id: wizardStackLayout
            objectName: "wizardStackLayout"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: wizardDialog.currentStep

            // ==========================================
            // STEP 0: AI SETUP (LOCAL MODELS OR REMOTE REST)
            // ==========================================
            ColumnLayout {
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        id: step0Header
                        objectName: "step0Header"
                        text: wizardDialog.hasLocalPlugins ? qsTr("AI Model Repositories") : qsTr("Remote / REST API Setup")
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 14
                    }
                    Item { Layout.fillWidth: true }

                    // Mode Switcher: Local vs Remote REST
                    RowLayout {
                        spacing: 6
                        visible: wizardDialog.hasLocalPlugins
                        Button {
                            id: localModeTabBtn
                            objectName: "localModeTabBtn"
                            text: qsTr("Local Models")
                            implicitHeight: 26
                            font.pixelSize: 11
                            background: Rectangle {
                                color: !wizardDialog.useRemoteSetup ? "#1A4A68" : "#0B1B26"
                                border.color: !wizardDialog.useRemoteSetup ? "#00F0FF" : "#1A4A68"
                                border.width: 1
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text
                                color: !wizardDialog.useRemoteSetup ? "#00F0FF" : "#AAAAAA"
                                font.bold: !wizardDialog.useRemoteSetup
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: wizardDialog.useRemoteSetup = false
                        }
                        Button {
                            id: remoteModeTabBtn
                            objectName: "remoteModeTabBtn"
                            text: qsTr("Remote / REST API")
                            implicitHeight: 26
                            font.pixelSize: 11
                            background: Rectangle {
                                color: wizardDialog.useRemoteSetup ? "#1A4A68" : "#0B1B26"
                                border.color: wizardDialog.useRemoteSetup ? "#00F0FF" : "#1A4A68"
                                border.width: 1
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text
                                color: wizardDialog.useRemoteSetup ? "#00F0FF" : "#AAAAAA"
                                font.bold: wizardDialog.useRemoteSetup
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: wizardDialog.useRemoteSetup = true
                        }
                    }
                }

                // --- LOCAL MODELS PANEL ---
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: !wizardDialog.useRemoteSetup  && wizardDialog.hasLocalPlugins

                    Text {
                        text: qsTr("Point to where your local models live (/text, /images, /audio, /video). If you are using cloud APIs or Ollama instead, switch to 'Remote / REST API' above or skip this step.")
                        color: "#AAAAAA"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    // Base Models Path
                    Text {
                        text: qsTr("Base Models Directory:")
                        color: "#CCCCCC"
                        font.pixelSize: 12
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: modelsDirField
                            objectName: "modelsDirField"
                            Layout.fillWidth: true
                            placeholderText: Qt.platform.os === "windows"
                                             ? qsTr("e.g. D:/AI_Models or C:/Models")
                                             : qsTr("e.g. /home/user/AI_Models")
                            text: wizardDialog.initialModelsDir
                            color: "#FFFFFF"
                            background: Rectangle {
                                color: "#08121A"
                                border.color: modelsDirField.activeFocus ? "#00F0FF" : "#1A4A68"
                                radius: 4
                            }
                            onTextChanged: wizardDialog.triggerScan()
                        }
                        Button {
                            id: browseModelsBtn
                            objectName: "browseModelsBtn"
                            text: qsTr("Browse...")
                            Layout.preferredHeight: 30
                            background: Rectangle {
                                color: browseModelsBtn.down ? "#0077A8" : (browseModelsBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                            onClicked: modelsFolderPicker.open()
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Select root folder containing AI model subdirectories")
                        }
                    }

                    // T5XXL Path
                    Text {
                        text: qsTr("T5XXL Clip Encoder (Recommended for FLUX / SD3 / Wan):")
                        color: "#CCCCCC"
                        font.pixelSize: 12
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: t5Field
                            objectName: "t5Field"
                            Layout.fillWidth: true
                            placeholderText: Qt.platform.os === "windows"
                                             ? qsTr("e.g. D:/AI_Models/images/t5xxl_fp16.safetensors")
                                             : qsTr("e.g. /home/user/AI_Models/images/t5xxl_fp16.safetensors")
                            text: wizardDialog.initialT5Path
                            color: "#FFFFFF"
                            background: Rectangle {
                                color: "#08121A"
                                border.color: t5Field.activeFocus ? "#00F0FF" : "#1A4A68"
                                radius: 4
                            }
                            onTextChanged: wizardDialog.triggerScan()
                        }
                        Button {
                            id: browseT5Btn
                            objectName: "browseT5Btn"
                            text: qsTr("Browse...")
                            Layout.preferredHeight: 30
                            background: Rectangle {
                                color: browseT5Btn.down ? "#0077A8" : (browseT5Btn.hovered ? "#00B4F0" : "#0B1B26")
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
                            onClicked: t5FilePicker.open()
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Select standalone T5XXL weights checkpoint file")
                        }
                    }

                    // Dynamic Health Check Summary Card
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: healthCol.implicitHeight + 14
                        color: "#0B1B26" // Secondary well
                        border.color: "#1A4A68"
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            id: healthCol
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 4

                            Text {
                                text: qsTr("Detected Models Summary:")
                                color: "#1A4A68"
                                font.bold: true
                                font.pixelSize: 10
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 16

                                Text {
                                    id: scriptSummaryText
                                    objectName: "scriptSummaryText"
                                    text: qsTr("• Script (GGUF): %1").arg(wizardDialog.scanResults.textCount || 0)
                                    color: (wizardDialog.scanResults.textCount > 0) ? "#00F0FF" : "#888888"
                                    style: (wizardDialog.scanResults.textCount > 0) ? Text.Outline : Text.Normal
                                    styleColor: "#000000"
                                    font.pixelSize: 11
                                }
                                Text {
                                    id: imageSummaryText
                                    objectName: "imageSummaryText"
                                    text: qsTr("• Image/Flux: %1").arg(wizardDialog.scanResults.imageCount || 0)
                                    color: (wizardDialog.scanResults.imageCount > 0) ? "#00F0FF" : "#888888"
                                    style: (wizardDialog.scanResults.imageCount > 0) ? Text.Outline : Text.Normal
                                    styleColor: "#000000"
                                    font.pixelSize: 11
                                }
                                Text {
                                    id: audioSummaryText
                                    objectName: "audioSummaryText"
                                    text: qsTr("• Audio: %1").arg(wizardDialog.scanResults.audioCount || 0)
                                    color: (wizardDialog.scanResults.audioCount > 0) ? "#00F0FF" : "#888888"
                                    style: (wizardDialog.scanResults.audioCount > 0) ? Text.Outline : Text.Normal
                                    styleColor: "#000000"
                                    font.pixelSize: 11
                                }
                                Text {
                                    id: videoSummaryText
                                    objectName: "videoSummaryText"
                                    text: qsTr("• Video: %1").arg(wizardDialog.scanResults.videoCount || 0)
                                    color: (wizardDialog.scanResults.videoCount > 0) ? "#00F0FF" : "#888888"
                                    style: (wizardDialog.scanResults.videoCount > 0) ? Text.Outline : Text.Normal
                                    styleColor: "#000000"
                                    font.pixelSize: 11
                                }
                            }

                            Text {
                                id: t5StatusText
                                objectName: "t5StatusText"
                                text: wizardDialog.scanResults.hasT5 ? qsTr("✔ T5XXL Encoder found") : qsTr("⚠ T5XXL not linked (high-res diffusion will be limited)")
                                color: wizardDialog.scanResults.hasT5 ? "#00F0FF" : "#FFAA00"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.pixelSize: 10
                            }
                        }
                    }
                }

                // --- REMOTE / REST API PANEL ---
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    visible: wizardDialog.useRemoteSetup

                    Text {
                        text: qsTr("Configure a cloud provider (Gemini, OpenAI) or a local REST server (Ollama). You can add additional image, audio, and video endpoints later in Settings.")
                        color: "#AAAAAA"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        Text {
                            text: qsTr("Primary Remote Provider:")
                            color: "#CCCCCC"
                            font.pixelSize: 12
                        }
                        ComboBox {
                            id: wizardRemoteDriverCombo
                            objectName: "wizardRemoteDriverCombo"
                            Layout.fillWidth: true
                            model: ["gemini_bridge", "openai_bridge", "ollama_local"]
                        }
                    }

                    RemoteApiConfigCard {
                        id: wizardRemoteConfigCard
                        objectName: "wizardRemoteConfigCard"
                        category: "script"
                        driverId: wizardRemoteDriverCombo.currentText
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ==========================================
            // STEP 1: SERIES & PRODUCTION WORKSPACE
            // ==========================================
            ColumnLayout {
                spacing: 12

                Text {
                    id: step1Header
                    objectName: "step1Header"
                    text: qsTr("Series & Production Workspace")
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 14
                }
                Text {
                    text: qsTr("Choose where your projects are saved by default, then create your first series or open an existing directory.")
                    color: "#AAAAAA"
                    font.pixelSize: 11
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // Default Projects Save Directory
                Text {
                    text: qsTr("Default Projects Directory:")
                    color: "#CCCCCC"
                    font.pixelSize: 12
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: defaultSavePathField
                        objectName: "defaultSavePathField"
                        Layout.fillWidth: true
                        placeholderText: Qt.platform.os === "windows"
                                         ? qsTr("e.g. C:/Users/You/Documents/StoryBuilderProjects")
                                         : qsTr("e.g. /home/user/Documents/StoryBuilderProjects")
                        text: wizardDialog.initialSavePath
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#08121A"
                            border.color: defaultSavePathField.activeFocus ? "#00F0FF" : "#1A4A68"
                            radius: 4
                        }
                    }
                    Button {
                        id: browseSavePathBtn
                        objectName: "browseSavePathBtn"
                        text: qsTr("Browse...")
                        Layout.preferredHeight: 30
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
                        onClicked: defaultSaveFolderPicker.open()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    color: "#132B3A"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 6

                    ColumnLayout {
                        anchors.centerIn: parent
                        spacing: 10

                        Text {
                            id: activeSeriesStatusText
                            objectName: "activeSeriesStatusText"
                            text: wizardDialog.activeSeriesRoot.length > 0
                                  ? qsTr("Active Series: %1").arg(wizardDialog.activeSeriesName)
                                  : qsTr("No Series Workspace Initialized Yet")
                            color: wizardDialog.activeSeriesRoot.length > 0 ? "#00F0FF" : "#FFAA00"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.bold: true
                            font.pixelSize: 13
                            Layout.alignment: Qt.AlignHCenter
                        }

                        RowLayout {
                            spacing: 12
                            Button {
                                id: createSeriesBtn
                                objectName: "createSeriesBtn"
                                text: qsTr("＋ Create New Series...")
                                Layout.preferredHeight: 30
                                background: Rectangle {
                                    color: createSeriesBtn.down ? "#0077A8" : (createSeriesBtn.hovered ? "#00B4F0" : "#1A4A68")
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
                                    if (typeof settingsManager !== "undefined" && settingsManager !== null && defaultSavePathField.text.trim().length > 0) {
                                        settingsManager.defaultSavePath = defaultSavePathField.text.trim()
                                    }
                                    if (typeof newSeriesDialog !== "undefined") {
                                        newSeriesDialog.open()
                                    }
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Bootstrap a new series workspace")
                            }

                            Button {
                                id: browseSeriesBtn
                                objectName: "browseSeriesBtn"
                                text: qsTr("📁 Browse Series Folder...")
                                Layout.preferredHeight: 30
                                background: Rectangle {
                                    color: browseSeriesBtn.down ? "#0077A8" : (browseSeriesBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                                onClicked: openSeriesFolderDialog.open()
                                ToolTip.visible: hovered
                                ToolTip.text: qsTr("Locate an existing series on disk")
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // ==========================================
            // STEP 2: FINISH & CONFIRM
            // ==========================================
            ColumnLayout {
                spacing: 12

                Text {
                    id: step2Header
                    objectName: "step2Header"
                    text: qsTr("Setup Complete")
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 15
                }
                Text {
                    text: qsTr("Your workspace is configured! Story Builder Studio is ready to use for writing, storyboarding, gallery reviews, and AI-assisted generation.")
                    color: "#CCCCCC"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#08121A"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 6

                        Text {
                            text: qsTr("Summary Configuration:")
                            color: "#1A4A68"
                            font.bold: true
                            font.pixelSize: 11
                        }
                        Text {
                            id: summaryModelsPathText
                            objectName: "summaryModelsPathText"
                            text: wizardDialog.useRemoteSetup
                                  ? qsTr("• AI Mode: Remote / REST (%1)").arg(wizardRemoteDriverCombo.currentText)
                                  : qsTr("• Models Path: %1").arg(modelsDirField.text.length > 0 ? modelsDirField.text : qsTr("(None / Remote or Manual Mode)"))
                            color: "#FFFFFF"
                            font.pixelSize: 11
                        }
                        Text {
                            id: summarySavePathText
                            objectName: "summarySavePathText"
                            text: qsTr("• Projects Directory: %1").arg(defaultSavePathField.text.length > 0 ? defaultSavePathField.text : qsTr("(System Default)"))
                            color: "#FFFFFF"
                            font.pixelSize: 11
                        }
                        Text {
                            id: summarySeriesText
                            objectName: "summarySeriesText"
                            text: qsTr("• Active Series: %1").arg(wizardDialog.activeSeriesName.length > 0 ? wizardDialog.activeSeriesName : qsTr("(Will create or load later)"))
                            color: "#FFFFFF"
                            font.pixelSize: 11
                        }
                        Text {
                            text: qsTr("• Storyboard & Editor: Ready for manual and offline use")
                            color: "#00F0FF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }

        // Stepper Navigation Footer
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                id: backStepBtn
                objectName: "backStepBtn"
                text: qsTr("Back")
                Layout.preferredHeight: 30
                visible: wizardDialog.currentStep > 0
                background: Rectangle {
                    color: backStepBtn.down ? "#0077A8" : (backStepBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: wizardDialog.prevStep()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Return to previous setup step")
            }

            Item { Layout.fillWidth: true }

            Button {
                id: nextStepBtn
                objectName: "nextStepBtn"
                text: wizardDialog.currentStep === 2 ? qsTr("Launch Studio") : qsTr("Next →")
                Layout.preferredHeight: 30
                background: Rectangle {
                    color: nextStepBtn.down ? "#0077A8" : (nextStepBtn.hovered ? "#00B4F0" : "#1A4A68")
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
                    if (wizardDialog.currentStep === 0) {
                        if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                            if (modelsDirField.text.trim().length > 0) {
                                settingsManager.baseModelsDir = modelsDirField.text.trim()
                            }
                            if (t5Field.text.trim().length > 0) {
                                settingsManager.globalT5Path = t5Field.text.trim()
                            }
                        }
                        wizardDialog.currentStep++
                    } else if (wizardDialog.currentStep === 1) {
                        if (typeof settingsManager !== "undefined" && settingsManager !== null && defaultSavePathField.text.trim().length > 0) {
                            settingsManager.defaultSavePath = defaultSavePathField.text.trim()
                        }
                        wizardDialog.currentStep++
                    } else if (wizardDialog.currentStep === 2) {
                        if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                            settingsManager.isFirstRun = false
                        }
                        wizardDialog.close()
                        if (typeof seriesLauncherModal !== "undefined" && (!wizardDialog.activeSeriesRoot || wizardDialog.activeSeriesRoot.length === 0)) {
                            seriesLauncherModal.open()
                        }
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: wizardDialog.currentStep === 2 ? qsTr("Complete setup and open studio") : qsTr("Proceed to next step")
            }
        }
    }
}

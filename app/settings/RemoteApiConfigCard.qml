// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

ColumnLayout {
    id: remoteConfigRoot
    objectName: "remoteApiConfigCardRoot"
    spacing: 10
    Layout.fillWidth: true

    property string category: "script"          // "script", "images", "audio", "video"
    property string driverId: "gemini_bridge"   // "gemini_*", "openai_*", "ollama_local", or "custom_*"
    property alias baseUrl: baseUrlField.text
    property alias modelIdentifier: modelField.text
    property bool isPinging: false

    // Mock property for isolated unit test assertion
    property var mockIsKeyConfigured: null
    property string currentCustomName: ""

    readonly property string assetType: (category === "images") ? "image" : category
    readonly property bool isGemini: driverId.startsWith("gemini_")
    readonly property bool isOpenAi: driverId.startsWith("openai_")
    readonly property bool isOllama: driverId === "ollama_local"
    readonly property bool isLocalRest: isOllama ||
                                        baseUrlField.text.indexOf("localhost") !== -1 ||
                                        baseUrlField.text.indexOf("127.0.0.1") !== -1
    readonly property bool isCustom: !isGemini && !isOpenAi && !isOllama

    readonly property bool isKeyConfigured: {
        if (mockIsKeyConfigured !== null) return mockIsKeyConfigured
        if (isLocalRest) return true
        if (typeof settingsManager === "undefined" || settingsManager === null) return false
        return settingsManager.resolveApiKey(driverId, customEnvName).length > 0
    }

    readonly property string targetEnvName: {
        if (isGemini) return "GEMINI_API_KEY"
        if (isOpenAi) return "OPENAI_API_KEY"
        if (isOllama) return ""
        return customEnvField.text.trim().toUpperCase()
    }

    readonly property string customEnvName: isCustom ? customEnvField.text.trim() : ""

    readonly property var activeDriverObj: (typeof engineRegistry !== "undefined" && engineRegistry !== null)
                                               ? engineRegistry.getDriver(remoteConfigRoot.driverId) : null

    function defaultBaseUrlForDriver() {
        if (driverId.startsWith("gemini_")) return "https://generativelanguage.googleapis.com"
        if (driverId.startsWith("openai_")) return "https://api.openai.com/v1"
        if (driverId === "ollama_local")    return "http://localhost:11434/v1"
        return ""
    }

    function defaultModelForDriver() {
        if (driverId === "gemini_image")    return "imagen-3.0-generate-002"
        if (driverId === "gemini_video")    return "veo-2.0-generate-001"
        if (driverId === "gemini_audio")    return "gemini-3.8-flash-tts"
        if (driverId.startsWith("gemini_")) return "gemini-3.5-flash-lite"
        if (driverId.startsWith("openai_")) return "gpt-4o"
        if (driverId === "ollama_local")    return "qwen2.5:7b"
        return ""
    }

    function syncFieldsFromSettings() {
        let saved = (typeof settingsManager !== "undefined" && settingsManager !== null &&
                     typeof settingsManager.getRemoteDriverConfig === "function")
                    ? settingsManager.getRemoteDriverConfig(category, driverId)
                    : ({})

        let custom = !driverId.startsWith("gemini_") && !driverId.startsWith("openai_") && driverId !== "ollama_local"
        currentCustomName = saved.name || ""
        baseUrlField.text = (saved.baseUrl && saved.baseUrl.length > 0) ? saved.baseUrl : defaultBaseUrlForDriver()
        modelField.text   = (saved.modelName && saved.modelName.length > 0) ? saved.modelName : defaultModelForDriver()
        if (custom) {
            customEnvField.text = saved.envVar || ""
        }
        saveFeedbackText.text = ""
    }

    onDriverIdChanged: syncFieldsFromSettings()
    Component.onCompleted: syncFieldsFromSettings()

    // 1. Environment Key Status Banner
    Rectangle {
        id: statusBanner
        objectName: "statusBanner"
        Layout.fillWidth: true
        implicitHeight: bannerRow.implicitHeight + 16
        color: remoteConfigRoot.isKeyConfigured ? "#0B1B26" : "#381818"
        border.color: remoteConfigRoot.isKeyConfigured ? "#1A4A68" : "#FF4444"
        border.width: 1
        radius: 6

        RowLayout {
            id: bannerRow
            anchors.fill: parent
            anchors.margins: 10
            spacing: 12

            Text {
                id: bannerIcon
                objectName: "bannerIcon"
                text: remoteConfigRoot.isLocalRest ? "🏠" : (remoteConfigRoot.isKeyConfigured ? "🔒" : "⚠️")
                font.pixelSize: 18
                verticalAlignment: Text.AlignVCenter
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    id: bannerTitleText
                    objectName: "bannerTitleText"
                    text: {
                        if (remoteConfigRoot.isPinging) return qsTr("Pinging server...")
                        if (remoteConfigRoot.activeDriverObj && remoteConfigRoot.activeDriverObj.verificationSummary &&
                            remoteConfigRoot.activeDriverObj.verificationSummary.length > 0) {
                            return remoteConfigRoot.activeDriverObj.connectionVerifiedState
                                   ? qsTr("API Key Verified") : qsTr("Connection Failed")
                        }
                        if (remoteConfigRoot.isLocalRest) return qsTr("Local REST Service (No Key Required)")
                        if (remoteConfigRoot.isKeyConfigured) {
                            return qsTr("API Key Detected in Environment: %1").arg(remoteConfigRoot.targetEnvName)
                        }
                        return qsTr("Missing Environment Variable: %1").arg(remoteConfigRoot.targetEnvName.length > 0 ? remoteConfigRoot.targetEnvName : "(Not Specified)")
                    }
                    color: {
                        if (remoteConfigRoot.isPinging) return "#00F0FF"
                        if (remoteConfigRoot.activeDriverObj && remoteConfigRoot.activeDriverObj.verificationSummary &&
                            remoteConfigRoot.activeDriverObj.verificationSummary.length > 0) {
                            return remoteConfigRoot.activeDriverObj.connectionVerifiedState ? "#00FF88" : "#FF4444"
                        }
                        return remoteConfigRoot.isKeyConfigured ? "#00F0FF" : "#FF6666"
                    }
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 11
                }

                Text {
                    id: bannerSubtitleText
                    objectName: "bannerSubtitleText"
                    text: {
                        if (remoteConfigRoot.activeDriverObj && remoteConfigRoot.activeDriverObj.verificationSummary &&
                            remoteConfigRoot.activeDriverObj.verificationSummary.length > 0) {
                            return remoteConfigRoot.activeDriverObj.verificationSummary
                        }
                        if (remoteConfigRoot.isLocalRest) return qsTr("Runs locally over HTTP; no remote credentials needed.")
                        if (remoteConfigRoot.isKeyConfigured) return qsTr("Authentication verified via system environment variables.")
                        return qsTr("Set the environment variable on your OS, then restart the application.")
                    }
                    color: "#888888"
                    font.pixelSize: 10
                }
            }

            Button {
                id: setupGuideBtn
                objectName: "setupGuideBtn"
                visible: !remoteConfigRoot.isLocalRest
                text: qsTr("Setup Guide ↗")
                implicitHeight: 28
                font.pixelSize: 10

                background: Rectangle {
                    color: setupGuideBtn.down ? "#0077A8" : (setupGuideBtn.hovered ? "#00B4F0" : "#1A4A68")
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
                    if (remoteConfigRoot.isGemini) {
                        Qt.openUrlExternally("https://aistudio.google.com/docs/api-key")
                    } else if (remoteConfigRoot.isOpenAi) {
                        Qt.openUrlExternally("https://platform.openai.com/docs/quickstart")
                    } else {
                        envSetupDialog.open()
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("View documentation on configuring API environment keys")
            }

            Button {
                id: verifyConnectionBtn
                objectName: "verifyConnectionBtn"
                text: qsTr("Verify Connection")
                implicitHeight: 28
                font.pixelSize: 10
                enabled: remoteConfigRoot.isKeyConfigured

                background: Rectangle {
                    color: verifyConnectionBtn.down ? "#0077A8" : (verifyConnectionBtn.hovered ? "#00B4F0" : "#1A4A68")
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: verifyConnectionBtn.enabled ? "#FFFFFF" : "#555555"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (remoteConfigRoot.activeDriverObj && typeof remoteConfigRoot.activeDriverObj.verifyConnection === "function") {
                        remoteConfigRoot.isPinging = true
                        remoteConfigRoot.activeDriverObj.verifyConnection()
                    }
                }
            }
        }
    }

    // 2. Custom Environment Input (Only for custom drivers)
    ColumnLayout {
        id: customEnvSection
        objectName: "customEnvSection"
        Layout.fillWidth: true
        spacing: 4
        visible: remoteConfigRoot.isCustom

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Environment Variable Name:")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 11
                font.bold: true
            }
            Item { Layout.fillWidth: true }
            Text {
                id: validationHintText
                objectName: "validationHintText"
                text: qsTr("Must end with '_API_KEY' or '_TOKEN' (blank for local servers)")
                color: (customEnvField.text.trim().length === 0 || customEnvField.acceptableInput) ? "#00F0FF" : "#FFAA00"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
        }

        TextField {
            id: customEnvField
            objectName: "customEnvField"
            Layout.fillWidth: true
            placeholderText: "e.g. DEEPSEEK_API_KEY or leave blank for localhost"
            validator: RegularExpressionValidator { regularExpression: /^[A-Z0-9_]+_(API_KEY|TOKEN)$/i }
            color: "#FFFFFF"
            background: Rectangle {
                color: "#08121A"
                border.color: customEnvField.acceptableInput ? "#00F0FF" : (customEnvField.activeFocus ? "#1A4A68" : "#1B3B4F")
                border.width: 1
                radius: 4
            }
        }
    }

    // 3. Endpoint URL & Target Model
    RowLayout {
        Layout.fillWidth: true
        spacing: 10

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: qsTr("Endpoint Base URL:")
                color: "#888888"
                font.pixelSize: 11
            }
            TextField {
                id: baseUrlField
                objectName: "baseUrlField"
                Layout.fillWidth: true
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#08121A"
                    border.color: baseUrlField.activeFocus ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 200
            spacing: 4
            Text {
                text: qsTr("Model Identifier:")
                color: "#888888"
                font.pixelSize: 11
            }
            TextField {
                id: modelField
                objectName: "modelField"
                Layout.fillWidth: true
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#08121A"
                    border.color: modelField.activeFocus ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }
        }
    }

    // 4. Persistence & Custom Provider Actions
    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Button {
            id: saveConfigBtn
            objectName: "saveConfigBtn"
            text: qsTr("💾 Save Changes")
            implicitHeight: 28
            font.pixelSize: 11
            enabled: baseUrlField.text.trim().length > 0 && modelField.text.trim().length > 0

            background: Rectangle {
                color: saveConfigBtn.down ? "#0077A8" : (saveConfigBtn.hovered ? "#00B4F0" : "#1A4A68")
                border.color: "#000000"
                border.width: 1
                radius: 4
            }
            contentItem: Text {
                text: parent.text
                color: saveConfigBtn.enabled ? "#FFFFFF" : "#666666"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                    let ok = settingsManager.saveRemoteDriverConfig(
                        remoteConfigRoot.category,
                        remoteConfigRoot.driverId,
                        baseUrlField.text,
                        modelField.text,
                        remoteConfigRoot.customEnvName
                    )
                    saveFeedbackText.text = ok ? qsTr("✔ Saved & reloaded driver") : qsTr("✖ Save failed")
                    saveFeedbackText.color = ok ? "#00FF88" : "#FF6666"
                }
            }
        }

        Button {
            id: addCustomProviderBtn
            objectName: "addCustomProviderBtn"
            text: qsTr("+ Add Provider")
            implicitHeight: 28
            font.pixelSize: 11

            background: Rectangle {
                color: addCustomProviderBtn.down ? "#0077A8" : (addCustomProviderBtn.hovered ? "#00B4F0" : "#0B1B26")
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

            onClicked: {
                newProviderNameField.text = ""
                newProviderUrlField.text = "http://127.0.0.1:11434/v1"
                newProviderModelField.text = ""
                newProviderEnvField.text = ""
                addProviderErrorText.text = ""
                addProviderDialog.open()
            }
        }

        Button {
            id: removeCustomProviderBtn
            objectName: "removeCustomProviderBtn"
            visible: remoteConfigRoot.isCustom && remoteConfigRoot.currentCustomName.length > 0
            text: qsTr("Remove")
            implicitHeight: 28
            font.pixelSize: 11

            background: Rectangle {
                color: removeCustomProviderBtn.hovered ? "#331515" : "#0B1B26"
                border.color: "#FF4444"
                border.width: 1
                radius: 4
            }
            contentItem: Text {
                text: parent.text
                color: "#FF6666"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onClicked: {
                if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                    settingsManager.removeCustomProvider(remoteConfigRoot.category, remoteConfigRoot.currentCustomName)
                }
            }
        }

        Text {
            id: saveFeedbackText
            objectName: "saveFeedbackText"
            text: ""
            font.pixelSize: 11
            font.bold: true
        }

        Item { Layout.fillWidth: true }
    }

    // 5. Add Custom Provider Modal
    Dialog {
        id: addProviderDialog
        objectName: "addProviderDialog"
        title: qsTr("Add Custom REST Provider (%1)").arg(remoteConfigRoot.category)
        anchors.centerIn: Overlay.overlay
        width: 440
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        background: Rectangle {
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 8

            Text { text: qsTr("Display Name:"); color: "#CCCCCC"; font.pixelSize: 11 }
            TextField {
                id: newProviderNameField
                objectName: "newProviderNameField"
                Layout.fillWidth: true
                placeholderText: "e.g. OpenAI, Local SD Server, Ollama"
                color: "#FFFFFF"
                background: Rectangle { color: "#0B1B26"; border.color: "#1A4A68"; radius: 4 }
            }

            Text { text: qsTr("Endpoint Base URL:"); color: "#CCCCCC"; font.pixelSize: 11 }
            TextField {
                id: newProviderUrlField
                objectName: "newProviderUrlField"
                Layout.fillWidth: true
                placeholderText: "https://api.openai.com/v1 or http://127.0.0.1:7860/v1"
                color: "#FFFFFF"
                background: Rectangle { color: "#0B1B26"; border.color: "#1A4A68"; radius: 4 }
            }

            Text { text: qsTr("Model Identifier:"); color: "#CCCCCC"; font.pixelSize: 11 }
            TextField {
                id: newProviderModelField
                objectName: "newProviderModelField"
                Layout.fillWidth: true
                placeholderText: "e.g. gpt-4o, flux1-schnell, qwen2.5:7b"
                color: "#FFFFFF"
                background: Rectangle { color: "#0B1B26"; border.color: "#1A4A68"; radius: 4 }
            }

            Text { text: qsTr("Environment Variable (optional for localhost):"); color: "#CCCCCC"; font.pixelSize: 11 }
            TextField {
                id: newProviderEnvField
                objectName: "newProviderEnvField"
                Layout.fillWidth: true
                placeholderText: "e.g. OPENAI_API_KEY (leave blank for local servers)"
                color: "#FFFFFF"
                background: Rectangle { color: "#0B1B26"; border.color: "#1A4A68"; radius: 4 }
            }

            Text {
                id: addProviderErrorText
                color: "#FF6666"
                font.pixelSize: 10
                visible: text.length > 0
            }
        }

        onAccepted: {
            let name  = newProviderNameField.text.trim()
            let url   = newProviderUrlField.text.trim()
            let model = newProviderModelField.text.trim()
            let env   = newProviderEnvField.text.trim().toUpperCase()

            if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                let added = settingsManager.addCustomProvider(remoteConfigRoot.category, name, url, model, env)
                if (added && typeof engineRegistry !== "undefined" && engineRegistry !== null) {
                    let slug = "custom_" + remoteConfigRoot.assetType + "_" + name.toLowerCase().replace(/ /g, "_")
                    engineRegistry.setActiveDriver(remoteConfigRoot.assetType, slug)
                }
            }
        }
    }

    // 6. Modal Dialog for OS-Specific Environment Instructions
    Dialog {
        id: envSetupDialog
        objectName: "envSetupDialog"
        title: qsTr("Configuring %1 on %2").arg(remoteConfigRoot.targetEnvName).arg(Qt.platform.os.toUpperCase())
        anchors.centerIn: parent
        width: 480
        standardButtons: Dialog.Close

        background: Rectangle {
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 8
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Text {
                text: qsTr("System Environment Variable Setup:")
                font.bold: true
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
            }

            Text {
                id: instructionsText
                objectName: "instructionsText"
                text: Qt.platform.os === "windows"
                      ? qsTr("In PowerShell (User session):\n\n[System.Environment]::SetEnvironmentVariable('%1', 'your_key_here', 'User')\n\nRestart the application after executing.")
                        .arg(remoteConfigRoot.targetEnvName)
                      : qsTr("In your terminal profile (~/.bashrc or ~/.zshrc):\n\nexport %1=\"your_key_here\"\n\nThen run 'source ~/.bashrc' and restart.")
                        .arg(remoteConfigRoot.targetEnvName)
                font.family: "Courier New, monospace"
                font.pixelSize: 11
                color: "#FFFFFF"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }
        }
    }

    Connections {
        target: remoteConfigRoot.activeDriverObj
        ignoreUnknownSignals: true
        function onConnectionVerified(success, message) {
            remoteConfigRoot.isPinging = false
        }
    }
}

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
** File: ProviderSelectorWidget.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: providerWidgetRoot
    objectName: "providerSelectorWidgetRoot"

    property string category: "script" // "script", "images", "audio", "video"
    property string audioSubCategory: (category === "audio" && typeof settingsManager !== "undefined")
                                      ? settingsManager.activeAudioCategory
                                      : ""

    // Mock property for isolated unit test assertion
    property var mockProviderModel: null

    readonly property string selectedLocalModel: {
        let cur = currentItem()
        return (cur && cur.type === "local") ? (cur.localModel || "") : ""
    }

    readonly property string selectedProviderType: {
        let cur = currentItem()
        return cur ? (cur.type || "") : ""
    }

    function currentItem() {
        if (providerCombo.currentIndex >= 0 && providerCombo.currentIndex < providerCombo.model.length) {
            return providerCombo.model[providerCombo.currentIndex]
        }
        return null
    }

    property var providerModel: {
        if (mockProviderModel !== null) return mockProviderModel
        if (typeof settingsManager === "undefined" || settingsManager === null) return []
        if (category === "audio") return settingsManager.getProvidersForCategory("audio", audioSubCategory)
        return settingsManager.getProvidersForCategory(category)
    }

    property string activeProviderName: {
        if (typeof settingsManager === "undefined" || settingsManager === null) return ""
        if (category === "script") return settingsManager.activeScriptProvider
        if (category === "images") return settingsManager.activeImageProvider
        if (category === "video") return settingsManager.activeVideoProvider
        if (category === "audio") return settingsManager.activeAudioProvider
        return ""
    }

    function setActiveProvider(name) {
        if (typeof settingsManager === "undefined" || settingsManager === null) return
        if (category === "script") settingsManager.activeScriptProvider = name
        else if (category === "images") settingsManager.activeImageProvider = name
        else if (category === "video") settingsManager.activeVideoProvider = name
        else if (category === "audio") settingsManager.activeAudioProvider = name
    }

    Layout.fillWidth: true
    implicitHeight: mainCol.implicitHeight + 16
    color: "#08121A" // Dark Well
    border.color: "#1A4A68" // Accent Border
    border.width: 1
    radius: 6

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                id: labelText
                objectName: "labelText"
                text: qsTr("Provider / Model Backend:")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
                font.bold: true
                Layout.preferredWidth: 160
            }

            ComboBox {
                id: providerCombo
                objectName: "providerCombo"
                Layout.fillWidth: true
                textRole: "name"

                model: {
                    let items = []
                    for (let i = 0; i < providerWidgetRoot.providerModel.length; ++i) {
                        items.push(providerWidgetRoot.providerModel[i])
                    }
                    items.push({ name: qsTr("+ Add Custom Provider..."), url: "", isBuiltIn: true, type: "add_action", localModel: "" })
                    return items
                }

                Component.onCompleted: syncIndex()
                onModelChanged: syncIndex()

                function syncIndex() {
                    for (let i = 0; i < count; ++i) {
                        let item = model[i]
                        if (item && item.name === providerWidgetRoot.activeProviderName) {
                            currentIndex = i
                            return
                        }
                    }
                    if (count > 0 && currentIndex === -1) currentIndex = 0
                }

                onActivated: (index) => {
                    let selected = model[index]
                    if (selected.type === "add_action") {
                        addFormBox.visible = true
                    } else {
                        addFormBox.visible = false
                        providerWidgetRoot.setActiveProvider(selected.name)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // Local Model Indicator
            Rectangle {
                id: localModelIndicator
                objectName: "localModelIndicator"
                visible: providerWidgetRoot.selectedProviderType === "local"
                Layout.fillWidth: true
                height: 28
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Text {
                        id: localModelText
                        objectName: "localModelText"
                        text: qsTr("✔ Local Model Selected: ") + providerWidgetRoot.selectedLocalModel
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("Offline execution via C-API")
                        color: "#888888"
                        font.pixelSize: 10
                    }
                }
            }

            // Gemini Bridge Indicator
            Rectangle {
                id: geminiBridgeIndicator
                objectName: "geminiBridgeIndicator"
                visible: providerCombo.currentText === "Gemini Bridge"
                Layout.fillWidth: true
                height: 28
                color: (typeof settingsManager !== "undefined" && settingsManager.hasGeminiKey) ? "#0B1B26" : "#381818"
                border.color: (typeof settingsManager !== "undefined" && settingsManager.hasGeminiKey) ? "#1A4A68" : "#FF4444"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Text {
                        id: geminiStatusText
                        objectName: "geminiStatusText"
                        text: (typeof settingsManager !== "undefined") ? settingsManager.geminiKeyStatusText : ""
                        color: (typeof settingsManager !== "undefined" && settingsManager.hasGeminiKey) ? "#00F0FF" : "#FF6666"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("GEMINI_API_KEY environment variable")
                        color: "#888888"
                        font.pixelSize: 10
                    }
                }
            }

            // Custom Endpoint Indicator
            Rectangle {
                id: customEndpointIndicator
                objectName: "customEndpointIndicator"
                visible: providerWidgetRoot.selectedProviderType === "custom"
                Layout.fillWidth: true
                height: 28
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Text {
                        id: customUrlText
                        objectName: "customUrlText"
                        text: qsTr("Endpoint URL: ") + (providerWidgetRoot.currentItem() ? providerWidgetRoot.currentItem().url : "")
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Button {
                        id: removeCustomBtn
                        objectName: "removeCustomBtn"
                        text: qsTr("Remove")
                        Layout.preferredHeight: 22
                        font.pixelSize: 10
                        background: Rectangle {
                            color: removeCustomBtn.down ? "#771111" : (removeCustomBtn.hovered ? "#AA2222" : "#381818")
                            border.color: "#FF4444"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FF8888"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (typeof settingsManager !== "undefined") {
                                settingsManager.removeCustomProvider(category, providerCombo.currentText)
                            }
                            providerCombo.syncIndex()
                        }
                    }
                }
            }
        }

        // Inline Custom Provider Adder Form
        Rectangle {
            id: addFormBox
            objectName: "addFormBox"
            visible: false
            Layout.fillWidth: true
            implicitHeight: formCol.implicitHeight + 16
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4

            ColumnLayout {
                id: formCol
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Text {
                    text: qsTr("Add Custom Endpoint Provider (%1)").arg(category.toUpperCase())
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 12
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    TextField {
                        id: newNameField
                        objectName: "newNameField"
                        Layout.preferredWidth: 180
                        placeholderText: qsTr("Name (e.g. Local Audio Server)")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#0B1B26"
                            border.color: newNameField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                    }

                    TextField {
                        id: newUrlField
                        objectName: "newUrlField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Full URL with auth params")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#0B1B26"
                            border.color: newUrlField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                    }

                    Button {
                        id: saveCustomBtn
                        objectName: "saveCustomBtn"
                        text: qsTr("Save & Apply")
                        Layout.preferredHeight: 30
                        enabled: newNameField.text.trim().length > 0 && newUrlField.text.trim().length > 0
                        background: Rectangle {
                            color: !saveCustomBtn.enabled ? "#1B3B4F" : (saveCustomBtn.down ? "#0077A8" : (saveCustomBtn.hovered ? "#00B4F0" : "#1A4A68"))
                            border.color: "#000000"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: saveCustomBtn.enabled ? "#FFFFFF" : "#888888"
                            style: saveCustomBtn.enabled ? Text.Outline : Text.Normal
                            styleColor: "#000000"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (typeof settingsManager !== "undefined") {
                                let ok = settingsManager.addCustomProvider(category, newNameField.text.trim(), newUrlField.text.trim())
                                if (ok) {
                                    newNameField.text = ""
                                    newUrlField.text = ""
                                    addFormBox.visible = false
                                }
                            }
                        }
                    }

                    Button {
                        id: cancelCustomBtn
                        objectName: "cancelCustomBtn"
                        text: qsTr("Cancel")
                        Layout.preferredHeight: 30
                        background: Rectangle {
                            color: cancelCustomBtn.down ? "#0077A8" : (cancelCustomBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                            addFormBox.visible = false
                            providerCombo.syncIndex()
                        }
                    }
                }
            }
        }
    }
}

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
** File: InferenceSettingsPanel.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: settingsPanelRoot
    objectName: "inferenceSettingsPanelRoot"
    color: "#08121A" // Dark Well
    border.color: "#1A4A68" // Accent Border
    border.width: 1
    radius: 6
    implicitWidth: 320
    implicitHeight: 360

    property string targetAssetType: "image" // "script", "image", "audio", "video"
    property var currentOptions: ({})

    // Mock options hook for headless unit testing
    property var mockDriverOptions: null
    property string mockDriverId: ""

    // Resolve active driver from EngineRegistry based on asset modality
    readonly property var activeDriver: {
        if (typeof engineRegistry === "undefined" || !engineRegistry) return null
        if (targetAssetType === "image") return engineRegistry.activeImageDriver
        if (targetAssetType === "video") return engineRegistry.activeVideoDriver
        if (targetAssetType === "audio") return engineRegistry.activeAudioDriver
        if (targetAssetType === "script") return engineRegistry.activeScriptDriver
        return null
    }

    readonly property string effectiveDriverId: {
        if (mockDriverId.length > 0) return mockDriverId
        if (activeDriver && activeDriver.driverId) return activeDriver.driverId
        return "Default"
    }

    readonly property var driverOptions: {
        if (mockDriverOptions !== null) return mockDriverOptions
        if (activeDriver && activeDriver.availableOptions) return activeDriver.availableOptions
        return []
    }

    function getOptionsPayload() {
        return Object.assign({}, currentOptions)
    }

    function resetToDefaults() {
        let defaults = {}
        for (let i = 0; i < driverOptions.length; ++i) {
            let opt = driverOptions[i]
            defaults[opt.key] = opt.defaultValue
        }
        currentOptions = defaults
    }

    onDriverOptionsChanged: {
        resetToDefaults()
    }

    Component.onCompleted: {
        resetToDefaults()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Header Bar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: headerTitleText
                objectName: "headerTitleText"
                text: qsTr("⚙ Runtime Parameters (%1)").arg(settingsPanelRoot.effectiveDriverId)
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Button {
                id: resetDefaultsBtn
                objectName: "resetDefaultsBtn"
                text: qsTr("Reset Defaults")
                implicitHeight: 22
                background: Rectangle {
                    color: resetDefaultsBtn.down ? "#0077A8" : (resetDefaultsBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    font.pixelSize: 10
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: settingsPanelRoot.resetToDefaults()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Scrollable Parameter Form (Eliminates vertical clipping)
        ScrollView {
            id: paramsScrollView
            objectName: "paramsScrollView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                id: paramsLayout
                width: paramsScrollView.availableWidth
                spacing: 10

                Repeater {
                    id: optionsRepeater
                    objectName: "optionsRepeater"
                    model: settingsPanelRoot.driverOptions

                    delegate: ColumnLayout {
                        id: optionDelegate
                        objectName: "optionDelegate_" + (modelData.key || index)
                        Layout.fillWidth: true
                        spacing: 4

                        // --- 1. Integer / Float Slider Row ---
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            visible: modelData.type === "int" || modelData.type === "float"

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: modelData.label || ""
                                    color: "#FFFFFF"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: {
                                        let val = settingsPanelRoot.currentOptions[modelData.key]
                                        if (val === undefined) val = modelData.defaultValue
                                        return modelData.type === "float" ? Number(val).toFixed(2) : Math.round(val)
                                    }
                                    color: "#00F0FF"
                                    font.family: "Courier New, monospace"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Slider {
                                id: numSlider
                                objectName: "slider_" + modelData.key
                                Layout.fillWidth: true
                                from: modelData.minValue !== undefined ? modelData.minValue : 0
                                to: modelData.maxValue !== undefined ? modelData.maxValue : 100
                                stepSize: modelData.step !== undefined ? modelData.step : (modelData.type === "float" ? 0.05 : 1)
                                value: {
                                    let val = settingsPanelRoot.currentOptions[modelData.key]
                                    return val !== undefined ? val : (modelData.defaultValue || 0)
                                }
                                onMoved: {
                                    let updated = Object.assign({}, settingsPanelRoot.currentOptions)
                                    updated[modelData.key] = modelData.type === "float" ? value : Math.round(value)
                                    settingsPanelRoot.currentOptions = updated
                                }
                            }
                        }

                        // --- 2. Discrete Choice / Dropdown Row ---
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: modelData.type === "choice" || modelData.type === "enum"

                            Text {
                                text: modelData.label || ""
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                Layout.preferredWidth: 100
                                elide: Text.ElideRight
                            }

                            ComboBox {
                                id: choiceCombo
                                objectName: "choiceCombo_" + modelData.key
                                Layout.fillWidth: true
                                model: modelData.choices || []
                                currentIndex: {
                                    let cur = settingsPanelRoot.currentOptions[modelData.key] || modelData.defaultValue
                                    return Math.max(0, model.indexOf(cur))
                                }
                                onActivated: (index) => {
                                    let updated = Object.assign({}, settingsPanelRoot.currentOptions)
                                    updated[modelData.key] = model[index]
                                    settingsPanelRoot.currentOptions = updated
                                }
                            }
                        }

                        // --- 3. Boolean / Toggle Row ---
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: modelData.type === "boolean" || modelData.type === "bool"

                            Text {
                                text: modelData.label || ""
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }

                            Switch {
                                id: boolSwitch
                                objectName: "boolSwitch_" + modelData.key
                                checked: {
                                    let val = settingsPanelRoot.currentOptions[modelData.key]
                                    return val !== undefined ? Boolean(val) : Boolean(modelData.defaultValue)
                                }
                                onToggled: {
                                    let updated = Object.assign({}, settingsPanelRoot.currentOptions)
                                    updated[modelData.key] = checked
                                    settingsPanelRoot.currentOptions = updated
                                }
                            }
                        }

                        // --- 4. String / Freeform Text Row ---
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            visible: modelData.type === "string"

                            Text {
                                text: modelData.label || ""
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                Layout.preferredWidth: 100
                                elide: Text.ElideRight
                            }

                            TextField {
                                id: strInput
                                objectName: "strInput_" + modelData.key
                                Layout.fillWidth: true
                                text: {
                                    let val = settingsPanelRoot.currentOptions[modelData.key]
                                    return val !== undefined ? String(val) : String(modelData.defaultValue || "")
                                }
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                background: Rectangle {
                                    color: "#0B1B26"
                                    border.color: strInput.activeFocus ? "#00F0FF" : "#1A4A68"
                                    border.width: 1
                                    radius: 3
                                }
                                onEditingFinished: {
                                    let updated = Object.assign({}, settingsPanelRoot.currentOptions)
                                    updated[modelData.key] = text
                                    settingsPanelRoot.currentOptions = updated
                                }
                            }
                        }
                    }
                }

                // Fallback indicator
                Text {
                    visible: settingsPanelRoot.driverOptions.length === 0
                    text: qsTr("No configurable runtime settings for this driver.")
                    color: "#888888"
                    font.italic: true
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8
                }
            }
        }
    }
}

// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: manifestCardRoot
    objectName: "LocalModalManifestCardRoot"

    property string category: "images"      // "script", "images", "video", "audio"
    property string modelName: ""           // Clean name or "Local: <name>"
    property string subCategory: ""         // "speech", "sfx", "bgm" (audio only)

    // Fallback/mockable items property for QTest harnesses
    property var mockItems: null

    readonly property var manifestItems: {
        if (mockItems !== null) {
            return mockItems
        }
        if (typeof settingsManager === "undefined" || settingsManager === null || modelName.length === 0) {
            return []
        }
        return settingsManager.verifyModelManifest(category, modelName, subCategory)
    }

    readonly property bool allFound: {
        if (!manifestItems || manifestItems.length === 0) return false
        for (let i = 0; i < manifestItems.length; ++i) {
            if (!manifestItems[i].found) return false
        }
        return true
    }

    Layout.fillWidth: true
    implicitHeight: mainCol.implicitHeight + 16
    color: "#08121A" // Dark Well
    border.color: manifestCardRoot.allFound ? "#1A4A68" : (manifestItems.length > 0 ? "#FF4444" : "#1B3B4F")
    border.width: 1
    radius: 6

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: statusIconText
                objectName: "statusIconText"
                text: manifestCardRoot.allFound ? "✔" : "⚠️"
                color: manifestCardRoot.allFound ? "#00F0FF" : "#FFAA00"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 13
                font.bold: true
            }

            Text {
                id: cardTitleText
                objectName: "cardTitleText"
                text: qsTr("Required Model Weights & Components")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 11
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            Text {
                id: statusSummaryBadge
                objectName: "statusSummaryBadge"
                text: manifestCardRoot.allFound ? qsTr("Ready for Inference")
                                                : (manifestCardRoot.manifestItems.length > 0 ? qsTr("Missing Required Files") : qsTr("Unconfigured"))
                color: manifestCardRoot.allFound ? "#00F0FF" : (manifestCardRoot.manifestItems.length > 0 ? "#FF6666" : "#888888")
                style: (manifestCardRoot.manifestItems.length > 0) ? Text.Outline : Text.Normal
                styleColor: "#000000"
                font.pixelSize: 10
                font.bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#132B3A"
        }

        Repeater {
            id: manifestRepeater
            objectName: "manifestRepeater"
            model: manifestCardRoot.manifestItems

            RowLayout {
                id: rowDelegate
                objectName: "manifestRow_" + index
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    width: 16
                    height: 16
                    radius: 8
                    color: modelData.found ? "#132B3A" : "#381818"
                    border.color: modelData.found ? "#00F0FF" : "#FF4444"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData.found ? "✓" : "✕"
                        color: modelData.found ? "#00F0FF" : "#FF6666"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 9
                        font.bold: true
                    }
                }

                Text {
                    text: modelData.label
                    color: modelData.found ? "#FFFFFF" : "#FF8888"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.pixelSize: 11
                    font.bold: !modelData.found
                    Layout.preferredWidth: 160
                }

                Text {
                    text: modelData.path
                    color: modelData.found ? "#888888" : "#FFAAAA"
                    font.pixelSize: 10
                    font.family: "Courier New, monospace"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }
        }

        Text {
            id: emptyManifestLabel
            objectName: "emptyManifestLabel"
            visible: manifestCardRoot.manifestItems.length === 0
            text: qsTr("Select or enter a valid model name to inspect components.")
            color: "#888888"
            font.italic: true
            font.pixelSize: 11
        }
    }
}

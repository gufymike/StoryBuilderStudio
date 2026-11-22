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
** File: StoryBoardAssetList.qml
** Manages attached media assets for an individual storyboard shot.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

ColumnLayout {
    id: assetListRoot
    objectName: "storyBoardAssetListRoot"
    spacing: 6

    property var modelDataList: []
    property string category: "images" // "images", "audio", "videos"
    readonly property bool isImageCategory: category === "images"
    readonly property bool isVideoCategory: category === "videos"
    readonly property bool isScriptCategory: category === "scripts"

    signal setKeyRequested(string filePath)
    signal setFinalRequested(string filePath)
    signal removeRequested(string filePath)
    signal attachClicked()

    ScrollView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true

        ListView {
            id: listView
            objectName: "attachedAssetsListView"
            anchors.fill: parent
            model: assetListRoot.modelDataList
            spacing: 4

            delegate: Rectangle {
                id: delegateRect
                objectName: "assetItem_" + index
                width: listView.width
                height: 38
                color: itemMouse.containsMouse ? "#0B1B26" : "#0A1924"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 8

                    Text {
                        text: {
                            let parts = modelData.split(/[\\/]/)
                            return parts[parts.length - 1]
                        }
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 11
                        font.bold: true
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }

                    // Primary Script button
                    Button {
                        id: primaryScriptBtn
                        objectName: "primaryScriptBtn_" + index
                        visible: assetListRoot.isScriptCategory
                        text: qsTr("Primary Script")
                        implicitHeight: 24
                        font.pixelSize: 10
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Designate as the primary script or note for this shot")
                        background: Rectangle {
                            color: primaryScriptBtn.down ? "#0077A8" : (primaryScriptBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#00F0FF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        // We can reuse setFinalRequested, or add a specific signal in the root if preferred
                        onClicked: assetListRoot.setFinalRequested(modelData)
                    }
                    // Key Image button (Only for images)
                    Button {
                        id: keyBtn
                        objectName: "keyBtn_" + index
                        visible: assetListRoot.isImageCategory
                        text: qsTr("Key")
                        implicitHeight: 24
                        font.pixelSize: 10
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Designate as the key reference frame for this shot")
                        background: Rectangle {
                            color: keyBtn.down ? "#0077A8" : (keyBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#00F0FF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: assetListRoot.setKeyRequested(modelData)
                    }

                    // Final Cut button (For images or video)
                    Button {
                        id: finalBtn
                        objectName: "finalBtn_" + index
                        visible: assetListRoot.isImageCategory || assetListRoot.isVideoCategory
                        text: qsTr("Final")
                        implicitHeight: 24
                        font.pixelSize: 10
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Mark as the final deliverable asset for export")
                        background: Rectangle {
                            color: finalBtn.down ? "#0077A8" : (finalBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#00F0FF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: assetListRoot.setFinalRequested(modelData)
                    }

                    // Delete from shot
                    Button {
                        id: removeBtn
                        objectName: "removeBtn_" + index
                        text: "✕"
                        implicitWidth: 24
                        implicitHeight: 24
                        font.pixelSize: 10
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Detach asset from this shot")
                        background: Rectangle {
                            color: removeBtn.hovered ? "#441111" : "#08121A"
                            border.color: "#FF5555"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FF5555"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: assetListRoot.removeRequested(modelData)
                    }
                }

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.NoButton
                }
            }

            Text {
                id: emptyLabel
                objectName: "emptyLabel"
                visible: listView.count === 0
                anchors.centerIn: parent
                text: qsTr("No %1 attached to this shot").arg(assetListRoot.category)
                color: "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.italic: true
                font.pixelSize: 11
            }
        }
    }

    Button {
        id: attachBtn
        objectName: "attachAssetBtn"
        text: qsTr("+ Attach %1 Asset").arg(assetListRoot.category.toUpperCase())
        Layout.fillWidth: true
        implicitHeight: 28
        font.pixelSize: 11
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Browse library or project to attach %1").arg(assetListRoot.category)
        background: Rectangle {
            color: attachBtn.down ? "#0077A8" : (attachBtn.hovered ? "#00B4F0" : "#0B1B26")
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
        onClicked: assetListRoot.attachClicked()
    }
}

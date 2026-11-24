// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: cardRoot
    objectName: "storyBoardCardRoot"
    width: 170
    height: 195
    color: isSelected ? "#0B1B26" : "#08121A"
    border.color: isSelected ? "#00F0FF" : (cardMouse.containsMouse ? "#00B4F0" : "#1A4A68")
    border.width: isSelected ? 2 : 1
    radius: 6

    property var shotData: ({})
    property int shotIndex: -1
    property bool isSelected: false

    signal clicked()
    signal moveUpRequested()
    signal moveDownRequested()
    signal deleteRequested()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Thumbnail Frame
        Rectangle {
            id: thumbFrame
            objectName: "thumbFrame"
            Layout.fillWidth: true
            Layout.preferredHeight: 95
            color: "#0A1924"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
            clip: true

            Image {
                id: keyImage
                objectName: "keyImage"
                anchors.fill: parent
                fillMode: Image.PreserveAspectCrop
                source: {
                    if (!cardRoot.shotData || !cardRoot.shotData.key_image) return ""
                    let path = cardRoot.shotData.key_image.toString()

                    if (path.startsWith("file:") || path.startsWith("http:") || path.startsWith("https:")) {
                        return path
                    }

                    // Native QML/JS conversion for local file paths
                    let urlPath = path.replace(/\\/g, "/")
                    if (Qt.platform.os === "windows" && !urlPath.startsWith("/")) {
                        urlPath = "/" + urlPath
                    }
                    return "file://" + urlPath
                }
                visible: status === Image.Ready
            }

            ColumnLayout {
                id: emptyKeyPlaceholder
                objectName: "emptyKeyPlaceholder"
                anchors.centerIn: parent
                visible: !cardRoot.shotData || !cardRoot.shotData.key_image
                spacing: 2
                Text {
                    text: "🎬"
                    font.pixelSize: 20
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: qsTr("No Key Image")
                    color: "#888888"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.pixelSize: 10
                    Layout.alignment: Qt.AlignHCenter
                }
            }

            // Order Badge
            Rectangle {
                id: orderBadge
                objectName: "orderBadge"
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 4
                width: 22
                height: 18
                color: "#CC0E222F"
                border.color: "#1A4A68"
                border.width: 1
                radius: 3
                Text {
                    id: orderBadgeText
                    objectName: "orderBadgeText"
                    anchors.centerIn: parent
                    text: (cardRoot.shotIndex + 1).toString().padStart(2, '0')
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 10
                }
            }

            // Final Cut Badge
            Rectangle {
                id: finalCutBadge
                objectName: "finalCutBadge"
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 4
                width: 16
                height: 16
                color: "#CC00A3E0"
                border.color: "#00F0FF"
                border.width: 1
                radius: 8
                visible: cardRoot.shotData && cardRoot.shotData.final_shot && cardRoot.shotData.final_shot.length > 0
                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "#FFFFFF"
                    font.bold: true
                    font.pixelSize: 9
                }
            }
        }

        // Title
        Text {
            id: shotTitleText
            objectName: "shotTitleText"
            Layout.fillWidth: true
            text: cardRoot.shotData ? cardRoot.shotData.name : ""
            color: "#FFFFFF"
            style: Text.Outline
            styleColor: "#000000"
            font.bold: true
            font.pixelSize: 11
            elide: Text.ElideRight
        }

        // Asset Counts
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                id: scriptCountText
                objectName: "scriptCountText"
                text: "📝 " + (cardRoot.shotData && cardRoot.shotData.scripts ? cardRoot.shotData.scripts.length : 0)
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
            Text {
                id: imgCountText
                objectName: "imgCountText"
                text: "🖼 " + (cardRoot.shotData && cardRoot.shotData.images ? cardRoot.shotData.images.length : 0)
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
            Text {
                id: audioCountText
                objectName: "audioCountText"
                text: "🔊 " + (cardRoot.shotData && cardRoot.shotData.audio ? cardRoot.shotData.audio.length : 0)
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
            Text {
                id: videoCountText
                objectName: "videoCountText"
                text: "🎥 " + (cardRoot.shotData && cardRoot.shotData.videos ? cardRoot.shotData.videos.length : 0)
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
            Item { Layout.fillWidth: true }
        }

        // Actions
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                id: moveUpBtn
                objectName: "moveUpBtn"
                text: "◀"
                Layout.preferredWidth: 26
                Layout.preferredHeight: 22
                enabled: cardRoot.shotIndex > 0
                font.pixelSize: 9
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Move shot left / earlier in sequence")
                background: Rectangle {
                    color: moveUpBtn.down ? "#0077A8" : (moveUpBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: moveUpBtn.enabled ? "#00F0FF" : "#555555"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: cardRoot.moveUpRequested()
            }

            Button {
                id: moveDownBtn
                objectName: "moveDownBtn"
                text: "▶"
                Layout.preferredWidth: 26
                Layout.preferredHeight: 22
                font.pixelSize: 9
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Move shot right / later in sequence")
                background: Rectangle {
                    color: moveDownBtn.down ? "#0077A8" : (moveDownBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#00F0FF"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: cardRoot.moveDownRequested()
            }

            Item { Layout.fillWidth: true }

            Button {
                id: deleteBtn
                objectName: "deleteBtn"
                text: "✕"
                Layout.preferredWidth: 24
                Layout.preferredHeight: 22
                font.pixelSize: 10
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Remove this shot from storyboard")
                background: Rectangle {
                    color: deleteBtn.hovered ? "#441111" : "#0B1B26"
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
                onClicked: cardRoot.deleteRequested()
            }
        }
    }

    MouseArea {
        id: cardMouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        z: -1
        onClicked: cardRoot.clicked()
    }
}

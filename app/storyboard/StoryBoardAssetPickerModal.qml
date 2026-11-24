// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Popup {
    id: pickerModal
    objectName: "storyBoardAssetPickerModalRoot"
    anchors.centerIn: parent
    width: Math.min(740, parent ? parent.width - 40 : 740)
    height: Math.min(520, parent ? parent.height - 40 : 520)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: "#08121A"
        border.color: "#1A4A68"
        border.width: 1
        radius: 6
    }

    property string category: "images" // "images", "audio", "videos"
    property string projectPath: ""

    signal assetSelected(string filePath)

    function openForCategory(cat, path) {
        category = cat
        projectPath = path
        console.log("[StoryBoardAssetPickerModal] setting path " + path + " for category " + cat)
        if (activeModel && typeof activeModel.setProjectPath === "function") {
            // Setting the path automatically triggers a scan if projectPath is empty
            activeModel.setProjectPath(path)

            // If the path was already set, setProjectPath won't scan again,
            // so we must explicitly refresh it to catch new imports!
            if (typeof activeModel.refresh === "function") {
                activeModel.refresh()
            }
        }
        pickerModal.open()
    }

    // Dynamic model selection based on category
    readonly property var activeModel: {
        if (category === "audio") return audioModel
        if (category === "videos") return videoModel
        if (category === "scripts") return scriptModel
        return imageModel
    }

    ScriptsDirectoryModel { id: scriptModel; objectName: "scriptModel" }
    ImageDirectoryModel { id: imageModel; objectName: "imageModel" }
    AudioDirectoryModel { id: audioModel; objectName: "audioModel" }
    VideoDirectoryModel { id: videoModel; objectName: "videoModel" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Header
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Text {
                    id: headerTitleText
                    objectName: "headerTitleText"
                    text: qsTr("Select %1 from Project Library").arg(pickerModal.category.toUpperCase())
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 14
                }
                Text {
                    text: qsTr("Choose an existing generated or imported file to attach to this shot.")
                    color: "#CCCCCC"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: closeBtn
                objectName: "closePickerBtn"
                text: "✕"
                implicitWidth: 26
                implicitHeight: 26
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Close Asset Picker")
                background: Rectangle {
                    color: closeBtn.hovered ? "#00B4F0" : "#0B1B26"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: pickerModal.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // List View of Available Assets
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: assetList
                objectName: "availableAssetsListView"
                model: pickerModal.activeModel
                spacing: 4

                delegate: Rectangle {
                    id: delegateItem
                    objectName: "pickerItem_" + index
                    width: assetList.width
                    height: 48
                    color: itemMouse.containsMouse ? "#0B1B26" : "#0A1924"
                    border.color: itemMouse.containsMouse ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 10

                        // Visual thumbnail for images
                        Rectangle {
                            visible: pickerModal.category === "images"
                            Layout.preferredWidth: 48
                            Layout.preferredHeight: 34
                            color: "#000000"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: model.filePath || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                            }
                        }

                        // Badge icon for audio/video
                        Text {
                            visible: pickerModal.category !== "images"
                            text: pickerModal.category === "audio" ? "🔊" : "🎥"
                            font.pixelSize: 16
                            Layout.alignment: Qt.AlignVCenter
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: model.fileName || ""
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: true
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                            }

                            Text {
                                text: qsTr("Size: %1 | Modified: %2").arg(model.fileSize || "0 KB").arg(model.modifiedDate || "")
                                color: "#888888"
                                font.pixelSize: 9
                            }
                        }

                        Button {
                            id: attachItemBtn
                            objectName: "attachItemBtn_" + index
                            text: qsTr("Attach")
                            implicitHeight: 26
                            font.pixelSize: 10
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Attach this asset to the active shot")
                            background: Rectangle {
                                color: attachItemBtn.down ? "#0077A8" : (attachItemBtn.hovered ? "#00B4F0" : "#08121A")
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
                            onClicked: {
                                pickerModal.assetSelected(model.filePath)
                                pickerModal.close()
                            }
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onDoubleClicked: {
                            pickerModal.assetSelected(model.filePath)
                            pickerModal.close()
                        }
                    }
                }

                Text {
                    id: emptyLibraryText
                    objectName: "emptyLibraryText"
                    anchors.centerIn: parent
                    visible: assetList.count === 0
                    text: qsTr("No %1 assets found in this project.").arg(pickerModal.category)
                    color: "#888888"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.italic: true
                    font.pixelSize: 12
                }
            }
        }

        // Footer
        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            Button {
                id: cancelBtn
                objectName: "cancelBtn"
                text: qsTr("Cancel")
                implicitHeight: 26
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Dismiss asset selection")
                background: Rectangle {
                    color: cancelBtn.hovered ? "#00B4F0" : "#0B1B26"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: pickerModal.close()
            }
        }
    }
}

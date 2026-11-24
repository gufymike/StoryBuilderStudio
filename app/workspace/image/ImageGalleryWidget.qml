// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: imageGalleryRoot
    objectName: "imageGalleryWidgetRoot"
    color: "#08121A"
    border.color: "#1A4A68"
    border.width: 1
    radius: 4

    property string currentProjectPath: ""
    property var selectedPaths: []

    // Testing / Inspection hooks
    property alias directoryModel: imageDirModel
    readonly property int selectedCount: imageGalleryRoot.selectedPaths.length

    ImageDirectoryModel {
        id: imageDirModel
        objectName: "imageDirModel"
    }

    AssetImportAction {
        id: imageImporter
        objectName: "imageImporter"
        targetModel: imageDirModel
        dialogTitle: qsTr("Select Image(s) to Import")
        nameFilters: ["Image Files (*.png *.jpg *.jpeg *.webp *.bmp)", "All Files (*)"]
    }

    // Shared Single Context Menu at Root Level
    AssetContextMenu {
        id: sharedContextMenu
        objectName: "sharedContextMenu"
        assetType: "image"
    }

    function updateProjectPath(path) {
        currentProjectPath = path
        selectedPaths = []
        imageDirModel.projectPath = path
    }

    function isSelected(filePath) {
        return selectedPaths.indexOf(filePath) !== -1
    }

    function clearSelection() {
        selectedPaths = []
    }

    function handleItemClick(filePath, modifiers) {
        let list = selectedPaths.slice()
        let idx = list.indexOf(filePath)

        if (modifiers & Qt.ControlModifier) {
            if (idx !== -1) {
                list.splice(idx, 1)
            } else {
                list.push(filePath)
            }
        } else {
            list = [filePath]
        }
        selectedPaths = list
    }

    function selectAll() {
        let all = []
        for (let i = 0; i < imageDirModel.rowCount(); ++i) {
            let path = imageDirModel.data(imageDirModel.index(i, 0), Qt.UserRole + 1)
            if (path) all.push(path)
        }
        selectedPaths = all
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // --- TOP TOOLBAR ---
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: importBtn
                objectName: "importImagesBtn"
                text: qsTr("📥 Import Images")
                implicitHeight: 26
                font.pixelSize: 11
                enabled: typeof projectManager !== "undefined" && projectManager.isProjectOpen
                background: Rectangle {
                    color: importBtn.down ? "#0077A8" : (importBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: importBtn.enabled ? "#FFFFFF" : "#888888"
                    style: importBtn.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: imageImporter.open()
            }

            Button {
                id: refreshBtn
                objectName: "refreshGalleryBtn"
                text: qsTr("🔄 Refresh Gallery")
                implicitHeight: 26
                font.pixelSize: 11
                background: Rectangle {
                    color: refreshBtn.down ? "#0077A8" : (refreshBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
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
                onClicked: imageDirModel.refresh()
            }

            Text {
                id: selectionStatsText
                objectName: "selectionStatsText"
                text: imageGalleryRoot.selectedPaths.length > 0
                      ? qsTr("(%1 selected)").arg(imageGalleryRoot.selectedPaths.length)
                      : ""
                color: "#00F0FF"
                font.pixelSize: 11
                font.bold: true
                verticalAlignment: Text.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Button {
                id: selectAllBtn
                objectName: "selectAllBtn"
                text: qsTr("Select All")
                implicitHeight: 26
                font.pixelSize: 11
                visible: imageGalleryRoot.selectedPaths.length < imageDirModel.rowCount() && imageDirModel.rowCount() > 0
                background: Rectangle {
                    color: selectAllBtn.hovered ? "#00B4F0" : "#0B1B26"
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
                onClicked: imageGalleryRoot.selectAll()
            }

            Button {
                id: deselectAllBtn
                objectName: "deselectAllBtn"
                text: qsTr("Deselect All")
                implicitHeight: 26
                font.pixelSize: 11
                visible: imageGalleryRoot.selectedPaths.length > 0
                background: Rectangle {
                    color: deselectAllBtn.hovered ? "#00B4F0" : "#0B1B26"
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
                onClicked: imageGalleryRoot.clearSelection()
            }
        }

        // --- GRID VIEW ---
        GridView {
            id: imageGridView
            objectName: "imageGridView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            cellWidth: 140
            cellHeight: 150
            model: imageDirModel

            MouseArea {
                anchors.fill: parent
                z: -1
                onClicked: imageGalleryRoot.clearSelection()
            }

            delegate: Item {
                id: delegateItem
                objectName: "galleryDelegate_" + index
                width: 130
                height: 140

                readonly property bool itemSelected: imageGalleryRoot.isSelected(model.filePath)

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 4

                    Rectangle {
                        Layout.preferredWidth: 130
                        Layout.preferredHeight: 100
                        color: delegateItem.itemSelected ? "#0077A8" : "#0B1B26"
                        border.color: delegateItem.itemSelected ? "#00F0FF" : (cellMouseArea.containsMouse ? "#1A4A68" : "#0A1924")
                        border.width: delegateItem.itemSelected ? 2 : 1
                        radius: 4
                        clip: true

                        Image {
                            anchors.fill: parent
                            anchors.margins: delegateItem.itemSelected ? 3 : 4
                            source: model.filePath
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                        }

                        // Selection Checkmark Badge
                        Rectangle {
                            anchors.top: parent.top
                            anchors.right: parent.right
                            anchors.margins: 4
                            width: 16
                            height: 16
                            radius: 8
                            color: "#00F0FF"
                            visible: delegateItem.itemSelected

                            Text {
                                anchors.centerIn: parent
                                text: "✓"
                                color: "#000000"
                                font.bold: true
                                font.pixelSize: 10
                            }
                        }

                        // Consolidated Single MouseArea Handling Left & Right Clicks
                        MouseArea {
                            id: cellMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton

                            onClicked: (mouse) => {
                                if (mouse.button === Qt.RightButton) {
                                    sharedContextMenu.targetFilePath = model.filePath
                                    sharedContextMenu.popup()
                                } else {
                                    imageGalleryRoot.handleItemClick(model.filePath, mouse.modifiers)
                                }
                            }

                            onDoubleClicked: (mouse) => {
                                if (mouse.button === Qt.LeftButton) {
                                    fullSizeImage.source = model.filePath
                                    imageNameLabel.text = model.fileName
                                    imageInspectPopup.open()
                                }
                            }
                        }

                        ToolTip.visible: cellMouseArea.containsMouse
                        ToolTip.delay: 500
                        ToolTip.text: qsTr("File: ") + (model.fileName || "") + "\n" +
                                      qsTr("Size: ") + (model.fileSize || "") + "\n" +
                                      qsTr("Created: ") + (model.createdDate || "") + "\n" +
                                      qsTr("Dimensions: ") + (model.resolution || "")
                    }

                    Text {
                        Layout.fillWidth: true
                        text: model.fileName || ""
                        color: delegateItem.itemSelected ? "#00F0FF" : "#FFFFFF"
                        font.pixelSize: 11
                        font.bold: delegateItem.itemSelected
                        horizontalAlignment: Text.AlignHCenter
                        elide: Text.ElideMiddle
                    }
                }
            }
        }
    }

    // ==========================================
    // FULL-SIZE INSPECTION MODAL POPUP
    // ==========================================
    Popup {
        id: imageInspectPopup
        objectName: "imageInspectPopup"
        anchors.centerIn: parent
        width: Math.min(720, parent ? parent.width - 40 : 720)
        height: Math.min(560, parent ? parent.height - 40 : 560)
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

            RowLayout {
                Layout.fillWidth: true
                Text {
                    id: imageNameLabel
                    objectName: "imageInspectNameLabel"
                    text: qsTr("Image Inspection")
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 14
                }
                Item { Layout.fillWidth: true }
                Button {
                    id: closeInspectBtn
                    objectName: "closeInspectBtn"
                    text: qsTr("✕")
                    implicitWidth: 26
                    implicitHeight: 26
                    onClicked: imageInspectPopup.close()
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#1A4A68" }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                Image {
                    id: fullSizeImage
                    objectName: "fullSizeImage"
                    anchors.fill: parent
                    anchors.margins: 8
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                }
            }
        }
    }

    Connections {
        target: (typeof projectManager !== "undefined") ? projectManager : null
        ignoreUnknownSignals: true

        function onAssetDeleted(filePath) {
            imageDirModel.refresh()

            let cleanDeleted = filePath.replace(/^file:\/\//i, "").toLowerCase()
            let list = imageGalleryRoot.selectedPaths.slice()
            for (let i = list.length - 1; i >= 0; --i) {
                let cleanSelected = list[i].replace(/^file:\/\//i, "").toLowerCase()
                if (cleanSelected === cleanDeleted) {
                    list.splice(i, 1)
                }
            }
            imageGalleryRoot.selectedPaths = list

            let currentInspect = fullSizeImage.source.toString().replace(/^file:\/\//i, "").toLowerCase()
            if (currentInspect === cleanDeleted && imageInspectPopup.opened) {
                imageInspectPopup.close()
                fullSizeImage.source = ""
            }
        }
    }
}

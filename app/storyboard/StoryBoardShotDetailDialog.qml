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
** File: StoryBoardShotDetailDialog.qml
** Detailed inspector window for managing shot metadata and multimedia links.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

Window {
    id: detailWindow
    objectName: "storyBoardShotDetailWindow"
    title: qsTr("Shot Detail Inspector - Story Builder Studio")
    width: 780
    height: 600
    minimumWidth: 640
    minimumHeight: 480
    color: "#08121A"
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    property var manager: null
    property int activeIndex: -1
    readonly property var activeShot: (manager && activeIndex >= 0 && activeIndex < manager.shots.length)
                                      ? manager.shots[activeIndex] : null

    function openForShot(mgr, idx, parentWin) {
        manager = mgr
        activeIndex = idx
        if (parentWin) {
            transientParent = parentWin
            x = parentWin.x + Math.max(20, (parentWin.width - width) / 2)
            y = parentWin.y + Math.max(20, (parentWin.height - height) / 2)
        }
        show()
        raise()
        requestActivate()
    }

    StoryBoardAssetPickerModal {
        id: projectAssetPicker
        objectName: "projectAssetPicker"
        onAssetSelected: (filePath) => {
            if (manager && activeIndex >= 0) {
                let cleanPath = filePath.replace(/^file:\/\//i, "")
                if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) cleanPath = cleanPath.substring(1)
                manager.addAssetToShot(activeIndex, projectAssetPicker.category, cleanPath)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // ==========================================
        // 1. TOP METADATA & RENAME BAR
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("Shot Name:")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
                font.bold: true
            }

            TextField {
                id: nameField
                objectName: "shotNameField"
                Layout.fillWidth: true
                text: activeShot ? activeShot.name : ""
                color: "#00F0FF"
                font.bold: true
                background: Rectangle {
                    color: "#0B1B26"
                    border.color: nameField.activeFocus ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                }
                onAccepted: renameBtn.clicked()
            }

            Button {
                id: renameBtn
                objectName: "renameBtn"
                text: qsTr("Rename")
                implicitHeight: 28
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Update the name of this shot")
                background: Rectangle {
                    color: renameBtn.down ? "#0077A8" : (renameBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: {
                    if (manager && activeIndex >= 0) {
                        // 1. Send the edit to the C++ backend
                        manager.renameShot(activeIndex, nameField.text)

                        // 2. Clear focus so the keyboard commits
                        nameField.focus = false

                        // 3. Force-restore the declarative binding using Qt.binding()
                        nameField.text = Qt.binding(function() {
                            return activeShot ? activeShot.name : ""
                        })
                    }
                }
            }
        }

        // ==========================================
        // 2. KEY IMAGE, FINAL CUT & SCRIPT LINKAGE
        // ==========================================
        Rectangle {
            id: overviewCard
            objectName: "overviewCard"
            Layout.fillWidth: true
            Layout.preferredHeight: 110
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                Rectangle {
                    width: 130
                    height: 85
                    color: "#0A1924"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
                    clip: true

                    Image {
                        id: keyPreviewImg
                        objectName: "keyPreviewImg"
                        anchors.fill: parent
                        fillMode: Image.PreserveAspectCrop
                        source: {
                            if (!activeShot || !activeShot.key_image) return ""
                            let path = activeShot.key_image.toString()

                            if (path.startsWith("file:") || path.startsWith("http:") || path.startsWith("https:")) {
                                return path
                            }

                            let urlPath = path.replace(/\\/g, "/")
                            if (Qt.platform.os === "windows" && !urlPath.startsWith("/")) {
                                urlPath = "/" + urlPath
                            }
                            return "file://" + urlPath
                        }
                    }
                    Text {
                        id: noKeyImageText
                        objectName: "noKeyImageText"
                        anchors.centerIn: parent
                        visible: !activeShot || !activeShot.key_image
                        text: qsTr("No Key Image")
                        color: "#888888"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 10
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Text {
                        id: keyImageDisplay
                        objectName: "keyImageDisplay"
                        text: qsTr("Key Image: ") + (activeShot && activeShot.key_image ? activeShot.key_image : "<Unassigned>")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Text {
                        id: finalCutDisplay
                        objectName: "finalCutDisplay"
                        text: qsTr("Final Cut: ") + (activeShot && activeShot.final_shot ? activeShot.final_shot : "<Empty>")
                        color: activeShot && activeShot.final_shot ? "#00F0FF" : "#888888"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Text {
                        id: scriptLinkDisplay
                        objectName: "scriptLinkDisplay"
                        text: qsTr("Script: ") + (activeShot && activeShot.script_file ? activeShot.script_file : "<None Linked>")
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 11
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Text {
                        id: modifiedDateDisplay
                        objectName: "modifiedDateDisplay"
                        text: qsTr("Modified: ") + (activeShot ? activeShot.modified_at : "")
                        color: "#888888"
                        font.pixelSize: 10
                    }
                }
            }
        }

        // ==========================================
        // 3. MULTIMEDIA ASSET BUCKETS & TABS
        // ==========================================
        TabBar {
            id: assetTabBar
            objectName: "assetTabBar"
            Layout.fillWidth: true
            background: Rectangle { color: "transparent" }

            TabButton {
                id: scriptsTab
                objectName: "scriptsTab"
                text: qsTr("Scripts")
                implicitHeight: 28
                background: Rectangle {
                    color: imagesTab.checked ? "#1A4A68" : (imagesTab.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: imagesTab.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            TabButton {
                id: imagesTab
                objectName: "imagesTab"
                text: qsTr("Images")
                implicitHeight: 28
                background: Rectangle {
                    color: imagesTab.checked ? "#1A4A68" : (imagesTab.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: imagesTab.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TabButton {
                id: audioTab
                objectName: "audioTab"
                text: qsTr("Audio")
                implicitHeight: 28
                background: Rectangle {
                    color: audioTab.checked ? "#1A4A68" : (audioTab.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: audioTab.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }

            TabButton {
                id: videoTab
                objectName: "videoTab"
                text: qsTr("Video")
                implicitHeight: 28
                background: Rectangle {
                    color: videoTab.checked ? "#1A4A68" : (videoTab.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: videoTab.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        StackLayout {
            id: assetStack
            objectName: "assetStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: assetTabBar.currentIndex

            // 1 . Scripts
            StoryBoardAssetList {
                id: scriptsAssetList
                objectName: "scriptsAssetList"
                category: "scripts"
                modelDataList: activeShot ? activeShot.scripts : []
                onSetFinalRequested: (path) => manager.setShotScript(activeIndex, path) // Wires to script_file
                onRemoveRequested: (path) => manager.removeAssetFromShot(activeIndex, "scripts", path)
                onAttachClicked: {
                    let path = manager ? manager.episodePath : ""
                    projectAssetPicker.openForCategory("scripts", path)
                }
            }
            // 2. Images
            StoryBoardAssetList {
                id: imagesAssetList
                objectName: "imagesAssetList"
                category: "images"
                modelDataList: activeShot ? activeShot.images : []
                onSetKeyRequested: (path) => manager.setShotKeyImage(activeIndex, path)
                onSetFinalRequested: (path) => manager.setShotFinalCut(activeIndex, path)
                onRemoveRequested: (path) => manager.removeAssetFromShot(activeIndex, "images", path)
                onAttachClicked: {
                    let path = manager ? manager.episodePath : ""
                    projectAssetPicker.openForCategory("images", path)
                }
            }

            // 3. Audio
            StoryBoardAssetList {
                id: audioAssetList
                objectName: "audioAssetList"
                category: "audio"
                modelDataList: activeShot ? activeShot.audio : []
                onRemoveRequested: (path) => manager.removeAssetFromShot(activeIndex, "audio", path)
                onAttachClicked: {
                    let path = manager ? manager.episodePath : ""
                    projectAssetPicker.openForCategory("audio", path)
                }
            }

            // 4. Video
            StoryBoardAssetList {
                id: videoAssetList
                objectName: "videoAssetList"
                category: "videos"
                modelDataList: activeShot ? activeShot.videos : []
                onSetFinalRequested: (path) => manager.setShotFinalCut(activeIndex, path)
                onRemoveRequested: (path) => manager.removeAssetFromShot(activeIndex, "videos", path)
                onAttachClicked: {
                    let path = manager ? manager.episodePath : ""
                    projectAssetPicker.openForCategory("videos", path)
                }
            }
        }

        // ==========================================
        // 4. FOOTER CONTROLS
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Button {
                id: closeDialogBtn
                objectName: "closeDialogBtn"
                text: qsTr("Close")
                implicitHeight: 28
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Dismiss shot inspector")
                background: Rectangle {
                    color: closeDialogBtn.hovered ? "#00B4F0" : "#0B1B26"
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
                onClicked: detailWindow.close()
            }
        }
    }
}

// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

Rectangle {
    id: storyBoardWorkspace
    objectName: "storyBoardWorkspaceViewRoot"
    color: "#08121A"
    border.color: "#1A4A68"
    border.width: 1
    radius: 4

    property string episodeProjectPath: ""

    // Testing / Inspection hooks
    property alias detailInspector: detailDialog
    property alias shotGridView: shotGrid
    property alias statusMessage: statusText.text


    // Connect to global storyboardManager singleton
    Connections {
        target: (typeof storyBoardManager !== "undefined") ? storyBoardManager : null
        ignoreUnknownSignals: true

        function onBoardError(err) {
            statusText.text = qsTr("Error: ") + err
            statusText.color = "#FF4444"
        }
        function onBoardSaved() {
            statusText.text = qsTr("StoryBoard saved (storyboard.json)")
            statusText.color = "#00F0FF"
        }
    }

    // Auto-clean removed project assets from storyboard cards
    Connections {
        target: (typeof projectManager !== "undefined") ? projectManager : null
        ignoreUnknownSignals: true

        function onAssetDeleted(filePath) {
            if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                storyBoardManager.cleanupDeletedAsset(filePath)
            }
        }
    }

    StoryBoardShotDetailDialog {
        id: detailDialog
        objectName: "detailDialog"
    }

    function urlToLocalPath(url) {
            if (!url) return ""
            let path = url.toString().replace(/^file:\/\//i, "")
            if (Qt.platform.os === "windows" && path.startsWith("/")) {
                path = path.substring(1)
            }
            return decodeURIComponent(path)
        }

        function defaultExportFolderUrl() {
            let base = (typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.episodePath)
                       ? storyBoardManager.episodePath : ""
            if (base.length === 0) return ""
            let normalized = base.replace(/\\/g, "/")
            return (Qt.platform.os === "windows" ? "file:///" : "file://") + normalized
        }

        StoryBoardExporter {
            id: boardExporter
            objectName: "boardExporter"
            onExportSucceeded: (savedPath) => {
                statusText.text = qsTr("Exported storyboard: ") + savedPath
                statusText.color = "#00F0FF"
            }
            onExportFailed: (err) => {
                statusText.text = qsTr("Error: ") + err
                statusText.color = "#FF4444"
            }
        }

        FileDialog {
            id: exportPdfDialog
            objectName: "exportPdfDialog"
            title: qsTr("Export Storyboard to PDF")
            fileMode: FileDialog.SaveFile
            nameFilters: ["PDF Documents (*.pdf)"]
            onAccepted: {
                let nativePath = storyBoardWorkspace.urlToLocalPath(selectedFile)
                let epTitle = (typeof projectManager !== "undefined" && projectManager !== null) ? projectManager.projectName : ""
                let epAuthor = (typeof projectManager !== "undefined" && projectManager !== null) ? projectManager.author : ""
                boardExporter.exportToPdf(nativePath, storyBoardManager, epTitle, epAuthor)
            }
        }

        FileDialog {
            id: exportPptxDialog
            objectName: "exportPptxDialog"
            title: qsTr("Export Storyboard to PowerPoint")
            fileMode: FileDialog.SaveFile
            nameFilters: ["PowerPoint Presentations (*.pptx)"]
            onAccepted: {
                let nativePath = storyBoardWorkspace.urlToLocalPath(selectedFile)
                let epTitle = (typeof projectManager !== "undefined" && projectManager !== null) ? projectManager.projectName : ""
                let epAuthor = (typeof projectManager !== "undefined" && projectManager !== null) ? projectManager.author : ""
                boardExporter.exportToPptx(nativePath, storyBoardManager, epTitle, epAuthor)
            }
        }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // ==========================================
        // 1. TOP TOOLBAR & SHOT CREATION BAR
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                id: viewHeaderTitle
                objectName: "viewHeaderTitle"
                text: qsTr("Production Story Board")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 14
            }

            Text {
                id: shotStatsText
                objectName: "shotStatsText"
                text: (typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.hasValidPath)
                      ? qsTr("(%1 Shots)").arg(storyBoardManager.shotCount)
                      : qsTr("(No Episode Loaded)")
                color: (typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.hasValidPath)
                       ? "#00F0FF" : "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 11
            }

            Item { Layout.fillWidth: true }

            Button {
                id: addShotBtn
                objectName: "addShotBtn"
                text: qsTr("+ Add Shot Card")
                implicitHeight: 26
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Create a new shot card and append to sequence")
                background: Rectangle {
                    color: addShotBtn.down ? "#0077A8" : (addShotBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: addShotBtn.enabled ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: addShotBtn.enabled ? "#00F0FF" : "#555555"
                    style: addShotBtn.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                        addShotDialog.open()
                        dialogShotNameField.forceActiveFocus()
                }
            }
            Button {
                id: exportPdfBtn
                objectName: "exportPdfBtn"
                text: qsTr("Export PDF...")
                implicitHeight: 26
                font.pixelSize: 11
                enabled: typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.shotCount > 0
                opacity: enabled ? 1.0 : 0.45
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Export single-shot 16:9 storyboard deck as PDF")
                background: Rectangle {
                    color: exportPdfBtn.down ? "#0077A8" : (exportPdfBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: exportPdfBtn.enabled ? "#1A4A68" : "#112838"
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
                    let folderUrl = storyBoardWorkspace.defaultExportFolderUrl()
                    if (folderUrl.length > 0) exportPdfDialog.currentFolder = folderUrl
                    exportPdfDialog.open()
                }
            }

            Button {
                id: exportPptxBtn
                objectName: "exportPptxBtn"
                text: qsTr("Export PPTX...")
                implicitHeight: 26
                font.pixelSize: 11
                enabled: typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.shotCount > 0
                opacity: enabled ? 1.0 : 0.45
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Export single-shot 16:9 storyboard deck as PowerPoint (.pptx)")
                background: Rectangle {
                    color: exportPptxBtn.down ? "#0077A8" : (exportPptxBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: exportPptxBtn.enabled ? "#1A4A68" : "#112838"
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
                    let folderUrl = storyBoardWorkspace.defaultExportFolderUrl()
                    if (folderUrl.length > 0) exportPptxDialog.currentFolder = folderUrl
                    exportPptxDialog.open()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // ==========================================
        // 2. GRID FLOW OF SHOT CARDS
        // ==========================================
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            GridView {
                id: shotGrid
                objectName: "shotGrid"
                width: parent.width
                cellWidth: 185
                cellHeight: 210
                model: (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) ? storyBoardManager.shots : []

                delegate: StoryBoardCard {
                    objectName: "storyBoardCard_" + index
                    shotData: modelData
                    shotIndex: index
                    onClicked: {
                        if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                            detailDialog.openForShot(storyBoardManager, index, storyBoardWorkspace.Window.window)
                        }
                    }
                    onMoveUpRequested: {
                        if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                            storyBoardManager.moveShotUp(index)
                        }
                    }
                    onMoveDownRequested: {
                        if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                            storyBoardManager.moveShotDown(index)
                        }
                    }
                    onDeleteRequested: {
                        if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                            storyBoardManager.removeShot(index)
                        }
                    }
                }
            }
        }

        // ==========================================
        // 3. STATUS FOOTER
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Text {
                id: statusText
                objectName: "statusText"
                text: qsTr("Source of truth: storyboard.json")
                color: "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 10
            }
            Item { Layout.fillWidth: true }
        }
    }

    Dialog {
        id: addShotDialog
        objectName: "addShotDialog"
        title: qsTr("Create New Shot")
        anchors.centerIn: parent
        width: 350
        modal: true
        standardButtons: Dialog.Save | Dialog.Cancel

        background: Rectangle {
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 6
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 10

            Text {
                text: qsTr("Shot Name:")
                color: "#FFFFFF"
                font.bold: true
            }
            TextField {
                id: dialogShotNameField
                objectName: "dialogShotNameField"
                Layout.fillWidth: true
                placeholderText: qsTr("e.g., Shot 01 Ext")
                color: "#FFFFFF"
                background: Rectangle { color: "#0B1B26"; border.color: "#1A4A68" }
            }
            Text {
                id: shotErrorLabel
                objectName: "shotErrorLabel"
                color: "#FF5555"
                visible: false
                text: qsTr("Shot name cannot be empty.")
            }
        }

        onAccepted: {
            if (dialogShotNameField.text.trim().length === 0) {
                shotErrorLabel.visible = true
                // Prevent closing by rejecting the accept and keeping it open
                addShotDialog.reject()
                addShotDialog.open()
            } else {
                shotErrorLabel.visible = false
                if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                    storyBoardManager.addShot(dialogShotNameField.text.trim())
                    dialogShotNameField.text = ""
                }
            }
        }
        onRejected: {
            shotErrorLabel.visible = false
            dialogShotNameField.text = ""
        }
    }
}

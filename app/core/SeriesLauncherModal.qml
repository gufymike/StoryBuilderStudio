// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import QtQuick.Dialogs
import StoryBuilderStudio

Dialog {
    id: launcherDialog
    objectName: "launcherDialogRoot"
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 640
    height: 480

    property var availableSeriesList: (typeof seriesContainer !== "undefined" && seriesContainer.availableSeries)
                                          ? seriesContainer.availableSeries : []
    property string pendingDeleteSeriesPath: ""
    property string pendingDeleteSeriesName: ""

    function normalizeLocalPath(rawPath) {
        if (!rawPath) return ""
        let clean = String(rawPath).replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && clean.startsWith("/")) {
            clean = clean.substring(1)
        }
        return clean
    }

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 8
    }

    onAboutToShow: {
        if (typeof seriesContainer !== "undefined") {
            seriesContainer.refreshAvailableSeries()
        }
    }

    header: Rectangle {
        color: "#132B3A" // Stepper Active Tile
        height: 50
        radius: 8
        border.color: "#1A4A68"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            Text {
                id: modalTitleText
                objectName: "modalTitleText"
                text: qsTr("Story Builder Studio — Series Workspace")
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 15
            }
            Item { Layout.fillWidth: true }
            Text {
                id: modalSubtitleText
                objectName: "modalSubtitleText"
                text: qsTr("Select or create a series to continue")
                color: "#888888"
                font.pixelSize: 12
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 12
        anchors.margins: 12

        Text {
            id: sectionHeaderLabel
            objectName: "sectionHeaderLabel"
            text: qsTr("Available Series Projects:")
            color: "#FFFFFF"
            style: Text.Outline
            styleColor: "#000000"
            font.bold: true
            font.pixelSize: 13
        }

        // Series List View
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 6
            clip: true

            ListView {
                id: seriesListView
                objectName: "seriesListView"
                anchors.fill: parent
                model: launcherDialog.availableSeriesList
                spacing: 2

                delegate: Rectangle {
                    id: seriesRowDelegate
                    objectName: "seriesRowDelegate_" + index
                    width: seriesListView.width
                    height: 54
                    color: itemMouseArea.containsMouse ? "#132B3A" : (index % 2 === 0 ? "#08121A" : "#0B1B26")
                    border.color: itemMouseArea.containsMouse ? "#00F0FF" : "transparent"
                    border.width: 1

                    readonly property string seriesDisplayName: {
                        let normalized = String(modelData).replace(/\\/g, "/").replace(/\/+$/, "")
                        let parts = normalized.split("/")
                        return parts[parts.length - 1] || modelData
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            Text {
                                text: seriesRowDelegate.seriesDisplayName
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: true
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                            Text {
                                text: modelData
                                color: "#888888"
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }

                        Button {
                            id: openSeriesBtn
                            objectName: "openSeriesBtn_" + index
                            text: qsTr("Open")
                            Layout.preferredHeight: 28

                            background: Rectangle {
                                color: openSeriesBtn.down ? "#0077A8" : (openSeriesBtn.hovered ? "#00B4F0" : "#1A4A68")
                                border.color: "#000000"
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
                                font.bold: true
                            }
                            onClicked: {
                                let clean = launcherDialog.normalizeLocalPath(modelData)
                                if (typeof seriesContainer !== "undefined") {
                                    if (seriesContainer.loadFromDirectory(clean)) {
                                        launcherDialog.close()
                                    }
                                }
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Open this series workspace")
                        }

                        Button {
                            id: removeSeriesBtn
                            objectName: "removeSeriesBtn_" + index
                            text: "✕"
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 28
                            background: Rectangle {
                                color: removeSeriesBtn.down ? "#771111" : (removeSeriesBtn.hovered ? "#AA2222" : "transparent")
                                border.color: removeSeriesBtn.hovered ? "#FF5555" : "transparent"
                                border.width: 1
                                radius: 4
                            }
                            contentItem: Text {
                                text: parent.text
                                color: removeSeriesBtn.hovered ? "#FFFFFF" : "#888888"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: {
                                launcherDialog.pendingDeleteSeriesPath = launcherDialog.normalizeLocalPath(modelData)
                                launcherDialog.pendingDeleteSeriesName = seriesRowDelegate.seriesDisplayName
                                confirmDeleteSeriesDialog.open()
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Delete series and all episodes from disk")
                        }
                    }

                    MouseArea {
                        id: itemMouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }

                Text {
                    id: emptyStateLabel
                    objectName: "emptyStateLabel"
                    anchors.centerIn: parent
                    visible: seriesListView.count === 0
                    text: qsTr("No known series registered.\nCreate a new series or browse an existing folder below.")
                    color: "#888888"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 13
                }
            }
        }

        // Action Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                id: newSeriesBtn
                objectName: "newSeriesBtn"
                text: qsTr("＋ New Series...")
                Layout.preferredHeight: 30

                background: Rectangle {
                    color: newSeriesBtn.down ? "#0077A8" : (newSeriesBtn.hovered ? "#00B4F0" : "#1A4A68")
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
                    if (typeof newSeriesDialog !== "undefined") {
                        newSeriesDialog.open()
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Create and bootstrap a brand new series directory")
            }

            Button {
                id: browseSeriesBtn
                objectName: "browseSeriesBtn"
                text: qsTr("📁 Browse Other Series Folder...")
                Layout.preferredHeight: 30

                background: Rectangle {
                    color: browseSeriesBtn.down ? "#0077A8" : (browseSeriesBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: seriesFolderDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Locate an existing series directory on your filesystem")
            }

            Item { Layout.fillWidth: true }
        }
    }

    // ==========================================
    // PERMANENT SERIES DELETION CONFIRMATION
    // ==========================================
    Dialog {
        id: confirmDeleteSeriesDialog
        objectName: "confirmDeleteSeriesDialog"
        title: qsTr("Confirm Permanent Series Deletion")
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 460

        background: Rectangle {
            color: "#08121A"
            border.color: "#FF4444"
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            spacing: 10
            anchors.margins: 8

            Text {
                text: qsTr("⚠️ Delete Series \"%1\" Permanently?").arg(launcherDialog.pendingDeleteSeriesName)
                color: "#FF6666"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Text {
                text: qsTr("This will permanently delete the entire series directory, including all episodes, scripts, images, audio, and video assets inside:")
                color: "#CCCCCC"
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: deletePathText.implicitHeight + 12
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                Text {
                    id: deletePathText
                    anchors.fill: parent
                    anchors.margins: 6
                    text: launcherDialog.pendingDeleteSeriesPath
                    color: "#00F0FF"
                    font.family: "Courier New, monospace"
                    font.pixelSize: 11
                    wrapMode: Text.WrapAnywhere
                }
            }

            Text {
                text: qsTr("This action cannot be undone.")
                color: "#FFAA00"
                font.bold: true
                font.pixelSize: 11
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 4
                spacing: 10

                Item { Layout.fillWidth: true }

                Button {
                    id: cancelDeleteSeriesBtn
                    objectName: "cancelDeleteSeriesBtn"
                    text: qsTr("Cancel")
                    implicitHeight: 28
                    implicitWidth: 90
                    onClicked: confirmDeleteSeriesDialog.close()
                }

                Button {
                    id: confirmDeleteSeriesBtn
                    objectName: "confirmDeleteSeriesBtn"
                    text: qsTr("Delete Series")
                    implicitHeight: 28
                    implicitWidth: 120
                    background: Rectangle {
                        color: confirmDeleteSeriesBtn.down ? "#771111" : (confirmDeleteSeriesBtn.hovered ? "#CC2222" : "#991A1A")
                        border.color: "#FF5555"
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
                        let targetPath = launcherDialog.pendingDeleteSeriesPath
                        let targetName = launcherDialog.pendingDeleteSeriesName
                        confirmDeleteSeriesDialog.close()

                        if (typeof projectManager !== "undefined" && projectManager !== null && projectManager.isProjectOpen) {
                            let activeEp = launcherDialog.normalizeLocalPath(projectManager.currentProjectPath)
                            if (activeEp.toLowerCase().startsWith(targetPath.toLowerCase())) {
                                projectManager.closeProject()
                            }
                        }

                        let deleted = false
                        if (typeof seriesContainer !== "undefined" && typeof seriesContainer.deleteSeriesDirectory === "function") {
                            deleted = seriesContainer.deleteSeriesDirectory(targetPath)
                        }
                        if (typeof settingsManager !== "undefined" && typeof settingsManager.removeKnownSeriesPath === "function") {
                            settingsManager.removeKnownSeriesPath(targetPath)
                        }
                        if (typeof seriesContainer !== "undefined" && typeof seriesContainer.refreshAvailableSeries === "function") {
                            seriesContainer.refreshAvailableSeries()
                        }

                        if (typeof statusBannerText !== "undefined" && typeof statusBanner !== "undefined") {
                            statusBannerText.text = deleted
                                ? qsTr("Deleted series: %1").arg(targetName)
                                : qsTr("⚠️ Could not delete series folder: %1").arg(targetName)
                            statusBanner.open()
                        }
                    }
                }
            }
        }
    }

    FolderDialog {
        id: seriesFolderDialog
        objectName: "seriesFolderDialog"
        title: qsTr("Select Series Directory")
        currentFolder: StandardPaths.writableLocation(StandardPaths.HomeLocation)
        onAccepted: {
            let selectedPath = selectedFolder.toString().replace(/^file:\/\/\//, "")
            if (Qt.platform.os === "windows" && selectedPath.startsWith("/")) {
                selectedPath = selectedPath.substring(1)
            }
            if (typeof seriesContainer !== "undefined") {
                if (seriesContainer.loadFromDirectory(selectedPath)) {
                    seriesContainer.refreshAvailableSeries()
                    launcherDialog.close()
                }
            }
        }
    }
}

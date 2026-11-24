// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtCore
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import StoryBuilderStudio

Dialog {
    id: root
    objectName: "newSeriesDialogRoot"
    title: qsTr("Create New Series Container")
    modal: true
    anchors.centerIn: parent
    width: 540
    height: 520
    standardButtons: Dialog.Ok | Dialog.Cancel

    property string targetRootFolder: {
            if (typeof settingsManager !== "undefined" && settingsManager && settingsManager.defaultSavePath.length > 0) {
                return settingsManager.defaultSavePath
            }
            return urlToPath(StandardPaths.writableLocation(StandardPaths.HomeLocation)) + "/StoryBuilderStudioProjects"
        }

    function urlToPath(folderUrl) {
        if (!folderUrl) return "";
        let path = folderUrl.toString().replace(/^file:\/\//i, "");
        if (Qt.platform.os === "windows" && path.startsWith("/")) {
            path = path.substring(1);
        }
        return path;
    }

    FolderDialog {
        id: parentFolderPicker
        title: qsTr("Select Directory for New Series")
        currentFolder: (Qt.platform.os === "windows" ? "file:///" : "file://") + root.targetRootFolder
        onAccepted: {
            root.targetRootFolder = root.urlToPath(selectedFolder)
        }
    }

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 6
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            id: headerTitle
            objectName: "headerTitle"
            text: qsTr("Series Master Container")
            color: "#00F0FF"
            style: Text.Outline
            styleColor: "#000000"
            font.bold: true
            font.pixelSize: 15
        }

        Text {
            text: qsTr("Creates a top-level narrative umbrella with a season manifest and clean episode tracts.")
            color: "#888888"
            font.pixelSize: 12
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#1A4A68" }

        // --- Series Title ---
        Text { text: qsTr("Series Title:"); color: "#CCCCCC"; font.pixelSize: 12 }
        TextField {
            id: seriesTitleField
            objectName: "seriesTitleField"
            Layout.fillWidth: true
            placeholderText: qsTr("e.g. Chronicles of the Monolith")
            color: "#FFFFFF"
            background: Rectangle {
                color: "#132B3A"
                border.color: seriesTitleField.activeFocus ? "#00F0FF" : "#1A4A68"
                border.width: 1
                radius: 4
            }
            ToolTip.visible: hovered
            ToolTip.text: qsTr("High-level series umbrella or season title")
        }

        // --- Target Directory Selection ---
        Text { text: qsTr("Storage Location:"); color: "#CCCCCC"; font.pixelSize: 12 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            TextField {
                id: locationField
                objectName: "locationField"
                Layout.fillWidth: true
                text: root.targetRootFolder
                readOnly: true
                color: "#AAAAAA"
                background: Rectangle {
                    color: "#0B1B26"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }
            Button {
                id: browseBtn
                objectName: "browseBtn"
                text: qsTr("Browse...")
                contentItem: Text {
                    text: browseBtn.text
                    font: browseBtn.font
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: browseBtn.down ? "#0077A8" : (browseBtn.hovered ? "#00B4F0" : "#1A4A68")
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }
                onClicked: parentFolderPicker.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Choose parent folder for series structure")
            }
        }

        // --- Short Logline ---
        Text { text: qsTr("Short Logline / Hook:"); color: "#CCCCCC"; font.pixelSize: 12 }
        TextField {
            id: shortDescField
            objectName: "shortDescField"
            Layout.fillWidth: true
            placeholderText: qsTr("One-line logline for cards and summary badges")
            color: "#FFFFFF"
            background: Rectangle {
                color: "#132B3A"
                border.color: shortDescField.activeFocus ? "#00F0FF" : "#1A4A68"
                border.width: 1
                radius: 4
            }
        }

        // --- Long Synopsis ---
        Text { text: qsTr("Series Synopsis:"); color: "#CCCCCC"; font.pixelSize: 12 }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            TextArea {
                id: longDescField
                objectName: "longDescField"
                placeholderText: qsTr("Detailed background synopsis, premise, or universe outline...")
                color: "#FFFFFF"
                wrapMode: Text.WordWrap
                background: Rectangle {
                    color: "#132B3A"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }
        }

        // --- Optional Episode Creation Checkbox ---
        CheckBox {
            id: createEpisodeCheckbox
            objectName: "createEpisodeCheckbox"
            text: qsTr("Immediately initialize the first episode for this series")
            checked: true

            contentItem: Text {
                text: createEpisodeCheckbox.text
                font.pixelSize: 11
                color: createEpisodeCheckbox.checked ? "#00F0FF" : "#CCCCCC"
                style: createEpisodeCheckbox.checked ? Text.Outline : Text.Normal
                styleColor: "#000000"
                leftPadding: createEpisodeCheckbox.indicator.width + 6
                verticalAlignment: Text.AlignVCenter
            }

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                radius: 3
                color: "#0B1B26"
                border.color: createEpisodeCheckbox.checked ? "#00F0FF" : "#1A4A68"

                Rectangle {
                    width: 8
                    height: 8
                    anchors.centerIn: parent
                    radius: 1
                    color: "#00F0FF"
                    visible: createEpisodeCheckbox.checked
                }
            }
        }
    }

    onAccepted: {
        let title = seriesTitleField.text.trim();
        if (title.length === 0) return;

        let slug = title.toLowerCase().replace(/[^a-z0-9_-]/g, "_");
        let cleanBase = root.urlToPath(root.targetRootFolder);
        let fullSeriesPath = cleanBase + "/" + slug;
        let shouldCreateEpisode = createEpisodeCheckbox.checked;

        if (typeof seriesContainer !== "undefined") {
            if (seriesContainer.createNewContainer(fullSeriesPath, title, shortDescField.text.trim(), longDescField.text.trim())) {
                if (typeof statusBannerText !== "undefined" && typeof statusBanner !== "undefined") {
                    statusBannerText.text = qsTr("Series container created: %1").arg(title);
                    statusBanner.open();
                }

                if (shouldCreateEpisode && typeof newProjectDialog !== "undefined") {
                    if (typeof newProjectDialog.resetAndOpen === "function") {
                        newProjectDialog.resetAndOpen(fullSeriesPath);
                    } else {
                        newProjectDialog.open();
                    }
                }
            } else {
                if (typeof statusBannerText !== "undefined" && typeof statusBanner !== "undefined") {
                    statusBannerText.text = qsTr("⚠️ Failed to create series container.");
                    statusBanner.open();
                }
            }
        }

        seriesTitleField.text = "";
        shortDescField.text = "";
        longDescField.text = "";
        createEpisodeCheckbox.checked = true;
    }
}

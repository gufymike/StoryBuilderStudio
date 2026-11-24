// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Dialog {
    id: newProjectDialog
    objectName: "newProjectDialogRoot"
    title: qsTr("Create Story Builder Studio Project")
    modal: true
    standardButtons: Dialog.Save | Dialog.Cancel
    width: 550
    height: 650
    anchors.centerIn: parent

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 6
    }

    property string targetSeriesDirectory: ""
    property string targetRootFolder: (typeof seriesContainer !== "undefined" && seriesContainer.rootDirectory && seriesContainer.rootDirectory.length > 0)
                                            ? seriesContainer.episodesDirectory
                                            : (StandardPaths.writableLocation(StandardPaths.HomeLocation) + "/StoryBuilderStudioProjects")

    contentItem: ColumnLayout {
        spacing: 12
        anchors.fill: parent
        anchors.margins: 16

        // Title / Header with black outline style
        Text {
            id: headerTitle
            objectName: "headerTitle"
            text: qsTr("Initialize New Project Structure")
            color: "#00F0FF"
            style: Text.Outline
            styleColor: "#000000"
            font.bold: true
            font.pixelSize: 16
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: newProjectDialog.width - 48
                spacing: 12

                // Project Name
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Project Name *"); color: "#CCCCCC"; font.pixelSize: 12 }
                    TextField {
                        id: projNameField
                        objectName: "projNameField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("e.g. Intergalactic Chronicles")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#132B3A"
                            radius: 4
                            border.color: projNameField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Unique title or episode descriptor")
                    }
                }

                // Primary Author & Smart Email Auto-fill parser
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Primary Author (Supports 'Name <email>')"); color: "#CCCCCC"; font.pixelSize: 12 }
                    TextField {
                        id: authorField
                        objectName: "authorField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("e.g. Mike Ramirez <me@home.com>")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#132B3A"
                            radius: 4
                            border.color: authorField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                        }

                        onTextChanged: {
                            let match = text.match(/<([^>]+)>/);
                            if (match && match[1]) {
                                emailField.text = match[1];
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Lead writer or director handle")
                    }
                }

                // Additional Authors / Contributors (One per line)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Additional Authors (Optional, one per line)"); color: "#CCCCCC"; font.pixelSize: 12 }
                    ScrollView {
                        Layout.fillWidth: true
                        implicitHeight: 70
                        clip: true
                        background: Rectangle { color: "#132B3A"; radius: 4; border.color: "#1A4A68"; border.width: 1 }

                        TextArea {
                            id: authorsListArea
                            objectName: "authorsListArea"
                            placeholderText: qsTr("Contributor One <email@domain.com>\nContributor Two")
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            wrapMode: TextArea.Wrap
                        }
                    }
                }

                // Main Account Email
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Account / Contact Email"); color: "#CCCCCC"; font.pixelSize: 12 }
                    TextField {
                        id: emailField
                        objectName: "emailField"
                        Layout.fillWidth: true
                        placeholderText: qsTr("me@work.com")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#132B3A"
                            radius: 4
                            border.color: emailField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                        }
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Contact email for project ownership")
                    }
                }

                // Short Description / Tagline (Max 50 chars)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Short Description / Tagline"); color: "#CCCCCC"; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: shortDescField.length + "/50"
                            color: shortDescField.length > 50 ? "#FF5555" : "#00B4F0"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 11
                        }
                    }
                    TextField {
                        id: shortDescField
                        objectName: "shortDescField"
                        Layout.fillWidth: true
                        maximumLength: 50
                        placeholderText: qsTr("Quick tagline for list displays...")
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#132B3A"
                            radius: 4
                            border.color: shortDescField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                        }
                    }
                }

                // Full Description
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("Full Project Description"); color: "#CCCCCC"; font.pixelSize: 12 }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: fullDescArea.length + "/10000"
                            color: "#00B4F0"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 11
                        }
                    }
                    ScrollView {
                        Layout.fillWidth: true
                        implicitHeight: 110
                        clip: true
                        background: Rectangle { color: "#132B3A"; radius: 4; border.color: "#1A4A68"; border.width: 1 }

                        TextArea {
                            id: fullDescArea
                            objectName: "fullDescArea"
                            placeholderText: qsTr("Comprehensive summary of the project universe, goals, and core outline...")
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            wrapMode: TextArea.Wrap
                        }
                    }
                }

                // Version Field (Editable, defaults to 1.0)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text { text: qsTr("Version"); color: "#CCCCCC"; font.pixelSize: 12 }
                    TextField {
                        id: versionField
                        objectName: "versionField"
                        Layout.fillWidth: true
                        text: "1.0"
                        color: "#FFFFFF"
                        background: Rectangle {
                            color: "#132B3A"
                            radius: 4
                            border.color: versionField.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: progressPopup
        objectName: "progressPopup"
        modal: true
        focus: true
        anchors.centerIn: parent
        width: 280
        height: 100
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: "#08121A"
            border.color: "#00F0FF"
            border.width: 1
            radius: 6
        }

        contentItem: ColumnLayout {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: qsTr("Creating Story Builder Studio Project...")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            ProgressBar {
                id: progressBar
                Layout.alignment: Qt.AlignHCenter
                indeterminate: true
            }
        }
    }

    function resetAndOpen(seriesDirPath) {
        if (seriesDirPath && seriesDirPath.length > 0) {
            targetSeriesDirectory = seriesDirPath
        } else if (typeof seriesContainer !== "undefined" && seriesContainer.rootDirectory && seriesContainer.rootDirectory.length > 0) {
            targetSeriesDirectory = seriesContainer.rootDirectory
        } else {
            targetSeriesDirectory = ""
        }

        projNameField.text = "ep01"
        authorField.text = (typeof seriesContainer !== "undefined" && seriesContainer.creator.length > 0)
            ? seriesContainer.creator : ""
        emailField.text = (typeof seriesContainer !== "undefined" && seriesContainer.email.length > 0)
            ? seriesContainer.email : ""
        shortDescField.text = ""
        fullDescArea.text = ""
        versionField.text = "1.0"
        open()
    }

    onAccepted: {
        let name = projNameField.text.trim();
        if (name.length === 0) return;

        progressPopup.open();

        let episodeSlug = name.toLowerCase().replace(/[^a-z0-9_-]/g, "_");

        let seriesRoot = targetSeriesDirectory.length > 0
            ? targetSeriesDirectory
            : ((typeof seriesContainer !== "undefined" && seriesContainer.rootDirectory) ? seriesContainer.rootDirectory : "");

        seriesRoot = seriesRoot.toString().replace(/^file:\/\//i, "");
        if (Qt.platform.os === "windows" && seriesRoot.startsWith("/")) {
            seriesRoot = seriesRoot.substring(1);
        }

        let episodesBaseDir = seriesRoot.length > 0
            ? (seriesRoot + "/episodes")
            : (StandardPaths.writableLocation(StandardPaths.HomeLocation) + "/StoryBuilderStudioProjects").replace(/^file:\/\//i, "");
        if (Qt.platform.os === "windows" && episodesBaseDir.startsWith("/")) {
            episodesBaseDir = episodesBaseDir.substring(1);
        }

        let fullEpisodePath = episodesBaseDir + "/" + episodeSlug;

        let rawAuthorsText = authorsListArea.text;
        let additionalAuthors = rawAuthorsText.split("\n")
            .map(function(s) { return s.trim(); })
            .filter(function(s) { return s.length > 0; });

        if (typeof projectManager !== "undefined") {
            let success = projectManager.createProject(
                fullEpisodePath,
                name,
                fullDescArea.text.trim(),
                shortDescField.text.trim(),
                authorField.text.trim(),
                additionalAuthors,
                emailField.text.trim(),
                versionField.text.trim()
            );

            if (success) {
                if (typeof seriesContainer !== "undefined" && seriesContainer.rootDirectory && seriesContainer.rootDirectory.length > 0) {
                    seriesContainer.registerSubProject(episodeSlug, name);
                }
                progressTimer.restart();
            } else {
                progressPopup.close();
                if (typeof statusBannerText !== "undefined" && typeof statusBanner !== "undefined") {
                    statusBannerText.text = qsTr("⚠️ Failed to create episode at: %1").arg(fullEpisodePath);
                    statusBanner.open();
                }
            }
        } else {
            progressTimer.restart();
        }
    }

    Timer {
        id: progressTimer
        interval: 1000
        repeat: false
        onTriggered: {
            progressPopup.close();
            newProjectDialog.close();
        }
    }
}

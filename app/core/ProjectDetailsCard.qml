// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: root
    objectName: "projectDetailsCardRoot"
    Layout.fillWidth: true
    implicitHeight: contentCol.implicitHeight + 32
    color: "#08121A" // Dark Well
    border.color: "#1A4A68" // Accent Border
    border.width: 1
    radius: 6

    property bool isProjectOpen: (typeof projectManager !== "undefined" && projectManager.isProjectOpen)
                                     ? projectManager.isProjectOpen : false
    property string projectName: (typeof projectManager !== "undefined" && projectManager.projectName)
                                     ? projectManager.projectName : ""
    property string projectVersion: (typeof projectManager !== "undefined" && projectManager.projectVersion)
                                        ? projectManager.projectVersion : ""
    property string projectDescription: (typeof projectManager !== "undefined" && projectManager.projectDescription)
                                            ? projectManager.projectDescription : ""
    property string author: (typeof projectManager !== "undefined" && projectManager.author)
                                ? projectManager.author : ""
    property string email: (typeof projectManager !== "undefined" && projectManager.email)
                               ? projectManager.email : ""
    property string dateCreated: (typeof projectManager !== "undefined" && projectManager.dateCreated)
                                     ? projectManager.dateCreated : ""
    property string dateLastEdited: (typeof projectManager !== "undefined" && projectManager.dateLastEdited)
                                        ? projectManager.dateLastEdited : ""

    ColumnLayout {
        id: contentCol
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // Card Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            HoverHandler {
                id: epTitleHoverHandler
                cursorShape: Qt.PointingHandCursor
            }

            ToolTip.visible: epTitleHoverHandler.hovered
            ToolTip.text: root.isProjectOpen
                          ? qsTr("Details for Episode: %1").arg(root.projectName)
                          : qsTr("No Episode Currently Open")

            Text {
                id: headerTitleText
                objectName: "headerTitleText"
                text: root.isProjectOpen
                      ? qsTr("Active Episode Configuration: %1").arg(root.projectName)
                      : qsTr("No Episode Currently Open")
                color: root.isProjectOpen ? "#00F0FF" : "#FF5555"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 15
            }

            Item { Layout.fillWidth: true }

            Text {
                id: versionText
                objectName: "versionText"
                text: root.isProjectOpen ? qsTr("Version: %1").arg(root.projectVersion) : ""
                color: "#00B4F0"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 11
                visible: text.length > 0
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Editable Metadata Form
        ColumnLayout {
            id: formContainer
            objectName: "formContainer"
            Layout.fillWidth: true
            spacing: 12
            enabled: root.isProjectOpen
            opacity: enabled ? 1.0 : 0.4

            EditableFieldPair {
                id: epNameField
                objectName: "epNameField"
                labelText: qsTr("Episode Name:")
                fieldValue: root.projectName
                enabled: root.isProjectOpen
                onValueSubmitted: (newVal) => {
                    root.projectName = newVal
                    if (typeof projectManager !== "undefined") {
                        projectManager.projectName = newVal
                    }
                }
                toolTipText: qsTr("The episode name.")
            }

            EditableFieldPair {
                id: descField
                objectName: "descField"
                labelText: qsTr("Description:")
                fieldValue: root.projectDescription
                enabled: root.isProjectOpen
                onValueSubmitted: (newVal) => {
                    root.projectDescription = newVal
                    if (typeof projectManager !== "undefined") {
                        projectManager.projectDescription = newVal
                    }
                }
                toolTipText: qsTr("Description of the episode.")
            }

            EditableFieldPair {
                id: writerField
                objectName: "writerField"
                labelText: qsTr("Primary Writer:")
                fieldValue: root.author
                enabled: root.isProjectOpen
                onValueSubmitted: (newVal) => {
                    root.author = newVal
                    if (typeof projectManager !== "undefined") {
                        projectManager.author = newVal
                    }
                }
                toolTipText: qsTr("The primary writer behind this episode.")
            }

            EditableFieldPair {
                id: contactField
                objectName: "contactField"
                labelText: qsTr("Contact Email:")
                fieldValue: root.email
                enabled: root.isProjectOpen
                onValueSubmitted: (newVal) => {
                    root.email = newVal
                    if (typeof projectManager !== "undefined") {
                        projectManager.email = newVal
                    }
                }
                toolTipText: qsTr("The contact email for this episode.")
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text { text: qsTr("Date Created:"); color: "#888888"; font.pixelSize: 12; Layout.preferredWidth: 120 }
                Text {
                    id: createdDateText
                    objectName: "createdDateText"
                    text: root.dateCreated
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text { text: qsTr("Last Edited:"); color: "#888888"; font.pixelSize: 12; Layout.preferredWidth: 120 }
                Text {
                    id: editedDateText
                    objectName: "editedDateText"
                    text: root.dateLastEdited
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }
            }
        }
    }
}

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
** File: OpenProjectDialog.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Popup {
    id: openProjectPopup
    objectName: "openProjectPopupRoot"
    anchors.centerIn: parent
    width: 440
    height: 380
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var projectList: (typeof projectManager !== "undefined" && projectManager.availableProjects)
                                  ? projectManager.availableProjects : []

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        // Header Title with black outline
        Text {
            id: headerTitle
            objectName: "headerTitle"
            text: qsTr("Open Existing Project")
            color: "#00F0FF"
            style: Text.Outline
            styleColor: "#000000"
            font.bold: true
            font.pixelSize: 16
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Project List View or Empty State Message
        StackLayout {
            id: contentStack
            objectName: "contentStack"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: openProjectPopup.projectList.length > 0 ? 0 : 1

            // State 0: Projects Available
            ScrollView {
                id: listScrollView
                clip: true

                ListView {
                    id: projectListView
                    objectName: "projectListView"
                    width: listScrollView.width
                    model: openProjectPopup.projectList

                    delegate: ItemDelegate {
                        id: projectDelegate
                        width: projectListView.width
                        height: 40

                        contentItem: Text {
                            text: modelData
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: projectDelegate.down ? "#0077A8" : (projectDelegate.hovered ? "#132B3A" : "transparent")
                            border.color: projectDelegate.hovered ? "#00F0FF" : "transparent"
                            border.width: 1
                            radius: 4
                        }

                        onClicked: {
                            if (typeof projectManager !== "undefined") {
                                if (projectManager.openProject(modelData)) {
                                    openProjectPopup.close()
                                }
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("Open project: %1").arg(modelData)
                    }
                }
            }

            // State 1: No Projects Found
            ColumnLayout {
                id: emptyStateLayout
                objectName: "emptyStateLayout"
                spacing: 12

                Item { Layout.fillHeight: true }

                Text {
                    id: emptyStateLabel
                    objectName: "emptyStateLabel"
                    text: qsTr("No existing projects found in your workspace repository.")
                    color: "#AAAAAA"
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Button {
                    id: createNewProjectBtn
                    objectName: "createNewProjectBtn"
                    text: qsTr("Create a New Project Now")
                    Layout.alignment: Qt.AlignHCenter

                    contentItem: Text {
                        text: createNewProjectBtn.text
                        font.bold: true
                        font.pixelSize: 12
                        color: "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        color: createNewProjectBtn.down ? "#0077A8" : (createNewProjectBtn.hovered ? "#00B4F0" : "#1A4A68")
                        border.color: "#000000"
                        border.width: 1
                        radius: 4
                    }

                    onClicked: {
                        openProjectPopup.close()
                        if (typeof newProjectDialog !== "undefined") {
                            newProjectDialog.open()
                        }
                    }

                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Launch the project creation wizard")
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Action Bar
        RowLayout {
            Layout.fillWidth: true

            Item { Layout.fillWidth: true }

            Button {
                id: cancelBtn
                objectName: "cancelBtn"
                text: qsTr("Cancel")

                contentItem: Text {
                    text: cancelBtn.text
                    font.pixelSize: 12
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: cancelBtn.down ? "#0077A8" : (cancelBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
                }

                onClicked: openProjectPopup.close()

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Dismiss without opening")
            }
        }
    }
}

// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: root
    objectName: "episodeListCardRoot"
    Layout.fillWidth: true
    implicitHeight: mainLayout.implicitHeight + 32
    color: "#08121A" // Dark Well
    border.color: "#1A4A68" // Accent Border
    border.width: 1
    radius: 6

    property string seriesName: (root.isSeriesLoaded && seriesContainer.name) ? seriesContainer.name : ""
    property string projectName: (typeof projectManager !== "undefined" && projectManager.projectName) ? projectManager.projectName : ""
    property int subProjectCount: (typeof seriesContainer !== "undefined" && seriesContainer.subProjectCount)
                                      ? seriesContainer.subProjectCount : 0
    property string rootDirectory: (typeof seriesContainer !== "undefined" && seriesContainer.rootDirectory)
                                       ? seriesContainer.rootDirectory : ""
    property bool isProjectOpen: (typeof projectManager !== "undefined" && projectManager.isProjectOpen)
                                     ? projectManager.isProjectOpen : false
    property string currentProjectPath: (typeof projectManager !== "undefined" && projectManager.currentProjectPath)
                                            ? projectManager.currentProjectPath : ""

    signal newEpisodeRequested()
    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Card Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                id: cardHeaderTitle
                objectName: "cardHeaderTitle"
                text: qsTr("Series Episodes / Chapters")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 15
            }

            HoverHandler {
                id: epListTitleHoverHandler
                cursorShape: Qt.PointingHandCursor
            }

            ToolTip.visible: epListTitleHoverHandler.hovered && !epNewEpisodeBtn.hovered
            ToolTip.text: qsTr("The current series episodes list.")

            Rectangle {
                id: countBadge
                objectName: "countBadge"
                color: "#132B3A" // Context Badge Background
                border.color: "#1A4A68"
                radius: 4
                implicitWidth: countText.implicitWidth + 12
                implicitHeight: countText.implicitHeight + 4

                Text {
                    id: countText
                    objectName: "countText"
                    anchors.centerIn: parent
                    text: qsTr("%1 Total").arg(root.subProjectCount)
                    color: "#00F0FF" // Active Highlight
                    style: Text.Outline
                    styleColor: "#000000"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: epNewEpisodeBtn
                objectName: "epNewEpisodeBtn"
                text: qsTr("＋ New Episode")
                font.pixelSize: 11
                Layout.preferredHeight: 26
                enabled: root.rootDirectory.length > 0

                background: Rectangle {
                    color: parent.enabled
                               ? (parent.down ? "#0077A8" : (parent.hovered ? "#00B4F0" : "#1A4A68"))
                               : "#0B1B26"
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }

                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#FFFFFF" : "#666666"
                    style: parent.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (typeof newProjectDialog !== "undefined") {
                        episodeListCardRoot.newEpisodeRequested()
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Add a new episode.")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Episode List
        ListView {
            id: episodeListView
            objectName: "episodeListView"
            Layout.fillWidth: true
            implicitHeight: Math.min(contentHeight, 280)
            clip: true
            spacing: 4
            model: root.subProjectCount

            delegate: Rectangle {
                id: rowDelegate
                objectName: "rowDelegate_" + index
                width: episodeListView.width
                height: 44
                radius: 4

                readonly property string epPath: (typeof seriesContainer !== "undefined")
                    ? seriesContainer.getSubProjectAbsolutePath(index) : ""
                readonly property string epTitle: (typeof seriesContainer !== "undefined" && seriesContainer.getSubProjectTitle(index).length > 0)
                    ? seriesContainer.getSubProjectTitle(index) : epPath.split("/").pop()
                readonly property bool isActive: (root.isProjectOpen && root.currentProjectPath === epPath)

                color: isActive ? "#132B3A" : (itemMouseArea.containsMouse ? "#0B1B26" : "#08121A")
                border.color: isActive ? "#00F0FF" : (itemMouseArea.containsMouse ? "#00B4F0" : "#1A4A68")
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: rowDelegate.isActive ? "#00F0FF" : "#444444"
                    }

                    Text {
                        text: qsTr("%1.").arg(index + 1)
                        color: "#888888"
                        font.pixelSize: 12
                    }

                    Text {
                        text: rowDelegate.epTitle
                        color: rowDelegate.isActive ? "#00F0FF" : "#FFFFFF"
                        style: rowDelegate.isActive ? Text.Outline : Text.Normal
                        styleColor: "#000000"
                        font.bold: rowDelegate.isActive
                        font.pixelSize: 13
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: rowDelegate.isActive ? qsTr("CURRENTLY LOADED") : ""
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 10
                        font.bold: true
                        visible: rowDelegate.isActive
                    }

                    Button {
                        id: loadBtn
                        objectName: "loadBtn_" + index
                        text: rowDelegate.isActive ? qsTr("Active") : qsTr("Load")
                        enabled: !rowDelegate.isActive
                        Layout.preferredHeight: 26

                        background: Rectangle {
                            color: rowDelegate.isActive
                                       ? "#132B3A"
                                       : (parent.down ? "#0077A8" : (parent.hovered ? "#00B4F0" : "#1A4A68"))
                            border.color: "#000000"
                            border.width: 1
                            radius: 4
                        }

                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onClicked: {
                            if (rowDelegate.epPath.length > 0 && typeof projectManager !== "undefined") {
                                projectManager.openProject(rowDelegate.epPath)
                            }
                        }

                        ToolTip.visible: hovered
                        ToolTip.text: rowDelegate.isActive
                            ? qsTr("Active episode: %1").arg(rowDelegate.epTitle)
                            : qsTr("Load episode: %1").arg(rowDelegate.epTitle)
                    }
                }

                MouseArea {
                    id: itemMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: rowDelegate.isActive ? Qt.ArrowCursor : Qt.PointingHandCursor

                    onClicked: {
                        if (!rowDelegate.isActive && rowDelegate.epPath.length > 0 && typeof projectManager !== "undefined") {
                            projectManager.openProject(rowDelegate.epPath)
                        }
                    }

                    ToolTip.visible: itemMouseArea.containsMouse && !loadBtn.hovered
                    ToolTip.text: qsTr("Click to load: %1").arg(rowDelegate.epTitle)
                }
            }

            Text {
                id: emptyListPlaceholder
                objectName: "emptyListPlaceholder"
                anchors.centerIn: parent
                visible: episodeListView.count === 0
                text: qsTr("No episodes created for this series yet. Click '＋ New Episode' above to begin.")
                color: "#888888"
                font.pixelSize: 12
            }
        }
    }
}

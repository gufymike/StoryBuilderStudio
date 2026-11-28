// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: root
    objectName: "seriesHeaderCardRoot"
    Layout.fillWidth: true
    implicitHeight: mainLayout.implicitHeight + 32
    color: "#08121A" // Dark Well
    border.color: "#1A4A68" // Accent Border
    border.width: 1
    radius: 6


    readonly property bool hasSeriesContainer: typeof seriesContainer !== "undefined" && seriesContainer !== null

    property bool isSeriesLoaded: root.hasSeriesContainer && seriesContainer.rootDirectory.length > 0

    property string seriesName: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.name) ? seriesContainer.name : ""
    property int subProjectCount: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.subProjectCount) ? seriesContainer.subProjectCount : 0
    property string creationDateStr: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.creationDate) ? seriesContainer.creationDate.toLocaleDateString() : "-"
    property string lastModifiedStr: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.lastModified) ? seriesContainer.lastModified.toLocaleDateString() : "-"
    property string rootDirectory: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.rootDirectory) ? seriesContainer.rootDirectory : ""
    property string shortDescription: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.shortDescription) ? seriesContainer.shortDescription : ""
    property string longDescription: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.longDescription) ? seriesContainer.longDescription : ""
    property string creator: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.creator) ? seriesContainer.creator : ""
    property var coCreators: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.coCreators) ? seriesContainer.coCreators : []
    property string email: (root.isSeriesLoaded && root.hasSeriesContainer && seriesContainer.email) ? seriesContainer.email : ""
    property bool collapsed: true

    signal newSeriesRequested()

    ColumnLayout {
        id: mainLayout
        anchors.fill: parent
        anchors.margins: 16
        spacing: 14

        // ==========================================
        // TOP ROW: Title, Badges, Dates & Actions
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            HoverHandler {
                id: seriesTitleHoverHandler
                cursorShape: Qt.PointingHandCursor
            }

            ToolTip.visible: seriesTitleHoverHandler.hovered && !newEpisodeBtn.hovered
            ToolTip.text: root.isSeriesLoaded
                  ? (root.seriesName.length > 0 ? root.seriesName : qsTr("Untitled Series / Book"))
                  : qsTr("No Series Currently Loaded")

            // Series Name / Status
            Text {
                id: headerTitleText
                objectName: "headerTitleText"
                text: root.isSeriesLoaded
                      ? (root.seriesName.length > 0 ? root.seriesName : qsTr("Untitled Series / Book"))
                      : qsTr("No Series Currently Loaded")
                color: root.isSeriesLoaded ? "#00F0FF" : "#FF5555"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 16
                elide: Text.ElideRight
            }

            // Episode / Sub-Project Count Badge
            Rectangle {
                id: episodeBadge
                objectName: "episodeBadge"
                visible: root.isSeriesLoaded
                color: "#132B3A" // Stepper Active Tile / Context Badge
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
                implicitWidth: badgeText.implicitWidth + 12
                implicitHeight: badgeText.implicitHeight + 6

                Text {
                    id: badgeText
                    objectName: "badgeText"
                    anchors.centerIn: parent
                    text: qsTr("%1 Episodes").arg(root.subProjectCount)
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Item { Layout.fillWidth: true }

            // Timestamp tracking
            ColumnLayout {
                id: timestampCol
                objectName: "timestampCol"
                spacing: 2
                Layout.alignment: Qt.AlignRight
                visible: root.isSeriesLoaded

                Text {
                    id: createdDateText
                    objectName: "createdDateText"
                    text: qsTr("Created: %1").arg(root.creationDateStr)
                    color: "#888888"
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignRight
                }
                Text {
                    id: updatedDateText
                    objectName: "updatedDateText"
                    text: qsTr("Updated: %1").arg(root.lastModifiedStr)
                    color: "#888888"
                    font.pixelSize: 10
                    horizontalAlignment: Text.AlignRight
                    Layout.alignment: Qt.AlignRight
                }
            }

            // Quick Action: Add Episode
            Button {
                id: newEpisodeBtn
                objectName: "newEpisodeBtn"
                text: qsTr("＋ New Episode")
                font.pixelSize: 11
                Layout.preferredHeight: 26
                enabled: root.isSeriesLoaded

                contentItem: Text {
                    text: newEpisodeBtn.text
                    color: newEpisodeBtn.enabled ? "#FFFFFF" : "#666666"
                    style: newEpisodeBtn.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: newEpisodeBtn.enabled
                           ? (newEpisodeBtn.down ? "#0077A8" : (newEpisodeBtn.hovered ? "#00B4F0" : "#1A4A68"))
                           : "#0B1B26"
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }

                onClicked: {
                    if (typeof newProjectDialog !== "undefined") {
                        seriesHeaderCardRoot.newSeriesRequested()
                    }
                }

                ToolTip.visible: hovered
                ToolTip.text: qsTr("Add a new episode.")
            }

            // Expand / Collapse Card Details
            Button {
                id: toggleCollapseBtn
                objectName: "toggleCollapseBtn"
                text: root.collapsed ? qsTr("▾ Details") : qsTr("▴ Hide")
                font.pixelSize: 11
                Layout.preferredHeight: 26

                contentItem: Text {
                    text: toggleCollapseBtn.text
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: toggleCollapseBtn.down ? "#0077A8" : (toggleCollapseBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4
                }

                onClicked: root.collapsed = !root.collapsed

                ToolTip.visible: hovered
                ToolTip.text: root.collapsed ? qsTr("Expand series metadata fields") : qsTr("Collapse series metadata fields")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
            visible: !root.collapsed
        }

        // ==========================================
        // EDITABLE SERIES METADATA FORM
        // ==========================================
        ColumnLayout {
            id: formLayout
            objectName: "formLayout"
            Layout.fillWidth: true
            spacing: 12
            visible: !root.collapsed
            enabled: root.isSeriesLoaded
            opacity: enabled ? 1.0 : 0.4

            // 1. Series Title
            EditableFieldPair {
                id: seriesTitlePair
                objectName: "seriesTitlePair"
                labelText: qsTr("Series Title:")
                fieldValue: root.seriesName
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    root.seriesName = newVal
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.name = newVal
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("The series title.")
            }

            // 2. Short Description / Logline
            EditableFieldPair {
                id: shortDescPair
                objectName: "shortDescPair"
                labelText: qsTr("Short Description:")
                fieldValue: root.shortDescription
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    root.shortDescription = newVal
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.shortDescription = newVal
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("The series tagline or a short description.")
            }

            // 3. Long Overview / Series Bible Synopsis
            EditableFieldPair {
                id: longDescPair
                objectName: "longDescPair"
                labelText: qsTr("Overview / Bible:")
                fieldValue: root.longDescription
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    root.longDescription = newVal
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.longDescription = newVal
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("The long description of the story. This is for reference and more of an outline.")
            }

            // 4. Primary Creator
            EditableFieldPair {
                id: creatorPair
                objectName: "creatorPair"
                labelText: qsTr("Creator:")
                fieldValue: root.creator
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    root.creator = newVal
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.creator = newVal
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("The name of the Main Creator behind this story.")
            }

            // 5. Co-Creators (Comma-Separated)
            EditableFieldPair {
                id: coCreatorsPair
                objectName: "coCreatorsPair"
                labelText: qsTr("Co-Creators:")
                fieldValue: (root.coCreators && root.coCreators.length > 0) ? root.coCreators.join(", ") : ""
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    let cleanedList = newVal.split(",")
                                            .map(item => item.trim())
                                            .filter(item => item.length > 0)
                    root.coCreators = cleanedList
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.coCreators = cleanedList
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("The list of Co-Creators and show runners behind the series.")
            }

            // 6. Contact Email
            EditableFieldPair {
                id: emailPair
                objectName: "emailPair"
                labelText: qsTr("Contact Email:")
                fieldValue: root.email
                enabled: root.isSeriesLoaded
                onValueSubmitted: (newVal) => {
                    root.email = newVal
                    if (typeof seriesContainer !== "undefined") {
                        seriesContainer.email = newVal
                        seriesContainer.saveManifest()
                    }
                }
                toolTipText: qsTr("Main contact email for this series.")
            }

            // 7. Directory Location (Read-Only)
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Text {
                    text: qsTr("Manifest Path:")
                    color: "#888888"
                    font.pixelSize: 12
                    Layout.preferredWidth: 120
                }

                Text {
                    id: manifestPathText
                    objectName: "manifestPathText"
                    text: root.rootDirectory
                    color: "#AAAAAA"
                    font.family: "Courier"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    elide: Text.ElideMiddle
                }
            }
        }
    }
}

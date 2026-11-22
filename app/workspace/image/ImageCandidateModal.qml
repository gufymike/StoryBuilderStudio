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
** File: ImageCandidateModal.qml
** Modal popup displaying generated batch image candidates.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Popup {
    id: candidatePopup
    objectName: "imageCandidateModalRoot"
    anchors.centerIn: parent
    width: Math.min(880, parent ? parent.width - 40 : 880)
    height: Math.min(700, parent ? parent.height - 40 : 700)
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape

    background: Rectangle {
        color: "#08121A"
        border.color: "#1A4A68"
        border.width: 1
        radius: 6
    }

    property var candidateList: [] // Array of "data:image/png;base64,..." strings or file paths
    property int targetShotIndex: -1

    signal imageSelected(string base64Data)
    signal imageAssignedToShot(string base64Data, int shotIndex)
    signal allDiscarded()

    function openWithCandidates(candidates, shotIdx) {
        candidateList = candidates || []
        targetShotIndex = (shotIdx !== undefined) ? shotIdx : -1
        candidatePopup.open()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Header
        RowLayout {
            Layout.fillWidth: true

            ColumnLayout {
                spacing: 2
                Text {
                    text: qsTr("Generated Image Candidates")
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 15
                }
                Text {
                    text: qsTr("Select one image to save to your project assets. Unselected candidates will be discarded.")
                    color: "#CCCCCC"
                    font.pixelSize: 11
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: discardAllBtn
                objectName: "discardAllBtn"
                text: qsTr("✕ Discard All")
                implicitHeight: 26
                font.pixelSize: 11
                background: Rectangle {
                    color: discardAllBtn.hovered ? "#331515" : "#0B1B26"
                    border.color: "#FF4444"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FF6666"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    candidatePopup.allDiscarded()
                    candidatePopup.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // Empty state label
        Text {
            id: emptyLabel
            objectName: "emptyCandidatesLabel"
            visible: candidatePopup.candidateList.length === 0
            text: qsTr("No candidates were generated or returned by the engine.")
            color: "#888888"
            font.italic: true
            font.pixelSize: 12
            Layout.alignment: Qt.AlignHCenter
            Layout.fillHeight: true
            verticalAlignment: Text.AlignVCenter
        }

        // 2x2 Candidate Preview Grid
        GridLayout {
            id: candidateGrid
            objectName: "candidateGrid"
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: candidatePopup.candidateList.length > 0
            columns: candidatePopup.candidateList.length > 2 ? 2 : Math.max(1, candidatePopup.candidateList.length)
            rowSpacing: 8
            columnSpacing: 8

            Repeater {
                id: candidateRepeater
                objectName: "candidateRepeater"
                model: candidatePopup.candidateList

                Rectangle {
                    id: candidateCard
                    objectName: "candidateCard_" + index
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#0B1B26"
                    border.color: cardMouse.containsMouse ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                    clip: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Image {
                            id: candidateImg
                            objectName: "candidateImg_" + index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            source: modelData
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            cache: false
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Button {
                                id: saveCandidateBtn
                                objectName: "saveCandidateBtn_" + index
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                text: qsTr("✔ Save Image")
                                font.pixelSize: 11
                                background: Rectangle {
                                    color: saveCandidateBtn.down ? "#0077A8" : (saveCandidateBtn.hovered ? "#00B4F0" : "#08121A")
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
                                    candidatePopup.imageSelected(modelData)
                                    candidatePopup.close()
                                }
                            }

                            Button {
                                id: toStoryboardBtn
                                objectName: "toStoryboardBtn_" + index
                                visible: candidatePopup.targetShotIndex >= 0 || (typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.shotCount > 0)
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 28
                                text: qsTr("✔ To Shot")
                                font.pixelSize: 11
                                background: Rectangle {
                                    color: toStoryboardBtn.down ? "#0077A8" : (toStoryboardBtn.hovered ? "#00B4F0" : "#08121A")
                                    border.color: "#00F0FF"
                                    border.width: 1
                                    radius: 3
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "#00F0FF"
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    let shotIdx = candidatePopup.targetShotIndex >= 0 ? candidatePopup.targetShotIndex : 0
                                    candidatePopup.imageAssignedToShot(modelData, shotIdx)
                                    candidatePopup.close()
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.NoButton
                    }
                }
            }
        }

        // Bottom status bar
        RowLayout {
            Layout.fillWidth: true
            Text {
                id: candidateCountText
                objectName: "candidateCountText"
                text: qsTr("Candidates loaded: %1").arg(candidatePopup.candidateList.length)
                color: "#888888"
                font.pixelSize: 10
            }
        }
    }
}

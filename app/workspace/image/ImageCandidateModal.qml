// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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
    property var selectedIndices: [] // Single best candidate by default; Ctrl+Click adds more
    property int targetShotIndex: -1

    signal imageSelected(string base64Data)
    signal imagesSelected(var base64List)
    signal imageAssignedToShot(string base64Data, int shotIndex)
    signal allDiscarded()

    function isCandidateSelected(idx) {
        return selectedIndices.indexOf(idx) >= 0
    }

    function selectCandidate(idx, modifiers) {
        if (idx < 0 || idx >= candidateList.length) return

        if (modifiers & Qt.ControlModifier) {
            let next = selectedIndices.slice()
            const pos = next.indexOf(idx)
            if (pos >= 0) {
                if (next.length > 1) {
                    next.splice(pos, 1)
                }
            } else {
                next.push(idx)
                next.sort(function(a, b) { return a - b })
            }
            selectedIndices = next
        } else {
            selectedIndices = [idx]
        }
    }

    function commitSelectedCandidates() {
        if (candidateList.length === 0) return

        let indices = selectedIndices.length > 0 ? selectedIndices.slice() : [0]
        indices.sort(function(a, b) { return a - b })

        let chosenPayloads = []
        for (let i = 0; i < indices.length; ++i) {
            const idx = indices[i]
            if (idx >= 0 && idx < candidateList.length) {
                const payload = candidateList[idx]
                chosenPayloads.push(payload)
                candidatePopup.imageSelected(payload)
            }
        }

        if (chosenPayloads.length > 0) {
            candidatePopup.imagesSelected(chosenPayloads)
        }
        candidatePopup.close()
    }

    function openWithCandidates(candidates, shotIdx) {
        candidateList = candidates || []
        selectedIndices = candidateList.length > 0 ? [0] : []
        targetShotIndex = (shotIdx !== undefined) ? shotIdx : -1
        candidatePopup.open()
    }

    Keys.onReturnPressed: commitSelectedCandidates()
    Keys.onEnterPressed: commitSelectedCandidates()

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
                    text: qsTr("Click to pick the best candidate (Ctrl+Click to select multiple). Unselected candidates are discarded.")
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
                    readonly property bool isSelected: candidatePopup.isCandidateSelected(index)

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: isSelected ? "#0F2432" : "#0B1B26"
                    border.color: isSelected ? "#00F0FF" : (cardMouse.containsMouse ? "#00B4F0" : "#1A4A68")
                    border.width: isSelected ? 2 : 1
                    radius: 4
                    clip: true

                    MouseArea {
                        id: cardMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        acceptedButtons: Qt.LeftButton
                        onClicked: (mouse) => {
                            candidatePopup.selectCandidate(index, mouse.modifiers)
                        }
                        onDoubleClicked: {
                            candidatePopup.selectedIndices = [index]
                            candidatePopup.commitSelectedCandidates()
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Image {
                                id: candidateImg
                                objectName: "candidateImg_" + index
                                anchors.fill: parent
                                source: modelData
                                fillMode: Image.PreserveAspectFit
                                asynchronous: true
                                cache: false
                            }

                            Rectangle {
                                visible: candidateCard.isSelected
                                anchors.top: parent.top
                                anchors.left: parent.left
                                anchors.margins: 6
                                height: 20
                                width: badgeLabel.implicitWidth + 14
                                radius: 3
                                color: "#08121A"
                                border.color: "#00F0FF"
                                border.width: 1

                                Text {
                                    id: badgeLabel
                                    anchors.centerIn: parent
                                    text: candidatePopup.selectedIndices.length > 1
                                          ? qsTr("✓ Selected")
                                          : qsTr("★ Best Pick")
                                    color: "#00F0FF"
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            z: 1

                            Button {
                                id: saveCandidateBtn
                                objectName: "saveCandidateBtn_" + index
                                Layout.fillWidth: true
                                Layout.preferredHeight: 28
                                text: candidateCard.isSelected
                                      ? qsTr("✔ Save Pick")
                                      : qsTr("Select & Save")
                                font.pixelSize: 11
                                background: Rectangle {
                                    color: saveCandidateBtn.down ? "#0077A8" : (saveCandidateBtn.hovered ? "#00B4F0" : (candidateCard.isSelected ? "#0F2432" : "#08121A"))
                                    border.color: candidateCard.isSelected ? "#00F0FF" : "#1A4A68"
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
                                    candidatePopup.selectedIndices = [index]
                                    candidatePopup.commitSelectedCandidates()
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
                }
            }
        }

        // Bottom status bar & primary save action
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                id: candidateCountText
                objectName: "candidateCountText"
                text: qsTr("Candidates: %1 · Selected: %2 (Ctrl+Click for multiple)")
                      .arg(candidatePopup.candidateList.length)
                      .arg(candidatePopup.selectedIndices.length)
                color: "#888888"
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }

            Button {
                id: saveSelectedBtn
                objectName: "saveSelectedBtn"
                visible: candidatePopup.candidateList.length > 0
                enabled: candidatePopup.selectedIndices.length > 0
                implicitHeight: 28
                leftPadding: 14
                rightPadding: 14
                text: candidatePopup.selectedIndices.length > 1
                      ? qsTr("✔ Save %1 Selected Images").arg(candidatePopup.selectedIndices.length)
                      : qsTr("✔ Save Best Image")
                font.pixelSize: 11
                background: Rectangle {
                    color: saveSelectedBtn.down ? "#0077A8" : (saveSelectedBtn.hovered ? "#00B4F0" : "#0E222F")
                    border.color: "#00F0FF"
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
                onClicked: candidatePopup.commitSelectedCandidates()
            }
        }
    }
}

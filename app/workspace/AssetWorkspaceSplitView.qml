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
** File: AssetWorkspaceSplitView.qml
** Reusable SplitView wrapper for workspace viewers and prompt builders.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Item {
    id: splitRoot
    objectName: "assetWorkspaceSplitViewRoot"

    property string targetAssetType: "script"
    default property alias contentData: leftPaneContainer.data
    property alias promptBuilder: builderInstance
    property bool promptPaneVisible: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 6
        spacing: 6

        // Top Toolbar
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                id: togglePromptPaneBtn
                objectName: "togglePromptPaneBtn"
                text: splitRoot.promptPaneVisible ? qsTr("Hide Prompt Builder ➔") : qsTr("◀ Show Prompt Builder")
                implicitHeight: 26
                font.pixelSize: 11
                background: Rectangle {
                    color: togglePromptPaneBtn.down ? "#0077A8" : (togglePromptPaneBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: splitRoot.promptPaneVisible = !splitRoot.promptPaneVisible
            }


        }

        // Split View
        SplitView {
            id: mainSplitView
            objectName: "mainSplitView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            // Custom branded divider handle
            handle: Rectangle {
                implicitWidth: 4
                color: SplitHandle.pressed ? "#00F0FF" : (SplitHandle.hovered ? "#1A4A68" : "#0A1924")
                border.color: "#1A4A68"
                border.width: 1
            }

            // Primary Viewer / Editor Container (Left)
            Rectangle {
                id: leftPaneContainer
                objectName: "leftPaneContainer"
                SplitView.fillWidth: true
                SplitView.minimumWidth: 320
                color: "#08121A"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
                clip: true
            }

            // Prompt Builder Container (Right)
            Rectangle {
                id: rightPaneContainer
                objectName: "rightPaneContainer"
                SplitView.preferredWidth: 600
                SplitView.minimumWidth: 260
                SplitView.maximumWidth: 700
                visible: splitRoot.promptPaneVisible
                color: "#08121A"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
                clip: true

                PromptBuilder {
                    id: builderInstance
                    objectName: "promptBuilderInstance"
                    anchors.fill: parent
                    anchors.margins: 4
                    targetAssetType: splitRoot.targetAssetType

                    onImageCandidatesReady: (candidateList) => {
                        if (candidateList && candidateList.length > 0) {
                            // ImageCandidateModal lives here in the workspace view hierarchy
                            candidateModal.openWithCandidates(candidateList)
                        }
                    }

                    onAudioGenerated: (filePath) => {
                        if (audioWorkspaceWidget && typeof audioWorkspaceWidget.loadTrack === "function") {
                            audioWorkspaceWidget.loadTrack(filePath)
                        }
                    }

                    onScriptGenerated: (scriptText, category) => {
                        // if (editorPane && typeof editorPane.insertGeneratedContent === "function") {
                        //     editorPane.insertGeneratedContent(scriptText)
                        // }
                    }
                }
            }
        }
    }
}

/****************************************************************************
**
** Copyright (C) 2026 Mike Ramirez
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
** File: VideoWorkspaceWidget.qml
** Video workspace viewer with integrated playback viewport, scrubbing,
** asset model integration, and external tool dispatching.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import StoryBuilderStudio

Rectangle {
    id: videoRoot
    objectName: "videoWorkspaceWidgetRoot"
    color: "#08121A"
    border.color: "#1A4A68"
    border.width: 1
    radius: 4

    property string currentProjectPath: ""

    // Testing / Inspection hooks
    property alias playerSource: videoPlayer.source
    property alias playbackState: videoPlayer.playbackState
    property alias directoryModel: videoDirModel

    VideoDirectoryModel {
        id: videoDirModel
        objectName: "videoDirModel"
    }

    function previewVideoFile(filePath) {
        videoDirModel.refresh()
        if (!filePath || filePath.length === 0) return

        let cleanPath = filePath.toString().replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) {
            cleanPath = cleanPath.substring(1)
        }

        let mediaUrl = "file:///" + cleanPath.replace(/\\/g, "/")
        videoPlayer.source = mediaUrl
        videoPlayer.play()
    }

    function updateProjectPath(path) {
        currentProjectPath = path
        videoDirModel.projectPath = path
    }

    function formatTime(ms) {
        if (!ms || ms <= 0) return "00:00"
        let totalSecs = Math.floor(ms / 1000)
        let mins = Math.floor(totalSecs / 60)
        let secs = totalSecs % 60
        return (mins < 10 ? "0" + mins : mins) + ":" + (secs < 10 ? "0" + secs : secs)
    }

    MediaPlayer {
        id: videoPlayer
        objectName: "videoPlayer"
        videoOutput: videoViewport
        audioOutput: AudioOutput {
            id: videoAudioOut
            objectName: "videoAudioOut"
            volume: 0.8
        }
    }

    // Shared Root Context Menu
    AssetContextMenu {
        id: sharedContextMenu
        objectName: "sharedContextMenu"
        assetType: "video"
    }

    AssetImportAction {
        id: videoImporter
        objectName: "videoImporter"
        targetModel: videoDirModel
        dialogTitle: qsTr("Select Video File(s) to Import")
        nameFilters: ["Video Files (*.mp4 *.mkv *.webm *.mov *.avi)", "All Files (*)"]
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // ==========================================
        // 1. TOP TOOLBAR & CONTROLS
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                id: importBtn
                objectName: "importVideoBtn"
                text: qsTr("📥 Import Video")
                implicitHeight: 26
                font.pixelSize: 11
                enabled: typeof projectManager !== "undefined" && projectManager.isProjectOpen
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Select video clips to import into this project.")
                background: Rectangle {
                    color: importBtn.down ? "#0077A8" : (importBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: importBtn.enabled ? "#FFFFFF" : "#888888"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: videoImporter.open()
            }

            Button {
                id: refreshBtn
                objectName: "refreshVideoBtn"
                text: qsTr("🔄 Refresh Video Assets")
                implicitHeight: 26
                font.pixelSize: 11
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Looks in the video asset folder for new video assets and loads them.")
                background: Rectangle {
                    color: refreshBtn.down ? "#0077A8" : (refreshBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: videoDirModel.refresh()
            }

            Item { Layout.fillWidth: true }

            Text {
                id: playStatusText
                objectName: "playStatusText"
                text: videoPlayer.source.toString().length > 0
                    ? qsTr("Playing: ") + videoPlayer.source.toString().substring(videoPlayer.source.toString().lastIndexOf('/') + 1)
                    : qsTr("No video selected")
                color: videoPlayer.source.toString().length > 0 ? "#00F0FF" : "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 11
                elide: Text.ElideMiddle
                Layout.maximumWidth: 320
            }
        }

        // ==========================================
        // 2. VIDEO VIEWPORT
        // ==========================================
        Rectangle {
            id: viewportCard
            objectName: "viewportCard"
            Layout.fillWidth: true
            Layout.preferredHeight: 300
            color: "#000000"
            border.color: videoPlayer.playbackState === MediaPlayer.PlayingState ? "#00F0FF" : "#1A4A68"
            border.width: 1
            radius: 4
            clip: true

            VideoOutput {
                id: videoViewport
                objectName: "videoViewport"
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }

            Text {
                id: emptyViewportText
                objectName: "emptyViewportText"
                anchors.centerIn: parent
                visible: videoPlayer.source.toString().length === 0
                text: qsTr("Select a video asset below to preview")
                color: "#888888"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
            }
        }

        // ==========================================
        // 3. TIMELINE SCRUB BAR & TRANSPORT CONTROLS
        // ==========================================
        Rectangle {
            id: transportControlsCard
            objectName: "transportControlsCard"
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 8

                Button {
                    id: playPauseBtn
                    objectName: "playPauseBtn"
                    text: videoPlayer.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 26
                    enabled: videoPlayer.source.toString().length > 0
                    ToolTip.visible: hovered
                    ToolTip.text: videoPlayer.playbackState === MediaPlayer.PlayingState ? qsTr("Pause") : qsTr("Play")
                    background: Rectangle {
                        color: playPauseBtn.down ? "#0077A8" : (playPauseBtn.hovered ? "#00B4F0" : "#08121A")
                        border.color: "#1A4A68"
                        border.width: 1
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#00F0FF" : "#555555"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: {
                        if (videoPlayer.playbackState === MediaPlayer.PlayingState) {
                            videoPlayer.pause()
                        } else {
                            videoPlayer.play()
                        }
                    }
                }

                Button {
                    id: stopBtn
                    objectName: "stopBtn"
                    text: "⏹"
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 26
                    enabled: videoPlayer.playbackState === MediaPlayer.PlayingState || videoPlayer.position > 0
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Stop Playback")
                    background: Rectangle {
                        color: stopBtn.down ? "#0077A8" : (stopBtn.hovered ? "#00B4F0" : "#08121A")
                        border.color: "#1A4A68"
                        border.width: 1
                        radius: 3
                    }
                    contentItem: Text {
                        text: parent.text
                        color: parent.enabled ? "#00F0FF" : "#555555"
                        font.bold: true
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                    onClicked: videoPlayer.stop()
                }

                Slider {
                    id: scrubSlider
                    objectName: "scrubSlider"
                    Layout.fillWidth: true
                    from: 0
                    to: videoPlayer.duration > 0 ? videoPlayer.duration : 1
                    value: videoPlayer.position
                    enabled: videoPlayer.duration > 0
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Seek Video Position")
                    onMoved: {
                        videoPlayer.position = value
                    }
                }

                Text {
                    id: timeDisplay
                    objectName: "timeDisplay"
                    text: formatTime(videoPlayer.position) + " / " + formatTime(videoPlayer.duration)
                    color: "#00F0FF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.family: "Courier New, monospace"
                    font.pixelSize: 11
                    font.bold: true
                }

                Text {
                    text: qsTr("Vol")
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.pixelSize: 11
                }

                Slider {
                    id: volSlider
                    objectName: "volSlider"
                    from: 0.0
                    to: 1.0
                    value: 0.8
                    Layout.preferredWidth: 80
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Volume: %1%").arg(Math.round(value * 100))
                    onValueChanged: videoAudioOut.volume = value
                }
            }
        }

        // ==========================================
        // 4. VIDEO ASSET LISTVIEW
        // ==========================================
        ListView {
            id: videoListView
            objectName: "videoListView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: videoDirModel
            spacing: 4

            delegate: Rectangle {
                id: videoItemRect
                objectName: "videoDelegate_" + index
                width: videoListView.width
                height: 42
                color: delegateMouse.containsMouse ? "#0B1B26" : "#0A1924"
                border.color: (videoPlayer.source.toString().indexOf(model.fileName) !== -1) ? "#00F0FF" : "#1A4A68"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 10

                    Button {
                        text: (videoPlayer.source.toString().indexOf(model.fileName) !== -1 && videoPlayer.playbackState === MediaPlayer.PlayingState) ? "⏸" : "▶"
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        background: Rectangle {
                            color: parent.down ? "#0077A8" : (parent.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
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
                            if (videoPlayer.source.toString().indexOf(model.fileName) !== -1) {
                                if (videoPlayer.playbackState === MediaPlayer.PlayingState) {
                                    videoPlayer.pause()
                                } else {
                                    videoPlayer.play()
                                }
                            } else {
                                videoPlayer.source = model.filePath
                                videoPlayer.play()
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: model.fileName || ""
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.bold: true
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        Text {
                            text: qsTr("Size: %1 | Modified: %2").arg(model.fileSize || "0 MB").arg(model.modifiedDate || "")
                            color: "#888888"
                            font.pixelSize: 9
                        }
                    }
                }

                MouseArea {
                    id: delegateMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.RightButton
                    onClicked: {
                        sharedContextMenu.targetFilePath = model.filePath
                        sharedContextMenu.popup()
                    }
                }
            }
        }
    }

    Connections {
        target: (typeof projectManager !== "undefined") ? projectManager : null
        ignoreUnknownSignals: true

        function onAssetDeleted(filePath) {
            videoDirModel.refresh()
            let cleanCurrent = videoPlayer.source.toString().replace(/^file:\/\//i, "").toLowerCase()
            let cleanDeleted = filePath.replace(/^file:\/\//i, "").toLowerCase()

            if (cleanCurrent === cleanDeleted) {
                videoPlayer.stop()
                videoPlayer.source = ""
            }
        }
    }
}

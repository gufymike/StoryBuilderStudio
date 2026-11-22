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
** File: AudioWorkspaceWidget.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtMultimedia
import StoryBuilderStudio

Rectangle {
    id: audioRoot
    objectName: "audioWorkspaceWidgetRoot"
    color: "#08121A"
    border.color: "#1A4A68"
    border.width: 1
    radius: 4

    property string currentProjectPath: ""

    // Testing / Inspection hooks
    property alias playerSource: player.source
    property alias playbackState: player.playbackState
    property alias waveformModel: waveModel
    property alias directoryModel: audioDirModel

    AudioDirectoryModel {
        id: audioDirModel
        objectName: "audioDirModel"
    }

    AudioWaveformModel {
        id: waveModel
        objectName: "audioWaveformModel"
    }

    function updateProjectPath(path) {
        currentProjectPath = path
        audioDirModel.projectPath = path
    }

    function previewAudioFile(filePath) {
        if (!filePath || filePath.length === 0) return

        audioDirModel.refresh()

        let cleanPath = filePath.toString().replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) {
            cleanPath = cleanPath.substring(1)
        }

        let mediaUrl = "file:///" + cleanPath.replace(/\\/g, "/")
        player.source = mediaUrl
        waveModel.loadAudioFile(cleanPath, 140)
        player.play()
    }

    function formatTime(ms) {
        if (!ms || ms <= 0) return "00:00"
        let totalSecs = Math.floor(ms / 1000)
        let mins = Math.floor(totalSecs / 60)
        let secs = totalSecs % 60
        return (mins < 10 ? "0" + mins : mins) + ":" + (secs < 10 ? "0" + secs : secs)
    }

    MediaPlayer {
        id: player
        objectName: "mediaPlayer"
        audioOutput: AudioOutput {
            id: audioOut
            volume: 0.8
        }

        onSourceChanged: {
            if (player.source.toString().length > 0) {
                let clean = player.source.toString().replace(/^file:\/\//i, "")
                if (Qt.platform.os === "windows" && clean.startsWith("/")) {
                    clean = clean.substring(1)
                }
                waveModel.loadAudioFile(clean, 140)
            }
        }
    }

    // Shared Root Context Menu
    AssetContextMenu {
        id: sharedContextMenu
        objectName: "sharedContextMenu"
        assetType: "audio"
    }

    AssetImportAction {
        id: audioImporter
        objectName: "audioImporter"
        targetModel: audioDirModel
        dialogTitle: qsTr("Select Audio Track(s) to Import")
        nameFilters: ["Audio Files (*.wav *.mp3 *.ogg *.flac)", "All Files (*)"]
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
                objectName: "importAudioBtn"
                text: qsTr("📥 Import Audio")
                implicitHeight: 26
                font.pixelSize: 11
                enabled: typeof projectManager !== "undefined" && projectManager.isProjectOpen
                background: Rectangle {
                    color: importBtn.down ? "#0077A8" : (importBtn.hovered ? "#00B4F0" : "#0B1B26")
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }
                contentItem: Text {
                    text: parent.text
                    color: importBtn.enabled ? "#FFFFFF" : "#888888"
                    style: importBtn.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: audioImporter.open()
            }

            Button {
                id: refreshBtn
                objectName: "refreshAudioBtn"
                text: qsTr("🔄 Refresh")
                implicitHeight: 26
                font.pixelSize: 11
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
                onClicked: audioDirModel.refresh()
            }

            Item { Layout.fillWidth: true }

            Text {
                id: trackStatusText
                objectName: "trackStatusText"
                text: player.source.toString().length > 0
                    ? qsTr("Loaded: ") + player.source.toString().substring(player.source.toString().lastIndexOf('/') + 1)
                    : qsTr("No track selected")
                color: player.source.toString().length > 0 ? "#00F0FF" : "#888888"
                font.pixelSize: 11
                elide: Text.ElideMiddle
                Layout.maximumWidth: 260
            }
        }

        // ==========================================
        // 2. ACTIVE WAVEFORM & TRANSPORT
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 125
            color: "#0B1B26"
            border.color: player.playbackState === MediaPlayer.PlayingState ? "#00F0FF" : "#1A4A68"
            border.width: 1
            radius: 4
            clip: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Item {
                    id: waveformBox
                    objectName: "waveformBox"
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Canvas {
                        id: waveCanvas
                        objectName: "waveCanvas"
                        anchors.fill: parent

                        Connections {
                            target: waveModel
                            function onWavePointsChanged() { waveCanvas.requestPaint() }
                        }

                        onPaint: {
                            let ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)

                            let points = waveModel.wavePoints
                            if (!points || points.length === 0) {
                                ctx.strokeStyle = "#0A1924"
                                ctx.lineWidth = 1
                                ctx.beginPath()
                                ctx.moveTo(0, height / 2)
                                ctx.lineTo(width, height / 2)
                                ctx.stroke()
                                return
                            }

                            let barCount = points.length
                            let totalWidth = width
                            let barSpacing = 1
                            let barWidth = Math.max(1, (totalWidth / barCount) - barSpacing)
                            let midY = height / 2

                            for (let i = 0; i < barCount; i++) {
                                let amp = points[i]
                                let barHeight = Math.max(2, amp * (height * 0.85))
                                let x = i * (barWidth + barSpacing)

                                ctx.fillStyle = "#1A4A68"
                                ctx.fillRect(x, midY - (barHeight / 2), barWidth, barHeight)
                            }
                        }
                    }

                    Rectangle {
                        id: playheadLine
                        objectName: "playheadLine"
                        width: 2
                        height: parent.height
                        color: "#00F0FF"
                        visible: player.duration > 0
                        x: player.duration > 0 ? (player.position / player.duration) * (waveformBox.width - width) : 0
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: (mouse) => {
                            if (player.duration > 0) {
                                let ratio = Math.max(0, Math.min(1, mouse.x / width))
                                player.position = ratio * player.duration
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Button {
                        id: playPauseBtn
                        objectName: "playPauseBtn"
                        text: player.playbackState === MediaPlayer.PlayingState ? "⏸" : "▶"
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 24
                        enabled: player.source.toString().length > 0
                        background: Rectangle {
                            color: playPauseBtn.down ? "#0077A8" : (playPauseBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "#FFFFFF" : "#666666"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: {
                            if (player.playbackState === MediaPlayer.PlayingState) {
                                player.pause()
                            } else {
                                player.play()
                            }
                        }
                    }

                    Button {
                        id: stopBtn
                        objectName: "stopBtn"
                        text: "⏹"
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 24
                        enabled: player.playbackState === MediaPlayer.PlayingState || player.position > 0
                        background: Rectangle {
                            color: stopBtn.down ? "#0077A8" : (stopBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 3
                        }
                        contentItem: Text {
                            text: parent.text
                            color: parent.enabled ? "#FFFFFF" : "#666666"
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: player.stop()
                    }

                    Text {
                        id: timeDisplay
                        objectName: "timeDisplay"
                        text: formatTime(player.position) + " / " + formatTime(player.duration)
                        color: "#00F0FF"
                        font.family: "Courier New, monospace"
                        font.pixelSize: 11
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("Vol")
                        color: "#FFFFFF"
                        font.pixelSize: 11
                    }

                    Slider {
                        id: volSlider
                        objectName: "volSlider"
                        from: 0.0
                        to: 1.0
                        value: 0.8
                        Layout.preferredWidth: 80
                        onValueChanged: audioOut.volume = value
                    }
                }
            }
        }

        // ==========================================
        // 3. ASSET LISTVIEW
        // ==========================================
        ListView {
            id: audioListView
            objectName: "audioListView"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: audioDirModel
            spacing: 4

            delegate: Rectangle {
                id: delegateRect
                objectName: "audioDelegate_" + index
                width: audioListView.width
                height: 42
                color: delegateMouse.containsMouse ? "#0B1B26" : "#0A1924"
                border.color: (player.source.toString() === model.filePath || player.source.toString() === ("file:///" + model.filePath)) ? "#00F0FF" : "#1A4A68"
                border.width: 1
                radius: 4

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 10

                    Button {
                        text: (player.source.toString().indexOf(model.fileName) !== -1 && player.playbackState === MediaPlayer.PlayingState) ? "⏸" : "▶"
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
                            if (player.source.toString().indexOf(model.fileName) !== -1) {
                                if (player.playbackState === MediaPlayer.PlayingState) {
                                    player.pause()
                                } else {
                                    player.play()
                                }
                            } else {
                                audioRoot.previewAudioFile(model.filePath)
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: model.fileName || ""
                            color: "#FFFFFF"
                            font.bold: true
                            font.pixelSize: 11
                            elide: Text.ElideMiddle
                        }

                        Text {
                            text: qsTr("Size: %1 | Modified: %2").arg(model.fileSize || "0 KB").arg(model.modifiedDate || "")
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
            audioDirModel.refresh()
            let cleanCurrent = player.source.toString().replace(/^file:\/\//i, "").toLowerCase()
            let cleanDeleted = filePath.replace(/^file:\/\//i, "").toLowerCase()
            if (cleanCurrent === cleanDeleted) {
                player.stop()
                player.source = ""
            }
        }
    }
}

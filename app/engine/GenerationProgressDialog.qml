// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    objectName: "generationProgressDialog"
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 450
    height: 235

    property string modalityLabel: qsTr("Content")
    property string driverName: ""
    property string statusMessage: qsTr("Initializing inference...")
    property real progressValue: -1.0 // < 0 = indeterminate, 0.0..1.0 = determinate
    property int elapsedSeconds: 0
    property bool cancelling: false

    signal cancelRequested()

    function formatModality(rawType) {
        if (!rawType || rawType.length === 0) return qsTr("Content")
        return rawType.charAt(0).toUpperCase() + rawType.slice(1)
    }

    function startTracking(modality, driver, initialStatus) {
        modalityLabel = formatModality(modality)
        driverName = driver || ""
        statusMessage = initialStatus || qsTr("Running inference...")
        progressValue = -1.0
        elapsedSeconds = 0
        cancelling = false
        elapsedTimer.restart()
        open()
    }

    function updateProgress(status, progress) {
        if (status !== undefined && status !== null && status.length > 0) {
            statusMessage = status
        }
        if (progress !== undefined && progress !== null) {
            progressValue = progress
        }
    }

    function finishTracking() {
        elapsedTimer.stop()
        cancelling = false
        close()
    }

    function formatElapsed(totalSec) {
        let mins = Math.floor(totalSec / 60)
        let secs = totalSec % 60
        return (mins < 10 ? "0" : "") + mins + ":" + (secs < 10 ? "0" : "") + secs
    }

    Timer {
        id: elapsedTimer
        interval: 1000
        repeat: true
        running: root.opened
        onTriggered: root.elapsedSeconds += 1
    }

    background: Rectangle {
        color: "#08121A"
        border.color: root.cancelling ? "#FF5555" : "#00F0FF"
        border.width: 1
        radius: 6
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 12

        RowLayout {
            Layout.fillWidth: true

            Text {
                id: progressTitleText
                objectName: "progressTitleText"
                text: qsTr("Generating %1").arg(root.modalityLabel)
                color: "#00F0FF"
                font.bold: true
                font.pixelSize: 16
                style: Text.Outline
                styleColor: "#000000"
            }

            Item { Layout.fillWidth: true }

            Text {
                id: elapsedTimeText
                objectName: "elapsedTimeText"
                text: root.formatElapsed(root.elapsedSeconds)
                color: "#CCCCCC"
                font.family: "Consolas"
                font.pixelSize: 13
            }
        }

        Text {
            id: driverInfoText
            objectName: "driverInfoText"
            visible: root.driverName.length > 0
            text: qsTr("Driver: %1").arg(root.driverName)
            color: "#88B4CC"
            font.pixelSize: 12
        }

        ProgressBar {
            id: genProgressBar
            objectName: "genProgressBar"
            Layout.fillWidth: true
            indeterminate: root.progressValue < 0.0
            from: 0.0
            to: 1.0
            value: root.progressValue >= 0.0 ? root.progressValue : 0.0
        }

        Text {
            id: statusLabel
            objectName: "generationStatusText"
            Layout.fillWidth: true
            text: root.cancelling ? qsTr("Cancelling after current step...") : root.statusMessage
            color: root.cancelling ? "#FF8888" : "#FFFFFF"
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                id: runInBackgroundBtn
                objectName: "runInBackgroundBtn"
                text: qsTr("Run in Background")
                onClicked: root.close()

                contentItem: Text {
                    text: runInBackgroundBtn.text
                    color: "#CCCCCC"
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 135
                    implicitHeight: 32
                    radius: 4
                    color: runInBackgroundBtn.hovered ? "#132B3A" : "#0B1B26"
                    border.color: "#1A4A68"
                    border.width: 1
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: cancelBtn
                objectName: "cancelGenerationBtn"
                text: root.cancelling ? qsTr("Cancelling...") : qsTr("Cancel Generation")
                enabled: !root.cancelling
                onClicked: {
                    root.cancelling = true
                    root.cancelRequested()
                }

                contentItem: Text {
                    text: cancelBtn.text
                    color: cancelBtn.enabled ? "#FFFFFF" : "#888888"
                    font.bold: true
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    implicitWidth: 150
                    implicitHeight: 32
                    radius: 4
                    color: !cancelBtn.enabled ? "#2A1A1E" : (cancelBtn.hovered ? "#A8242C" : "#7A181E")
                    border.color: "#FF5555"
                    border.width: 1
                }
            }
        }
    }
}

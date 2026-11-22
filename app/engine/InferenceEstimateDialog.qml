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
** File: InferenceEstimateDialog.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Dialog {
    id: estimateDialog
    objectName: "inferenceEstimateDialogRoot"
    title: qsTr("Inference & Resource Profiler")
    modal: true
    focus: true
    anchors.centerIn: parent
    width: 480
    standardButtons: Dialog.Ok

    // Proxy properties forwarding to backend singleton or local mock
    property string engineType: (typeof inferenceEstimator !== "undefined" && inferenceEstimator) ? inferenceEstimator.engineType : "flux_schnell"
    onEngineTypeChanged: {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.engineType = engineType
        }
    }

    property string promptText: (typeof inferenceEstimator !== "undefined" && inferenceEstimator) ? inferenceEstimator.promptText : ""
    onPromptTextChanged: {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.promptText = promptText
        }
    }

    property int steps: (typeof inferenceEstimator !== "undefined" && inferenceEstimator) ? inferenceEstimator.steps : 4
    onStepsChanged: {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.steps = steps
        }
    }

    property int widthVal: (typeof inferenceEstimator !== "undefined" && inferenceEstimator) ? inferenceEstimator.width : 512
    onWidthValChanged: {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.width = widthVal
        }
    }

    property int heightVal: (typeof inferenceEstimator !== "undefined" && inferenceEstimator) ? inferenceEstimator.height : 512
    onHeightValChanged: {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.height = heightVal
        }
    }

    // Direct mock properties for headless QTest harness assertion
    property string mockEstimatedTime: ""
    property real mockPeakRamGb: -1.0
    property real mockDiskSizeGb: -1.0
    property var mockBreakdownItems: null

    readonly property string displayEstimatedTime: {
        if (mockEstimatedTime.length > 0) return mockEstimatedTime
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            return inferenceEstimator.estimatedTimeFormatted
        }
        return "0s"
    }

    readonly property real displayPeakRamGb: {
        if (mockPeakRamGb >= 0.0) return mockPeakRamGb
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            return inferenceEstimator.estimatedPeakRamGb
        }
        return 0.0
    }

    readonly property real displayDiskSizeGb: {
        if (mockDiskSizeGb >= 0.0) return mockDiskSizeGb
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            return inferenceEstimator.totalDiskSizeGb
        }
        return 0.0
    }

    readonly property var displayBreakdownItems: {
        if (mockBreakdownItems !== null) return mockBreakdownItems
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            return inferenceEstimator.breakdownItems
        }
        return []
    }

    function setReferences(filePaths) {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.referenceFilePaths = filePaths
        }
    }

    function setReferenceFolder(dirPath) {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.setReferenceDirectory(dirPath)
        }
    }

    function recalculate() {
        if (typeof inferenceEstimator !== "undefined" && inferenceEstimator) {
            inferenceEstimator.recalculate()
        }
    }

    onAboutToShow: {
        estimateDialog.recalculate()
    }

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 8
    }

    contentItem: ColumnLayout {
        spacing: 14
        anchors.margins: 4

        // Header Title Card
        RowLayout {
            spacing: 10
            Text {
                text: "⚡"
                font.pixelSize: 20
            }
            ColumnLayout {
                spacing: 2
                Text {
                    id: headerTitleText
                    objectName: "headerTitleText"
                    text: qsTr("Resource & Latency Estimate (%1)").arg(estimateDialog.engineType ? estimateDialog.engineType.toUpperCase() : "CPU")
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    font.pixelSize: 14
                }
                Text {
                    id: headerSubtitleText
                    objectName: "headerSubtitleText"
                    text: estimateDialog.engineType === "remote"
                          ? qsTr("Remote REST API Execution Profile (Cloud / Server)")
                          : qsTr("Offline CPU Execution Profile (AVX2 Active)")
                    color: "#888888"
                    font.pixelSize: 10
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1A4A68"
        }

        // 3 Key Metric Cards
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // 1. Total Runtime
            Rectangle {
                Layout.fillWidth: true
                height: 70
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text {
                        text: qsTr("TOTAL RUNTIME")
                        color: "#888888"
                        font.pixelSize: 9
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        id: totalRuntimeText
                        objectName: "totalRuntimeText"
                        text: estimateDialog.displayEstimatedTime
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 16
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // 2. Peak RAM
            Rectangle {
                id: ramCardRect
                objectName: "ramCardRect"
                Layout.fillWidth: true
                height: 70
                color: "#0B1B26"
                border.color: estimateDialog.displayPeakRamGb > 12.0 ? "#FF4444" : "#1A4A68"
                border.width: 1
                radius: 4
                // Flat alias for clean, non-nested test inspection
                readonly property color currentBorderColor: border.color
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text {
                        text: qsTr("PEAK RAM")
                        color: "#888888"
                        font.pixelSize: 9
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        id: peakRamText
                        objectName: "peakRamText"
                        text: estimateDialog.displayPeakRamGb.toFixed(2) + " GB"
                        color: estimateDialog.displayPeakRamGb > 12.0 ? "#FF6666" : "#FFFFFF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 15
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }

            // 3. Weights on Disk
            Rectangle {
                Layout.fillWidth: true
                height: 70
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 2
                    Text {
                        text: qsTr("WEIGHTS ON DISK")
                        color: "#888888"
                        font.pixelSize: 9
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                    Text {
                        id: totalDiskText
                        objectName: "totalDiskText"
                        text: estimateDialog.displayDiskSizeGb.toFixed(2) + " GB"
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.pixelSize: 15
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }
                }
            }
        }

        // Dynamic Engine Breakdown Table
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: breakdownCol.implicitHeight + 16
            color: "#08121A"
            radius: 4
            border.color: "#132B3A"
            border.width: 1

            ColumnLayout {
                id: breakdownCol
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Repeater {
                    id: breakdownRepeater
                    objectName: "breakdownRepeater"
                    model: estimateDialog.displayBreakdownItems

                    RowLayout {
                        id: breakdownRow
                        objectName: "breakdownRow_" + index
                        Layout.fillWidth: true

                        Text {
                            text: modelData.label
                            color: "#CCCCCC"
                            font.pixelSize: 11
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: modelData.time
                            color: "#00F0FF"
                            style: Text.Outline
                            styleColor: "#000000"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }

                Text {
                    id: emptyBreakdownLabel
                    objectName: "emptyBreakdownLabel"
                    visible: estimateDialog.displayBreakdownItems.length === 0
                    text: qsTr("No engine profile stages registered.")
                    color: "#666666"
                    font.italic: true
                    font.pixelSize: 10
                }
            }
        }
    }
}

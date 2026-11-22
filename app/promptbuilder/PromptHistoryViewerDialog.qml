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
** File: PromptHistoryViewerDialog.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Window {
    id: historyViewerWindow
    objectName: "promptHistoryViewerWindowRoot"
    title: qsTr("Prompt & Thought History Inspector")
    width: 920
    height: 600
    minimumWidth: 700
    minimumHeight: 450
    color: "#08121A" // Dark Well

    // Helper: Centers this window over its transient parent (main rootWindow)
    function showCentered(parentWindow) {
        if (parentWindow) {
            historyViewerWindow.transientParent = parentWindow
            historyViewerWindow.x = parentWindow.x + Math.max(0, (parentWindow.width - historyViewerWindow.width) / 2)
            historyViewerWindow.y = parentWindow.y + Math.max(0, (parentWindow.height - historyViewerWindow.height) / 2)
        }
        historyViewerWindow.show()
        historyViewerWindow.raise()
        historyViewerWindow.requestActivate()
    }

    // State data for currently selected transaction
    property string selectedTimestamp: ""
    property string selectedPromptText: ""
    property string selectedResponseText: ""
    property string selectedThinkText: ""
    property int currentStackIndex: 0

    // Mock property for isolated unit test assertion
    property var mockEntries: null

    function loadMergedHistory() {
        historyListModel.clear()

        if (mockEntries !== null) {
            for (let m = 0; m < mockEntries.length; ++m) {
                historyListModel.append(mockEntries[m])
            }
            if (historyListModel.count > 0) {
                historyListView.currentIndex = 0
                selectEntry(0)
            } else {
                clearSelection()
            }
            return
        }

        if (typeof promptHistoryStore === "undefined" || promptHistoryStore === null) return

        let promptEntries = promptHistoryStore.getHistoryEntries()
        let thinkEntries = promptHistoryStore.getThinkEntries()

        let thinkMap = {}
        for (let t = 0; t < thinkEntries.length; ++t) {
            let item = thinkEntries[t]
            if (item.timestamp) {
                thinkMap[item.timestamp] = item.thinkText || ""
            }
        }

        for (let i = 0; i < promptEntries.length; ++i) {
            let pItem = promptEntries[i]
            let ts = pItem.timestamp || ""
            let thought = thinkMap[ts] || ""

            historyListModel.append({
                "timestamp": ts,
                "category": pItem.category || "script",
                "promptText": pItem.promptText || "",
                "responseText": pItem.responseText || "",
                "thinkText": thought
            })
        }

        if (historyListModel.count > 0) {
            historyListView.currentIndex = 0
            selectEntry(0)
        } else {
            clearSelection()
        }
    }

    function selectEntry(index) {
        if (index < 0 || index >= historyListModel.count) return
        let item = historyListModel.get(index)
        selectedTimestamp = item.timestamp
        selectedPromptText = item.promptText
        selectedResponseText = item.responseText
        selectedThinkText = item.thinkText
    }

    function clearSelection() {
        selectedTimestamp = ""
        selectedPromptText = ""
        selectedResponseText = ""
        selectedThinkText = ""
    }

    onVisibleChanged: {
        if (visible) {
            loadMergedHistory()
        }
    }

    // Shared Reusable Context Menu for all TextAreas
    component TextAreaContextMenu: Menu {
        id: menuRoot
        required property TextArea targetArea

        background: Rectangle {
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
        }

        MenuItem {
            text: qsTr("Copy Selection")
            enabled: menuRoot.targetArea.selectedText.length > 0
            onClicked: menuRoot.targetArea.copy()
        }
        MenuItem {
            text: qsTr("Copy All")
            enabled: menuRoot.targetArea.text.length > 0
            onClicked: {
                menuRoot.targetArea.selectAll()
                menuRoot.targetArea.copy()
                menuRoot.targetArea.deselect()
            }
        }
        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: "#1A4A68" }
        }
        MenuItem {
            text: qsTr("Select All")
            enabled: menuRoot.targetArea.text.length > 0
            onClicked: menuRoot.targetArea.selectAll()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // ==========================================
        // LEFT PANE: CHRONOLOGICAL TIMESTAMP LIST
        // ==========================================
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillHeight: true
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 6

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        id: historyLogTitle
                        objectName: "historyLogTitle"
                        text: qsTr("History Log (%1)").arg(historyListModel.count)
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 12
                    }
                    Item { Layout.fillWidth: true }
                    Button {
                        id: refreshHistoryBtn
                        objectName: "refreshHistoryBtn"
                        text: qsTr("Refresh")
                        implicitHeight: 24
                        font.pixelSize: 10
                        background: Rectangle {
                            color: refreshHistoryBtn.down ? "#0077A8" : (refreshHistoryBtn.hovered ? "#00B4F0" : "#08121A")
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            style: Text.Outline
                            styleColor: "#000000"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: historyViewerWindow.loadMergedHistory()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#1A4A68"
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    ListView {
                        id: historyListView
                        objectName: "historyListView"
                        model: ListModel { id: historyListModel }
                        spacing: 4

                        delegate: Rectangle {
                            id: delegateCard
                            objectName: "historyDelegate_" + index
                            width: historyListView.width
                            implicitHeight: col.implicitHeight + 12
                            radius: 4
                            color: historyListView.currentIndex === index ? "#132B3A" : (mouseArea.containsMouse ? "#08121A" : "#0B1B26")
                            border.color: historyListView.currentIndex === index ? "#00F0FF" : "#1B3B4F"
                            border.width: 1

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: {
                                    historyListView.currentIndex = index
                                    historyViewerWindow.selectEntry(index)
                                }
                            }

                            ColumnLayout {
                                id: col
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 2

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text {
                                        text: (model && model.category) ? String(model.category).toUpperCase() : "PROMPT"
                                        color: (model && model.thinkText && model.thinkText.length > 0) ? "#00F0FF" : "#1A4A68"
                                        style: Text.Outline
                                        styleColor: "#000000"
                                        font.pixelSize: 9
                                        font.bold: true
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text {
                                        text: (model && model.thinkText && model.thinkText.length > 0) ? "🧠 <think>" : ""
                                        color: "#00F0FF"
                                        style: Text.Outline
                                        styleColor: "#000000"
                                        font.pixelSize: 9
                                    }
                                }

                                Text {
                                    text: (model && model.timestamp) ? String(model.timestamp).replace("T", " ").substring(0, 19) : "-"
                                    color: "#888888"
                                    font.family: "Courier New, monospace"
                                    font.pixelSize: 10
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: (model && model.promptText) ? model.promptText : ""
                                    color: "#CCCCCC"
                                    font.pixelSize: 11
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }
                }
            }
        }

        // ==========================================
        // RIGHT PANE: STACKED VIEWS & WORKSPACES
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 6

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // Header Switcher
                // 1. DYNAMIC PROMPT CARD
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: promptBannerCol.implicitHeight + 12
                    color: "#08121A"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        id: promptBannerCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            Text {
                                text: qsTr("Dispatched Prompt:")
                                color: "#00F0FF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.pixelSize: 11
                                font.bold: true
                            }

                            Item { Layout.fillWidth: true }

                            Text {
                                id: timestampHeaderDisplay
                                objectName: "timestampHeaderDisplay"
                                text: historyViewerWindow.selectedTimestamp
                                color: "#888888"
                                font.family: "Courier New, monospace"
                                font.pixelSize: 10
                            }
                        }

                        Text {
                            id: promptHeaderDisplay
                            objectName: "promptHeaderDisplay"
                            Layout.fillWidth: true
                            text: historyViewerWindow.selectedPromptText.length > 0
                                  ? historyViewerWindow.selectedPromptText
                                  : qsTr("<No prompt selected>")
                            color: historyViewerWindow.selectedPromptText.length > 0 ? "#FFFFFF" : "#888888"
                            font.pixelSize: 12
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }
                }

                // 2. VIEW SWITCHER TOOLBAR
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    ButtonGroup { id: viewGroup }

                    Button {
                        id: btnViewResponse
                        objectName: "btnViewResponse"
                        text: qsTr("Response / Output")
                        checkable: true
                        checked: historyViewerWindow.currentStackIndex === 0
                        ButtonGroup.group: viewGroup
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: btnViewResponse.checked ? "#132B3A" : (btnViewResponse.hovered ? "#00B4F0" : "#08121A")
                            border.color: btnViewResponse.checked ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: btnViewResponse.checked ? "#00F0FF" : "#FFFFFF"
                            style: btnViewResponse.checked ? Text.Outline : Text.Normal
                            styleColor: "#000000"
                            font.bold: btnViewResponse.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: historyViewerWindow.currentStackIndex = 0
                    }

                    Button {
                        id: btnViewThink
                        objectName: "btnViewThink"
                        text: qsTr("🧠 Reasoning / Think")
                        checkable: true
                        checked: historyViewerWindow.currentStackIndex === 1
                        ButtonGroup.group: viewGroup
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: btnViewThink.checked ? "#132B3A" : (btnViewThink.hovered ? "#00B4F0" : "#08121A")
                            border.color: btnViewThink.checked ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: btnViewThink.checked ? "#00F0FF" : "#FFFFFF"
                            style: btnViewThink.checked ? Text.Outline : Text.Normal
                            styleColor: "#000000"
                            font.bold: btnViewThink.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: historyViewerWindow.currentStackIndex = 1
                    }

                    Button {
                        id: btnViewCombined
                        objectName: "btnViewCombined"
                        text: qsTr("Combined (Stacked)")
                        checkable: true
                        checked: historyViewerWindow.currentStackIndex === 2
                        ButtonGroup.group: viewGroup
                        implicitHeight: 26
                        font.pixelSize: 11
                        background: Rectangle {
                            color: btnViewCombined.checked ? "#132B3A" : (btnViewCombined.hovered ? "#00B4F0" : "#08121A")
                            border.color: btnViewCombined.checked ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                        contentItem: Text {
                            text: parent.text
                            color: btnViewCombined.checked ? "#00F0FF" : "#FFFFFF"
                            style: btnViewCombined.checked ? Text.Outline : Text.Normal
                            styleColor: "#000000"
                            font.bold: btnViewCombined.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: historyViewerWindow.currentStackIndex = 2
                    }

                    Item { Layout.fillWidth: true }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#1A4A68"
                }

                // Stacked Display
                StackLayout {
                    id: viewerStack
                    objectName: "viewerStack"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: historyViewerWindow.currentStackIndex

                    // View 0: Prompt & Response History
                    ScrollView {
                        clip: true
                        TextArea {
                            id: responseArea
                            objectName: "responseArea"
                            readOnly: true
                            selectByMouse: true
                            text: historyViewerWindow.selectedResponseText.length > 0
                                  ? historyViewerWindow.selectedResponseText
                                  : qsTr("<No Response Payload Recorded>")
                            color: "#FFFFFF"
                            font.family: "Courier New, monospace"
                            font.pixelSize: 12
                            wrapMode: TextArea.Wrap
                            background: Rectangle {
                                color: "#08121A"
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                onClicked: responseMenu.popup()
                            }
                            TextAreaContextMenu { id: responseMenu; targetArea: responseArea }
                        }
                    }

                    // View 1: Think Text
                    ScrollView {
                        clip: true
                        TextArea {
                            id: thinkArea
                            objectName: "thinkArea"
                            readOnly: true
                            selectByMouse: true
                            text: historyViewerWindow.selectedThinkText.length > 0
                                  ? historyViewerWindow.selectedThinkText
                                  : qsTr("<No <think> reasoning trace captured for this turn>")
                            color: "#00F0FF"
                            font.family: "Courier New, monospace"
                            font.pixelSize: 12
                            wrapMode: TextArea.Wrap
                            background: Rectangle {
                                color: "#08121A"
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 4
                            }

                            MouseArea {
                                anchors.fill: parent
                                acceptedButtons: Qt.RightButton
                                onClicked: thinkMenu.popup()
                            }
                            TextAreaContextMenu { id: thinkMenu; targetArea: thinkArea }
                        }
                    }

                    // View 2: Both Stacked
                    SplitView {
                        orientation: Qt.Vertical

                        // Top: Think Text
                        Rectangle {
                            SplitView.preferredHeight: 180
                            SplitView.fillWidth: true
                            color: "transparent"

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2

                                Text {
                                    text: qsTr("🧠 Internal Thoughts (<think>):")
                                    color: "#00F0FF"
                                    style: Text.Outline
                                    styleColor: "#000000"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    TextArea {
                                        id: stackedThinkArea
                                        objectName: "stackedThinkArea"
                                        readOnly: true
                                        selectByMouse: true
                                        text: historyViewerWindow.selectedThinkText.length > 0
                                              ? historyViewerWindow.selectedThinkText
                                              : qsTr("<No think trace>")
                                        color: "#00F0FF"
                                        font.family: "Courier New, monospace"
                                        font.pixelSize: 11
                                        wrapMode: TextArea.Wrap
                                        background: Rectangle {
                                            color: "#08121A"
                                            border.color: "#1A4A68"
                                            border.width: 1
                                            radius: 4
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton
                                            onClicked: stackedThinkMenu.popup()
                                        }
                                        TextAreaContextMenu { id: stackedThinkMenu; targetArea: stackedThinkArea }
                                    }
                                }
                            }
                        }

                        // Bottom: Model Response
                        Rectangle {
                            SplitView.fillHeight: true
                            SplitView.fillWidth: true
                            color: "transparent"

                            ColumnLayout {
                                anchors.fill: parent
                                spacing: 2

                                Text {
                                    text: qsTr("💬 Final Assistant Response:")
                                    color: "#FFFFFF"
                                    style: Text.Outline
                                    styleColor: "#000000"
                                    font.bold: true
                                    font.pixelSize: 11
                                }

                                ScrollView {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    clip: true

                                    TextArea {
                                        id: stackedResponseArea
                                        objectName: "stackedResponseArea"
                                        readOnly: true
                                        selectByMouse: true
                                        text: historyViewerWindow.selectedResponseText
                                        color: "#FFFFFF"
                                        font.family: "Courier New, monospace"
                                        font.pixelSize: 12
                                        wrapMode: TextArea.Wrap
                                        background: Rectangle {
                                            color: "#08121A"
                                            border.color: "#1A4A68"
                                            border.width: 1
                                            radius: 4
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.RightButton
                                            onClicked: stackedResponseMenu.popup()
                                        }
                                        TextAreaContextMenu { id: stackedResponseMenu; targetArea: stackedResponseArea }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

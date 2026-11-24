// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Item {
    id: editorPaneRoot
    objectName: "editorPaneRoot"

    property alias textContent: rawTextArea.text
    property string paneTitle: "Editor Pane"

    property int currentViewIndex: 0
    property bool isDocumentLoaded: false

    // Script / Document Signals
    signal newScriptTriggered()
    signal openScriptTriggered()
    signal saveScriptTriggered()
    signal closeScriptTriggered()
    signal recentDocumentTriggered(string filePath)

    // Edit Signals
    signal selectAllTriggered()
    signal copyTriggered()
    signal cutTriggered()
    signal pasteTriggered()
    signal undoTriggered()
    signal redoTriggered()
    signal findTriggered()

    // Markdown Insert Signals
    signal addLink()
    signal addImage()

    // Dispatch Pipeline Signals
    signal sendToImagePromptTriggered(string text)
    signal sendToAudioPromptTriggered(string text)
    signal sendToVideoPromptTriggered(string text)

    function wrapSelection(prefix, suffix) {
        let start = rawTextArea.selectionStart
        let end = rawTextArea.selectionEnd
        let text = rawTextArea.text
        if (start !== end) {
            let selected = text.substring(start, end)
            let replacement = prefix + selected + suffix
            rawTextArea.remove(start, end)
            rawTextArea.insert(start, replacement)
            rawTextArea.select(start + prefix.length, start + prefix.length + selected.length)
        } else {
            let pos = rawTextArea.cursorPosition
            rawTextArea.insert(pos, prefix + suffix)
            rawTextArea.cursorPosition = pos + prefix.length
        }
        rawTextArea.forceActiveFocus()
    }

    function insertLinePrefix(prefix) {
        let pos = rawTextArea.cursorPosition
        let text = rawTextArea.text
        let lineStart = text.lastIndexOf("\n", pos - 1)
        lineStart = (lineStart === -1) ? 0 : lineStart + 1
        rawTextArea.insert(lineStart, prefix)
        rawTextArea.forceActiveFocus()
    }

    function findNext(pattern, backward) {
        if (!pattern || pattern.length === 0) return
        let text = rawTextArea.text
        let idx = -1

        if (backward) {
            let searchArea = text.substring(0, Math.max(0, rawTextArea.selectionStart))
            idx = searchArea.lastIndexOf(pattern)
            if (idx === -1) idx = text.lastIndexOf(pattern)
        } else {
            let startPos = rawTextArea.selectionEnd
            idx = text.indexOf(pattern, startPos)
            if (idx === -1) idx = text.indexOf(pattern, 0)
        }

        if (idx !== -1) {
            rawTextArea.select(idx, idx + pattern.length)
            rawTextArea.forceActiveFocus()
        }
    }

    function selectAllOccurrences(pattern) {
        if (!pattern || pattern.length === 0) return
        let text = rawTextArea.text
        let idx = text.indexOf(pattern)
        if (idx !== -1) {
            rawTextArea.select(idx, idx + pattern.length)
            rawTextArea.forceActiveFocus()
        }
    }

    function replaceCurrent(pattern, replacement) {
        if (!pattern || pattern.length === 0) return
        if (rawTextArea.selectedText === pattern) {
            let start = rawTextArea.selectionStart
            rawTextArea.remove(start, rawTextArea.selectionEnd)
            rawTextArea.insert(start, replacement)
            rawTextArea.select(start, start + replacement.length)
        } else {
            findNext(pattern, false)
        }
    }

    function replaceAllOccurrences(pattern, replacement) {
        if (!pattern || pattern.length === 0) return
        let text = rawTextArea.text
        let escaped = pattern.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')
        let newText = text.replace(new RegExp(escaped, 'g'), replacement)
        let curPos = rawTextArea.cursorPosition
        rawTextArea.text = newText
        rawTextArea.cursorPosition = Math.min(curPos, newText.length)
    }

    function loadFileFromDisk(filePath) {
        if (!filePath || filePath.length === 0) return false

        let cleanPath = filePath.toString().replace(/^file:\/\//i, "")
        if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) {
            cleanPath = cleanPath.substring(1)
        }

        let content = ""
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            content = promptManager.readPromptFile(cleanPath)
        }

        if (content.length > 0 || content === "") {
            rawTextArea.text = content
            editorPaneRoot.isDocumentLoaded = true

            let parts = cleanPath.split(/[\\/]/)
            editorPaneRoot.paneTitle = parts[parts.length - 1]

            if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                settingsManager.addRecentFile(cleanPath)
            }
            return true
        }
        return false
    }

    onRecentDocumentTriggered: (filePath) => {
        loadFileFromDisk(filePath)
    }

    ButtonGroup {
        id: viewModeGroup
    }

    Rectangle {
        anchors.fill: parent
        color: "#08121A"
        border.color: "#1A4A68"
        border.width: 1
        radius: 4

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 6

            // ==========================================
            // 1. UNIFIED SUB-HEADER & MENU BAR
            // ==========================================
            ToolBar {
                id: editorHeaderBar
                objectName: "editorHeaderBar"
                Layout.fillWidth: true
                implicitHeight: 34

                background: Rectangle {
                    color: "#0B1B26"
                    border.color: "#1A4A68"
                    border.width: 1
                    radius: 3
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 6
                    spacing: 8

                    MenuBar {
                        id: editorMenuBar
                        objectName: "editorMenuBar"
                        Layout.alignment: Qt.AlignVCenter
                        background: Rectangle { color: "transparent" }

                        // --- SCRIPT (FILE) MENU ---
                        Menu {
                            title: qsTr("Scr&ipt")
                            delegate: MenuItem {
                                id: menuItem
                                implicitHeight: 28

                                contentItem: RowLayout {
                                    spacing: 8
                                    Image {
                                        visible: menuItem.icon.source.toString().length > 0
                                        source: menuItem.icon.source
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    Text {
                                        text: menuItem.text
                                        color: menuItem.hovered ? "#00F0FF" : "#FFFFFF"
                                        style: Text.Outline
                                        styleColor: "#000000"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }

                                background: Rectangle {
                                    color: menuItem.hovered ? "#0077A8" : "transparent"
                                    radius: 3
                                }
                            }

                            Action {
                                text: qsTr("New Script...")
                                shortcut: "Ctrl+N"
                                onTriggered: editorPaneRoot.newScriptTriggered()
                            }
                            Action {
                                text: qsTr("Open Script...")
                                shortcut: "Ctrl+O"
                                onTriggered: editorPaneRoot.openScriptTriggered()
                            }
                            Action {
                                text: qsTr("Save Script...")
                                shortcut: "Ctrl+S"
                                onTriggered: editorPaneRoot.saveScriptTriggered()
                            }
                            Action {
                                text: qsTr("Close Script")
                                enabled: editorPaneRoot.isDocumentLoaded
                                onTriggered: editorPaneRoot.closeScriptTriggered()
                            }
                            MenuSeparator {}
                            Menu {
                                id: recentFilesMenu
                                title: qsTr("Recent Documents")
                                enabled: (typeof settingsManager !== "undefined" && settingsManager !== null) &&
                                         settingsManager.recentFiles && settingsManager.recentFiles.length > 0

                                Instantiator {
                                    model: (typeof settingsManager !== "undefined" && settingsManager !== null) ? settingsManager.recentFiles : []
                                    onObjectAdded: (index, object) => recentFilesMenu.insertAction(index, object)
                                    onObjectRemoved: (index, object) => recentFilesMenu.removeAction(object)

                                    delegate: Action {
                                        text: {
                                            let full = modelData.toString()
                                            let parts = full.split(/[\\/]/)
                                            return parts[parts.length - 1] + " (" + full + ")"
                                        }
                                        onTriggered: editorPaneRoot.recentDocumentTriggered(modelData)
                                    }
                                }

                                MenuSeparator {
                                    visible: recentFilesMenu.count > 0
                                }

                                Action {
                                    text: qsTr("Clear Recent Documents")
                                    enabled: recentFilesMenu.count > 0
                                    onTriggered: {
                                        if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                                            settingsManager.clearRecentFiles()
                                        }
                                    }
                                }
                            }
                        }

                        // --- EDIT MENU ---
                        Menu {
                            title: qsTr("Edit")
                            delegate: MenuItem {
                                id: menuItemEdit
                                implicitHeight: 28

                                contentItem: RowLayout {
                                    spacing: 8
                                    Image {
                                        visible: menuItemEdit.icon.source.toString().length > 0
                                        source: menuItemEdit.icon.source
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                    Text {
                                        text: menuItemEdit.text
                                        color: menuItemEdit.hovered ? "#00F0FF" : "#FFFFFF"
                                        style: Text.Outline
                                        styleColor: "#000000"
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                        Layout.alignment: Qt.AlignVCenter
                                    }
                                }

                                background: Rectangle {
                                    color: menuItemEdit.hovered ? "#0077A8" : "transparent"
                                    radius: 3
                                }
                            }

                            Action {
                                text: qsTr("Undo")
                                shortcut: StandardKey.Undo
                                enabled: rawTextArea.canUndo
                                onTriggered: rawTextArea.undo()
                            }
                            Action {
                                text: qsTr("Redo")
                                shortcut: StandardKey.Redo
                                enabled: rawTextArea.canRedo
                                onTriggered: rawTextArea.redo()
                            }
                            MenuSeparator {}
                            Action {
                                text: qsTr("Cut")
                                shortcut: StandardKey.Cut
                                enabled: rawTextArea.selectedText.length > 0
                                onTriggered: rawTextArea.cut()
                            }
                            Action {
                                text: qsTr("Copy")
                                shortcut: StandardKey.Copy
                                enabled: rawTextArea.selectedText.length > 0
                                onTriggered: rawTextArea.copy()
                            }
                            Action {
                                text: qsTr("Paste")
                                shortcut: StandardKey.Paste
                                enabled: rawTextArea.canPaste
                                onTriggered: rawTextArea.paste()
                            }
                            Action {
                                text: qsTr("Select All")
                                shortcut: StandardKey.SelectAll
                                onTriggered: rawTextArea.selectAll()
                            }
                            MenuSeparator {}
                            Action {
                                text: qsTr("Find...")
                                shortcut: "Ctrl+F"
                                onTriggered: {
                                    findReplaceBar.visible = true
                                    findField.forceActiveFocus()
                                }
                            }
                            Action {
                                text: qsTr("Replace...")
                                shortcut: "Ctrl+H"
                                onTriggered: {
                                    findReplaceBar.visible = true
                                    replaceField.forceActiveFocus()
                                }
                            }
                        }

                        // --- MARKDOWN FORMATTING MENU ---
                        Menu {
                            title: qsTr("MarkDown")

                            Menu {
                                title: qsTr("Headings")
                                Action { text: qsTr("Heading 1 (#)"); onTriggered: editorPaneRoot.insertLinePrefix("# ") }
                                Action { text: qsTr("Heading 2 (##)"); onTriggered: editorPaneRoot.insertLinePrefix("## ") }
                                Action { text: qsTr("Heading 3 (###)"); onTriggered: editorPaneRoot.insertLinePrefix("### ") }
                                Action { text: qsTr("Heading 4 (####)"); onTriggered: editorPaneRoot.insertLinePrefix("#### ") }
                                Action { text: qsTr("Heading 5 (#####)"); onTriggered: editorPaneRoot.insertLinePrefix("##### ") }
                                Action { text: qsTr("Heading 6 (######)"); onTriggered: editorPaneRoot.insertLinePrefix("###### ") }
                            }

                            Menu {
                                title: qsTr("Inline Styles")
                                Action { text: qsTr("Bold (**bold**)"); shortcut: "Ctrl+B"; onTriggered: editorPaneRoot.wrapSelection("**", "**") }
                                Action { text: qsTr("Italic (*italic*)"); shortcut: "Ctrl+I"; onTriggered: editorPaneRoot.wrapSelection("*", "*") }
                                Action { text: qsTr("Strikethrough (~~text~~)"); onTriggered: editorPaneRoot.wrapSelection("~~", "~~") }
                                Action { text: qsTr("Inline Code (`code`)"); onTriggered: editorPaneRoot.wrapSelection("`", "`") }
                            }

                            Menu {
                                title: qsTr("Blocks & Structure")
                                Action { text: qsTr("Blockquote (> )"); onTriggered: editorPaneRoot.insertLinePrefix("> ") }
                                Action { text: qsTr("Code Block (```)"); onTriggered: editorPaneRoot.wrapSelection("```\n", "\n```") }
                                Action { text: qsTr("Horizontal Rule (---)"); onTriggered: editorPaneRoot.wrapSelection("\n---\n", "") }
                                Action {
                                    text: qsTr("Markdown Table (3x2)")
                                    onTriggered: editorPaneRoot.wrapSelection("\n| Col 1 | Col 2 | Col 3 |\n| :--- | :---: | ---: |\n| Item 1 | Item 2 | Item 3 |\n", "")
                                }
                            }

                            Menu {
                                title: qsTr("Lists")
                                Action { text: qsTr("Bullet List (- )"); onTriggered: editorPaneRoot.insertLinePrefix("- ") }
                                Action { text: qsTr("Numbered List (1. )"); onTriggered: editorPaneRoot.insertLinePrefix("1. ") }
                                Action { text: qsTr("Task List (- [ ] )"); onTriggered: editorPaneRoot.insertLinePrefix("- [ ] ") }
                            }

                            MenuSeparator {}
                            Action {
                                text: qsTr("Insert Link ([text](url))")
                                shortcut: "Ctrl+K"
                                onTriggered: {
                                    if (rawTextArea.selectedText.length > 0) editorPaneRoot.wrapSelection("[", "](url)")
                                    else editorPaneRoot.wrapSelection("[Link Text](", "https://)")
                                    editorPaneRoot.addLink()
                                }
                            }
                            Action {
                                text: qsTr("Insert Image (![alt](path))")
                                shortcut: "Ctrl+Shift+I"
                                onTriggered: {
                                    editorPaneRoot.wrapSelection("![Alt Text](", "image.png)")
                                    editorPaneRoot.addImage()
                                }
                            }
                        }
                    }

                    Text {
                        id: titleDisplay
                        objectName: "titleDisplay"
                        text: paneTitle
                        color: "#00F0FF"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        Layout.maximumWidth: 260
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Item { Layout.fillWidth: true }

                    RowLayout {
                        spacing: 2
                        Layout.alignment: Qt.AlignVCenter

                        Button {
                            id: rawModeBtn
                            objectName: "rawModeBtn"
                            text: qsTr("Raw Edit")
                            checkable: true
                            checked: editorPaneRoot.currentViewIndex === 0
                            ButtonGroup.group: viewModeGroup
                            implicitHeight: 24
                            font.pixelSize: 11
                            font.bold: true
                            background: Rectangle {
                                color: rawModeBtn.checked ? "#1A4A68" : (rawModeBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: rawModeBtn.text
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: rawModeBtn.checked
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: editorPaneRoot.currentViewIndex = 0
                        }

                        Button {
                            id: mdModeBtn
                            objectName: "mdModeBtn"
                            text: qsTr("Markdown")
                            checkable: true
                            checked: editorPaneRoot.currentViewIndex === 1
                            ButtonGroup.group: viewModeGroup
                            implicitHeight: 24
                            font.pixelSize: 11
                            font.bold: true
                            background: Rectangle {
                                color: mdModeBtn.checked ? "#1A4A68" : (mdModeBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: mdModeBtn.text
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: mdModeBtn.checked
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: editorPaneRoot.currentViewIndex = 1
                        }

                        Button {
                            id: htmlModeBtn
                            objectName: "htmlModeBtn"
                            text: qsTr("HTML")
                            checkable: true
                            checked: editorPaneRoot.currentViewIndex === 2
                            ButtonGroup.group: viewModeGroup
                            implicitHeight: 24
                            font.pixelSize: 11
                            font.bold: true
                            background: Rectangle {
                                color: htmlModeBtn.checked ? "#1A4A68" : (htmlModeBtn.hovered ? "#00B4F0" : "#0B1B26")
                                border.color: "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            contentItem: Text {
                                text: htmlModeBtn.text
                                color: "#FFFFFF"
                                style: Text.Outline
                                styleColor: "#000000"
                                font.bold: htmlModeBtn.checked
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: editorPaneRoot.currentViewIndex = 2
                        }
                    }
                }
            }

            // ==========================================
            // 2. MAIN WORKSPACE STACK
            // ==========================================
            StackLayout {
                id: editorStack
                objectName: "editorStack"
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: editorPaneRoot.currentViewIndex

                // Page 0: Raw Text Editor
                ScrollView {
                    clip: true

                    TextArea {
                        id: rawTextArea
                        objectName: "rawTextArea"
                        placeholderText: qsTr("Type raw markdown, screenplay beats, or text here...")
                        color: "#FFFFFF"
                        font.family: "Courier New, monospace"
                        font.pixelSize: (typeof settingsManager !== "undefined" && settingsManager !== null && settingsManager.editorFontSize) ? settingsManager.editorFontSize : 13
                        wrapMode: TextArea.Wrap
                        selectByMouse: true

                        background: Rectangle {
                            color: "#0B1B26"
                            border.color: rawTextArea.activeFocus ? "#00F0FF" : "#1A4A68"
                            border.width: 1
                            radius: 4
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            onClicked: (mouse) => editorContextMenu.popup()
                        }

                        Menu {
                            id: editorContextMenu
                            objectName: "editorContextMenu"

                            Action { text: qsTr("Undo"); enabled: rawTextArea.canUndo; onTriggered: rawTextArea.undo() }
                            Action { text: qsTr("Redo"); enabled: rawTextArea.canRedo; onTriggered: rawTextArea.redo() }
                            MenuSeparator {}
                            Action { text: qsTr("Cut"); enabled: rawTextArea.selectedText.length > 0; onTriggered: rawTextArea.cut() }
                            Action { text: qsTr("Copy"); enabled: rawTextArea.selectedText.length > 0; onTriggered: rawTextArea.copy() }
                            Action { text: qsTr("Paste"); enabled: rawTextArea.canPaste; onTriggered: rawTextArea.paste() }
                            MenuSeparator {}

                            Menu {
                                id: sendToMain
                                title: qsTr("Send To Prompt...")
                                enabled: rawTextArea.text.length > 0

                                function getPayloadText() {
                                    return rawTextArea.selectedText.length > 0
                                        ? rawTextArea.selectedText.trim()
                                        : rawTextArea.text.trim()
                                }

                                Action {
                                    text: qsTr("🖼 Image Prompt")
                                    onTriggered: editorPaneRoot.sendToImagePromptTriggered(sendToMain.getPayloadText())
                                }

                                Action {
                                    text: qsTr("🎵 Audio Prompt")
                                    onTriggered: editorPaneRoot.sendToAudioPromptTriggered(sendToMain.getPayloadText())
                                }

                                Action {
                                    text: qsTr("🎬 Video Prompt")
                                    onTriggered: editorPaneRoot.sendToVideoPromptTriggered(sendToMain.getPayloadText())
                                }
                            }

                            MenuSeparator {}
                            Action { text: qsTr("Select All"); onTriggered: rawTextArea.selectAll() }
                        }
                    }
                }

                // Page 1: Markdown Rendered Preview
                ScrollView {
                    clip: true

                    TextArea {
                        id: markdownPreviewArea
                        objectName: "markdownPreviewArea"
                        readOnly: true
                        selectByMouse: true
                        text: "**[Markdown Preview]**\n\n" + rawTextArea.text
                        textFormat: Text.MarkdownText
                        color: "#DDDDDD"
                        font.family: "Courier New, monospace"
                        font.pixelSize: 13
                        wrapMode: TextArea.Wrap
                        background: Rectangle {
                            color: "#0B1B26"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                    }
                }

                // Page 2: HTML Source / Structured Preview
                ScrollView {
                    clip: true

                    TextArea {
                        id: htmlPreviewArea
                        objectName: "htmlPreviewArea"
                        readOnly: true
                        selectByMouse: true
                        text: "<b>[HTML Preview]</b><br /><br />" + rawTextArea.text
                        textFormat: Text.RichText
                        color: "#00F0FF"
                        font.family: "Courier New, monospace"
                        font.pixelSize: 13
                        wrapMode: TextArea.Wrap
                        background: Rectangle {
                            color: "#0B1B26"
                            border.color: "#1A4A68"
                            border.width: 1
                            radius: 4
                        }
                    }
                }
            }

            // ==========================================
            // 3. FIND & REPLACE PANEL
            // ==========================================
            Rectangle {
                id: findReplaceBar
                objectName: "findReplaceBar"
                Layout.fillWidth: true
                implicitHeight: 74
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
                visible: false

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: qsTr("Find:")
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 50
                        }

                        TextField {
                            id: findField
                            objectName: "findField"
                            Layout.fillWidth: true
                            implicitHeight: 24
                            font.pixelSize: 11
                            color: "#FFFFFF"
                            placeholderText: qsTr("Text to search...")
                            background: Rectangle {
                                color: "#08121A"
                                border.color: findField.activeFocus ? "#00F0FF" : "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            onAccepted: editorPaneRoot.findNext(text, false)
                        }

                        Button {
                            id: findPrevBtn
                            objectName: "findPrevBtn"
                            text: qsTr("Find Prev")
                            implicitHeight: 24
                            font.pixelSize: 10
                            background: Rectangle {
                                color: findPrevBtn.hovered ? "#00B4F0" : "#08121A"
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
                            onClicked: editorPaneRoot.findNext(findField.text, true)
                        }

                        Button {
                            id: findNextBtn
                            objectName: "findNextBtn"
                            text: qsTr("Find Next")
                            implicitHeight: 24
                            font.pixelSize: 10
                            background: Rectangle {
                                color: findNextBtn.hovered ? "#00B4F0" : "#08121A"
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
                            onClicked: editorPaneRoot.findNext(findField.text, false)
                        }

                        ToolButton {
                            text: "✕"
                            implicitWidth: 24
                            implicitHeight: 24
                            contentItem: Text {
                                text: "✕"
                                color: "#FFFFFF"
                                font.pixelSize: 11
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            background: Rectangle { color: "transparent" }
                            onClicked: findReplaceBar.visible = false
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: qsTr("Replace:")
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                            Layout.preferredWidth: 50
                        }

                        TextField {
                            id: replaceField
                            objectName: "replaceField"
                            Layout.fillWidth: true
                            implicitHeight: 24
                            font.pixelSize: 11
                            color: "#FFFFFF"
                            placeholderText: qsTr("Replacement text...")
                            background: Rectangle {
                                color: "#08121A"
                                border.color: replaceField.activeFocus ? "#00F0FF" : "#1A4A68"
                                border.width: 1
                                radius: 3
                            }
                            onAccepted: editorPaneRoot.replaceCurrent(findField.text, text)
                        }

                        Button {
                            id: replaceBtn
                            objectName: "replaceBtn"
                            text: qsTr("Replace")
                            implicitHeight: 24
                            font.pixelSize: 10
                            background: Rectangle {
                                color: replaceBtn.hovered ? "#00B4F0" : "#08121A"
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
                            onClicked: editorPaneRoot.replaceCurrent(findField.text, replaceField.text)
                        }

                        Button {
                            id: replaceAllBtn
                            objectName: "replaceAllBtn"
                            text: qsTr("Replace All")
                            implicitHeight: 24
                            font.pixelSize: 10
                            background: Rectangle {
                                color: replaceAllBtn.hovered ? "#00B4F0" : "#08121A"
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
                            onClicked: editorPaneRoot.replaceAllOccurrences(findField.text, replaceField.text)
                        }

                        Item { implicitWidth: 24 }
                    }
                }
            }

            // ==========================================
            // 4. EDITOR STATUS BAR
            // ==========================================
            Rectangle {
                id: statusBar
                objectName: "statusBar"
                Layout.fillWidth: true
                implicitHeight: 22
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 3

                readonly property int lineCount: {
                    let txt = rawTextArea.text
                    if (txt.length === 0) return 1
                    return txt.split("\n").length
                }

                readonly property int wordCount: {
                    let trimmed = rawTextArea.text.trim()
                    if (trimmed.length === 0) return 0
                    return trimmed.split(/\s+/).length
                }

                readonly property int charCount: rawTextArea.text.length

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 12

                    Text {
                        text: qsTr("UTF-8")
                        color: "#888888"
                        font.pixelSize: 10
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        id: lineCountText
                        objectName: "lineCountText"
                        text: qsTr("Lines: %1").arg(statusBar.lineCount)
                        color: "#FFFFFF"
                        font.pixelSize: 10
                    }

                    Text {
                        id: wordCountText
                        objectName: "wordCountText"
                        text: qsTr("Words: %1").arg(statusBar.wordCount)
                        color: "#FFFFFF"
                        font.pixelSize: 10
                    }

                    Text {
                        id: charCountText
                        objectName: "charCountText"
                        text: qsTr("Chars: %1").arg(statusBar.charCount)
                        color: "#FFFFFF"
                        font.pixelSize: 10
                    }

                    Rectangle { width: 1; height: 12; color: "#1A4A68" }

                    Text {
                        text: qsTr("Ln 1, Col 1")
                        color: "#00F0FF"
                        font.pixelSize: 10
                        font.bold: true
                    }
                }
            }
        }
    }
}

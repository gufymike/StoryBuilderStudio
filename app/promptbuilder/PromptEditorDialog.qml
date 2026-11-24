// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Window {
    id: promptEditorWindow
    objectName: "promptEditorWindowRoot"
    title: qsTr("Edit Default Prompts - Story Builder Studio")
    width: 720
    height: 540
    minimumWidth: 550
    minimumHeight: 400
    color: "#08121A" // Dark Well
    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.WindowTitleHint

    property string originalContent: ""
    property bool isDirty: promptEditor.text !== originalContent

    // Mock property for isolated unit tests
    property var mockAvailablePrompts: null

    function showCentered(parentWindow) {
        if (parentWindow) {
            promptEditorWindow.transientParent = parentWindow
            promptEditorWindow.x = parentWindow.x + Math.max(20, (parentWindow.width - promptEditorWindow.width) / 2)
            promptEditorWindow.y = parentWindow.y + Math.max(20, (parentWindow.height - promptEditorWindow.height) / 2)
        }

        if (typeof promptManager !== "undefined" && promptManager) {
            promptManager.refreshPrompts()
        }

        promptEditorWindow.show()
        promptEditorWindow.raise()
        promptEditorWindow.requestActivate()
    }

    Connections {
        target: (typeof promptManager !== "undefined") ? promptManager : null
        function onPromptSaved(fullPath) {
            promptEditorWindow.originalContent = promptEditor.text
            statusLabel.text = qsTr("Saved to: ") + fullPath
        }
        function onPromptError(err) {
            statusLabel.text = qsTr("Error: ") + err
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // ==========================================
        // 1. TIER SELECTORS (Category & File)
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: qsTr("Category:")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
                font.bold: true
            }

            ComboBox {
                id: categoryCombo
                objectName: "categoryCombo"
                model: ["text", "image", "audio", "video"]
                Layout.preferredWidth: 120
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Select the model category you want the prompt associated with.")

                Component.onCompleted: {
                    if (typeof promptManager !== "undefined" && promptManager) {
                        let idx = indexOfValue(promptManager.activeCategory)
                        if (idx !== -1) currentIndex = idx
                    }
                }

                onCurrentTextChanged: {
                    if (promptEditorWindow.checkDirtyPromptSwitch(() => {
                        if (typeof promptManager !== "undefined" && promptManager) {
                            promptManager.activeCategory = currentText
                        }
                    })) {
                        if (typeof promptManager !== "undefined" && promptManager) {
                            promptManager.activeCategory = currentText
                        }
                    }
                }
            }

            Text {
                text: qsTr("Prompt File:")
                color: "#FFFFFF"
                style: Text.Outline
                styleColor: "#000000"
                font.pixelSize: 12
                font.bold: true
            }

            ComboBox {
                id: fileCombo
                objectName: "fileCombo"
                Layout.fillWidth: true
                textRole: "label"
                model: promptEditorWindow.mockAvailablePrompts !== null
                       ? promptEditorWindow.mockAvailablePrompts
                       : ((typeof promptManager !== "undefined" && promptManager) ? promptManager.availablePrompts : [])
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Select the prompt you want to edit.")

                onActivated: (index) => {
                    let item = model[index]
                    if (index === 0) {
                        nameRow.visible = true
                        fileNameInput.text = ""
                        promptEditor.text = ""
                        promptEditorWindow.originalContent = ""
                    } else {
                        nameRow.visible = false
                        let content = (typeof promptManager !== "undefined" && promptManager)
                                      ? promptManager.readPromptFile(item.fullPath)
                                      : (item.content || "")
                        promptEditor.text = content
                        promptEditorWindow.originalContent = content
                        fileNameInput.text = item.fileName || ""
                    }
                }
            }
        }

        // ==========================================
        // 2. FILE CREATION & SCOPE DIRECTIVES
        // ==========================================
        RowLayout {
            id: nameRow
            objectName: "nameRow"
            Layout.fillWidth: true
            visible: fileCombo.currentIndex === 0
            spacing: 10

            Text {
                text: qsTr("New Name:")
                color: "#888888"
                font.pixelSize: 12
            }

            TextField {
                id: fileNameInput
                objectName: "fileNameInput"
                Layout.fillWidth: true
                placeholderText: "e.g., Establishing_Shot_Exterior"
                color: "#FFFFFF"
                background: Rectangle {
                    color: "#0B1B26"
                    border.color: fileNameInput.activeFocus ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }

            CheckBox {
                id: seriesScopeCheck
                objectName: "seriesScopeCheck"
                text: qsTr("Series Default (Uncheck for Episode)")
                checked: true
                contentItem: Text {
                    text: seriesScopeCheck.text
                    color: "#FFFFFF"
                    font.pixelSize: 11
                    leftPadding: seriesScopeCheck.indicator.width + 4
                    verticalAlignment: Text.AlignVCenter
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Save for the whole series or only the active episode.")
            }
        }

        // ==========================================
        // 3. PROMPT WORK AREA
        // ==========================================
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            TextArea {
                id: promptEditor
                objectName: "promptEditor"
                color: "#FFFFFF"
                font.family: "Courier New, monospace"
                font.pixelSize: 12
                wrapMode: TextEdit.Wrap
                placeholderText: qsTr("Enter prompt directives, tags, or story grammar here...")
                background: Rectangle {
                    color: "#0B1B26"
                    border.color: promptEditorWindow.isDirty ? "#00F0FF" : "#1A4A68"
                    border.width: 1
                    radius: 4
                }
            }
        }

        // ==========================================
        // 4. STATUS & COMMIT BAR
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                id: statusLabel
                objectName: "statusLabel"
                text: promptEditorWindow.isDirty ? qsTr("● Unsaved changes") : ""
                color: promptEditorWindow.isDirty ? "#00F0FF" : "#888888"
                style: promptEditorWindow.isDirty ? Text.Outline : Text.Normal
                styleColor: "#000000"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            Button {
                id: cancelBtn
                objectName: "cancelBtn"
                text: qsTr("Cancel")
                implicitHeight: 28
                background: Rectangle {
                    color: cancelBtn.down ? "#0077A8" : (cancelBtn.hovered ? "#00B4F0" : "#0B1B26")
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
                onClicked: promptEditorWindow.handleCloseRequest()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Clear prompt and close without saving.")
            }

            Button {
                id: savePromptBtn
                objectName: "savePromptBtn"
                text: qsTr("Save Prompt")
                implicitHeight: 28
                enabled: promptEditor.text.trim().length > 0
                background: Rectangle {
                    color: !savePromptBtn.enabled ? "#1B3B4F" : (savePromptBtn.down ? "#0077A8" : (savePromptBtn.hovered ? "#00B4F0" : "#1A4A68"))
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: savePromptBtn.enabled ? "#FFFFFF" : "#888888"
                    style: savePromptBtn.enabled ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    let fName = fileNameInput.text.trim()
                    if (typeof promptManager !== "undefined" && promptManager) {
                        promptManager.savePrompt(fName, promptEditor.text, categoryCombo.currentText, seriesScopeCheck.checked)
                    }
                    promptEditorWindow.originalContent = promptEditor.text
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Save the current prompt text to the selected file.")
            }
        }
    }

    Dialog {
        id: discardConfirmDialog
        objectName: "discardConfirmDialog"
        title: qsTr("Unsaved Changes")
        modal: true
        implicitWidth: 360
        x: Math.round((promptEditorWindow.width - width) / 2)
        y: Math.round((promptEditorWindow.height - height) / 2)
        standardButtons: Dialog.Save | Dialog.Discard | Dialog.Cancel
        closePolicy: isDirty ? Popup.NoAutoClose : (Popup.CloseOnEscape | Popup.CloseOnPressOutside)

        background: Rectangle {
            implicitWidth: 360
            color: "#08121A"
            border.color: "#1A4A68"
            border.width: 1
            radius: 8
        }

        contentItem: Text {
            text: qsTr("You have unsaved edits. Do you want to save before proceeding?")
            color: "#FFFFFF"
            font.pixelSize: 12
        }

        onAccepted: {
            if (typeof promptManager !== "undefined" && promptManager) {
                promptManager.savePrompt(fileNameInput.text.trim(), promptEditor.text, categoryCombo.currentText, seriesScopeCheck.checked)
            }
            promptEditorWindow.close()
        }
        onDiscarded: {
            promptEditor.text = promptEditorWindow.originalContent
            promptEditorWindow.close()
        }
    }

    onClosing: (close) => {
        if (isDirty) {
            close.accepted = false
            discardConfirmDialog.open()
        }
    }

    function handleCloseRequest() {
        if (isDirty) {
            discardConfirmDialog.open()
        } else {
            promptEditorWindow.close()
        }
    }

    function checkDirtyPromptSwitch(actionCallback) {
        if (isDirty) {
            discardConfirmDialog.open()
            return false
        }
        return true
    }
}

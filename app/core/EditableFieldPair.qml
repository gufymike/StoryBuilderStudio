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
** File: EditableFieldPair.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: fieldRoot
    objectName: "editableFieldPairRoot"
    Layout.fillWidth: true
    spacing: 10

    property string labelText: qsTr("Field:")
    property string fieldValue: ""
    property string toolTipText: ""
    property bool isEditing: false

    signal valueSubmitted(string newValue)

    HoverHandler {
        id: efpHoverHandler
        cursorShape: Qt.PointingHandCursor
    }

    ToolTip.visible: efpHoverHandler.hovered && !editBtn.hovered && toolTipText.length > 0
    ToolTip.text: toolTipText

    // Static Field Label
    Text {
        id: labelDisplay
        objectName: "labelDisplay"
        text: fieldRoot.labelText
        color: "#888888"
        font.pixelSize: 13
        Layout.preferredWidth: 120
    }

    // Display Mode (With black border outline on active color)
    Text {
        id: valueDisplay
        objectName: "valueDisplay"
        visible: !fieldRoot.isEditing
        Layout.fillWidth: true
        text: fieldRoot.fieldValue !== "" ? fieldRoot.fieldValue : qsTr("<Not Set>")
        color: fieldRoot.fieldValue !== "" ? "#FFFFFF" : "#555555"
        style: fieldRoot.fieldValue !== "" ? Text.Outline : Text.Normal
        styleColor: "#000000"
        font.pixelSize: 13
        elide: Text.ElideRight
    }

    // Edit Mode (Input with black outline and #00F0FF highlight)
    TextField {
        id: editField
        objectName: "editField"
        visible: fieldRoot.isEditing
        Layout.fillWidth: true
        text: fieldRoot.fieldValue
        color: "#00F0FF"
        font.pixelSize: 13
        selectByMouse: true
        background: Rectangle {
            color: "#08121A" // Dark Well
            border.color: editField.activeFocus ? "#00F0FF" : "#1A4A68" // Highlight / Accent
            border.width: 1
            radius: 4
        }

        onAccepted: {
            fieldRoot.valueSubmitted(text)
            fieldRoot.isEditing = false
        }

        Keys.onEscapePressed: {
            text = fieldRoot.fieldValue
            fieldRoot.isEditing = false
        }
    }

    // Action Button with color palette states
    Button {
        id: editBtn
        objectName: "editBtn"
        text: fieldRoot.isEditing ? qsTr("Save") : qsTr("Edit")
        font.pixelSize: 11
        Layout.preferredWidth: 60

        contentItem: Text {
            text: editBtn.text
            font: editBtn.font
            color: "#FFFFFF"
            style: Text.Outline
            styleColor: "#000000"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            color: editBtn.down ? "#0077A8" : (editBtn.hovered ? "#00B4F0" : "#1A4A68")
            border.color: "#000000"
            border.width: 1
            radius: 4
        }

        onClicked: {
            if (fieldRoot.isEditing) {
                fieldRoot.valueSubmitted(editField.text)
                fieldRoot.isEditing = false
            } else {
                editField.text = fieldRoot.fieldValue
                fieldRoot.isEditing = true
                editField.forceActiveFocus()
                editField.selectAll()
            }
        }

        ToolTip.visible: hovered
        ToolTip.text: fieldRoot.isEditing
            ? qsTr("Click to save edits to %1") + fieldRoot.labelText
            : qsTr("Click to edit field %1") + fieldRoot.labelText
    }
}

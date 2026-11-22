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
** File: PromptOptionsPanel.qml
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import StoryBuilderStudio

Rectangle {
    id: promptOptionsPanelRoot
    objectName: "promptOptionsPanelRoot"
    color: "#08121A"
    border.color: "#1A4A68"
    border.width: 1
    radius: 6

    property string currentProjectPath: ""
    property string targetCategory: "text" // "text", "image", "audio", "video"
    property var activeSelectedList: []

    signal promptOptionToggled(string optionContent, bool isChecked)

    // Fallback/Test Mock Data
    property var mockCategoriesData: null

    readonly property var defaultCategories: ["text", "image", "audio", "video"]

    property var activeCategories: {
        if (mockCategoriesData !== null) {
            return Object.keys(mockCategoriesData)
        }
        return defaultCategories
    }

    function getFiles(category) {
        if (mockCategoriesData !== null) {
            return mockCategoriesData[category] || []
        }
        //console.log("[PromptOptionsPanel.qml] Getting files for : " + category)
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            if (typeof promptManager.getFilesForCategory === "function") {
                return promptManager.getFilesForCategory(category)
            }
            // Fallback to checking availablePrompts
            let items = promptManager.availablePrompts || []
            let fileList = []
            for (let i = 0; i < items.length; ++i) {
                if (items[i].category === category && items[i].fileName) {
                    fileList.push(items[i].fileName)
                }
            }
            //console.log("[PromptOptionsPanel.qml] found " + fileList.length + " files")
            return fileList
        }
        return []
    }

    function loadContent(category, fileName) {
        if (mockCategoriesData !== null) return fileName
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            let cat = (category === "script") ? "text" : category
            let items = promptManager.availablePrompts || []
            for (let i = 0; i < items.length; ++i) {
                if (items[i].fileName === fileName && items[i].category === cat) {
                    return promptManager.readPromptFile(items[i].fullPath)
                }
            }
        }
        return fileName
    }

    function updateProjectPath(seriesPath, episodePath) {
        currentProjectPath = seriesPath
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            promptManager.setPaths(seriesPath, episodePath || "")
        }
    }

    ScrollView {
        anchors.fill: parent
        anchors.margins: 8
        clip: true

        RowLayout {
            id: categoriesRow
            objectName: "categoriesRow"
            spacing: 16

            Repeater {
                id: categoryRepeater
                objectName: "categoryRepeater"
                model: promptOptionsPanelRoot.activeCategories

                delegate: ColumnLayout {
                    id: categoryColumn
                    objectName: "categoryColumn_" + modelData
                    property string categoryName: modelData
                    spacing: 6

                    // Highlight the column header corresponding to the active tab
                    Text {
                        text: (categoryColumn.categoryName.charAt(0).toUpperCase() + categoryColumn.categoryName.slice(1)) +
                              (categoryColumn.categoryName === promptOptionsPanelRoot.targetCategory ? " ★" : "")
                        color: categoryColumn.categoryName === promptOptionsPanelRoot.targetCategory ? "#00F0FF" : "#888888"
                        style: Text.Outline
                        styleColor: "#000000"
                        font.bold: true
                        font.pixelSize: 11
                    }

                    ListView {
                        id: fileListView
                        objectName: "fileListView_" + modelData
                        implicitWidth: 160
                        implicitHeight: 120
                        clip: true
                        model: promptOptionsPanelRoot.getFiles(categoryColumn.categoryName)
                        spacing: 2

                        delegate: CheckBox {
                            id: optionCheckBox
                            objectName: "optionCheckBox_" + index
                            width: fileListView.width
                            height: 24
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Add %1 to prompt").arg(modelData)
                            property string cachedContent: ""

                            Component.onCompleted: {
                                let loaded = promptOptionsPanelRoot.loadContent(categoryColumn.categoryName, modelData)
                                cachedContent = loaded.trim().length > 0 ? loaded.trim() : modelData
                                checked = promptOptionsPanelRoot.activeSelectedList.indexOf(cachedContent) !== -1
                            }

                            contentItem: Text {
                                text: modelData
                                color: optionCheckBox.checked ? "#00F0FF" : (optionCheckBox.hovered ? "#FFFFFF" : "#CCCCCC")
                                style: optionCheckBox.checked ? Text.Outline : Text.Normal
                                styleColor: "#000000"
                                font.pixelSize: 11
                                leftPadding: optionCheckBox.indicator.width + 6
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            indicator: Rectangle {
                                implicitWidth: 16
                                implicitHeight: 16
                                x: 2
                                y: parent.height / 2 - height / 2
                                radius: 3
                                color: "#0B1B26"
                                border.color: optionCheckBox.checked ? "#00F0FF" : "#1A4A68"
                                border.width: 1

                                Rectangle {
                                    width: 8
                                    height: 8
                                    anchors.centerIn: parent
                                    radius: 2
                                    color: "#00F0FF"
                                    visible: optionCheckBox.checked
                                }
                            }

                            onCheckedChanged: {
                                promptOptionsPanelRoot.promptOptionToggled(cachedContent, checked)
                            }
                        }
                    }
                }
            }
        }
    }
}

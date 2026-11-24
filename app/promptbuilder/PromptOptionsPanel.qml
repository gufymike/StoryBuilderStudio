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
import QtCore
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
    property string currentEpisodePath: ""
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
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            if (typeof promptManager.getFilesForCategory === "function") {
                return promptManager.getFilesForCategory(category)
            }
            let items = promptManager.availablePrompts || []
            let fileList = []
            for (let i = 0; i < items.length; ++i) {
                if (items[i].category === category && items[i].fileName) {
                    fileList.push(items[i].fileName)
                }
            }
            return fileList
        }
        return []
    }

    function loadContent(category, fileName) {
        if (mockCategoriesData !== null) return fileName
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            let cat = (category === "script") ? "text" : category

            // 1. Check active availablePrompts list first
            let items = promptManager.availablePrompts || []
            for (let i = 0; i < items.length; ++i) {
                if (items[i].fileName === fileName && items[i].category === cat && items[i].fullPath) {
                    let content = promptManager.readPromptFile(items[i].fullPath)
                    if (content && content.trim().length > 0) return content
                }
            }

            // 2. Check Episode-scoped prompt directory
            if (currentEpisodePath && currentEpisodePath.length > 0) {
                let epContent = promptManager.readPromptFile(currentEpisodePath + "/prompts/" + cat + "/" + fileName)
                if (epContent && epContent.trim().length > 0) return epContent
            }

            // 3. Check Series-scoped prompt directory
            if (currentProjectPath && currentProjectPath.length > 0) {
                let seriesContent = promptManager.readPromptFile(currentProjectPath + "/prompts/" + cat + "/" + fileName)
                if (seriesContent && seriesContent.trim().length > 0) return seriesContent
            }

            // 4. Read directly from embedded QRC system prompts
            let sysContent = promptManager.readPromptFile(":/prompts/" + cat + "/" + fileName)
            if (sysContent && sysContent.trim().length > 0) return sysContent
        }
        return fileName
    }

    function updateProjectPath(seriesPath, episodePath) {
        currentProjectPath = seriesPath || ""
        currentEpisodePath = episodePath || ""
        if (typeof promptManager !== "undefined" && promptManager !== null) {
            promptManager.setPaths(currentProjectPath, currentEpisodePath)
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
                            ToolTip.text: cachedContent !== modelData
                                          ? cachedContent
                                          : qsTr("Add %1 to prompt").arg(modelData)

                            property string cachedContent: ""
                            property bool isInitialized: false

                            function refreshResolvedContent() {
                                let loaded = promptOptionsPanelRoot.loadContent(categoryColumn.categoryName, modelData)
                                cachedContent = (loaded && loaded.trim().length > 0) ? loaded.trim() : modelData
                            }

                            Component.onCompleted: {
                                refreshResolvedContent()
                                checked = promptOptionsPanelRoot.activeSelectedList.indexOf(cachedContent) !== -1
                                isInitialized = true
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
                                if (!isInitialized) return
                                if (cachedContent === modelData) {
                                    refreshResolvedContent()
                                }
                                promptOptionsPanelRoot.promptOptionToggled(cachedContent, checked)
                            }
                        }
                    }
                }
            }
        }
    }
}

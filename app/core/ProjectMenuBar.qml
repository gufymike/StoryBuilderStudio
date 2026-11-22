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
** File: app/core/ProjectMenuBar.qml
** Top-level application menu bar using Action definitions bound to MenuItems.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import StoryBuilderStudio

MenuBar {
    id: projectMenuBar
    objectName: "projectMenuBarRoot"
    implicitHeight: 30

    // --- State Gating Properties ---
    property bool isSeriesOpen: false
    property bool isEpisodeOpen: false
    property bool isDocumentLoaded: false

    // --- Series Signals (File Menu) ---
    signal newSeriesTriggered()
    signal openSeriesTriggered()
    signal closeSeriesTriggered()
    signal exitTriggered()

    // --- Sub-Project / Episode Signals (Project Menu) ---
    signal newEpisodeTriggered()
    signal closeEpisodeTriggered()

    // --- Script Document Signals ---
    signal openScriptTriggered()
    signal saveScriptTriggered()
    signal closeScriptTriggered()
    signal recentDocumentTriggered(string filePath)
    signal closeDocumentTriggered()
    signal saveDocumentTriggered()
    signal openDocumentTriggered()
    signal newDocumentTriggered()

    // --- Options Signals ---
    signal settingsTriggered()

    // --- Help Signals ---
    signal documentationTriggered()
    signal openManualBrowserTriggered()
    signal aboutTriggered()
    signal promptHistoryTriggered()

    function formatMnemonic(rawText) {
        if (!rawText) return ""
        let ampIndex = rawText.indexOf("&")
        if (ampIndex === -1 || ampIndex === rawText.length - 1) {
            return rawText
        }
        let mnChar = rawText.charAt(ampIndex + 1)
        return rawText.substring(0, ampIndex) + "<u>" + mnChar + "</u>" + rawText.substring(ampIndex + 2)
    }

    background: Rectangle {
        color: "#08121A"
        border.color: "#1A4A68"
        border.width: 1
    }

    delegate: MenuBarItem {
        id: customMenuBarItem
        implicitHeight: 28
        implicitWidth: textLabel.implicitWidth + 24
        hoverEnabled: true

        background: Rectangle {
            color: customMenuBarItem.highlighted || (customMenuBarItem.menu && customMenuBarItem.menu.visible)
                   ? "#0077A8"
                   : (customMenuBarItem.hovered ? "#1A4A68" : "transparent")
            radius: 2
        }

        contentItem: Text {
            id: textLabel
            text: projectMenuBar.formatMnemonic(customMenuBarItem.text)
            textFormat: Text.StyledText
            font.pixelSize: 12
            font.bold: customMenuBarItem.highlighted || (customMenuBarItem.menu && customMenuBarItem.menu.visible)
            color: customMenuBarItem.enabled ? "#FFFFFF" : "#666666"
            style: Text.Outline
            styleColor: "#000000"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    // ==========================================
    // 1. FILE: ROOT WORKSPACE SCOPE
    // ==========================================
    Menu {
        id: fileMenu
        objectName: "fileMenu"
        title: qsTr("&File")

        background: Rectangle {
            implicitWidth: 230
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
        }

        Action {
            id: newSeriesAction
            objectName: "newSeriesAction"
            text: qsTr("&New Series...")
            shortcut: "Ctrl+Shift+N"
            onTriggered: projectMenuBar.newSeriesTriggered()
        }

        Action {
            id: openSeriesAction
            objectName: "openSeriesAction"
            text: qsTr("&Open Series...")
            shortcut: "Ctrl+Shift+O"
            onTriggered: projectMenuBar.openSeriesTriggered()
        }

        Action {
            id: closeSeriesAction
            objectName: "closeSeriesAction"
            text: qsTr("&Close Series")
            enabled: projectMenuBar.isSeriesOpen
            onTriggered: projectMenuBar.closeSeriesTriggered()
        }

        Action {
            id: exitAction
            objectName: "exitAction"
            text: qsTr("E&xit")
            shortcut: StandardKey.Quit
            onTriggered: projectMenuBar.exitTriggered()
        }

        MenuItem { action: newSeriesAction; objectName: "newSeriesMenuItem" }
        MenuItem { action: openSeriesAction; objectName: "openSeriesMenuItem" }

        Menu {
            id: recentFilesSubMenu
            objectName: "recentFilesSubMenu"
            title: qsTr("Open &Recent")
            enabled: typeof settingsManager !== "undefined" && settingsManager !== null &&
                     settingsManager.recentFiles && settingsManager.recentFiles.length > 0

            background: Rectangle {
                implicitWidth: 240
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
            }

            Instantiator {
                model: (typeof settingsManager !== "undefined" && settingsManager !== null)
                       ? settingsManager.recentFiles : []
                onObjectAdded: (index, object) => recentFilesSubMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => recentFilesSubMenu.removeItem(object)

                delegate: MenuItem {
                    text: modelData
                    onTriggered: projectMenuBar.recentDocumentTriggered(modelData)
                }
            }
        }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: "#1A4A68" }
        }

        MenuItem { action: closeSeriesAction; objectName: "closeSeriesMenuItem" }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: "#1A4A68" }
        }

        MenuItem { action: exitAction; objectName: "exitMenuItem" }
    }

    // ==========================================
    // 2. PROJECT: EPISODES & SCRIPT DOCUMENTS
    // ==========================================
    Menu {
        id: projectMenu
        objectName: "projectMenu"
        title: qsTr("&Project")
        enabled: projectMenuBar.isSeriesOpen

        background: Rectangle {
            implicitWidth: 260
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
        }

        Action {
            id: newEpisodeAction
            objectName: "newEpisodeAction"
            text: qsTr("&New Episode / Chapter...")
            shortcut: "Ctrl+N"
            enabled: projectMenuBar.isSeriesOpen
            onTriggered: projectMenuBar.newEpisodeTriggered()
        }

        Action {
            id: closeEpisodeAction
            objectName: "closeEpisodeAction"
            text: qsTr("&Close Episode")
            enabled: projectMenuBar.isEpisodeOpen
            onTriggered: projectMenuBar.closeEpisodeTriggered()
        }

        Action {
            id: promptHistoryAction
            objectName: "promptHistoryAction"
            text: qsTr("Prompt & Thought History...")
            shortcut: "Ctrl+H"
            enabled: projectMenuBar.isEpisodeOpen
            onTriggered: projectMenuBar.promptHistoryTriggered()
        }

        // MenuItem { action: newEpisodeAction; objectName: "newEpisodeMenuItem" }

        Menu {
            id: openEpisodeSubMenu
            objectName: "openEpisodeSubMenu"
            title: qsTr("&Open Episode")
            enabled: projectMenuBar.isSeriesOpen && (typeof seriesContainer !== "undefined" && seriesContainer && seriesContainer.subProjectCount > 0)

            background: Rectangle {
                implicitWidth: 200
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
            }

            Instantiator {
                model: (typeof seriesContainer !== "undefined" && seriesContainer) ? seriesContainer.subProjectCount : 0
                onObjectAdded: (index, object) => openEpisodeSubMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => openEpisodeSubMenu.removeItem(object)

                delegate: MenuItem {
                    text: {
                        if (typeof seriesContainer !== "undefined" && seriesContainer && typeof seriesContainer.getSubProjectTitle === "function") {
                            let title = seriesContainer.getSubProjectTitle(index);
                            return title.length > 0 ? title : (qsTr("Episode ") + (index + 1));
                        }
                        return qsTr("Episode ") + (index + 1);
                    }
                    onTriggered: {
                        if (typeof seriesContainer !== "undefined" && seriesContainer && typeof projectManager !== "undefined") {
                            let epPath = seriesContainer.getSubProjectAbsolutePath(index)
                            if (epPath.length > 0) {
                                projectManager.loadProject(epPath)
                            }
                        }
                    }
                }
            }
        }

        MenuItem { action: closeEpisodeAction; objectName: "closeEpisodeMenuItem" }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: "#1A4A68" }
        }

        MenuItem { action: promptHistoryAction; objectName: "promptHistoryMenuItem" }
    }

    // ==========================================
    // 3. OPTIONS: CONFIGURATION & PREFERENCES
    // ==========================================
    Menu {
        id: optionsMenu
        objectName: "optionsMenu"
        title: qsTr("&Options")

        background: Rectangle {
            implicitWidth: 190
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
        }

        Action {
            id: settingsAction
            objectName: "settingsAction"
            text: qsTr("&Settings...")
            shortcut: "Ctrl+Comma"
            onTriggered: projectMenuBar.settingsTriggered()
        }

        // MenuItem { action: settingsAction; objectName: "settingsMenuItem" }
    }

    // ==========================================
    // 4. HELP: ABOUT & SYSTEM DOCUMENTATION
    // ==========================================
    Menu {
        id: helpMenu
        objectName: "helpMenu"
        title: qsTr("&Help")

        background: Rectangle {
            implicitWidth: 270
            color: "#0B1B26"
            border.color: "#1A4A68"
            border.width: 1
            radius: 4
        }

        Action {
            id: documentationAction
            objectName: "documentationAction"
            text: qsTr("Documentation & Manual...")
            shortcut: "F1"
            onTriggered: projectMenuBar.documentationTriggered()
        }

        Action {
            id: openManualBrowserAction
            objectName: "openManualBrowserAction"
            text: qsTr("Open Manual in External Browser (HTML)")
            onTriggered: projectMenuBar.openManualBrowserTriggered()
        }

        Action {
            id: aboutAction
            objectName: "aboutAction"
            text: qsTr("&About Story Builder Studio...")
            onTriggered: projectMenuBar.aboutTriggered()
        }

        // MenuItem { action: documentationAction; objectName: "documentationMenuItem" }
        // MenuItem { action: openManualBrowserAction; objectName: "openManualBrowserMenuItem" }

        MenuSeparator {
            contentItem: Rectangle { implicitHeight: 1; color: "#1A4A68" }
        }

        // MenuItem { action: aboutAction; objectName: "aboutMenuItem" }
    }
}

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
** File: AssetContextMenu.qml
** Reusable Context Menu for external tool dispatch, asset deletion,
** and direct Storyboard shot assignment.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import StoryBuilderStudio

Menu {
    id: assetMenuRoot
    objectName: "assetContextMenuRoot"

    property string targetFilePath: ""
    property string assetType: "audio" // "image", "audio", "video"

    // Testing / Headless injection hooks
    property var mockShots: null
    property var mockTools: null

    // Mapping helper to align with StoryBoardManager categories ("images", "audio", "videos")
    readonly property string storyboardCategory: {
        if (assetType === "image") return "images"
        if (assetType === "video") return "videos"
        return "audio"
    }

    readonly property var effectiveShots: {
        if (mockShots !== null) return mockShots
        if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null && storyBoardManager.hasValidPath) {
            return storyBoardManager.shots || []
        }
        return []
    }

    readonly property var effectiveTools: {
        if (mockTools !== null) return mockTools
        if (typeof settingsManager !== "undefined" && settingsManager !== null) {
            return settingsManager.getExternalToolsForType(assetMenuRoot.assetType) || []
        }
        return []
    }

    // Submenu: Attach directly to an existing Storyboard Shot
    Menu {
        id: storyboardSubMenu
        objectName: "storyboardSubMenu"
        title: qsTr("Attach to Storyboard Shot")
        enabled: assetMenuRoot.effectiveShots.length > 0

        Instantiator {
            id: shotRepeater
            model: assetMenuRoot.effectiveShots

            delegate: MenuItem {
                text: qsTr("Shot %1: %2").arg((index + 1).toString().padStart(2, '0')).arg(modelData.name || qsTr("Untitled"))
                onTriggered: {
                    let cleanPath = assetMenuRoot.targetFilePath.replace(/^file:\/\//i, "")
                    if (Qt.platform.os === "windows" && cleanPath.startsWith("/")) cleanPath = cleanPath.substring(1)
                    if (typeof storyBoardManager !== "undefined" && storyBoardManager !== null) {
                        storyBoardManager.addAssetToShot(index, assetMenuRoot.storyboardCategory, cleanPath)
                    }
                }
            }

            onObjectAdded: (index, object) => storyboardSubMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => storyboardSubMenu.removeItem(object)
        }
    }

    MenuSeparator {}

    // Submenu: Open In External Tool
    Menu {
        id: externalToolsSubMenu
        objectName: "externalToolsSubMenu"
        title: qsTr("Open with External Tool")
        enabled: assetMenuRoot.effectiveTools.length > 0

        Instantiator {
            id: toolRepeater
            model: assetMenuRoot.effectiveTools

            delegate: MenuItem {
                text: modelData.name || ""
                onTriggered: {
                    if (typeof settingsManager !== "undefined" && settingsManager !== null) {
                        settingsManager.launchToolByName(modelData.name, assetMenuRoot.targetFilePath)
                    }
                }
            }

            // Fixed: Target externalToolsSubMenu directly instead of fragile children[1] index
            onObjectAdded: (index, object) => externalToolsSubMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => externalToolsSubMenu.removeItem(object)
        }
    }

    MenuSeparator {}

    MenuItem {
        id: deleteAssetItem
        objectName: "deleteAssetItem"
        text: qsTr("Delete Asset")
        onTriggered: {
            if (typeof projectManager !== "undefined" && projectManager !== null) {
                projectManager.deleteAsset(assetMenuRoot.targetFilePath)
            }
        }
    }
}

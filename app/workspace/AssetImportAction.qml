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
** File: AssetImportAction.qml
** Reusable media file dialog and importer for asset models.
**
****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import StoryBuilderStudio

Item {
    id: importActionRoot
    objectName: "assetImportActionRoot"

    property var targetModel: null
    property string dialogTitle: qsTr("Import Media Asset(s)")
    property var nameFilters: ["All Files (*)"]

    signal assetsImported(var files)

    function importFiles(fileList) {
        if (!fileList || fileList.length === 0) return

        let sanitizedPaths = []
        for (let i = 0; i < fileList.length; ++i) {
            let item = fileList[i]
            let pathStr = (typeof item === "string") ? item : item.toString()

            // Strip file:/// or file:// URL schemes
            pathStr = pathStr.replace(/^file:\/\//i, "")
            // On Windows, fix leading slash from file:///C:/ -> /C:/ -> C:/
            if (Qt.platform.os === "windows" && pathStr.startsWith("/") && pathStr.length > 2 && pathStr.charAt(2) === ":") {
                pathStr = pathStr.substring(1)
            }

            sanitizedPaths.push(pathStr)
        }

        if (targetModel && typeof targetModel.importAssets === "function") {
            targetModel.importAssets(sanitizedPaths)
        }
        importActionRoot.assetsImported(sanitizedPaths)
    }

    FileDialog {
        id: fileDialog
        objectName: "fileDialog"
        title: importActionRoot.dialogTitle
        fileMode: FileDialog.OpenFiles
        nameFilters: importActionRoot.nameFilters

        onAccepted: {
            importActionRoot.importFiles(selectedFiles)
        }
    }

    function open() {
        fileDialog.open()
    }
}

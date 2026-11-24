// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

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

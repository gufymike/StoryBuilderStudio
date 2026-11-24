// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore
import StoryBuilderStudio

Dialog {
    id: helpDialog
    objectName: "helpManualDialogRoot"
    title: qsTr("Story Builder Studio Documentation & Reference Hub")
    modal: true
    focus: true
    width: 820
    height: 620
    anchors.centerIn: parent

    background: Rectangle {
        color: "#08121A" // Dark Well
        border.color: "#1A4A68" // Accent Border
        border.width: 1
        radius: 8
    }

    header: Rectangle {
        implicitHeight: 48
        color: "#132B3A" // Active Tile Bar
        border.color: "#1A4A68"
        border.width: 1
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 10

            Text {
                id: headerTitleText
                objectName: "headerTitleText"
                text: "📖 " + helpDialog.title
                color: "#00F0FF"
                style: Text.Outline
                styleColor: "#000000"
                font.bold: true
                font.pixelSize: 14
            }

            Item { Layout.fillWidth: true }

            Button {
                id: openBrowserBtn
                objectName: "openBrowserBtn"
                text: qsTr("🌐 Open Full Manual in Browser")
                font.pixelSize: 11
                font.bold: true
                implicitHeight: 28
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Opens the full manual in an external browser.")

                background: Rectangle {
                    color: openBrowserBtn.down ? "#0077A8" : (openBrowserBtn.hovered ? "#00B4F0" : "#1A4A68")
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    let appDir = StandardPaths.writableLocation(StandardPaths.AppDataLocation)
                    let manualUrl = "file:///" + (typeof applicationDirPath !== "undefined" ? applicationDirPath.replace(/\\/g, "/") : appDir) + "/docs/index.html"
                    Qt.openUrlExternally(manualUrl)
                }
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 10

        // Navigation Bar Tabs
        TabBar {
            id: helpTabBar
            objectName: "helpTabBar"
            Layout.fillWidth: true
            implicitHeight: 32

            background: Rectangle {
                color: "#0B1B26"
                border.color: "#1A4A68"
                border.width: 1
                radius: 4
            }

            TabButton {
                id: tab0
                objectName: "tabOverview"
                text: qsTr("Overview & Pipelines")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("A quick overview of the app architecture and models.")
            }

            TabButton {
                id: tab1
                objectName: "tabDirectories"
                text: qsTr("Directory Architecture")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Where the studio discovers and loads models.")
            }

            TabButton {
                id: tab2
                objectName: "tabPrompts"
                text: qsTr("Prompts & Ledger")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Audit trail ledger and reasoning prompt storage.")
            }

            TabButton {
                id: tab3
                objectName: "tabShortcuts"
                text: qsTr("Shortcuts & Runtime")
                implicitHeight: 32
                contentItem: Text {
                    text: parent.text
                    color: parent.checked ? "#00F0FF" : "#888888"
                    style: parent.checked ? Text.Outline : Text.Normal
                    styleColor: "#000000"
                    font.bold: parent.checked
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.checked ? "#08121A" : "transparent"
                    border.color: parent.checked ? "#00F0FF" : "transparent"
                    border.width: 1
                    radius: 3
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Common shortcut keys and runtime diagnostic tips.")
            }
        }

        // Tab Content Pages
        StackLayout {
            id: helpStackLayout
            objectName: "helpStackLayout"
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: helpTabBar.currentIndex

            // PAGE 0: Overview & Generation Engines
            ScrollView {
                clip: true
                TextArea {
                    id: page0TextArea
                    objectName: "page0TextArea"
                    readOnly: true
                    textFormat: Text.MarkdownText
                    color: "#D0D0D0"
                    font.pixelSize: 13
                    wrapMode: TextArea.Wrap
                    background: null
                    text: "### Story Builder Studio Architecture & Modalities\n\n" +
                          "Story Builder Studio operates offline-first by dispatching tasks to dedicated C++ runtime engines:\n\n" +
                          "* **Narrative / Screenplay:** Local Qwen GGUF via `llama.cpp` using ChatML formatting, or Gemini Bridge cloud API.\n" +
                          "* **Visual Artwork:** FLUX.1 Schnell image diffusion via `stable-diffusion.cpp` producing 4-sample candidate grids.\n" +
                          "* **Video Generation:** Wan 2.1 Diffusion Transformer (DiT). Supports pure Text-to-Video (T2V) or Image-to-Video (I2V) by attaching reference PNG/JPG frames.\n" +
                          "* **Speech Synthesis (TTS):** Kokoro multi-speaker engine via `sherpa-onnx`. Supports reading attached `.txt` and `.md` scene scripts with seamless multi-line concatenation.\n" +
                          "* **Sound Effects & Foley:** GGUF sound generation via `audio.cpp` with non-blocking seek waveform playback."
                }
            }

            // PAGE 1: Directories
            ScrollView {
                clip: true
                TextArea {
                    id: page1TextArea
                    objectName: "page1TextArea"
                    readOnly: true
                    textFormat: Text.MarkdownText
                    color: "#D0D0D0"
                    font.pixelSize: 13
                    wrapMode: TextArea.Wrap
                    background: null
                    text: "### Application Data & Model Hierarchy\n\n" +
                          "By default, models are discovered in `%APPDATA%\\storyBuilderStudio\\`:\n\n" +
                          "```text\n" +
                          "storyBuilderStudio/\n" +
                          "├── text/       -> Qwen3-8B GGUF weights & tokenizer.json\n" +
                          "├── images/     -> flux/ (ae.safetensors, clip_l, t5-encoder, schnell)\n" +
                          "├── video/      -> wan/ (DiT GGUF, VAE, UMT5 text encoder)\n" +
                          "└── audio/\n" +
                          "    ├── speech/ -> Kokoro/ (model.onnx, voices.bin, tokens, espeak-ng-data)\n" +
                          "    ├── sfx/    -> audio.cpp GGUF sound models\n" +
                          "    └── bgm/    -> Background score models\n" +
                          "```\n\n" +
                          "Active project files and episodes store generated outputs directly in `<ProjectRoot>/assets/`."
                }
            }

            // PAGE 2: Prompts & Audit Ledger
            ScrollView {
                clip: true
                TextArea {
                    id: page2TextArea
                    objectName: "page2TextArea"
                    readOnly: true
                    textFormat: Text.MarkdownText
                    color: "#D0D0D0"
                    font.pixelSize: 13
                    wrapMode: TextArea.Wrap
                    background: null
                    text: "### Audit Trail & Shared UUID Synchronization\n\n" +
                          "* **Audit Ledger:** Every prompt dispatch across text, image, audio, and video is permanently captured in `audit_trail.json` with execution latency, timestamps, and parameters.\n" +
                          "* **Prompt vs Thought Stores:** Clean model replies are recorded in `prompt_history.json`. Internal chain-of-thought (`<think>` tags) is stripped and stored in `think_history.json`.\n" +
                          "* **Shared UUID:** Both JSON records share the exact same UTC ISO-8601 millisecond timestamp (`YYYY-MM-DDTHH:mm:ss.zzzZ`), allowing dual-view UIs to join prompt completions with reasoning steps."
                }
            }

            // PAGE 3: Shortcuts & Runtime Setup
            ScrollView {
                clip: true
                TextArea {
                    id: page3TextArea
                    objectName: "page3TextArea"
                    readOnly: true
                    textFormat: Text.MarkdownText
                    color: "#D0D0D0"
                    font.pixelSize: 13
                    wrapMode: TextArea.Wrap
                    background: null
                    text: "### Keyboard Shortcuts & Diagnostic Tips\n\n" +
                          "* **Ctrl+N:** New Script Document\n" +
                          "* **Ctrl+O:** Open Script Document\n" +
                          "* **Ctrl+S:** Save Active Script\n" +
                          "* **Ctrl+F / Ctrl+H:** Find and Replace Panel\n" +
                          "* **Ctrl+B / Ctrl+I:** Markdown Bold / Italic wrappers\n\n" +
                          "**Engine Diagnostics:**\n" +
                          "If an engine fails to load, open the **Model Settings** panel and verify that all manifest entries are highlighted green. On Windows portable deployments, ensure that `libomp.dll`, `libc++.dll`, and `libunwind.dll` remain in `bin/` alongside the executable."
                }
            }
        }
    }

    footer: Rectangle {
        implicitHeight: 48
        color: "#132B3A"
        border.color: "#1A4A68"
        border.width: 1
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10

            Item { Layout.fillWidth: true }

            Button {
                id: closeHelpBtn
                objectName: "closeHelpBtn"
                text: qsTr("Close")
                implicitHeight: 28
                Layout.preferredWidth: 90

                background: Rectangle {
                    color: closeHelpBtn.down ? "#0077A8" : (closeHelpBtn.hovered ? "#00B4F0" : "#1A4A68")
                    border.color: "#000000"
                    border.width: 1
                    radius: 4
                }
                contentItem: Text {
                    text: parent.text
                    color: "#FFFFFF"
                    style: Text.Outline
                    styleColor: "#000000"
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: helpDialog.close()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Dismiss the manual dialog")
            }
        }
    }
}

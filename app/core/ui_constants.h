// Copyright (c) 2026 Mike Ramirez
// SPDX-License-Identifier: MIT

#pragma once

namespace UI {

// start CORE fields.
namespace EditableFieldPair {
constexpr const char* Root              = "editableFieldPairRoot";
constexpr const char* LabelDisplay      = "labelDisplay";
constexpr const char* ValueDisplay      = "valueDisplay";
constexpr const char* EditField         = "editField";
constexpr const char* EditBtn           = "editBtn";
}

namespace EpisodeListCard {
constexpr const char* Root              = "episodeListCardRoot";
constexpr const char* HeaderTitle       = "cardHeaderTitle";
constexpr const char* CountBadge        = "countBadge";
constexpr const char* CountText         = "countText";
constexpr const char* NewEpisodeBtn     = "epNewEpisodeBtn";
constexpr const char* ListView          = "episodeListView";
constexpr const char* EmptyPlaceholder  = "emptyListPlaceholder";
}

namespace ProjectDetailsCard {
constexpr const char* Root              = "projectDetailsCardRoot";
constexpr const char* HeaderTitle       = "headerTitleText";
constexpr const char* VersionText       = "versionText";
constexpr const char* FormContainer     = "formContainer";
constexpr const char* EpNameField       = "epNameField";
constexpr const char* DescField         = "descField";
constexpr const char* WriterField       = "writerField";
constexpr const char* ContactField      = "contactField";
constexpr const char* CreatedDateText   = "createdDateText";
constexpr const char* EditedDateText    = "editedDateText";
}

namespace SeriesHeaderCard {
constexpr const char* Root              = "seriesHeaderCardRoot";
constexpr const char* HeaderTitle       = "headerTitleText";
constexpr const char* EpisodeBadge      = "episodeBadge";
constexpr const char* BadgeText         = "badgeText";
constexpr const char* FormLayout        = "formLayout";
constexpr const char* NewEpisodeBtn     = "newEpisodeBtn";
constexpr const char* SeriesTitlePair   = "seriesTitlePair";
constexpr const char* ShortDescPair     = "shortDescPair";
constexpr const char* CreatorPair       = "creatorPair";
constexpr const char* CoCreatorsPair    = "coCreatorsPair";
constexpr const char* EmailPair         = "emailPair";
constexpr const char* ManifestPathText  = "manifestPathText";
constexpr const char* CreatedDateText       = "createdDateText";
}

namespace SeriesLauncherModal {
constexpr const char* Root              = "launcherDialogRoot";
constexpr const char* ModalTitle        = "modalTitleText";
constexpr const char* ModalSubtitle     = "modalSubtitleText";
constexpr const char* ListView          = "seriesListView";
constexpr const char* EmptyStateLabel   = "emptyStateLabel";
constexpr const char* NewSeriesBtn      = "newSeriesBtn";
constexpr const char* BrowseSeriesBtn   = "browseSeriesBtn";
}
namespace NewProjectDialog {
constexpr const char* Root              = "newProjectDialogRoot";
constexpr const char* HeaderTitle       = "headerTitle";
constexpr const char* ProjNameField     = "projNameField";
constexpr const char* AuthorField       = "authorField";
constexpr const char* EmailField        = "emailField";
constexpr const char* AuthorsListArea   = "authorsListArea";
constexpr const char* ShortDescField    = "shortDescField";
constexpr const char* FullDescArea      = "fullDescArea";
constexpr const char* VersionField      = "versionField";
constexpr const char* ProgressPopup     = "progressPopup";
}

namespace NewSeriesDialog {
constexpr const char* Root              = "newSeriesDialogRoot";
constexpr const char* HeaderTitle       = "headerTitle";
constexpr const char* SeriesTitleField  = "seriesTitleField";
constexpr const char* LocationField     = "locationField";
constexpr const char* BrowseBtn         = "browseBtn";
constexpr const char* ShortDescField    = "shortDescField";
constexpr const char* LongDescField     = "longDescField";
constexpr const char* CreateEpisodeBox  = "createEpisodeCheckbox";
}

namespace OpenProjectDialog {
constexpr const char* Root              = "openProjectPopupRoot";
constexpr const char* HeaderTitle       = "headerTitle";
constexpr const char* ContentStack      = "contentStack";
constexpr const char* ProjectListView   = "projectListView";
constexpr const char* EmptyStateLayout  = "emptyStateLayout";
constexpr const char* EmptyStateLabel   = "emptyStateLabel";
constexpr const char* CreateNewBtn      = "createNewProjectBtn";
constexpr const char* CancelBtn         = "cancelBtn";
}

namespace ProjectMenuBar {
constexpr const char* Root              = "projectMenuBarRoot";
constexpr const char* FileMenu          = "fileMenu";
constexpr const char* ProjectMenu       = "projectMenu";
constexpr const char* OptionsMenu       = "optionsMenu";
constexpr const char* HelpMenu          = "helpMenu";
constexpr const char* NewSeriesAct      = "newSeriesAction";
constexpr const char* OpenSeriesAct     = "openSeriesAction";
constexpr const char* CloseSeriesAct    = "closeSeriesAction";
constexpr const char* ExitAct           = "exitAction";
constexpr const char* RecentFilesMenu   = "recentFilesSubMenu";
constexpr const char* NewEpisodeAct     = "newEpisodeAction";
constexpr const char* CloseEpisodeAct   = "closeEpisodeAction";
constexpr const char* PromptHistoryAct  = "promptHistoryAction";
constexpr const char* OpenEpisodeMenu   = "openEpisodeSubMenu";
constexpr const char* SettingsAct       = "settingsAction";
constexpr const char* DocumentationAct  = "documentationAction";
constexpr const char* OpenManualAct     = "openManualBrowserAction";
constexpr const char* AboutAct          = "aboutAction";
}

namespace FirstRunWizardModal {
constexpr const char* Root              = "firstRunWizardModalRoot";
constexpr const char* WizardStack       = "wizardStackLayout";
constexpr const char* Step0Header       = "step0Header";
constexpr const char* ModelsDirField    = "modelsDirField";
constexpr const char* T5Field           = "t5Field";
constexpr const char* ScriptSummary     = "scriptSummaryText";
constexpr const char* T5StatusText      = "t5StatusText";
constexpr const char* Step1Header       = "step1Header";
constexpr const char* Step2Header       = "step2Header";
constexpr const char* SummarySeriesText = "summarySeriesText";
constexpr const char* BackStepBtn       = "backStepBtn";
constexpr const char* NextStepBtn       = "nextStepBtn";
}

namespace HelpManualDialog {
constexpr const char* Root              = "helpManualDialogRoot";
constexpr const char* HeaderTitle       = "headerTitleText";
constexpr const char* OpenBrowserBtn    = "openBrowserBtn";
constexpr const char* HelpTabBar        = "helpTabBar";
constexpr const char* HelpStackLayout   = "helpStackLayout";
constexpr const char* Page0TextArea     = "page0TextArea";
constexpr const char* Page1TextArea     = "page1TextArea";
constexpr const char* CloseHelpBtn      = "closeHelpBtn";
}
// end CORE


// start SETTINGS field
namespace ManifestCard {
constexpr const char* EmptyLabel        = "emptyManifestLabel";
constexpr const char* StatusIconText    = "statusIconText";
constexpr const char* Root              = "LocalModalManifestCardRoot";
constexpr const char* Repeater          = "manifestRepeater";
constexpr const char* StatusBadge       = "statusSummaryBadge";
}

namespace RemoteConfig {
constexpr const char* CustomEnvSection  = "customEnvSection";
constexpr const char* CustomEnvField    = "customEnvField";
constexpr const char* SetupGuideBtn     = "setupGuideBtn";
constexpr const char* Root              = "remoteApiConfigCardRoot";
constexpr const char* StatusBanner      = "statusBanner";
constexpr const char* VerifyBtn         = "verifyConnectionBtn";
constexpr const char* BaseUrlField      = "baseUrlField";
constexpr const char* ModelField        = "modelField";
constexpr const char* BannerTitle       = "bannerTitleText";
constexpr const char* BannerSubtitle    = "bannerSubtitleText";
}

namespace Settings {
constexpr const char* PanelTitle        = "panelTitleText";
constexpr const char* ClosePanelToolBtn = "closePanelToolBtn";
constexpr const char* ResetDefaultsBtn  = "resetDefaultsBtn";
constexpr const char* RescanWeightsBtn  = "rescanWeightsBtn";
constexpr const char* CloseBtn          = "closeBtn";
constexpr const char* Root              = "settingsControlPanelRoot";
constexpr const char* TabBar            = "settingsTabBar";
constexpr const char* Stack             = "settingsStack";
constexpr const char* TabScript         = "tabScript";
constexpr const char* TabImage          = "tabImage";
constexpr const char* TabVideo          = "tabVideo";
constexpr const char* TabAudio          = "tabAudio";

constexpr const char* ScriptDriverBox   = "scriptDriverBox";
constexpr const char* ImageDriverBox    = "imageDriverBox";
constexpr const char* VideoDriverBox    = "videoDriverBox";
constexpr const char* AudioDriverBox    = "audioDriverBox";

constexpr const char* RemoteScriptCard  = "remoteScriptConfigCard";
constexpr const char* RemoteImageCard  = "remoteImageConfigCard";
constexpr const char* RemoteAudioCard  = "remoteAudioConfigCard";
constexpr const char* RemoteVideoCard  = "remoteVideoConfigCard";
}
// end SETTINGS

// start ENGINE field
namespace ProviderSelector {
constexpr const char* Root              = "providerSelectorWidgetRoot";
constexpr const char* ProviderCombo     = "providerCombo";
constexpr const char* LocalIndicator    = "localModelIndicator";
constexpr const char* LocalModelText    = "localModelText";
constexpr const char* GeminiIndicator   = "geminiBridgeIndicator";
constexpr const char* CustomIndicator   = "customEndpointIndicator";
constexpr const char* AddFormBox        = "addFormBox";
constexpr const char* NewNameField      = "newNameField";
constexpr const char* NewUrlField       = "newUrlField";
constexpr const char* SaveCustomBtn     = "saveCustomBtn";
constexpr const char* CancelCustomBtn   = "cancelCustomBtn";
constexpr const char* CustomUrlText     = "customUrlText";
}

namespace InferenceEstimateDialog {
constexpr const char* Root              = "inferenceEstimateDialogRoot";
constexpr const char* HeaderTitle       = "headerTitleText";
constexpr const char* HeaderSubtitle    = "headerSubtitleText";
constexpr const char* TotalRuntimeText  = "totalRuntimeText";
constexpr const char* RamCardRect       = "ramCardRect";
constexpr const char* PeakRamText       = "peakRamText";
constexpr const char* TotalDiskText     = "totalDiskText";
constexpr const char* BreakdownRepeater = "breakdownRepeater";
constexpr const char* EmptyBreakdownLbl = "emptyBreakdownLabel";
}

namespace InferenceSettingsPanel {
constexpr const char* Root              = "inferenceSettingsPanelRoot";
constexpr const char* HeaderTitle       = "headerTitleText";
constexpr const char* ResetDefaultsBtn  = "resetDefaultsBtn";
constexpr const char* ParamsScrollView  = "paramsScrollView";
constexpr const char* OptionsRepeater   = "optionsRepeater";
}
// end ENGINE

// start PROMPTBUILDER fields
namespace PromptBuilder {
constexpr const char* Root              = "promptBuilderRoot";
constexpr const char* Splitter          = "promptBuilderSplitter";
constexpr const char* UpperResponsePane = "upperResponsePane";
constexpr const char* LowerInputPane    = "lowerInputPane";
constexpr const char* DriverSelectorBox = "driverSelectorBox";
constexpr const char* ModelSelectorBox  = "modelSelectorBox";
constexpr const char* SubmitButton      = "submitButton";
constexpr const char* WorkspaceStack    = "workspaceStack";
constexpr const char* PromptTextArea    = "promptTextArea";
constexpr const char* SettingsTabBtn    = "settingsTabBtn";
constexpr const char* PromptTabBtn      = "promptTabBtn";
constexpr const char* DriverLoadBtn      = "driverLoadBtn";
constexpr const char* ProfilerBtn        = "profilerBtn";
constexpr const char* ResponseDisplay    = "responseDisplayArea";
constexpr const char* ToggleDrawerBtn    = "toggleDrawerBtn";
constexpr const char* EditDefaultsBtn    = "editDefaultsBtn";
constexpr const char* ClearInputBtn      = "clearInputBtn";
constexpr const char* OptionsPanel       = "promptOptionsPanel";
constexpr const char* AttachFileBtn      = "attachFileBtn";
constexpr const char* SettingsPanel      = "inferenceSettingsPanel";
constexpr const char* SubmitPromptBtn    = "submitPromptBtn";
constexpr const char* EditorWindow       = "editorWindow";
constexpr const char* SubmitPreviewPopup = "submitPreviewPopup";
constexpr const char* PreviewTextArea    = "previewTextArea";
constexpr const char* CancelDispatchBtn  = "cancelDispatchBtn";
constexpr const char* ConfirmDispatchBtn = "confirmDispatchBtn";
}

namespace PromptEditorDialog {
constexpr const char* Root              = "promptEditorWindowRoot";
constexpr const char* CategoryCombo     = "categoryCombo";
constexpr const char* FileCombo         = "fileCombo";
constexpr const char* NameRow           = "nameRow";
constexpr const char* FileNameInput     = "fileNameInput";
constexpr const char* SeriesScopeCheck  = "seriesScopeCheck";
constexpr const char* PromptEditor      = "promptEditor";
constexpr const char* StatusLabel       = "statusLabel";
}

namespace PromptHistoryViewerDialog {
constexpr const char* Root              = "promptHistoryViewerWindowRoot";
constexpr const char* PromptHeader      = "promptHeaderDisplay";
constexpr const char* ResponseArea      = "responseArea";
constexpr const char* ViewerStack       = "viewerStack";
constexpr const char* BtnViewThink      = "btnViewThink";
constexpr const char* BtnViewCombined   = "btnViewCombined";
}

namespace PromptOptionsPanel {
constexpr const char* Root              = "promptOptionsPanelRoot";
constexpr const char* CategoryRepeater  = "categoryRepeater";
}
// end PROMPTBUILDER

// start STORYBOARD fields
namespace StoryBoardAssetList {
constexpr const char* Root              = "storyBoardAssetListRoot";
constexpr const char* ListView          = "attachedAssetsListView";
constexpr const char* EmptyLabel        = "emptyLabel";
constexpr const char* AttachAssetBtn    = "attachAssetBtn";
}

namespace StoryBoardAssetPickerModal {
constexpr const char* Root              = "storyBoardAssetPickerModalRoot";
constexpr const char* ScriptModel       = "scriptModel";
constexpr const char* ImageModel        = "imageModel";
constexpr const char* AudioModel        = "audioModel";
constexpr const char* VideoModel        = "videoModel";
constexpr const char* HeaderTitleText   = "headerTitleText";
constexpr const char* ClosePickerBtn    = "closePickerBtn";
constexpr const char* AvailableAssetsList = "availableAssetsListView";
constexpr const char* EmptyLibraryText  = "emptyLibraryText";
}

namespace StoryBoardCard {
constexpr const char* Root              = "storyBoardCardRoot";
constexpr const char* OrderBadgeText    = "orderBadgeText";
constexpr const char* ShotTitleText     = "shotTitleText";
constexpr const char* ScriptCountText   = "scriptCountText";
constexpr const char* ImgCountText      = "imgCountText";
constexpr const char* AudioCountText    = "audioCountText";
constexpr const char* VideoCountText    = "videoCountText";
constexpr const char* MoveUpBtn         = "moveUpBtn";
constexpr const char* MoveDownBtn       = "moveDownBtn";
constexpr const char* DeleteBtn         = "deleteBtn";
}

namespace StoryBoardShotDetailDialog {
constexpr const char* Root              = "storyBoardShotDetailWindow";
constexpr const char* ShotNameField     = "shotNameField";
constexpr const char* ScriptLinkDisplay = "scriptLinkDisplay";
constexpr const char* RenameBtn         = "renameBtn";
constexpr const char* AssetStack        = "assetStack";
constexpr const char* AssetTabBar       = "assetTabBar";
constexpr const char* CloseDialogBtn    = "closeDialogBtn";
}

namespace StoryBoardWorkspaceView {
constexpr const char* Root              = "storyBoardWorkspaceViewRoot";
constexpr const char* ViewHeaderTitle   = "viewHeaderTitle";
constexpr const char* ShotStatsText     = "shotStatsText";
constexpr const char* StatusText        = "statusText";
constexpr const char* AddShotBtn        = "addShotBtn";
constexpr const char* AddShotDialog     = "addShotDialog";
constexpr const char* DialogShotNameField = "dialogShotNameField";
}
// end STORYBOARD

//start WORKSPACE fields
namespace AudioWorkspaceWidget {
constexpr const char* Root              = "audioWorkspaceWidgetRoot";
constexpr const char* TrackStatusText   = "trackStatusText";
constexpr const char* SharedContextMenu = "sharedContextMenu";
constexpr const char* MediaPlayer       = "mediaPlayer";
constexpr const char* AudioWaveformModel= "audioWaveformModel";
}

namespace EditorPane {
constexpr const char* Root              = "editorPaneRoot";
constexpr const char* RawTextArea       = "rawTextArea";
constexpr const char* TitleDisplay      = "titleDisplay";
constexpr const char* EditorStack       = "editorStack";
constexpr const char* MdModeBtn         = "mdModeBtn";
constexpr const char* HtmlModeBtn       = "htmlModeBtn";
constexpr const char* RawModeBtn        = "rawModeBtn";
constexpr const char* LineCountText     = "lineCountText";
constexpr const char* WordCountText     = "wordCountText";
constexpr const char* CharCountText     = "charCountText";
}
namespace ImageCandidateModal {
constexpr const char* Root              = "imageCandidateModalRoot";
constexpr const char* DiscardAllBtn     = "discardAllBtn";
constexpr const char* EmptyLabel        = "emptyCandidatesLabel";
constexpr const char* CandidateGrid     = "candidateGrid";
constexpr const char* CandidateRepeater = "candidateRepeater";
}

namespace ImageGalleryWidget {
constexpr const char* Root              = "imageGalleryWidgetRoot";
constexpr const char* ImportImagesBtn   = "importImagesBtn";
constexpr const char* RefreshGalleryBtn = "refreshGalleryBtn";
constexpr const char* SelectionStatsText= "selectionStatsText";
constexpr const char* SelectAllBtn      = "selectAllBtn";
constexpr const char* DeselectAllBtn    = "deselectAllBtn";
constexpr const char* ImageGridView     = "imageGridView";
}

namespace VideoWorkspaceWidget {
constexpr const char* Root              = "videoWorkspaceWidgetRoot";
constexpr const char* VideoPlayer       = "videoPlayer";
constexpr const char* ImportVideoBtn    = "importVideoBtn";
constexpr const char* RefreshVideoBtn   = "refreshVideoBtn";
constexpr const char* PlayStatusText    = "playStatusText";
constexpr const char* EmptyViewportText = "emptyViewportText";
constexpr const char* TimeDisplay       = "timeDisplay";
constexpr const char* ScrubSlider       = "scrubSlider";
}

namespace AssetContextMenu {
constexpr const char* Root                 = "assetContextMenuRoot";
constexpr const char* StoryboardSubMenu    = "storyboardSubMenu";
constexpr const char* ExternalToolsSubMenu = "externalToolsSubMenu";
constexpr const char* DeleteAssetItem      = "deleteAssetItem";
}

namespace AssetWorkspaceSplitView {
constexpr const char* Root                 = "assetWorkspaceSplitViewRoot";
constexpr const char* TogglePromptPaneBtn  = "togglePromptPaneBtn";
constexpr const char* MainSplitView        = "mainSplitView";
constexpr const char* LeftPaneContainer    = "leftPaneContainer";
constexpr const char* RightPaneContainer   = "rightPaneContainer";
constexpr const char* PromptBuilderInstance= "promptBuilderInstance";
}

namespace WorkspaceView {
constexpr const char* Root                 = "workspaceViewRoot";
constexpr const char* AppTitleText         = "appTitleText";
constexpr const char* EpisodeStatusText    = "episodeStatusText";
constexpr const char* WorkspaceTabBar      = "workspaceTabBar";
constexpr const char* WorkspaceStack       = "workspaceStack";
constexpr const char* ProjectTabBtn        = "projectTabBtn";
constexpr const char* StoryboardTabBtn     = "storyboardTabBtn";
constexpr const char* ScriptTabBtn         = "scriptTabBtn";
constexpr const char* ImageTabBtn          = "imageTabBtn";
constexpr const char* AudioTabBtn          = "audioTabBtn";
constexpr const char* VideoTabBtn          = "videoTabBtn";
constexpr const char* ScriptWorkspace      = "scriptWorkspace";
constexpr const char* ImageWorkspace       = "imageWorkspace";
constexpr const char* AudioWorkspace       = "audioWorkspace";
constexpr const char* VideoWorkspace       = "videoWorkspace";
}
// end WORKSPACE


// start MAIN fields
namespace MainView {
constexpr const char* Root                 = "rootApplicationWindow";
constexpr const char* OpenProjectDialog    = "openProjectDialog";
constexpr const char* NewProjectDialog     = "newProjectDialog";
constexpr const char* NewSeriesDialog      = "newSeriesDialog";
constexpr const char* FirstRunWizard       = "firstRunWizard";
constexpr const char* SeriesLauncherModal  = "seriesLauncherModal";
constexpr const char* LocalFileLoader      = "localFileLoader";
constexpr const char* LocalFileSaver       = "localFileSaver";
constexpr const char* SettingsControlPanel = "settingsControlPanel";
constexpr const char* ThemeRestartNotice   = "themeRestartNotice";
constexpr const char* AppMenuBar           = "appMenuBar";
constexpr const char* CentralWorkspaceView = "centralWorkspaceView";
constexpr const char* StatusBanner         = "statusBanner";
constexpr const char* StatusBannerText     = "statusBannerText";
constexpr const char* AboutDialog          = "aboutDialog";
constexpr const char* AboutTitleText       = "aboutTitleText";
}
// end MAIN
} // end UI
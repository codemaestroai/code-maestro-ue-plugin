#pragma once

/**
 * Registers an "Explain with CM" button into three asset-editor toolbars:
 *   Blueprint Editor, Widget Blueprint (UMG) Editor, Material Editor.
 *
 * Each button emits a push_event back to CM Desktop via
 * FCodeMaestroBridgeModule::SendEvent("explain_asset", <payload>). CM Desktop
 * opens a new chat tab prefilled with a reference to the edited asset.
 *
 * Step 8 scope: menu-name probe only. Click handlers log which asset kind was
 * clicked and do not yet resolve the asset or call SendEvent. Step 9 fills
 * in the payload-building logic once we've confirmed the three toolbar menus
 * are reachable on the running engine version.
 */
class FAssetEditorExtension
{
public:
    /** Register the buttons. Call from StartupModule. */
    static void Register();

    /** Unregister the buttons. Call from ShutdownModule. */
    static void Unregister();

private:
    static void ExtendToolbars();
    static void ExtendSingleToolbar(const FName MenuName, const FName AssetKind);
    static void OnExplainClicked(const struct FToolMenuContext& MenuContext, FName AssetKind);
};

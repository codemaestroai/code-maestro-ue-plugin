#include "UI/AssetEditorExtension.h"
#include "UI/CodeMaestroBridgeStyle.h"
#include "CodeMaestroBridgeModule.h"
#include "ToolMenus.h"
#include "ToolMenuContext.h"
#include "ToolMenuDelegates.h"
#include "Framework/Commands/UIAction.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Misc/App.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// Editor context classes + editor interfaces we reach through them.
#include "BlueprintEditorContext.h"
#include "WidgetBlueprintToolMenuContext.h"
#include "WidgetBlueprintEditor.h"
#include "WidgetBlueprint.h"
#include "MaterialEditorContext.h"
#include "IMaterialEditor.h"
#include "Engine/Blueprint.h"
#include "Materials/MaterialInterface.h"

static const FName AssetEditorOwnerName("CodeMaestroBridgeAssetEditor");

// UE 5.7 asset-editor toolbar menu names. If any of these is wrong on a given
// engine version, ExtendSingleToolbar logs a warning. See PLUGIN-DISTRIBUTION.md Appendix A.
static const FName BlueprintToolbarMenu("AssetEditor.BlueprintEditor.ToolBar");
static const FName WidgetBlueprintToolbarMenu("AssetEditor.WidgetBlueprintEditor.ToolBar");
static const FName MaterialToolbarMenu("AssetEditor.MaterialEditor.ToolBar");

// Asset kind tags — used as button names, log tags, and the asset_kind field
// in the explain_asset push_event payload.
static const FName KindBlueprint("blueprint");
static const FName KindWidgetBlueprint("widget_blueprint");
static const FName KindMaterial("material");

void FAssetEditorExtension::Register()
{
    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateStatic(&FAssetEditorExtension::ExtendToolbars));
}

void FAssetEditorExtension::Unregister()
{
    if (UToolMenus* Menus = UToolMenus::TryGet())
    {
        Menus->UnregisterOwner(AssetEditorOwnerName);
    }
}

void FAssetEditorExtension::ExtendToolbars()
{
    ExtendSingleToolbar(BlueprintToolbarMenu, KindBlueprint);
    ExtendSingleToolbar(WidgetBlueprintToolbarMenu, KindWidgetBlueprint);
    ExtendSingleToolbar(MaterialToolbarMenu, KindMaterial);
}

void FAssetEditorExtension::ExtendSingleToolbar(const FName MenuName, const FName AssetKind)
{
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(MenuName);
    if (!ToolbarMenu)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CMBridge] Could not find toolbar menu '%s' to extend (Explain with CM button for '%s' will not appear)"),
            *MenuName.ToString(), *AssetKind.ToString());
        return;
    }

    FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("CodeMaestro");

    const FName EntryName(*FString::Printf(TEXT("CMExplain_%s"), *AssetKind.ToString()));

    // FToolMenuExecuteAction receives the FToolMenuContext at click time so we
    // can resolve the exact edited asset for the specific editor window. AssetKind
    // is a bound payload so a single static handler covers all three asset kinds.
    FToolUIAction UIAction;
    UIAction.ExecuteAction = FToolMenuExecuteAction::CreateStatic(
        &FAssetEditorExtension::OnExplainClicked, AssetKind);

    Section.AddEntry(
        FToolMenuEntry::InitToolBarButton(
            EntryName,
            FToolUIActionChoice(UIAction),
            FText::FromString(TEXT("Explain")),
            FText::FromString(TEXT("Open a CodeMaestro chat prefilled with this asset")),
            FSlateIcon(FCodeMaestroBridgeStyle::GetStyleSetName(), "CodeMaestro.LogoSquare")
        )
    );

    UE_LOG(LogTemp, Log, TEXT("[CMBridge] Registered 'Explain with CM' in %s"), *MenuName.ToString());
}

// Resolve the edited UObject from the menu context for each asset kind. Returns
// nullptr if the expected context object is missing or its weak editor pointer
// has expired.
static UObject* ResolveEditedAsset(const FToolMenuContext& MenuContext, FName AssetKind)
{
    if (AssetKind == KindBlueprint)
    {
        if (UBlueprintEditorToolMenuContext* Ctx = MenuContext.FindContext<UBlueprintEditorToolMenuContext>())
        {
            return Ctx->GetBlueprintObj();
        }
    }
    else if (AssetKind == KindWidgetBlueprint)
    {
        if (UWidgetBlueprintToolMenuContext* Ctx = MenuContext.FindContext<UWidgetBlueprintToolMenuContext>())
        {
            if (TSharedPtr<FWidgetBlueprintEditor> Editor = Ctx->WidgetBlueprintEditor.Pin())
            {
                return Editor->GetWidgetBlueprintObj();
            }
        }
        // Widget BP editors also push a UBlueprintEditorToolMenuContext (since
        // FWidgetBlueprintEditor inherits FBlueprintEditor) — fall back to it.
        if (UBlueprintEditorToolMenuContext* BpCtx = MenuContext.FindContext<UBlueprintEditorToolMenuContext>())
        {
            return BpCtx->GetBlueprintObj();
        }
    }
    else if (AssetKind == KindMaterial)
    {
        if (UMaterialEditorMenuContext* Ctx = MenuContext.FindContext<UMaterialEditorMenuContext>())
        {
            if (TSharedPtr<IMaterialEditor> Editor = Ctx->MaterialEditor.Pin())
            {
                return Editor->GetMaterialInterface();
            }
        }
    }
    return nullptr;
}

void FAssetEditorExtension::OnExplainClicked(const FToolMenuContext& MenuContext, FName AssetKind)
{
    UObject* Asset = ResolveEditedAsset(MenuContext, AssetKind);
    if (!Asset)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[CMBridge] Explain with CM: could not resolve asset from menu context (kind=%s)"),
            *AssetKind.ToString());
        return;
    }

    const FString AssetName = Asset->GetName();
    const FString AssetPath = Asset->GetPathName();
    const FString ProjectName = FApp::GetProjectName();

    TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
    Payload->SetStringField(TEXT("asset_kind"), AssetKind.ToString());
    Payload->SetStringField(TEXT("asset_name"), AssetName);
    Payload->SetStringField(TEXT("asset_path"), AssetPath);
    Payload->SetStringField(TEXT("project_name"), ProjectName);

    FString PayloadJson;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&PayloadJson);
    FJsonSerializer::Serialize(Payload.ToSharedRef(), Writer);

    const int Result = FCodeMaestroBridgeModule::Get().SendEvent(TEXT("explain_asset"), PayloadJson);
    UE_LOG(LogTemp, Log,
        TEXT("[CMBridge] explain_asset event sent: kind=%s name=%s path=%s result=%d"),
        *AssetKind.ToString(), *AssetName, *AssetPath, Result);
}

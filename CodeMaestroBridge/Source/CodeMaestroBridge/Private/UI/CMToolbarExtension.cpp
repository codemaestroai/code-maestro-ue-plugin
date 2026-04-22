#include "UI/CMToolbarExtension.h"
#include "UI/SCMStatusWidget.h"
#include "CodeMaestroBridgeModule.h"
#include "ToolMenus.h"

static const FName CMToolbarOwnerName("CodeMaestroBridge");

FCodeMaestroBridgeModule* FCMToolbarExtension::CachedModule = nullptr;

void FCMToolbarExtension::Register(FCodeMaestroBridgeModule* Module)
{
    CachedModule = Module;

    UToolMenus::RegisterStartupCallback(
        FSimpleMulticastDelegate::FDelegate::CreateStatic(&FCMToolbarExtension::ExtendToolbar));
}

void FCMToolbarExtension::ExtendToolbar()
{
    if (!CachedModule)
    {
        return;
    }

    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(
        "LevelEditor.LevelEditorToolBar.PlayToolBar");

    if (!ToolbarMenu)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CMBridge] Could not find PlayToolBar menu to extend"));
        return;
    }

    FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("CodeMaestro");

    Section.AddEntry(
        FToolMenuEntry::InitWidget(
            "CMStatusWidget",
            SNew(SCMStatusWidget)
                .Module(CachedModule),
            FText::GetEmpty()
        )
    );

    UE_LOG(LogTemp, Log, TEXT("[CMBridge] Toolbar status widget registered"));
}

void FCMToolbarExtension::Unregister()
{
    CachedModule = nullptr;

    if (UToolMenus* Menus = UToolMenus::TryGet())
    {
        Menus->UnregisterOwner(CMToolbarOwnerName);
    }
}

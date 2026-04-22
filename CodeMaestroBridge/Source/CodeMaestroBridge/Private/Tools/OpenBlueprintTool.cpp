#include "CodeMaestroBridgeModule.h"
#include "codemaestro_runtime.h"
#include "Async/Async.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "BlueprintEditor.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"

static FString BlueprintToJson(UBlueprint* Blueprint)
{
    FString Name = Blueprint->GetName().Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
    FString Path = Blueprint->GetPathName().Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
    FString Parent = (Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None"))
        .Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));

    return FString::Printf(
        TEXT(R"({"blueprint_name":"%s","blueprint_path":"%s","parent_class":"%s"})"),
        *Name, *Path, *Parent);
}

void OnGetOpenBlueprint(const char* request_id, const char* args_json, void* userdata)
{
    if (FCodeMaestroBridgeModule::bShuttingDown) return;

    FString RequestId(request_id);

    AsyncTask(ENamedThreads::GameThread, [RequestId]() {
        if (FCodeMaestroBridgeModule::bShuttingDown) return;

        FString Result = TEXT(R"({"blueprint_name":null,"blueprint_path":null,"parent_class":null})");

        if (GEditor)
        {
            UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
            if (AssetEditorSubsystem)
            {
                TArray<UObject*> EditedAssets = AssetEditorSubsystem->GetAllEditedAssets();

                UBlueprint* FocusedBlueprint = nullptr;
                UBlueprint* FallbackBlueprint = nullptr;

                for (UObject* Asset : EditedAssets)
                {
                    UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
                    if (!Blueprint) continue;

                    // Remember the first Blueprint as fallback
                    if (!FallbackBlueprint)
                    {
                        FallbackBlueprint = Blueprint;
                    }

                    // Check if this Blueprint's editor tab is the foreground tab
                    IAssetEditorInstance* EditorInstance = AssetEditorSubsystem->FindEditorForAsset(Blueprint, false);
                    if (!EditorInstance) continue;

                    FAssetEditorToolkit* Toolkit = static_cast<FAssetEditorToolkit*>(EditorInstance);
                    TSharedPtr<FTabManager> TabManager = Toolkit->GetTabManager();
                    if (!TabManager.IsValid()) continue;

                    TSharedPtr<SDockTab> OwnerTab = TabManager->GetOwnerTab();
                    if (OwnerTab.IsValid() && OwnerTab->IsForeground())
                    {
                        FocusedBlueprint = Blueprint;
                        break;
                    }
                }

                UBlueprint* ChosenBlueprint = FocusedBlueprint ? FocusedBlueprint : FallbackBlueprint;
                if (ChosenBlueprint)
                {
                    Result = BlueprintToJson(ChosenBlueprint);
                }
            }
        }

        FCodeMaestroBridgeModule::ToolRespond(TCHAR_TO_UTF8(*RequestId), TCHAR_TO_UTF8(*Result));
    });
}

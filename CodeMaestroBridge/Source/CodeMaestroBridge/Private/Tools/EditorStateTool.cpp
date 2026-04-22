#include "CodeMaestroBridgeModule.h"
#include "codemaestro_runtime.h"
#include "Async/Async.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "EngineUtils.h"
#if WITH_LIVE_CODING
#include "ILiveCodingModule.h"
#endif

void OnGetEditorState(const char* request_id, const char* args_json, void* userdata)
{
    if (FCodeMaestroBridgeModule::bShuttingDown) return;

    FString RequestId(request_id);

    AsyncTask(ENamedThreads::GameThread, [RequestId]() {
        if (FCodeMaestroBridgeModule::bShuttingDown) return;

        // Project name
        FString ProjectName = FApp::GetProjectName();

        // Content directory
        FString ContentDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());

        // Current map and actor count
        FString MapName = TEXT("");
        int32 ActorCount = 0;
        UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        if (World)
        {
            MapName = World->GetMapName();
            for (TActorIterator<AActor> It(World); It; ++It)
            {
                ++ActorCount;
            }
        }

        // Play state
        FString PlayState = TEXT("stopped");
        if (GEditor)
        {
            if (GEditor->IsSimulateInEditorInProgress())
            {
                PlayState = TEXT("simulating");
            }
            else if (GEditor->PlayWorld)
            {
                PlayState = GEditor->PlayWorld->IsPaused() ? TEXT("paused") : TEXT("playing");
            }
        }

        // Compilation status
        bool bIsCompiling = false;
#if WITH_LIVE_CODING
        ILiveCodingModule* LiveCoding = FModuleManager::GetModulePtr<ILiveCodingModule>(TEXT("LiveCoding"));
        if (LiveCoding)
        {
            bIsCompiling = LiveCoding->IsCompiling();
        }
#endif

        // Escape strings for JSON
        FString EscapedProjectName = ProjectName.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
        FString EscapedContentDir = ContentDir.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
        FString EscapedMapName = MapName.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));

        FString Result = FString::Printf(
            TEXT(R"({"project_name":"%s","content_dir":"%s","map_name":"%s","actor_count":%d,"play_state":"%s","is_compiling":%s})"),
            *EscapedProjectName,
            *EscapedContentDir,
            *EscapedMapName,
            ActorCount,
            *PlayState,
            bIsCompiling ? TEXT("true") : TEXT("false")
        );

        FCodeMaestroBridgeModule::ToolRespond(TCHAR_TO_UTF8(*RequestId), TCHAR_TO_UTF8(*Result));
    });
}

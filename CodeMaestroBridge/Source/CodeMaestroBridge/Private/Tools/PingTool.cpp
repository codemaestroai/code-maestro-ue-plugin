#include "CodeMaestroBridgeModule.h"
#include "codemaestro_runtime.h"
#include "Async/Async.h"
#include "Misc/EngineVersion.h"
#include "Misc/DateTime.h"

void OnPing(const char* request_id, const char* args_json, void* userdata)
{
    if (FCodeMaestroBridgeModule::bShuttingDown) return;

    FString RequestId(request_id);

    AsyncTask(ENamedThreads::GameThread, [RequestId]() {
        if (FCodeMaestroBridgeModule::bShuttingDown) return;

        int64 Timestamp = FDateTime::UtcNow().ToUnixTimestamp();
        FString EngineVersion = FEngineVersion::Current().ToString();

        FString Result = FString::Printf(
            TEXT(R"({"status":"ok","timestamp":%lld,"engine_version":"%s"})"),
            Timestamp,
            *EngineVersion
        );

        FCodeMaestroBridgeModule::ToolRespond(TCHAR_TO_UTF8(*RequestId), TCHAR_TO_UTF8(*Result));
    });
}

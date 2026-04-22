#include "CodeMaestroBridgeModule.h"
#include "codemaestro_runtime.h"
#include "Editor.h"
#include "UI/CMToolbarExtension.h"
#include "UI/AssetEditorExtension.h"
#include "UI/CodeMaestroBridgeStyle.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Async/Async.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/EngineVersion.h"

// Forward declarations for tool callbacks (defined in Private/Tools/*.cpp)
void OnPing(const char* request_id, const char* args_json, void* userdata);
void OnGetEditorState(const char* request_id, const char* args_json, void* userdata);
void OnGetOpenBlueprint(const char* request_id, const char* args_json, void* userdata);

// Static members
FCodeMaestroBridgeModule::cm_tool_respond_fn FCodeMaestroBridgeModule::ToolRespond = nullptr;
FCodeMaestroBridgeModule::cm_tool_error_fn FCodeMaestroBridgeModule::ToolError = nullptr;
std::atomic<bool> FCodeMaestroBridgeModule::bShuttingDown{false};

// Expected API version — increment when C API changes
static constexpr int EXPECTED_API_VERSION = 1;

IMPLEMENT_MODULE(FCodeMaestroBridgeModule, CodeMaestroBridge)

void FCodeMaestroBridgeModule::StartupModule()
{
    bShuttingDown = false;

    // Register Slate style set (brushes for plugin-owned icons like the CM logo).
    // Must happen before any widget that references the style is constructed.
    FCodeMaestroBridgeStyle::Initialize();

    // Step 1: Locate and load DLL
    if (!LoadDll()) return;

    // Step 2: Check version
    int major = 0, minor = 0, patch = 0;
    Fn_cm_get_version(&major, &minor, &patch);
    if (major != EXPECTED_API_VERSION)
    {
        BridgeStatus = EBridgeStatus::VersionMismatch;
        UE_LOG(LogTemp, Warning, TEXT("[CMBridge] CodeMaestroRuntime version mismatch: expected api %d, got %d"), EXPECTED_API_VERSION, major);
        // Still register toolbar so user sees the warning
        FCMToolbarExtension::Register(this);
        return;
    }

    BridgeStatus = EBridgeStatus::Active;

    // Step 3: Init
    // TCHAR_TO_UTF8 returns a temporary — we must keep conversions alive until
    // cm_init copies them into std::string.  Use named FTCHARToUTF8 objects
    // so the char* pointers remain valid through the cm_init call.
    CMConfig Config{};
    FString ProjectName(FApp::GetProjectName());
    FString LoginId = FPlatformMisc::GetLoginId();
    FString ContentPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
    FString EngVer = FString::Printf(TEXT("%d.%d"), FEngineVersion::Current().GetMajor(), FEngineVersion::Current().GetMinor());

    auto ProjectNameUtf8 = FTCHARToUTF8(*ProjectName);
    auto LoginIdUtf8 = FTCHARToUTF8(*LoginId);
    auto ContentPathUtf8 = FTCHARToUTF8(*ContentPath);
    auto EngVerUtf8 = FTCHARToUTF8(*EngVer);

    Config.project_name = ProjectNameUtf8.Get();
    Config.login_id = LoginIdUtf8.Get();
    Config.content_path = ContentPathUtf8.Get();
    Config.engine_version = EngVerUtf8.Get();
    Config.plugin_version = "0.1.0";
    Config.on_state_changed = &FCodeMaestroBridgeModule::OnStateChanged;
    Config.state_callback_userdata = this;

    Fn_cm_init(&Config);

    // Step 4: Register tools
    RegisterTools();

    // Step 5: Check initial ECA state before connecting — if ECABridge is
    // already loaded, cache its status so the register message includes it.
    if (FModuleManager::Get().IsModuleLoaded("ECABridge"))
    {
        Fn_cm_set_eca_status(R"({"available":true,"source":"project_plugin","port":3000})");
    }

    // Step 6: Connect
    Fn_cm_connect();

    // PIE lifecycle hooks
    PrePIEHandle = FEditorDelegates::PreBeginPIE.AddLambda([this](bool) {
        if (!bShuttingDown && Fn_cm_send_status) Fn_cm_send_status("reloading");
    });
    EndPIEHandle = FEditorDelegates::EndPIE.AddLambda([this](bool) {
        if (!bShuttingDown && Fn_cm_send_status) Fn_cm_send_status("ready");
    });

    // ECA detection via module changes (handles load/unload after startup)
    ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddLambda(
        [this](FName ModuleName, EModuleChangeReason Reason) {
            if (ModuleName == TEXT("ECABridge") && !bShuttingDown && Fn_cm_set_eca_status)
            {
                bool bAvailable = FModuleManager::Get().IsModuleLoaded("ECABridge");
                FString EcaJson = bAvailable
                    ? TEXT(R"({"available":true,"source":"project_plugin","port":3000})")
                    : TEXT(R"({"available":false,"source":"not_found"})");
                Fn_cm_set_eca_status(TCHAR_TO_UTF8(*EcaJson));
            }
        });

    // Register toolbar
    FCMToolbarExtension::Register(this);
    FAssetEditorExtension::Register();

    UE_LOG(LogTemp, Log, TEXT("[CMBridge] Module started"));
}

void FCodeMaestroBridgeModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("[CMBridge] Module shutting down"));

    // Step 1: Set bShuttingDown — tool callbacks check this before dispatching AsyncTask
    bShuttingDown = true;

    // Unbind delegates
    FEditorDelegates::PreBeginPIE.Remove(PrePIEHandle);
    FEditorDelegates::EndPIE.Remove(EndPIEHandle);
    FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);

    // Step 2: Disconnect
    if (Fn_cm_disconnect) Fn_cm_disconnect();

    // Step 3: Shutdown (non-blocking)
    if (Fn_cm_shutdown) Fn_cm_shutdown();

    // Step 4: DO NOT free DLL handle — it stays mapped for process lifetime
    // This ensures any lingering AsyncTask lambdas can safely call cm_tool_respond
    // (which returns CM_ERROR_NOT_INITIALIZED as a no-op after shutdown)

    FCMToolbarExtension::Unregister();
    FAssetEditorExtension::Unregister();

    FCodeMaestroBridgeStyle::Shutdown();

    UE_LOG(LogTemp, Log, TEXT("[CMBridge] Module shut down"));
}

bool FCodeMaestroBridgeModule::LoadDll()
{
    // Step 1: Find plugin base dir
    auto Plugin = IPluginManager::Get().FindPlugin(TEXT("CodeMaestroBridge"));
    if (!Plugin)
    {
        UE_LOG(LogTemp, Error, TEXT("[CMBridge] CodeMaestroBridge plugin not found in IPluginManager"));
        return false;
    }

    FString DllPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Binaries/Win64/CodeMaestroRuntime.dll"));

    // Step 2: Load DLL
    RuntimeDllHandle = FPlatformProcess::GetDllHandle(*DllPath);

    // Step 3: Handle missing DLL
    if (!RuntimeDllHandle)
    {
        BridgeStatus = EBridgeStatus::NoRuntime;
        UE_LOG(LogTemp, Warning, TEXT("[CMBridge] CodeMaestroRuntime.dll not found at %s"), *DllPath);
        FCMToolbarExtension::Register(this);
        return false;
    }

    // Step 4: Resolve function pointers
    #define RESOLVE(Name) \
        Fn_##Name = (Name##_fn)FPlatformProcess::GetDllExport(RuntimeDllHandle, TEXT(#Name)); \
        if (!Fn_##Name) { UE_LOG(LogTemp, Error, TEXT("[CMBridge] Failed to resolve " #Name)); return false; }

    RESOLVE(cm_init)
    RESOLVE(cm_connect)
    RESOLVE(cm_disconnect)
    RESOLVE(cm_shutdown)
    RESOLVE(cm_register_tool)
    RESOLVE(cm_get_state)
    RESOLVE(cm_send_status)
    RESOLVE(cm_set_eca_status)
    RESOLVE(cm_send_event)
    RESOLVE(cm_get_version)

    #undef RESOLVE

    // Resolve tool response functions (static, accessible from tool callbacks)
    ToolRespond = (cm_tool_respond_fn)FPlatformProcess::GetDllExport(RuntimeDllHandle, TEXT("cm_tool_respond"));
    ToolError = (cm_tool_error_fn)FPlatformProcess::GetDllExport(RuntimeDllHandle, TEXT("cm_tool_error"));

    if (!ToolRespond || !ToolError)
    {
        UE_LOG(LogTemp, Error, TEXT("[CMBridge] Failed to resolve cm_tool_respond or cm_tool_error"));
        return false;
    }

    return true;
}

void FCodeMaestroBridgeModule::RegisterTools()
{
    // Each tool: name, description, JSON schema, callback, userdata
    Fn_cm_register_tool("ping", "Health check. Returns OK status, timestamp, and engine version.",
        R"({"type":"object","properties":{}})",
        &OnPing, nullptr);
    Fn_cm_register_tool("get_editor_state", "Returns current editor state: project name, open map, play state, compilation status, actor count, and content directory.",
        R"({"type":"object","properties":{}})",
        &OnGetEditorState, nullptr);
    Fn_cm_register_tool("get_open_blueprint", "Returns the currently open Blueprint in the editor (name, asset path, parent class). Returns null fields if no Blueprint is open.",
        R"({"type":"object","properties":{}})",
        &OnGetOpenBlueprint, nullptr);
}

int FCodeMaestroBridgeModule::SendEvent(const FString& EventType, const FString& PayloadJson)
{
    if (bShuttingDown || !Fn_cm_send_event)
    {
        return CM_ERROR_NOT_INITIALIZED;
    }

    auto EventUtf8 = FTCHARToUTF8(*EventType);
    auto PayloadUtf8 = FTCHARToUTF8(*PayloadJson);
    const int Result = Fn_cm_send_event(EventUtf8.Get(), PayloadUtf8.Get());
    if (Result != CM_OK)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CMBridge] cm_send_event('%s') returned %d"), *EventType, Result);
    }
    return Result;
}

void FCodeMaestroBridgeModule::OnStateChanged(CMState new_state, void* userdata)
{
    auto* Module = static_cast<FCodeMaestroBridgeModule*>(userdata);
    // Fire delegate on game thread for UI update
    AsyncTask(ENamedThreads::GameThread, [Module, new_state]() {
        if (!FCodeMaestroBridgeModule::bShuttingDown)
        {
            Module->OnBridgeStateChanged.Broadcast(new_state);
        }
    });
}

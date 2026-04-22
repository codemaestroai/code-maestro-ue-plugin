#pragma once

#include "Modules/ModuleManager.h"
#include <atomic>

// Forward declare — the actual typedefs are in codemaestro_runtime.h
// We load function pointers dynamically, so we don't link against the DLL
#include "codemaestro_runtime.h"

// Wrapper-local status enum (includes states the DLL can't represent)
enum class EBridgeStatus : uint8
{
    NoRuntime,          // DLL failed to load
    VersionMismatch,    // DLL api_version doesn't match
    Active              // DLL loaded, CMState from cm_get_state() is authoritative
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBridgeStateChanged, CMState);

class FCodeMaestroBridgeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    static FCodeMaestroBridgeModule& Get()
    {
        return FModuleManager::LoadModuleChecked<FCodeMaestroBridgeModule>("CodeMaestroBridge");
    }

    EBridgeStatus GetBridgeStatus() const { return BridgeStatus; }

    FOnBridgeStateChanged OnBridgeStateChanged;

    // Emit a push_event to CM Desktop. Fire-and-forget — requires
    // CM_STATE_CONNECTED. Returns the underlying cm_send_event result code
    // (CM_OK, CM_ERROR_NOT_CONNECTED, CM_ERROR_INVALID_ARGUMENT, ...).
    int SendEvent(const FString& EventType, const FString& PayloadJson);

    // DLL function pointers (resolved at startup)
    using cm_init_fn = decltype(&cm_init);
    using cm_connect_fn = decltype(&cm_connect);
    using cm_disconnect_fn = decltype(&cm_disconnect);
    using cm_shutdown_fn = decltype(&cm_shutdown);
    using cm_register_tool_fn = decltype(&cm_register_tool);
    using cm_tool_respond_fn = decltype(&cm_tool_respond);
    using cm_tool_error_fn = decltype(&cm_tool_error);
    using cm_get_state_fn = decltype(&cm_get_state);
    using cm_send_status_fn = decltype(&cm_send_status);
    using cm_set_eca_status_fn = decltype(&cm_set_eca_status);
    using cm_send_event_fn = decltype(&cm_send_event);
    using cm_get_version_fn = decltype(&cm_get_version);

    // Accessible to tool callbacks
    static cm_tool_respond_fn ToolRespond;
    static cm_tool_error_fn ToolError;
    static std::atomic<bool> bShuttingDown;

private:
    void* RuntimeDllHandle = nullptr;
    EBridgeStatus BridgeStatus = EBridgeStatus::NoRuntime;

    cm_init_fn Fn_cm_init = nullptr;
    cm_connect_fn Fn_cm_connect = nullptr;
    cm_disconnect_fn Fn_cm_disconnect = nullptr;
    cm_shutdown_fn Fn_cm_shutdown = nullptr;
    cm_register_tool_fn Fn_cm_register_tool = nullptr;
    cm_get_state_fn Fn_cm_get_state = nullptr;
    cm_send_status_fn Fn_cm_send_status = nullptr;
    cm_set_eca_status_fn Fn_cm_set_eca_status = nullptr;
    cm_send_event_fn Fn_cm_send_event = nullptr;
    cm_get_version_fn Fn_cm_get_version = nullptr;

    FDelegateHandle PrePIEHandle;
    FDelegateHandle EndPIEHandle;
    FDelegateHandle ModulesChangedHandle;

    bool LoadDll();
    void RegisterTools();
    static void OnStateChanged(CMState new_state, void* userdata);
};

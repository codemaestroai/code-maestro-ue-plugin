#ifndef CODEMAESTRO_RUNTIME_H
#define CODEMAESTRO_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CM_RUNTIME_EXPORTS
    #ifdef _WIN32
        #define CM_API __declspec(dllexport)
    #else
        #define CM_API __attribute__((visibility("default")))
    #endif
#else
    #ifdef _WIN32
        #define CM_API __declspec(dllimport)
    #else
        #define CM_API
    #endif
#endif

#define CM_OK 0
#define CM_ERROR_ALREADY_INITIALIZED 1
#define CM_ERROR_NOT_INITIALIZED 2
#define CM_ERROR_INVALID_ARGUMENT 3
#define CM_ERROR_ALREADY_CONNECTED 4
#define CM_ERROR_NOT_CONNECTED 5

/* --- State --- */

typedef enum {
    CM_STATE_DISCONNECTED,
    CM_STATE_CONNECTING,
    CM_STATE_AUTHENTICATING,
    CM_STATE_REGISTERING,
    CM_STATE_CONNECTED,
    CM_STATE_SUSPENDED
} CMState;

typedef void (*CMStateCallback)(CMState new_state, void* userdata);

/* --- Tool callbacks --- */

typedef void (*CMToolCallback)(
    const char* request_id,
    const char* args_json,
    void* userdata
);

/* --- Configuration --- */

typedef struct {
    const char* project_name;
    const char* login_id;
    const char* content_path;
    const char* engine_version;
    const char* plugin_version;

    CMStateCallback on_state_changed;
    void* state_callback_userdata;
} CMConfig;

/* --- Lifecycle --- */

CM_API int cm_init(const CMConfig* config);
CM_API int cm_connect(void);
CM_API int cm_disconnect(void);
CM_API void cm_shutdown(void);

/* --- Tool registration --- */

CM_API int cm_register_tool(
    const char* name,
    const char* description,
    const char* schema_json,
    CMToolCallback callback,
    void* userdata
);

/* --- Tool responses --- */

CM_API int cm_tool_respond(const char* request_id, const char* result_json);
CM_API int cm_tool_error(const char* request_id, const char* error_message);

/* --- State & status --- */

CM_API CMState cm_get_state(void);
CM_API int cm_send_status(const char* status);

/* --- ECA status --- */

/*
 * Set ECA Bridge availability status.
 * Behavior depends on connection state:
 *   - CM_STATE_CONNECTED: immediately sends a push_event message to the Hub
 *   - Any other state: caches the value for inclusion in the next register message
 * The wrapper is responsible for detecting ECA module changes via
 * FModuleManager::OnModulesChanged() and calling this function.
 */
CM_API int cm_set_eca_status(const char* eca_status_json);

/* --- Generic push events --- */

/*
 * Emit an arbitrary structured event from the plugin back to CM Desktop.
 * Sends a push_event WebSocket message with the given event_type and payload.
 * payload_json must be a valid JSON object string; both arguments are required.
 *
 * Behavior depends on connection state:
 *   - CM_STATE_CONNECTED: frame is enqueued and sent immediately
 *   - Any other state:    returns CM_ERROR_NOT_CONNECTED (events are not buffered
 *                         across disconnects — callers should only emit when
 *                         cm_get_state() == CM_STATE_CONNECTED)
 *
 * Generalizes the push_event mechanism already used internally by
 * cm_set_eca_status. Additive in API v1 — old plugins that don't resolve this
 * symbol keep working.
 */
CM_API int cm_send_event(const char* event_type, const char* payload_json);

/* --- Version --- */

CM_API int cm_get_version(int* major, int* minor, int* patch);

#ifdef __cplusplus
}
#endif

#endif /* CODEMAESTRO_RUNTIME_H */

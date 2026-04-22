#pragma once

class FCodeMaestroBridgeModule;

/**
 * Registers the CM status widget into the Level Editor toolbar via UToolMenus.
 * Uses a singleton owner pattern for proper startup callback cancellation.
 */
class FCMToolbarExtension
{
public:
    /** Register the toolbar widget. Call from StartupModule. */
    static void Register(FCodeMaestroBridgeModule* Module);

    /** Unregister the toolbar widget. Call from ShutdownModule. */
    static void Unregister();

private:
    static void ExtendToolbar();

    static FCodeMaestroBridgeModule* CachedModule;
};

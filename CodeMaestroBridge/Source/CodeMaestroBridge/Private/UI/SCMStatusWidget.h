#pragma once

#include "Widgets/SCompoundWidget.h"
#include "CodeMaestroBridgeModule.h"

/**
 * Toolbar status widget showing CM Desktop connection state.
 * Displays a colored dot + text label that updates via OnBridgeStateChanged delegate.
 * Handles both EBridgeStatus (wrapper-local: NoRuntime, VersionMismatch, Active)
 * and CMState (from the DLL via callback when Active).
 */
class SCMStatusWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCMStatusWidget) {}
        SLATE_ARGUMENT(FCodeMaestroBridgeModule*, Module)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    ~SCMStatusWidget();

private:
    /** Called when DLL connection state changes */
    void OnStateChanged(CMState NewState);

    /** Map EBridgeStatus + CMState to display state */
    struct FDisplayState
    {
        FLinearColor Color;
        FText Label;
    };
    static FDisplayState GetDisplayStateForBridgeStatus(EBridgeStatus Status);
    static FDisplayState GetDisplayStateForCMState(CMState State);

    FCodeMaestroBridgeModule* Module = nullptr;
    FDelegateHandle DelegateHandle;

    // Cached display values
    FLinearColor CurrentColor = FLinearColor(0.4f, 0.4f, 0.4f);
    FText CurrentLabel;
};

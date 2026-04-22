#include "UI/SCMStatusWidget.h"
#include "UI/CodeMaestroBridgeStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "CMStatusWidget"

void SCMStatusWidget::Construct(const FArguments& InArgs)
{
    Module = InArgs._Module;

    // Set initial display state from bridge status
    FDisplayState InitialState;
    if (Module)
    {
        EBridgeStatus Status = Module->GetBridgeStatus();
        if (Status == EBridgeStatus::Active)
        {
            // DLL is loaded — show disconnected until we hear from it
            InitialState = GetDisplayStateForCMState(CM_STATE_DISCONNECTED);
        }
        else
        {
            InitialState = GetDisplayStateForBridgeStatus(Status);
        }
    }
    else
    {
        InitialState = GetDisplayStateForBridgeStatus(EBridgeStatus::NoRuntime);
    }

    CurrentColor = InitialState.Color;
    CurrentLabel = InitialState.Label;

    ChildSlot
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(6.0f, 0.0f, 6.0f, 0.0f)
        [
            SNew(SImage)
            .Image(FCodeMaestroBridgeStyle::Get().GetBrush("CodeMaestro.Logo"))
            .ColorAndOpacity_Lambda([this]() { return FSlateColor(CurrentColor); })
        ]
        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0.0f, 0.0f, 8.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text_Lambda([this]() { return CurrentLabel; })
            .TextStyle(FAppStyle::Get(), "SmallText")
        ]
    ];

    // Subscribe to state changes from the DLL
    if (Module)
    {
        DelegateHandle = Module->OnBridgeStateChanged.AddSP(
            SharedThis(this), &SCMStatusWidget::OnStateChanged);
    }
}

SCMStatusWidget::~SCMStatusWidget()
{
    if (Module)
    {
        Module->OnBridgeStateChanged.Remove(DelegateHandle);
    }
}

void SCMStatusWidget::OnStateChanged(CMState NewState)
{
    FDisplayState Display = GetDisplayStateForCMState(NewState);
    CurrentColor = Display.Color;
    CurrentLabel = Display.Label;
}

SCMStatusWidget::FDisplayState SCMStatusWidget::GetDisplayStateForBridgeStatus(EBridgeStatus Status)
{
    switch (Status)
    {
    case EBridgeStatus::NoRuntime:
        return { FLinearColor(0.8f, 0.0f, 0.0f), LOCTEXT("NoRuntime", "Runtime Missing") };

    case EBridgeStatus::VersionMismatch:
        return { FLinearColor(1.0f, 0.6f, 0.0f), LOCTEXT("VersionMismatch", "Version Mismatch") };

    case EBridgeStatus::Active:
    default:
        return { FLinearColor(0.4f, 0.4f, 0.4f), LOCTEXT("Disconnected", "Disconnected") };
    }
}

SCMStatusWidget::FDisplayState SCMStatusWidget::GetDisplayStateForCMState(CMState State)
{
    switch (State)
    {
    case CM_STATE_CONNECTED:
        return { FLinearColor(0.0f, 0.8f, 0.2f), LOCTEXT("Connected", "Connected") };

    case CM_STATE_CONNECTING:
    case CM_STATE_AUTHENTICATING:
    case CM_STATE_REGISTERING:
        return { FLinearColor(1.0f, 0.8f, 0.0f), LOCTEXT("Connecting", "Connecting...") };

    case CM_STATE_SUSPENDED:
        return { FLinearColor(1.0f, 0.6f, 0.0f), LOCTEXT("Reconnecting", "Reconnecting...") };

    case CM_STATE_DISCONNECTED:
    default:
        return { FLinearColor(0.4f, 0.4f, 0.4f), LOCTEXT("Disconnected", "Disconnected") };
    }
}

#undef LOCTEXT_NAMESPACE

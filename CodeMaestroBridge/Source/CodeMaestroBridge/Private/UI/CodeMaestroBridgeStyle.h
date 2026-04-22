#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

/**
 * Slate style set for CodeMaestroBridge plugin resources.
 * Registers the CM logo and any other plugin-owned brushes so widgets
 * can reference them via FCodeMaestroBridgeStyle::Get().GetBrush("CodeMaestro.Logo").
 */
class FCodeMaestroBridgeStyle
{
public:
    static void Initialize();
    static void Shutdown();

    static const ISlateStyle& Get();
    static FName GetStyleSetName();

private:
    static TSharedRef<FSlateStyleSet> Create();
    static TSharedPtr<FSlateStyleSet> StyleInstance;
};

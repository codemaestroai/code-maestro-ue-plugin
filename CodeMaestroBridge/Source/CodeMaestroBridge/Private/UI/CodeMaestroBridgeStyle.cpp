#include "UI/CodeMaestroBridgeStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyleMacros.h"

TSharedPtr<FSlateStyleSet> FCodeMaestroBridgeStyle::StyleInstance = nullptr;

// SlateStyleMacros.h expects RootToContentDir/RootToCoreContentDir to be
// defined locally. RootToContentDir is what IMAGE_BRUSH_SVG uses.
#define RootToContentDir StyleSet->RootToContentDir

void FCodeMaestroBridgeStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FCodeMaestroBridgeStyle::Shutdown()
{
    if (StyleInstance.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
        ensure(StyleInstance.IsUnique());
        StyleInstance.Reset();
    }
}

FName FCodeMaestroBridgeStyle::GetStyleSetName()
{
    static FName StyleName(TEXT("CodeMaestroBridgeStyle"));
    return StyleName;
}

const ISlateStyle& FCodeMaestroBridgeStyle::Get()
{
    return *StyleInstance;
}

TSharedRef<FSlateStyleSet> FCodeMaestroBridgeStyle::Create()
{
    TSharedRef<FSlateStyleSet> StyleSet = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CodeMaestroBridge"));
    if (Plugin.IsValid())
    {
        StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
    }

    // Logo source: 599 x 264 viewBox (~2.269:1). Toolbar-sized defaults keep that
    // aspect ratio so the vector renders without distortion at any DPI.
    const FVector2D LogoSize(36.0f, 16.0f);
    StyleSet->Set("CodeMaestro.Logo", new IMAGE_BRUSH_SVG(TEXT("CMLogo"), LogoSize));

    // Square variant for toolbar buttons — same wordmark letterboxed into a
    // 599x599 viewBox (see CMLogoSquare.svg). Used by AssetEditorExtension for
    // the "Explain with CM" button where a square icon slot would otherwise
    // distort the wide wordmark.
    const FVector2D IconSize(16.0f, 16.0f);
    StyleSet->Set("CodeMaestro.LogoSquare", new IMAGE_BRUSH_SVG(TEXT("CMLogoSquare"), IconSize));

    return StyleSet;
}

#undef RootToContentDir

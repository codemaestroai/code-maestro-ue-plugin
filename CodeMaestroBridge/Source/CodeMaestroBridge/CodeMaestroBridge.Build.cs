using UnrealBuildTool;
using System.IO;

public class CodeMaestroBridge : ModuleRules
{
    public CodeMaestroBridge(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "UnrealEd", "LiveCoding", "Projects",
            "Slate", "SlateCore", "ToolMenus", "LevelEditor", "InputCore",
            "Kismet", "UMGEditor", "MaterialEditor", "Json"
        });

        // ThirdParty headers (codemaestro_runtime.h)
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));
    }
}

using UnrealBuildTool;

public class GamejamRazigra : ModuleRules
{
    public GamejamRazigra(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "UMG",
            "OnlineSubsystem", "OnlineSubsystemEOS", "OnlineSubsystemUtils", "NavigationSystem", "AIModule", "EngineCameras",
            "MediaAssets", "AudioMixer"
        });

        PrivateDependencyModuleNames.AddRange(new[] { "Slate", "SlateCore" });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[] { "AssetRegistry", "UnrealEd" });
        }
    }
}

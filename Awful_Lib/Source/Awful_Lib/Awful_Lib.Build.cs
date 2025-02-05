// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

using UnrealBuildTool;

public class Awful_Lib : ModuleRules
{
	public Awful_Lib(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GameplayTags", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        PrivateIncludePaths.AddRange(new string[] { "Awful_Lib/Public/", "Awful_Lib" });
        PublicIncludePaths.AddRange(new string[] { "Awful_Lib/Private/", "Awful_Lib" });

    }
}

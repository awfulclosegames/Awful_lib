// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Awful_LibTarget : TargetRules
{
	public Awful_LibTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Awful_Lib");
	}
}

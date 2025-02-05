// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Awful_LibEditorTarget : TargetRules
{
	public Awful_LibEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Awful_Lib");
	}
}

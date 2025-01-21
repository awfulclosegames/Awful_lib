// Copyright Strati D. Zerbinis 2025. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SDZ_LibEditorTarget : TargetRules
{
	public SDZ_LibEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("SDZ_Lib");
	}
}

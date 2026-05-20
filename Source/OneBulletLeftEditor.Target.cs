using UnrealBuildTool;
using System.Collections.Generic;

public class OneBulletLeftEditorTarget : TargetRules
{
	public OneBulletLeftEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("OneBulletLeft");
	}
}

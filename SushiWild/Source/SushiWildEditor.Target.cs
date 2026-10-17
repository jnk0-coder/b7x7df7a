using UnrealBuildTool;

public class SushiWildEditorTarget : TargetRules
{
	public SushiWildEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SushiWild");
	}
}

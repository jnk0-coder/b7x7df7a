using UnrealBuildTool;

public class SushiWildTarget : TargetRules
{
	public SushiWildTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SushiWild");
	}
}

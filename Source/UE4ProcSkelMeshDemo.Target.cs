using UnrealBuildTool;
using System.Collections.Generic;

public class UE4ProcSkelMeshDemoTarget : TargetRules
{
	public UE4ProcSkelMeshDemoTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "UE4ProcSkelMeshDemo" } );
	}
}

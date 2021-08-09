// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RaspberryVehicle : ModuleRules
{
	public RaspberryVehicle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(
			new string[] 
			{
				"Core",
				"Engine", 
				"InputCore" ,
				"CoreUObject",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] 
			{
				"RHI",
				"Sockets",
				"Networking",
				"RenderCore",
			}
		);
	}
}

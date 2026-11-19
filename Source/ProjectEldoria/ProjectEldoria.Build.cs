// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectEldoria : ModuleRules
{
	public ProjectEldoria(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "PhysicsCore" });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectEldoria",
			"ProjectEldoria/Variant_Platforming",
			"ProjectEldoria/Variant_Platforming/Animation",
			"ProjectEldoria/Variant_Combat",
			"ProjectEldoria/Variant_Combat/AI",
			"ProjectEldoria/Variant_Combat/Animation",
			"ProjectEldoria/Variant_Combat/Gameplay",
			"ProjectEldoria/Variant_Combat/Interfaces",
			"ProjectEldoria/Variant_Combat/UI",
			"ProjectEldoria/Variant_SideScrolling",
			"ProjectEldoria/Variant_SideScrolling/AI",
			"ProjectEldoria/Variant_SideScrolling/Gameplay",
			"ProjectEldoria/Variant_SideScrolling/Interfaces",
			"ProjectEldoria/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

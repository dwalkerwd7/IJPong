// It's Just Pong

using UnrealBuildTool;

public class IJPongShaders : ModuleRules
{
	public IJPongShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "RenderCore" });
	}
}

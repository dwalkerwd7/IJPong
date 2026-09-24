// It's Just Pong

#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ShaderCore.h"

/**
 * Maps the virtual shader path "/IJPong" to the project's Shaders/ folder, so materials can
 * #include project shader files (e.g. a Custom node including "/IJPong/IJPCRT.ush").
 * A separate module because the mapping must exist before any shader compiles, which means
 * loading at PostConfigInit: too early for the game module, whose constructors load assets.
 */
class FIJPongShadersModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		AddShaderSourceDirectoryMapping(TEXT("/IJPong"), FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"))));
	}
};

IMPLEMENT_MODULE(FIJPongShadersModule, IJPongShaders);

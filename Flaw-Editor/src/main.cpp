#include <Flaw.h>

#include "FlawEditor.h"
#include "EditorLayer.h"

void main(int argc, char** argv) {
	flaw::Log::Initialize();

	flaw::ApplicationProps props;
	props.title = "Flaw Editor";
	props.width = 1920;
	props.height = 1080;
	props.argc = argc;
	props.argv = argv;

	//FLAW_PROFILE_BEGIN_SESSION("Startup", "FlawProfile-Startup.json");
	flaw::FlawEditor* sandbox = new flaw::FlawEditor(props);
	//FLAW_PROFILE_END_SESSION();

	//FLAW_PROFILE_BEGIN_SESSION("Runtime", "FlawProfile-Runtime.json");
	sandbox->Run();
	//FLAW_PROFILE_END_SESSION();

	delete sandbox;

	flaw::Log::Cleanup();
}
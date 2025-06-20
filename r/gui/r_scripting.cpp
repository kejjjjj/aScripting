#include "r_scripting.hpp"
#include "net/nvar_table.hpp"
#include "r/gui/r_main_gui.hpp"
#include "shared/sv_shared.hpp"

#include "scripting/sc_main.hpp"
#include <com/com_channel.hpp>

CScriptingWindow::CScriptingWindow(const std::string& name)
	: CGuiElement(name) {

}


void CScriptingWindow::Render()
{
#if(!DEBUG_SUPPORT)
	static auto func = CMain::Shared::GetFunctionSafe("GetContext");

	if (!func) {
		func = CMain::Shared::GetFunctionSafe("GetContext");
		return;
	}

	ImGui::SetCurrentContext(func->As<ImGuiContext*>()->Call());

#endif

	GUI_RenderNVars();

	auto size = ImGui::GetContentRegionAvail();
	size.y /= 1.25f;
	static std::string scriptBuf;
	ImGui::InputTextMultiline("##Script", &scriptBuf, size);

	if (ImGui::ButtonCentered("Execute")) {
		if (SC_Execute(scriptBuf))
			Com_Printf("^2Success!\n");
	}
}


#include "com/com_channel.hpp"
#include "fs/fs_globals.hpp"
#include "fs/fs_io.hpp"
#include "net/nvar_table.hpp"
#include "r/gui/r_main_gui.hpp"
#include "r_scripting.hpp"
#include "scripting/sc_main.hpp"
#include "shared/sv_shared.hpp"
#include "cg/cg_cleanup.hpp"

CScriptingWindow::CScriptingWindow(const std::string& name)
	: CGuiElement(name) {

}

auto GetScriptFiles() -> std::vector<std::string>
{
	const auto dir = AGENT_DIRECTORY() + "\\Scripts";

	if (!__fs::directory_exists(dir)) {
		if (!__fs::create_directory(dir)) {
			CG_SafeErrorExit("couldn't create " + dir);
			return {};
		}
	}

	auto thing = __fs::files_in_directory(dir) | std::views::filter([](const std::string& f) { 
		return __fs::get_extension(f) == ".var"; 
	});

	return std::vector<std::string>(thing.begin(), thing.end());
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

	if (ImGui::BeginTabBar("tab")) {

		if (ImGui::BeginTabItem("Editor")) {
			auto size = ImGui::GetContentRegionAvail();
			size.y /= 1.25f;
			static std::string scriptBuf;
			ImGui::InputTextMultiline("##Script", &scriptBuf, size);

			if (ImGui::ButtonCentered("Execute")) {
				if (SC_Execute(scriptBuf))
					Com_Printf("^2Success!\n");
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Scripts")) {
			static bool get_c_str = true;
			static std::vector<std::string> files;
			static std::vector<const char*> files_c_str;

			if (get_c_str || ImGui::Button("Refresh")) {
				files = GetScriptFiles();
				files_c_str.clear();
				for (auto& f : files)
					files_c_str.push_back(f.c_str());
				get_c_str = false;
			}

			static int idx{};

			if (ImGui::Combo("Script", &idx, files_c_str.data(), files_c_str.size())) {
				printf("%s\n", files_c_str[idx]);
			}

			if (files_c_str.empty())
				ImGui::BeginDisabled();

			if (ImGui::Button("Execute")) {
				if (SC_ExecuteFile(files_c_str[idx]))
					Com_Printf("^2Success!\n");
			}

			if (files_c_str.empty())
				ImGui::BeginDisabled();

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}


#include "com/com_channel.hpp"
#include "fs/fs_globals.hpp"
#include "fs/fs_io.hpp"
#include "net/nvar_table.hpp"
#include "r/gui/r_main_gui.hpp"
#include "r_scripting.hpp"
#include "scripting/sc_main.hpp"
#include "shared/sv_shared.hpp"
#include "cg/cg_cleanup.hpp"
#include "varjus_api/varjus_api.hpp"

CScriptingWindow::CScriptingWindow(const std::string& name)
	: CGuiElement(name) {

}

enum class ExecType { et_none, et_script, et_file };

static auto GetScriptFiles() -> std::vector<std::string>
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

static ExecType RenderEditor(const char*& receiver, bool& async)
{
	if (ImGui::BeginTabItem("Editor")) {
		auto size = ImGui::GetContentRegionAvail();
		size.y /= 1.25f;
		static std::string scriptBuf;
		ImGui::InputTextMultiline("##Script", &scriptBuf, size);

		ImGui::Checkbox("Asynchronous", &async);
		ImGui::SameLine();

		if (ImGui::ButtonCentered("Execute")) {
			receiver = scriptBuf.c_str();
			return ExecType::et_script;
		}
		ImGui::EndTabItem();
	}

	return ExecType::et_none;
}
static ExecType RenderScripts(const char*& receiver, bool& async)
{
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

		ImGui::Checkbox("Asynchronous", &async);
		ImGui::SameLine();

		if (ImGui::Button("Execute")) {
			receiver = files_c_str[idx];
			return ExecType::et_file;
		}

		if (files_c_str.empty())
			ImGui::BeginDisabled();

		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();

	return ExecType::et_none;
}
using ScriptExecutor_t = bool(*)(const std::string&);
using AsyncScriptExecutor_t = bool(*)(const std::string&, Varjus::State&);

constexpr std::array<ExecType(*)(const char*&, bool&), 2> renderFuncs = { { RenderEditor, RenderScripts } };

template<typename ... Args>
static auto ExecuteScript(bool(*callable)(Args...), Args&&... args) {
	return callable(std::forward<Args>(args)...);
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
	static bool async{};

	static std::unique_ptr<Varjus::State> state;

	if (ImGui::BeginTabBar("tab")) {
		const char* script{};

		for (const auto& f : renderFuncs) {
			const auto exectype = f(script, async);

			if (exectype == ExecType::et_none || !script)
				break;

			const auto asStr = std::string(script);
			auto status = false;

			if (async)
				state = std::make_unique<Varjus::State>();

			switch (exectype) {
			case ExecType::et_script:
				if (async)
					status = ExecuteScript(SC_ExecuteAsynchronously, *state, asStr);
				else
					status = ExecuteScript(SC_Execute, asStr);
				break;
			case ExecType::et_file:
				if (async)
					status = ExecuteScript(SC_ExecuteFileAsynchronously, *state, asStr);
				else
					status = ExecuteScript(SC_ExecuteFile, asStr);
				break;
			}
			if (!status) {
				if(state)
					state.reset();
				Com_Printf("^1failure!\n");
			}
		}

		if (state) {
			if (ImGui::ButtonCentered("Abort")) {
				[[maybe_unused]] const auto _ = state->Abort();
			}

			if (state->HasFinished()) {
				state.reset();
			}
		}
	}
}


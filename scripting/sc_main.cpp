#include "cg/cg_local.hpp"
#include "com/com_channel.hpp"
#include "sc_main.hpp"
#include "scripting/sc_init.hpp"
#include "varjus_api/varjus_api.hpp"
#include "global_interface.hpp"

#include <optional>

[[nodiscard]] bool SC_Execute(const std::string& script)
{
	Varjus::State state;

	const auto errHandler = [](const std::optional<std::string>& err) -> std::string {
		return err.value_or("unknown error");
	};

	const auto doError = [&errHandler, &state]() {
		const auto err = errHandler(state.GetErrorMessage());
		return Com_Printf("^1Error: %s\n", err.c_str()), false;
	};

	if (!state.UseStdLibrary()) {
		return doError();
	}

	if (!CMain::Shared::GetFunctionOrExit("SC_AddFunctions")->As<Success, Varjus::State&>()->Call(state))
		return doError();

	if (!CMain::Shared::GetFunctionOrExit("SC_AddObjects")->As<Success, Varjus::State&>()->Call(state))
		return doError();

	if (!state.LoadScript(script)) 
		return doError();
	

	if (const auto result = state.ExecuteScript()) {
		return true;
	}

	return doError();
}
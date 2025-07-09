#include "cg/cg_local.hpp"
#include "com/com_channel.hpp"
#include "sc_main.hpp"
#include "scripting/sc_init.hpp"
#include "varjus_api/varjus_api.hpp"
#include "global_interface.hpp"

#include <optional>
#include <thread>

static std::string SC_ErrorTranslator(const std::optional<std::string>& err) {
	return err.value_or("unknown error");
}
static bool SC_ThrowError(Varjus::State& state) {
	const auto err = SC_ErrorTranslator(state.GetErrorMessage());
	return Com_Printf("^1Error: %s\n", err.c_str()), false;
}
template<typename ... Args>
static bool SC_Prepare(Varjus::State& state, Varjus::Success(Varjus::State::* fn)(Args...), Args&&... args)
{

	if (!state.UseStdLibrary()) {
		return SC_ThrowError(state);
	}

#if(!DEBUG_SUPPORT)
	if (!CMain::Shared::GetFunctionOrExit("SC_AddFunctions")->As<Varjus::Success, Varjus::State&>()->Call(state))
		return SC_ThrowError(state);

	if (!CMain::Shared::GetFunctionOrExit("SC_AddObjects")->As<Varjus::Success, Varjus::State&>()->Call(state))
		return SC_ThrowError(state);
#endif
	if (!(state.*fn)(std::forward<Args>(args)...))
		return SC_ThrowError(state);

	return true;
}

bool SC_Execute(const std::string& script)
{
	Varjus::State state;

	
	if (!SC_Prepare(state, &Varjus::State::LoadScript, script))
		return false;

	if (const auto result = state.ExecuteScript()) {
		return true;
	}

	return SC_ThrowError(state);
}
bool SC_ExecuteFile(const std::string& path)
{
	Varjus::State state;
	
	if (!SC_Prepare(state, &Varjus::State::LoadScriptFromFile, path, Varjus::e_utf8))
		return false;

	if (const auto result = state.ExecuteScript()) {
		return true;
	}

	return SC_ThrowError(state);
}

static void AwaitFunc(Varjus::State& state) {
	[[maybe_unused]] const auto result = state.ExecuteScript();
}

bool SC_ExecuteAsynchronously(Varjus::State& state, const std::string& script)
{
	if (!SC_Prepare(state, &Varjus::State::LoadScript, script))
		return false;

	std::thread thread(AwaitFunc, std::ref(state));
	thread.detach();
	return true;
}
bool SC_ExecuteFileAsynchronously(Varjus::State& state, const std::string& path)
{
	if (!SC_Prepare(state, &Varjus::State::LoadScriptFromFile, path, Varjus::e_utf8))
		return false;

	std::thread thread(AwaitFunc, std::ref(state));
	thread.detach();
	return true;
}

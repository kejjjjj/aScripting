

#include "cg/cg_local.hpp"
#include "cg/cg_memory.hpp"
#include "cg/cg_offsets.hpp"
#include "cg_hooks.hpp"
#include "cg/cg_init.hpp"
#include "cg/cg_cleanup.hpp"

#include "cmd/cmd.hpp"
#include "cod4x/cod4x.hpp"
#include "net/im_defaults.hpp"
#include "net/nvar_table.hpp"
#include "r/r_drawactive.hpp"
#include "r/backend/rb_endscene.hpp"
#include "r/gui/r_scripting.hpp"
#include "shared/sv_shared.hpp"
#include "sys/sys_thread.hpp"
#include "utils/engine.hpp"
#include "scripting/sc_init.hpp"
#include "scripting/sc_main.hpp"

#include <thread>
#include <chrono>


using namespace std::chrono_literals;

#define RETURN(type, data, value) \
*reinterpret_cast<type*>((DWORD)data - sizeof(FARPROC)) = value;\
return 0;


static void NVar_Setup([[maybe_unused]]NVarTable* table)
{
    return;
}

#if(DEBUG_SUPPORT)
#include "r/gui/r_main_gui.hpp"
#include "utils/hook.hpp"

void CG_Init()
{
    while (!dx || !dx->device) {
        std::this_thread::sleep_for(100ms);
    }

    Sys_SuspendAllThreads();
    std::this_thread::sleep_for(300ms);

    if (!CStaticMainGui::Owner->Initialized()) {
#pragma warning(suppress : 6011)
        CStaticMainGui::Owner->Init(dx->device, FindWindow(NULL, COD4X::get() ? "Call of Duty 4 X" : "Call of Duty 4"));
    }

    Cmd_AddCommand("gui", CStaticMainGui::Toggle);

    CStaticMainGui::AddItem(std::make_unique<CScriptingWindow>(NVAR_TABLE_NAME));

    NVarTables::tables[NVAR_TABLE_NAME] = std::make_unique<NVarTable>(NVAR_TABLE_NAME);
    auto table = NVarTables::Get();

    NVar_Setup(table);

    if (table->SaveFileExists())
        table->ReadNVarsFromFile();

    table->WriteNVarsToFile();


    COD4X::initialize();
    CG_MemoryTweaks();
    CG_CreatePermaHooks();

    Sys_ResumeAllThreads();
}
#else
void CG_Init()
{
    auto numAttempts = 0u;
    while (!CMain::Shared::AddFunction || !CMain::Shared::GetFunction) {
        std::this_thread::sleep_for(200ms);

        if (++numAttempts > 25u) {
            return CG_SafeErrorExit("It seems that the module " + std::string(NVAR_TABLE_NAME) + " couldn't get a connection to the main module");
        }
    }

    Sys_SuspendAllThreads();
    std::this_thread::sleep_for(20ms);

    COD4X::initialize();

    NVarTables::tables = CMain::Shared::GetFunctionOrExit("GetNVarTables")->As<nvar_tables_t*>()->Call();
    (*NVarTables::tables)[NVAR_TABLE_NAME] = std::make_unique<NVarTable>(NVAR_TABLE_NAME);
    const auto table = (*NVarTables::tables)[NVAR_TABLE_NAME].get();

    NVar_Setup(table);

    if (table->SaveFileExists())
        table->ReadNVarsFromFile();

    table->WriteNVarsToFile();

    ImGui::SetCurrentContext(CMain::Shared::GetFunctionOrExit("GetContext")->As<ImGuiContext*>()->Call());

    CMain::Shared::GetFunctionOrExit("AddItem")->As<CGuiElement*, std::unique_ptr<CGuiElement>&&>()
        ->Call(std::make_unique<CScriptingWindow>(NVAR_TABLE_NAME));

    //add the functions that need to be managed by the main module
    //CMain::Shared::GetFunctionOrExit("Queue_CG_DrawActive")->As<void, drawactive_t>()->Call(CG_DrawActive);
    //CMain::Shared::GetFunctionOrExit("Queue_CL_CreateNewCommands")->As<void, createnewcommands_t>()->Call(CL_FinishMove);
    //CMain::Shared::GetFunctionOrExit("Queue_RB_EndScene")->As<void, rb_endscene_t>()->Call(RB_DrawDebug);
    CMain::Shared::GetFunctionOrExit("Queue_CG_Cleanup")->As<void, cg_cleanup_t>()->Call(CG_Cleanup);

    //so that all modules know what render method is currently available
#pragma warning(suppress : 6011) //shut up false positive

    CG_CreatePermaHooks();

    Sys_ResumeAllThreads();


}
#endif

void CG_Cleanup()
{
#if(DEBUG_SUPPORT)
    hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#endif

    CG_SafeExit();
}

#if(!DEBUG_SUPPORT)

using PE_EXPORT = std::unordered_map<std::string, DWORD>;

#include <nlohmann/json.hpp>
#include <shared/sv_shared.hpp>
#include <utils/hook.hpp>
#include <utils/errors.hpp>

using json = nlohmann::json;

static PE_EXPORT deserialize_data(const std::string& data)
{
    json j = json::parse(data);
    std::unordered_map<std::string, DWORD> map;
    for (auto it = j.begin(); it != j.end(); ++it) {
        map[it.key()] = it.value();
    }
    return map;

}

dll_export void L(void* data) {
    auto r = deserialize_data(reinterpret_cast<char*>(data));

    try {
        CMain::Shared::AddFunction = (decltype(CMain::Shared::AddFunction))r.at("public: static void __cdecl CSharedModuleData::AddFunction(class std::unique_ptr<class CSharedFunctionBase,struct std::default_delete<class CSharedFunctionBase> > &&)");
        CMain::Shared::GetFunction = (decltype(CMain::Shared::GetFunction))r.at("public: static class CSharedFunctionBase * __cdecl CSharedModuleData::GetFunction(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)");
    }
    catch ([[maybe_unused]] std::out_of_range& ex) {
        return FatalError(std::format("couldn't get a critical function"));
    }
}

#endif
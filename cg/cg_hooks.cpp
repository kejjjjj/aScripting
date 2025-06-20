#include "cg_init.hpp"

#include "utils/hook.hpp"

#include "bg/bg_pmove.hpp"
#include "cg/cg_local.hpp"
#include "cg/cg_memory.hpp"
#include "cg/cg_offsets.hpp"
#include "cl/cl_main.hpp"
#include "cod4x/cod4x.hpp"
#include "r/backend/rb_endscene.hpp"
#include "r/r_drawactive.hpp"
#include "r/gui/r_gui.hpp"

#include "shared/sv_shared.hpp"

#include <chrono>
#include <thread>

using namespace std::chrono_literals;

#if(DEBUG_SUPPORT)
#include "r/gui/r_gui.hpp"
#endif

static void CG_CreateHooks();
void CG_CreatePermaHooks()
{
#if(DEBUG_SUPPORT)
	hooktable::initialize();
#else
	hooktable::initialize(CMain::Shared::GetFunctionOrExit("GetHookTable")->As<std::vector<phookbase>*>()->Call());
#endif

	CG_CreateHooks();
}

void CG_CreateHooks()
{
	hooktable::preserver<void, int>(HOOK_PREFIX("CL_Disconnect"), 0x4696B0, CL_Disconnect);


#if(DEBUG_SUPPORT)

	CG_CreateEssentialHooks();

	hooktable::preserver<void>(HOOK_PREFIX("CG_DrawActive"), COD4X::get() ? COD4X::get() + 0x5464 : 0x42F7F0, CG_DrawActive);

#endif

}
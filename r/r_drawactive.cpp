#include "cg/cg_local.hpp"
#include "cg/cg_offsets.hpp"
#include "r/r_utils.hpp"
#include "r_drawactive.hpp"

#include "utils/hook.hpp"
#include "utils/typedefs.hpp"

void CG_DrawActive()
{
	if (R_NoRender())
#if(DEBUG_SUPPORT)
		return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#else
		return;
#endif


#if(DEBUG_SUPPORT)
	return hooktable::find<void>(HOOK_PREFIX(__func__))->call();
#endif

}
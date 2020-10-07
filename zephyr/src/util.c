#include "logging/log_core.h"
#include <kernel.h>
#include <logging/log.h>
#include <sys/util.h>

LOG_MODULE_REGISTER(util, LOG_LEVEL_DBG);

FUNC_NORETURN void k_halt(const char *msg)
{
    LOG_ERR("%s", log_strdup(msg));
    k_panic();
    CODE_UNREACHABLE;
}
#pragma once

#include "BeegfsFileEventLog.h"

namespace BeegfsEvents {
    const uint32_t  PUBLISHER_SERVICE_PORT = 9001;
    const uint32_t  NOTIFICATION_SERVICE_PORT = 9001;
    const char * const GET_PUBLISHER_COMMAND = "GET_PUBLISHER_COMMAND";
    const char * const REGISTER_PATH_COMMAND = "REGISTER_PATH_COMMAND";
    const char * const UNREGISTER_PATH_COMMAND = "UNREGISTER_PATH_COMMAND";
}

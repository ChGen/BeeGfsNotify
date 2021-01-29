#pragma once

#include <string>

namespace BeegfsEvents {

const int NOTIFICATION_SERVICE_PORT = 4322;
const int PUBLISHER_SERVICE_PORT = 4321;

const std::string GET_PUBLISHER_COMMAND = "GET_PUBLISHER"; //for now there's single Publisher
const std::string REGISTER_PATH_COMMAND = "REGISTER_PATH";
const std::string UNREGISTER_PATH_COMMAND = "UNREGISTER_PATH";

}

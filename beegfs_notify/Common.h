#pragma once

#include <string>

namespace BeegfsEvents {

const int NOTIFICATION_SERVICE_PORT = 4322;
const int PUBLISHER_SERVICE_PORT = 4321;

const std::string GET_PUBLISHER_COMMAND = "GET_PUBLISHER"; //for now there's single Publisher
const std::string REGISTER_PATH_COMMAND = "REGISTER_PATH";
const std::string UNREGISTER_PATH_COMMAND = "UNREGISTER_PATH";

const std::string FLUSH_OPERATION = "Flush";
const std::string TRUNCATE_OPERATION = "Truncate";
const std::string SET_ATTR_OPERATION = "SetAttr";
const std::string CLOSE_AFTER_WRITE_OPERATION = "CloseAfterWrite";
const std::string CREATE_OPERATION = "Create";
const std::string MKDIR_OPERATION = "MKdir";
const std::string MKNOD_OPERATION = "MKnod";
const std::string SYMLINK_OPERATION = "Symlink";
const std::string RMDIR_OPERATION = "RMdir";
const std::string UNLINK_OPERATION = "Unlink";
const std::string HARDLINK_OPERATION = "Hardlink";
const std::string RENAME_OPERATION = "Rename";

}

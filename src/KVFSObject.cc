#include "KVFSObject.h"

LogFSObject::LogFSObject():
                 fsId(0),
                 handle(-1),
                 type(LOG_ENTRY_TYPE_OBJ),
                 parentString(),
                 nameString(){
             }

LogFSObject::LogFSObject(uint64_t fsId, uint64_t handle, LogEntryType type, 
                             const std::string* name):
                         fsId(fsId),
                         handle(handle),
                         type(type),
                         parentString()
{
    if (name != nullptr) {
        nameString = *name;
    }
}

void LogFSObject::setHandle(uint64_t handle) {
    this->handle = handle;
}

LogFSObject::~LogFSObject() { }


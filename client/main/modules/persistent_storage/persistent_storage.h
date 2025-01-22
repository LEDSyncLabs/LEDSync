// PersistentStorage.h
#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include "nvs_flash.h"
#include "nvs.h"
#include <string>

class PersistentStorage {
public:
    PersistentStorage(const char* namespaceName);
    bool saveValue(const std::string& key, const std::string& value);
    std::string getValue(const std::string& key);
    bool eraseValue(const std::string& key);

private:
    const char* namespaceName;
};

#endif // PERSISTENT_STORAGE_H
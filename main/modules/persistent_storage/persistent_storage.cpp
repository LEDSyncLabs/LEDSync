#include "persistent_storage.h"
#include <iostream>

PersistentStorage::PersistentStorage(const char* namespaceName) {
    esp_err_t err = nvs_flash_init();
    // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     nvs_flash_erase();
    //     nvs_flash_init();
    //     printf("Erased flash in PersistentStorage\n");
    // }
    this->namespaceName = namespaceName;
}

bool PersistentStorage::saveValue(const std::string& key, const std::string& value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespaceName, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    err = nvs_set_str(handle, key.c_str(), value.c_str());
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK;
}

std::string PersistentStorage::getValue(const std::string& key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespaceName, NVS_READONLY, &handle);
    if (err != ESP_OK) return "";

    size_t required_size;
    err = nvs_get_str(handle, key.c_str(), nullptr, &required_size);
    if (err != ESP_OK) {
        nvs_close(handle);
        return "";
    }

    char* value = new char[required_size];
    err = nvs_get_str(handle, key.c_str(), value, &required_size);
    std::string result;
    if (err == ESP_OK) {
        result = std::string(value);
    }
    delete[] value;
    nvs_close(handle);
    return result;
}

bool PersistentStorage::eraseValue(const std::string& key) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespaceName, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    err = nvs_erase_key(handle, key.c_str());
    if (err == ESP_OK) {
        nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK;
}

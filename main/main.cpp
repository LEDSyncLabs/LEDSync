#include "modules/gatt_client/gatt_client.h"

extern "C" void app_main(void) {
    std::map<uint16_t, std::vector<uint16_t>> service_data = {
        {0xDEAD, {0xDE01, 0xDE02}},
        {0xBEEF, {0xBE01, 0xBE02}},
    };

    GattClient::create(service_data);
}

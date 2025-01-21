idf_component_register(SRCS "main.cpp" "modules/gatt_client/gatt_client.cpp" "modules/gatt_server/gatt_server.cpp" "modules/http_server/http_server.cpp" "modules/wifi/wifi_manager.cpp" "modules/utils/utils.cpp" "modules/mqtt/mqtt_manager.cpp" "modules/persistent_storage/persistent_storage.cpp"
                    INCLUDE_DIRS "." "modules/gatt_client" "modules/gatt_server" "modules/wifi" "modules/utils" "modules/http_server" "modules/mqtt" "modules/persistent_storage"
                    )

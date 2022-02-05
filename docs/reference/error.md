
#Error handling

GrowNode relies on [ESP-IDF error handling](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/error-handling.html) API to handle errors. 

GrowNode specific error codes are handled via the `gn_err_t` enum. All platform functions are returning this enum to let the user handle the situation appropriately.
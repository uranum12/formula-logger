#include "meter.hpp"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <optional>
#include <tuple>

#include <cJSON.h>

uint8_t convertNumber(const int num) {
    assert(0 <= num && num < number_table_len);
    return number_table[num];
}

uint8_t convertMeter(const int num) {
    assert(0 <= num && num < meter_table_len);
    return meter_table[num];
}

int calcLevel(const int rpm) {
    for (int i = 0; i < level_thresholds_len; ++i) {
        if (rpm <= level_thresholds[i]) {
            return i;
        }
    }
    return level_thresholds_len;
}

std::optional<std::tuple<int, int>> parseUARTMessage(const char* str) {
    std::optional<std::tuple<int, int>> result;

    cJSON* root = cJSON_Parse(str);

    char* topic = cJSON_GetStringValue(cJSON_GetObjectItem(root, "topic"));
    if (strcmp(topic, "ecu") == 0) {
        char* payload =
            cJSON_GetStringValue(cJSON_GetObjectItem(root, "payload"));
        cJSON* payload_root = cJSON_Parse(payload);

        double gp =
            cJSON_GetNumberValue(cJSON_GetObjectItem(payload_root, "gp"));
        int rpm =
            cJSON_GetNumberValue(cJSON_GetObjectItem(payload_root, "rpm"));

        cJSON_Delete(payload_root);

        result = std::make_tuple(static_cast<uint8_t>(gp / 1.0), rpm);
    }

    cJSON_Delete(root);

    return result;
}

void fillBuf(int gear, int rpm, uint8_t* buf) {
    if (!buf) {
        return;
    }

    buf[0] = convertNumber(rpm % 10);
    buf[1] = convertNumber((rpm / 10) % 10);
    buf[2] = convertNumber((rpm / 100) % 10);
    buf[3] = convertNumber((rpm / 1000) % 10);
    buf[4] = convertNumber(gear);
    buf[5] = convertMeter(calcLevel(rpm));
}

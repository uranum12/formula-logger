#include "meter.hpp"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <optional>
#include <tuple>

#include <cJSON.h>

uint8_t convertNumber(const int num) {
    assert(0 <= num && num < number_table_len);
    return number_table[num];
}

uint8_t convertGear(const int num) {
    assert(0 <= num && num < gear_table_len);
    return gear_table[num];
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

int calc_gear(double v) {
    double v_ref[] = {0.0, 0.88, 1.10, 1.46, 1.77, 2.09, 2.38, 3.0};
    for (int i = 0; i < 6; i++) {
        double low = (v_ref[i] + v_ref[i + 1]) / 2.0;
        double high = (v_ref[i + 1] + v_ref[i + 2]) / 2.0;
        if (low <= v && v < high) {
            return i + 1;
        }
    }
    return -1;
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

        uint8_t gear = calc_gear(gp);

        result = std::make_tuple(gear, rpm);
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
    buf[4] = convertGear(gear);
    buf[5] = convertMeter(calcLevel(rpm));
}

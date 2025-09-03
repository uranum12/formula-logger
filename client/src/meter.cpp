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

int mapHalfYToInt(double halfY) {
    double y[] = {1.82, 2.288, 3.026, 3.69, 4.39, 5.37};
    int N = sizeof(y) / sizeof(y[0]);
    int i;
    for (i = 0; i < N - 1; i++) {
        double y1 = y[i] / 2.0;
        double y2 = y[i + 1] / 2.0;
        if (halfY >= y1 && halfY <= y2) {
            double x = (i + 1) + (halfY - y1) / (y2 - y1);
            int xInt = (int)round(x);
            if (xInt < 1)
                xInt = 1;
            if (xInt > N)
                xInt = N;
            return xInt;
        }
    }
    // 範囲外は端の値に丸める
    if (halfY < y[0] / 2.0)
        return 1;
    return N;
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

        uint8_t gear = mapHalfYToInt(gp);

        result = std::make_tuple(gear == 6 ? 0 : gear, rpm);
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

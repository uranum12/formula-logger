#ifndef JSON_HPP
#define JSON_HPP

#include <stdint.h>

#include <memory>
#include <string_view>

#include <pico/time.h>

#include <cJSON.h>

class Json {
public:
    explicit Json(std::string_view topic)
        : root_(cJSON_CreateObject(), cJSON_Delete),
          payload_(cJSON_CreateObject()) {
        cJSON_AddStringToObject(root_.get(), "topic", topic.data());
        cJSON_AddItemToObject(root_.get(), "payload", payload_);
    }

    void add(std::string_view key, double value) {
        cJSON_AddNumberToObject(payload_, key.data(), value);
    }

    void addTime(absolute_time_t time) {
        uint64_t usec_total = to_us_since_boot(time);
        uint32_t sec = static_cast<uint32_t>(usec_total / 1'000'000);
        uint32_t usec = static_cast<uint32_t>(usec_total % 1'000'000);
        add("sec", sec);
        add("usec", usec);
    }

    bool toBuffer(char* buf, int size) const {
        if (!buf || size == 0) {
            return false;
        }
        return cJSON_PrintPreallocated(root_.get(), buf, size, false);
    }

private:
    using CJSONPtr = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
    CJSONPtr root_;
    cJSON* payload_;
};

#endif /* end of include guard: JSON_HPP */

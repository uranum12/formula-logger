#ifndef JSON_HPP
#define JSON_HPP

#include <stdint.h>

#include <memory>

#include <pico/time.h>

#include <cJSON.h>

class Json {
public:
    Json(const char* topic)
        : root_(cJSON_CreateObject(), cJSON_Delete),
          payload_(cJSON_CreateObject()) {
        cJSON_AddStringToObject(root_.get(), "topic", topic);
        cJSON_AddItemToObject(root_.get(), "payload", payload_);
    }

    void add(const char* key, double value) {
        cJSON_AddNumberToObject(payload_, key, value);
    }
    void add(const char* key, const char* value) {
        cJSON_AddStringToObject(payload_, key, value);
    }

    void addTime(absolute_time_t time) {
        uint64_t usec_total = to_us_since_boot(time);
        uint32_t sec = usec_total / 1'000'000;
        uint32_t usec = usec_total % 1'000'000;
        add("sec", sec);
        add("usec", usec);
    }

    bool toBuffer(char* buf, int size) const {
        if (!buf || size == 0) {
            return false;
        }
        return cJSON_PrintPreallocated(root_.get(), buf, size, false);
    }

    cJSON* get() {
        return root_.get();
    }
    const cJSON* get() const {
        return root_.get();
    }

private:
    using CJSONPtr = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;
    CJSONPtr root_;
    cJSON* payload_;
};

#endif /* end of include guard: JSON_HPP */

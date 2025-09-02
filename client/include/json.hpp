#ifndef JSON_HPP
#define JSON_HPP

#include <stdint.h>

#include <memory>

#include <pico/time.h>

#include <cJSON.h>

class Json {
public:
    Json() : root_(cJSON_CreateObject(), cJSON_Delete) {}

    void addNumber(const char* key, double value) {
        cJSON_AddNumberToObject(root_.get(), key, value);
    }
    void addString(const char* key, const char* value) {
        cJSON_AddStringToObject(root_.get(), key, value);
    }

    void addTime(absolute_time_t time) {
        uint64_t usec_total = to_us_since_boot(time);
        uint32_t sec = usec_total / 1'000'000;
        uint32_t usec = usec_total % 1'000'000;
        addNumber("sec", sec);
        addNumber("usec", usec);
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
};

#endif /* end of include guard: JSON_HPP */

#ifndef MSGPACK_HPP
#define MSGPACK_HPP

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <string_view>
#include <type_traits>

#include <pico/time.h>

#include <cJSON.h>
#include <cmp.h>

#include "crc16.h"

template <int N>
class MsgPack {
public:
    explicit MsgPack(std::string_view json) : ok_(true), mem_({buf_, N, 4}) {
        memset(buf_, 0, N);

        cmp_init(&cmp_, &mem_, mem_read_, mem_skip_, mem_write_);

        cJSON* root = cJSON_Parse(json.data());
        if (!root) {
            ok_ = false;
            return;
        }

        const char* topic =
            cJSON_GetStringValue(cJSON_GetObjectItem(root, "topic"));
        cJSON* payload = cJSON_GetObjectItem(root, "payload");

        size_t size = cJSON_GetArraySize(payload);

        ok_ &= cmp_write_map(&cmp_, 2);
        ok_ &= cmp_write_str(&cmp_, "topic", 5);
        ok_ &= cmp_write_str(&cmp_, topic, strlen(topic));
        ok_ &= cmp_write_str(&cmp_, "payload", 7);
        ok_ &= cmp_write_map(&cmp_, size);

        cJSON* item = payload->child;
        while (item) {
            ok_ &= cmp_write_str(&cmp_, item->string, strlen(item->string));

            if (cJSON_IsNumber(item)) {
                double d = item->valuedouble;
                double i;
                if (fabs(modf(d, &i)) < 1e-9) {
                    ok_ &= cmp_write_integer(&cmp_, static_cast<int64_t>(i));
                } else {
                    ok_ &= cmp_write_double(&cmp_, d);
                }
            } else {
                ok_ &= cmp_write_nil(&cmp_);
            }

            item = item->next;
        }

        cJSON_Delete(root);
    }

    MsgPack(std::string_view topic, int16_t num)
        : ok_(true), mem_({buf_, N, 4}) {
        memset(buf_, 0, N);

        cmp_init(&cmp_, &mem_, mem_read_, mem_skip_, mem_write_);

        cmp_write_map(&cmp_, 2);

        cmp_write_str(&cmp_, "topic", 5);
        cmp_write_str(&cmp_, topic.data(), topic.size());

        cmp_write_str(&cmp_, "payload", 7);
        cmp_write_map(&cmp_, num + 2);
    }

    template <typename T>
    void add(std::string_view key, T value) {
        ok_ &= cmp_write_str(&cmp_, key.data(), key.size());
        if constexpr (std::is_integral_v<T> && std::is_signed_v<T>) {
            ok_ &= cmp_write_integer(&cmp_, value);
        } else if constexpr (std::is_integral_v<T> && std::is_unsigned_v<T>) {
            ok_ &= cmp_write_uinteger(&cmp_, value);
        } else if constexpr (std::is_floating_point_v<T>) {
            ok_ &= cmp_write_double(&cmp_, value);
        } else {
            ok_ &= cmp_write_nil(&cmp_);
        }
    }

    void addTime(absolute_time_t time) {
        uint64_t usec_total = to_us_since_boot(time);
        uint32_t sec = static_cast<uint32_t>(usec_total / 1'000'000);
        uint32_t usec = static_cast<uint32_t>(usec_total % 1'000'000);
        add("sec", sec);
        add("usec", usec);
    }

    uint8_t* getBuf() {
        if (!ok_) {
            return nullptr;
        }

        const uint16_t length = mem_.pos - 4;
        const uint16_t crc = crc16(buf_ + 4, length);

        buf_[0] = static_cast<uint8_t>(length >> 8);
        buf_[1] = static_cast<uint8_t>(length & 0xFF);
        buf_[2] = static_cast<uint8_t>(crc >> 8);
        buf_[3] = static_cast<uint8_t>(crc & 0xFF);

        return buf_;
    }

private:
    struct mem_t {
        uint8_t* buf;
        uint16_t size;
        uint16_t pos;
    };

    static bool mem_read_(cmp_ctx_t* ctx, void* data, size_t limit) {
        mem_t* m = static_cast<mem_t*>(ctx->buf);
        if (m->pos + limit > m->size) {
            return false;
        }
        memcpy(data, &m->buf[m->pos], limit);
        m->pos += limit;
        return true;
    }

    static bool mem_skip_(cmp_ctx_t* ctx, size_t count) {
        mem_t* m = static_cast<mem_t*>(ctx->buf);
        if (m->pos + count > m->size) {
            return false;
        }
        m->pos += count;
        return true;
    }

    static size_t mem_write_(cmp_ctx_t* ctx, const void* data, size_t count) {
        mem_t* m = static_cast<mem_t*>(ctx->buf);
        if (m->pos + count > m->size) {
            return 0;
        }
        memcpy(&m->buf[m->pos], data, count);
        m->pos += count;
        return count;
    }

    bool ok_;
    uint8_t buf_[N];
    mem_t mem_;
    cmp_ctx_t cmp_;
};

#endif /* end of include guard: MSGPACK_HPP */

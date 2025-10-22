#ifndef CRC16_H
#define CRC16_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief データ列に対してCRC16を計算する
 *
 * CRC16 CCITT False
 * poly:    0x1021
 * init:    0xFFFF
 * refin:   no
 * refout:  no
 * xoreout: 0x0000
 * check:   0x29B1
 * residue: 0x0000
 *
 * @param[in] data 計算対象の先頭ポインタ
 * @param[in] len  データの長さ
 * @return 計算結果のCRC
 */
uint16_t crc16(const uint8_t* data, size_t len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* end of include guard: CRC16_H */

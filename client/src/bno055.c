#include "bno055.h"

#include <stdbool.h>
#include <stdint.h>

#include <hardware/i2c.h>

static void write_register(bno055_dev_t* dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(dev->i2c_id, dev->addr, buf, 2, false);
}

static void read_registers(bno055_dev_t* dev, uint8_t reg, uint8_t* buf,
                           uint8_t len) {
    i2c_write_blocking(dev->i2c_id, dev->addr, &reg, 1, true);
    i2c_read_blocking(dev->i2c_id, dev->addr, buf, len, false);
}

static inline int16_t to_int16(uint8_t lsb, uint8_t msb) {
    return (int16_t)((msb << 8) | lsb);
}

uint8_t bno055_get_chip_id(bno055_dev_t* dev) {
    uint8_t buf;
    read_registers(dev, 0x00, &buf, 1);
    return buf;
}

bool bno055_is_chip_id_valid(uint8_t chip_id) {
    return chip_id == 0xA0;
}

void bno055_set_mode(bno055_dev_t* dev, uint8_t mode) {
    write_register(dev, 0x3D, 0x0C);
}

void bno055_read_accel(bno055_dev_t* dev, bno055_accel_t* acc) {
    uint8_t buf[6];
    read_registers(dev, 0x08, buf, 6);
    acc->x = to_int16(buf[0], buf[1]);
    acc->y = to_int16(buf[2], buf[3]);
    acc->z = to_int16(buf[4], buf[5]);
}

void bno055_read_gyro(bno055_dev_t* dev, bno055_gyro_t* gyro) {
    uint8_t buf[6];
    read_registers(dev, 0x14, buf, 6);
    gyro->x = to_int16(buf[0], buf[1]);
    gyro->y = to_int16(buf[2], buf[3]);
    gyro->z = to_int16(buf[4], buf[5]);
}

void bno055_read_mag(bno055_dev_t* dev, bno055_mag_t* mag) {
    uint8_t buf[6];
    read_registers(dev, 0x0E, buf, 6);
    mag->x = to_int16(buf[0], buf[1]);
    mag->y = to_int16(buf[2], buf[3]);
    mag->z = to_int16(buf[4], buf[5]);
}

void bno055_read_euler(bno055_dev_t* dev, bno055_euler_t* euler) {
    uint8_t buf[6];
    read_registers(dev, 0x1A, buf, 6);
    euler->heading = to_int16(buf[0], buf[1]);
    euler->roll = to_int16(buf[2], buf[3]);
    euler->pitch = to_int16(buf[4], buf[5]);
}

void bno055_read_quaternion(bno055_dev_t* dev,
                            bno055_quaternion_t* quaternion) {
    uint8_t buf[8];
    read_registers(dev, 0x20, buf, 8);
    quaternion->w = to_int16(buf[0], buf[1]);
    quaternion->x = to_int16(buf[2], buf[3]);
    quaternion->y = to_int16(buf[4], buf[5]);
    quaternion->z = to_int16(buf[6], buf[7]);
}

void bno055_read_linear_accel(bno055_dev_t* dev,
                              bno055_linear_accel_t* linear_accel) {
    uint8_t buf[6];
    read_registers(dev, 0x28, buf, 6);
    linear_accel->x = to_int16(buf[0], buf[1]);
    linear_accel->y = to_int16(buf[2], buf[3]);
    linear_accel->z = to_int16(buf[4], buf[5]);
}

void bno055_read_gravity(bno055_dev_t* dev, bno055_gravity_t* gravity) {
    uint8_t buf[6];
    read_registers(dev, 0x2E, buf, 6);
    gravity->x = to_int16(buf[0], buf[1]);
    gravity->y = to_int16(buf[2], buf[3]);
    gravity->z = to_int16(buf[4], buf[5]);
}

void bno055_read_calib_status(bno055_dev_t* dev,
                              bno055_calib_status_t* status) {
    uint8_t val;
    read_registers(dev, 0x35, &val, 1);
    status->sys = (val >> 6) & 0x03;
    status->gyro = (val >> 4) & 0x03;
    status->accel = (val >> 2) & 0x03;
    status->mag = val & 0x03;
}

void bno055_read_calib_data(bno055_dev_t* dev, bno055_calib_data_t* data) {
    uint8_t buf[22];
    read_registers(dev, 0x55, buf, 22);

    data->accel_x = to_int16(buf[0], buf[1]);
    data->accel_y = to_int16(buf[2], buf[3]);
    data->accel_z = to_int16(buf[4], buf[5]);

    data->gyro_x = to_int16(buf[6], buf[7]);
    data->gyro_y = to_int16(buf[8], buf[9]);
    data->gyro_z = to_int16(buf[10], buf[11]);

    data->mag_x = to_int16(buf[12], buf[13]);
    data->mag_y = to_int16(buf[14], buf[15]);
    data->mag_z = to_int16(buf[16], buf[17]);

    data->accel_radius = to_int16(buf[18], buf[19]);
    data->mag_radius = to_int16(buf[20], buf[21]);
}

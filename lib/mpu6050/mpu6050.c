#include "mpu6050.h"
#include "twi.h"
#include "uart.h"

static uint8_t mpu6050_write_reg(uint8_t reg, uint8_t data) {
    if (twi_start(MPU6050_ADDR | TWI_WRITE) != 0x18) {
        twi_stop();
        return 0;
    }
    twi_write(reg);
    twi_write(data);
    twi_stop();
    return 1;
}

void mpu6050_init(void) {
    twi_init(400000UL); // 400kHz
    
    uart_log("MPU6050: Init...");
    if (!mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x01)) {
        uart_log("MPU6050: FAIL (No Respond)");
        return;
    }
    mpu6050_write_reg(MPU6050_SMPLRT_DIV, 0x07);
    mpu6050_write_reg(MPU6050_CONFIG, 0x00);
    mpu6050_write_reg(MPU6050_GYRO_CONFIG, 0x00);
    mpu6050_write_reg(MPU6050_ACCEL_CONFIG, 0x00);
    uart_log("MPU6050: OK");
}

void mpu6050_read_all(mpu6050_data_t* data) {
    twi_start(MPU6050_ADDR | TWI_WRITE);
    twi_write(MPU6050_ACCEL_XOUT_H);
    twi_start(MPU6050_ADDR | TWI_READ);

    data->accel_x = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->accel_y = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->accel_z = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->temp    = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->gyro_x  = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->gyro_y  = (int16_t)((twi_read_ack() << 8) | twi_read_ack());
    data->gyro_z  = (int16_t)((twi_read_ack() << 8) | twi_read_nack());

    twi_stop();
}

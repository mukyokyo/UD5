/*!
 @file  sample14_IIC_IMU.cpp
 @brief I2C通信 IMU
 @note
  STEMMA QT/Qwiicコネクタを使用して9軸センサと通信
  adafruit社のLSM6DS3TR-C + LIS3MDLを想定
  ひとまず動作確認程度なので設定値や表示値に深い意
  味は無い
 */
#include <ud5.h>
#include <stdio.h>

CDXIF dx;

//! I2Cポートを初期化
CI2C Wire(CI2C::IIC_Fmp);

// 各センサのデフォルトアドレス
#define ADDR_LSM6 (0x6a)  //!< LSM6DS3TRのデバイスアドレス
#define ADDR_LIS3 (0x1c)  //!< LIS3MDLのデバイスアドレス

//! 9軸データ
typedef struct {
  int16_t acc[3];
  int16_t gyro[3];
  int16_t mag[3];
} TIMU;

//! 主要データ取り込み
void get_imu_data (TIMU *t){
  Wire.begin(ADDR_LSM6);
  Wire.write(0x28);
  Wire.begin(ADDR_LSM6, true);
  Wire.read((uint8_t *)&t->acc[0],6);
  Wire.end();

  Wire.begin(ADDR_LSM6);
  Wire.write(0x22);
  Wire.begin(ADDR_LSM6, true);
  Wire.read((uint8_t *)&t->gyro[0],6);
  Wire.end();

  Wire.begin(ADDR_LIS3);
  Wire.write(0x28);
  Wire.begin(ADDR_LIS3, true);
  Wire.read((uint8_t *)&t->mag[0],6);
  Wire.end();
}

//! main関数
int main (void){
  uint8_t x, y;
  dx.puts("\n\r    0 1 2 3 4 5 6 7 8 9 A B C D E F\n\r");
  for (y=0;y<8;y++) {
    x = 0;
    dx.printf("%02X:",y*16+x);
    for (x=0;x<16;x++) {
      dx.printf(" %c", Wire.ping(y*16+x) ? 'X' : '-');
    }
    dx.puts("\n\r");
  }

  if (Wire.ping(ADDR_LSM6) && Wire.ping(ADDR_LIS3)){
    // init LSM6DS3
    Wire.begin(ADDR_LSM6);
    Wire.write((const uint8_t[2]){0x10, 0x76},2); // 833Hz +/-16G
    Wire.end();
    Wire.begin(ADDR_LSM6);
    Wire.write((const uint8_t[2]){0x11, 0x7c},2); // 833Hz +/-2000dps
    Wire.end();

    // init LIS3MDL
    Wire.begin(ADDR_LIS3);
    Wire.write((const uint8_t[2]){0x21, 0b00},2); // +/-4gauss
    Wire.end();
    Wire.begin(ADDR_LIS3);
    Wire.write((const uint8_t[2]){0x22, 0b00},2); // Continuous converion
    Wire.end();

    while (1) {
      TIMU imu;
      get_imu_data (&imu);

      dx.printf("Acc :%5d %5d %5d, ", (imu.acc[0]*1600)/32767, (imu.acc[1]*1600)/32767, (imu.acc[2]*1600)/32767);
      dx.printf("Gyro:%5d %5d %5d, ", (imu.gyro[0]*2000)/32767, (imu.gyro[1]*2000)/32767, (imu.gyro[2]*2000)/32767);
      dx.printf("Mag :%5d %5d %5d\r", (imu.mag[0]*6842)/32767, (imu.mag[1]*6842)/32767, (imu.mag[2]*6842)/32767);

      if (dx.rxbuff()) { if(dx.getc() == '!') UD5_SOFTRESET(); }
      UD5_WAIT(20);
    }
  }
}

module Components {

  @ Simple port for sending IMU status to MorseBlinker.
  @ Encoding of 'status':
  @   0 -> IMU stopped
  @   1 -> IMU nominal (running, within thresholds)
  @   2 -> IMU accel norm high (possible acceleration disturbance / noise)
  @   3 -> IMU gyro norm high (possible rotational disturbance / noise)
  port ImuStatusPort(
    status: U8
  )

}

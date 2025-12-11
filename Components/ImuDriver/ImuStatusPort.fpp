module Components {

  @ Simple port used to send IMU status to the MorseBlinker
  @ status = 0  -> IMU stopped
  @ status = 1  -> IMU started
  port ImuStatusPort(
    status: U8
  )

}

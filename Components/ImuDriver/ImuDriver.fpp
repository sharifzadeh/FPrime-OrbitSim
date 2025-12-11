module Components {

  @ Simple placeholder IMU driver component
  @ Pattern aligned with MorseBlinker / HelloWorld
  active component ImuDriver {

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Command to start simulated IMU sampling
    async command IMU_START()

    @ Command to stop simulated IMU sampling
    async command IMU_STOP()

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Report that IMU sampling has been started
    event ImuStarted \
      severity activity high \
      format "ImuDriver: sampling started"

    @ Report that IMU sampling has been stopped
    event ImuStopped \
      severity activity high \
      format "ImuDriver: sampling stopped"

    # ----------------------------------------------------------------------
    # Ports
    # ----------------------------------------------------------------------

    @ Periodic scheduler tick (from a rate group)
    async input port schedIn: Svc.Sched

    @ Port for requesting the current time
    time get port timeCaller

    @ Ports for command dispatch / registration
    command reg port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut

    @ Ports for logging and telemetry
    text event port logTextOut
    event port logOut
    telemetry port tlmOut

    @ Connection to MorseBlinker:
    @   0 -> IMU stopped  (blink "F")
    @   1 -> IMU started  (blink "T")
    output port imuStatusOut: ImuStatusPort


    # ----------------------------------------------------------------------
    # Telemetry Channels
    # ----------------------------------------------------------------------

    @ Whether IMU sampling is enabled (1) or disabled (0)
    telemetry IMU_ENABLED: U8

    @ Simulated accelerometer outputs (body frame)
    telemetry IMU_ACCEL_X: F32
    telemetry IMU_ACCEL_Y: F32
    telemetry IMU_ACCEL_Z: F32

    @ Simulated gyroscope outputs (body frame)
    telemetry IMU_GYRO_X: F32
    telemetry IMU_GYRO_Y: F32
    telemetry IMU_GYRO_Z: F32
  }

}

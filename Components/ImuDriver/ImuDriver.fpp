module Components {

  active component ImuDriver {

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    async command IMU_START()

    async command IMU_STOP()

    @ Requires that IMU_STOP has been called before.
    async command IMU_RESET()

    @ Example:
    @   {"dt":0.01, "accelNoiseThresh": 12.0, "gyroNoiseThresh": 0.5}
    async command IMU_SET_CONFIG(
      config: string size 512 @< JSON configuration payload
    )

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    event ImuStarted \
      severity activity high \
      format "ImuDriver: sampling started"

    event ImuStopped \
      severity activity high \
      format "ImuDriver: sampling stopped"

    event ImuReset \
      severity activity high \
      format "ImuDriver: state reset"

    event ImuConfigReceived(
      config: string size 80 @< The raw config string received
    ) severity activity high \
      format "Rx Config: {}"

    event ImuConfigAccepted \
      severity activity high \
      format "ImuDriver: configuration accepted"

    event ImuConfigRejected(
      errorCode: U32 @< parse/validation error code
    ) severity warning high \
      format "ImuDriver: configuration rejected (error={})"

    # ----------------------------------------------------------------------
    # Ports
    # ----------------------------------------------------------------------

    async input port schedIn: Svc.Sched

    time get port timeCaller

    command reg  port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut

    text event port logTextOut
    event      port logOut
    telemetry  port tlmOut

    # 0 -> F, 1 -> T, 2 -> N/W, 3 -> E
    output port imuStatusOut: Components.ImuStatusPort

    # ----------------------------------------------------------------------
    # Telemetry Channels
    # ----------------------------------------------------------------------

    telemetry IMU_ENABLED: U8

    telemetry IMU_ACCEL_X: F32
    telemetry IMU_ACCEL_Y: F32
    telemetry IMU_ACCEL_Z: F32

    telemetry IMU_GYRO_X: F32
    telemetry IMU_GYRO_Y: F32
    telemetry IMU_GYRO_Z: F32

    telemetry IMU_POS_X: F32
    telemetry IMU_POS_Y: F32
    telemetry IMU_POS_Z: F32

    telemetry IMU_VEL_X: F32
    telemetry IMU_VEL_Y: F32
    telemetry IMU_VEL_Z: F32

    telemetry IMU_ACCEL_NORM: F32
    telemetry IMU_GYRO_NORM:  F32

    telemetry IMU_NOISE_FLAG: U8

    telemetry IMU_CONFIG_STATUS: U8
    telemetry IMU_CONFIG_ERROR: U32

    telemetry IMU_WORKFLOW_STATUS: U8
    telemetry IMU_WORKFLOW_ERROR: U32

    @ Human-readable view of config status
    telemetry IMU_CONFIG_STATUS_TEXT: string size 32

    @ Human-readable view of workflow status
    telemetry IMU_WORKFLOW_STATUS_TEXT: string size 64

  }

}

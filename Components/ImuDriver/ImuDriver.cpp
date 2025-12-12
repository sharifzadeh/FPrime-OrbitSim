#include <Components/ImuDriver/ImuDriver.hpp>

#include <Fw/Log/LogString.hpp>
#include <cmath>
#include <iostream>

namespace Components {

  // ----------------------------------------------------------------------
  // Construction / destruction
  // ----------------------------------------------------------------------

  ImuDriver::ImuDriver(const char* compName)
  : ImuDriverComponentBase(compName),
    m_enabled(false),
    m_sampleCount(0),
    m_timeSec(0.0F),
    m_ax(0.0F),
    m_ay(0.0F),
    m_az(0.0F),
    m_gx(0.0F),
    m_gy(0.0F),
    m_gz(0.0F)
  {
    // Zero orbital state
    m_rEci[0] = m_rEci[1] = m_rEci[2] = 0.0F;
    m_vEci[0] = m_vEci[1] = m_vEci[2] = 0.0F;

    // Simple constant biases (could be turned into parameters later)
    m_accelBias[0] = 0.01F;
    m_accelBias[1] = -0.02F;
    m_accelBias[2] = 0.015F;

    m_gyroBias[0] = 0.001F;
    m_gyroBias[1] = -0.0005F;
    m_gyroBias[2] = 0.0008F;
  }

  ImuDriver::~ImuDriver() = default;

  // ----------------------------------------------------------------------
  // Command handlers
  // ----------------------------------------------------------------------

  void ImuDriver::IMU_START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_enabled = true;
    this->resetState();

    std::cout << "[ImuDriver] IMU_START: enabling IMU, resetting state.\n";

    // Telemetry: IMU enabled
    this->tlmWrite_IMU_ENABLED(static_cast<U8>(1));

    // Event
    this->log_ACTIVITY_HI_ImuStarted();

    // Inform MorseBlinker via ImuStatusPort (1 = started)
    this->imuStatusOut_out(0, static_cast<U8>(1));

    // Cmd response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void ImuDriver::IMU_STOP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_enabled = false;

    std::cout << "[ImuDriver] IMU_STOP: disabling IMU.\n";

    // Telemetry: IMU disabled
    this->tlmWrite_IMU_ENABLED(static_cast<U8>(0));

    // Event
    this->log_ACTIVITY_HI_ImuStopped();

    // Inform MorseBlinker via ImuStatusPort (0 = stopped)
    this->imuStatusOut_out(0, static_cast<U8>(0));

    // Cmd response
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  // ----------------------------------------------------------------------
  // Scheduler tick
  // ----------------------------------------------------------------------

  void ImuDriver::schedIn_handler(
      FwIndexType portNum,
      U32 context
  ) {
    (void) portNum;
    (void) context;

    if (!this->m_enabled) {
      return;
    }

    // Advance simulation time
    this->m_sampleCount++;
    this->m_timeSec += SAMPLE_PERIOD_SEC;

    // 1) Propagate simple circular orbit
    this->propagateOrbit(SAMPLE_PERIOD_SEC);

    // 2) Compute "ideal" IMU signals from orbit + small maneuvers
    F32 accelIdeal[3] = {0.0F, 0.0F, 0.0F};
    F32 gyroIdeal[3]  = {0.0F, 0.0F, 0.0F};
    this->computeIdealImu(accelIdeal, gyroIdeal);

    // 3) Apply bias and deterministic pseudo-noise
    this->applyBiasAndNoise(accelIdeal, gyroIdeal);

    // 4) Write telemetry so it shows up in GDS
    this->tlmWrite_IMU_ACCEL_X(this->m_ax);
    this->tlmWrite_IMU_ACCEL_Y(this->m_ay);
    this->tlmWrite_IMU_ACCEL_Z(this->m_az);

    this->tlmWrite_IMU_GYRO_X(this->m_gx);
    this->tlmWrite_IMU_GYRO_Y(this->m_gy);
    this->tlmWrite_IMU_GYRO_Z(this->m_gz);
  }

  // ----------------------------------------------------------------------
  // Helper methods
  // ----------------------------------------------------------------------

  void ImuDriver::resetState() {
    m_sampleCount = 0;
    m_timeSec     = 0.0F;

    // Circular orbit radius
    const F32 rOrbit = R_EARTH + ORBIT_ALTITUDE;

    // Place spacecraft at x = rOrbit, y = 0, z = 0 in ECI
    m_rEci[0] = rOrbit;
    m_rEci[1] = 0.0F;
    m_rEci[2] = 0.0F;

    // Circular velocity magnitude
    const F32 n = std::sqrt(MU_EARTH / (rOrbit * rOrbit * rOrbit));  // [rad/s]
    const F32 v = n * rOrbit;                                         // [m/s]

    // Velocity along +y direction (right-handed orbit, equatorial)
    m_vEci[0] = 0.0F;
    m_vEci[1] = v;
    m_vEci[2] = 0.0F;

    // Reset last outputs
    m_ax = m_ay = m_az = 0.0F;
    m_gx = m_gy = m_gz = 0.0F;
  }

  void ImuDriver::propagateOrbit(const F32 dt) {
    // For now: keep altitude fixed, update only in-plane angle theta = n * t.
    const F32 rOrbit = R_EARTH + ORBIT_ALTITUDE;
    const F32 n      = std::sqrt(MU_EARTH / (rOrbit * rOrbit * rOrbit));  // [rad/s]

    const F32 theta  = n * this->m_timeSec;

    // Update position in equatorial plane
    const F32 c = std::cos(theta);
    const F32 s = std::sin(theta);

    m_rEci[0] = rOrbit * c;
    m_rEci[1] = rOrbit * s;
    m_rEci[2] = 0.0F;

    // Velocity is tangential: v = n x r
    const F32 v = n * rOrbit;
    m_vEci[0] = -v * s;
    m_vEci[1] =  v * c;
    m_vEci[2] =  0.0F;

    (void) dt;  // dt kept in case we later need non-uniform steps
  }

  void ImuDriver::computeIdealImu(
      F32 accel[3],
      F32 gyro[3]
  ) const {
    // --------------------------------------------------------------
    // Conceptual model:
    // - Spacecraft is in (nearly) free fall: specific force ~ 0
    // --------------------------------------------------------------
    const F32 t = this->m_timeSec;

    // Small lateral accelerations in body frame
    accel[0] = ACCEL_LATERAL_SCALE * std::sin(0.7F * t);
    accel[1] = ACCEL_LATERAL_SCALE * std::cos(0.5F * t);
    accel[2] = 0.02F * ACCEL_LATERAL_SCALE * std::sin(0.2F * t);

    // Gyro: base on orbit rate + small wobble
    const F32 rOrbit = R_EARTH + ORBIT_ALTITUDE;
    const F32 n      = std::sqrt(MU_EARTH / (rOrbit * rOrbit * rOrbit));  // [rad/s]

    gyro[0] = 0.2F * GYRO_SCALE * std::sin(0.3F * t);  // small roll rate
    gyro[1] = 0.2F * GYRO_SCALE * std::cos(0.4F * t);  // small pitch rate
    gyro[2] = n + 0.5F * GYRO_SCALE * std::sin(0.1F * t); // yaw ≈ orbit rate
  }

  void ImuDriver::applyBiasAndNoise(
      const F32 accelIdeal[3],
      const F32 gyroIdeal[3]
  ) {
    // ----------------------------------------------------------------
    // ----------------------------------------------------------------
    const F32 k = static_cast<F32>(this->m_sampleCount);

    const F32 noiseA0 = 0.005F * std::sin(0.11F * k);
    const F32 noiseA1 = 0.005F * std::cos(0.17F * k);
    const F32 noiseA2 = 0.005F * std::sin(0.07F * k + 0.5F);

    const F32 noiseG0 = 0.0003F * std::cos(0.13F * k);
    const F32 noiseG1 = 0.0003F * std::sin(0.09F * k + 0.2F);
    const F32 noiseG2 = 0.0003F * std::cos(0.05F * k - 0.7F);

    // Accel = ideal + bias + noise
    m_ax = accelIdeal[0] + m_accelBias[0] + noiseA0;
    m_ay = accelIdeal[1] + m_accelBias[1] + noiseA1;
    m_az = accelIdeal[2] + m_accelBias[2] + noiseA2;

    // Gyro = ideal + bias + noise
    m_gx = gyroIdeal[0] + m_gyroBias[0] + noiseG0;
    m_gy = gyroIdeal[1] + m_gyroBias[1] + noiseG1;
    m_gz = gyroIdeal[2] + m_gyroBias[2] + noiseG2;
  }

}  // namespace Components

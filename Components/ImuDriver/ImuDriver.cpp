#include <Components/ImuDriver/ImuDriver.hpp>

#include <cmath>
#include <iostream>

namespace Components {

  ImuDriver::ImuDriver(const char* compName)
  : ImuDriverComponentBase(compName),
    m_enabled(false),
    m_sampleCount(0),
    m_ax(0.0F),
    m_ay(0.0F),
    m_az(0.0F),
    m_gx(0.0F),
    m_gy(0.0F),
    m_gz(0.0F) {}

  ImuDriver::~ImuDriver() = default;

  void ImuDriver::IMU_START_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_enabled = true;
    this->m_sampleCount = 0;

    std::cout << "[ImuDriver] IMU_START command received. Enabling IMU."
              << std::endl;

    // Update IMU_ENABLED telemetry
    this->tlmWrite_IMU_ENABLED(static_cast<U8>(1));

    // Notify MorseBlinker: IMU started -> 1 ("T")
    this->imuStatusOut_out(0, static_cast<U8>(1));

    // Emit event
    this->log_ACTIVITY_HI_ImuStarted();

    // Respond OK
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void ImuDriver::IMU_STOP_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    this->m_enabled = false;

    std::cout << "[ImuDriver] IMU_STOP command received. Disabling IMU."
              << std::endl;

    // Update IMU_ENABLED telemetry
    this->tlmWrite_IMU_ENABLED(static_cast<U8>(0));

    // Notify MorseBlinker: IMU stopped -> 0 ("F")
    this->imuStatusOut_out(0, static_cast<U8>(0));

    // Emit event
    this->log_ACTIVITY_HI_ImuStopped();

    // Respond OK
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void ImuDriver::schedIn_handler(
      FwIndexType portNum,
      U32 context
  ) {
    (void) portNum;
    (void) context;

    if (!this->m_enabled) {
      return;
    }

    // Simple time variable for simulated motion
    ++this->m_sampleCount;
    const F32 t = static_cast<F32>(this->m_sampleCount) * 0.01F;

    // --------------------------------------------------------------------
    // Simulated accelerometer:
    //   ax, ay ~ small lateral motion
    //   az ~ gravity-like bias
    // --------------------------------------------------------------------
    this->m_ax = 0.1F * std::sin(t);
    this->m_ay = 0.1F * std::cos(t);
    this->m_az = 9.81F;  // gravity-like constant

    // --------------------------------------------------------------------
    // Simulated gyroscope:
    //   gx, gy, gz ~ slow rotation
    // --------------------------------------------------------------------
    this->m_gx = 0.05F * std::sin(0.5F * t);
    this->m_gy = 0.05F * std::cos(0.5F * t);
    this->m_gz = 0.02F;

    // --------------------------------------------------------------------
    // Publish telemetry so it appears in the GDS Telemetry view
    // --------------------------------------------------------------------
    this->tlmWrite_IMU_ACCEL_X(this->m_ax);
    this->tlmWrite_IMU_ACCEL_Y(this->m_ay);
    this->tlmWrite_IMU_ACCEL_Z(this->m_az);

    this->tlmWrite_IMU_GYRO_X(this->m_gx);
    this->tlmWrite_IMU_GYRO_Y(this->m_gy);
    this->tlmWrite_IMU_GYRO_Z(this->m_gz);
  }

}  // namespace Components

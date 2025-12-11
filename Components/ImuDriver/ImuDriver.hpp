#ifndef COMPONENTS_IMUDRIVER_IMUDRIVER_HPP
#define COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

#include <Fw/Types/BasicTypes.hpp>
#include <Components/ImuDriver/ImuDriverComponentAc.hpp>

namespace Components {

  class ImuDriver final : public ImuDriverComponentBase {
   public:
    ImuDriver(const char* compName);
    ~ImuDriver() override;

   private:
    // Whether IMU sampling is enabled
    bool m_enabled;

    // Simple sample counter used for simulated dynamics
    U32 m_sampleCount;

    // Simulated accelerometer readings [m/s^2]
    F32 m_ax;
    F32 m_ay;
    F32 m_az;

    // Simulated gyroscope readings [rad/s]
    F32 m_gx;
    F32 m_gy;
    F32 m_gz;

    // Command handlers
    void IMU_START_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    void IMU_STOP_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    // Scheduler tick handler (from Svc.Sched)
    void schedIn_handler(
        FwIndexType portNum,
        U32 context
    ) override;
  };

}  // namespace Components

#endif  // COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

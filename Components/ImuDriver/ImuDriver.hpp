#ifndef COMPONENTS_IMUDRIVER_IMUDRIVER_HPP
#define COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

#include <Components/ImuDriver/ImuDriverComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Types/String.hpp>

namespace Components {

  class ImuDriver final : public ImuDriverComponentBase {
   public:
    ImuDriver(const char* compName);
    ~ImuDriver() override;

   private:
    // Internal enums

    enum ImuState : U8 {
      IMU_STATE_NEVER_STARTED = 0,
      IMU_STATE_RUNNING       = 1,
      IMU_STATE_STOPPED       = 2,
      IMU_STATE_RESET         = 3
    };

    enum LastCommand : U8 {
      CMD_NONE  = 0,
      CMD_START = 1,
      CMD_STOP  = 2,
      CMD_RESET = 3
    };

    enum WorkflowError : U32 {
      WF_OK                     = 0,
      WF_NEED_START             = 1,
      WF_ALREADY_STARTED        = 2,
      WF_ALREADY_STOPPED        = 3,
      WF_NEED_STOP_BEFORE_RESET = 4,
      WF_DOUBLE_RESET           = 5,
      WF_CONFIG_PARSE_ERROR     = 10,
      WF_CONFIG_RANGE_ERROR     = 11,
      WF_RESET_EMPTY_CONFIG     = 12
    };

    // Internal state

    bool        m_enabled;
    ImuState    m_state;
    LastCommand m_lastCommand;
    bool        m_hasValidConfig;

    U32 m_sampleCount;

    // accel [m/s^2]
    F32 m_ax, m_ay, m_az;
    // gyro [rad/s]
    F32 m_gx, m_gy, m_gz;

    F32 m_dt;

    // position [m]
    F32 m_posX, m_posY, m_posZ;
    // velocity [m/s]
    F32 m_velX, m_velY, m_velZ;

    F32 m_accelNorm;
    F32 m_gyroNorm;

    F32 m_accelNoiseThresh;
    F32 m_gyroNoiseThresh;

    U8  m_noiseFlag;

    U8  m_configStatus;
    U32 m_configError;

    U8  m_workflowStatus;
    U32 m_workflowErrorCode;

    // 0=F, 1=T, 2=W, 3=E
    U8  m_imuStatus;

    // Command handlers

    void IMU_START_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    void IMU_STOP_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    void IMU_RESET_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    void IMU_SET_CONFIG_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq,
        const Fw::CmdStringArg& config
    ) override;

    // Scheduler tick handler

    void schedIn_handler(
        FwIndexType portNum,
        U32 context
    ) override;

    // Helpers

    bool parseConfig(
        const Fw::CmdStringArg& config,
        U32& errorCode,
        F32& outDt,
        F32& outAccelThresh,
        F32& outGyroThresh
    );

    void applyConfig(
        F32 dt,
        F32 accelThresh,
        F32 gyroThresh
    );

    void resetState();
    void updateHealthStatus();

    void publishConfigStatus(
        U8 status,
        U32 errorCode,
        const char* text
    );

    void publishWorkflowStatus(
        U8 status,
        WorkflowError error,
        const char* text
    );

    void registerDuplicateCommand(
        LastCommand cmd,
        WorkflowError& outError,
        const char*& outText
    );

    void handleConfigError(
        WorkflowError error,
        const char* text
    );
  };

}  // namespace Components

#endif  // COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

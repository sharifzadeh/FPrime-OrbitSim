#include <Components/ImuDriver/ImuDriver.hpp>
#include <Fw/Types/String.hpp>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>

namespace Components {

  // ----------------------------------------------------------------------
  // Construction / initialization
  // ----------------------------------------------------------------------

  ImuDriver::ImuDriver(const char* compName)
  : ImuDriverComponentBase(compName),
    m_enabled(false),
    m_state(IMU_STATE_NEVER_STARTED),
    m_lastCommand(CMD_NONE),
    m_hasValidConfig(false),
    m_sampleCount(0U),
    m_ax(0.0F), m_ay(0.0F), m_az(0.0F),
    m_gx(0.0F), m_gy(0.0F), m_gz(0.0F),
    m_dt(0.01F),
    m_posX(0.0F), m_posY(0.0F), m_posZ(0.0F),
    m_velX(0.0F), m_velY(0.0F), m_velZ(0.0F),
    m_accelNorm(0.0F),
    m_gyroNorm(0.0F),
    m_accelNoiseThresh(20.0F),
    m_gyroNoiseThresh(1.0F),
    m_noiseFlag(0U),
    m_configStatus(0U),
    m_configError(0U),
    m_workflowStatus(0U),
    m_workflowErrorCode(static_cast<U32>(WF_OK)),
    m_imuStatus(0U) {
  }

  ImuDriver::~ImuDriver() = default;

  // ----------------------------------------------------------------------
  // Small helpers
  // ----------------------------------------------------------------------

  void ImuDriver::publishConfigStatus(
      U8 status,
      U32 errorCode,
      const char* text
  ) {
    this->m_configStatus = status;
    this->m_configError  = errorCode;

    this->tlmWrite_IMU_CONFIG_STATUS(status);
    this->tlmWrite_IMU_CONFIG_ERROR(errorCode);

    if (text != nullptr) {
      Fw::String s(text);
      this->tlmWrite_IMU_CONFIG_STATUS_TEXT(s);
    }
  }

  void ImuDriver::publishWorkflowStatus(
      U8 status,
      WorkflowError error,
      const char* text
  ) {
    this->m_workflowStatus    = status;
    this->m_workflowErrorCode = static_cast<U32>(error);

    this->tlmWrite_IMU_WORKFLOW_STATUS(status);
    this->tlmWrite_IMU_WORKFLOW_ERROR(this->m_workflowErrorCode);

    if (text != nullptr) {
      Fw::String s(text);
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);
    }

    this->updateHealthStatus();
  }

  void ImuDriver::resetState() {
    this->m_sampleCount = 0U;

    this->m_ax = this->m_ay = this->m_az = 0.0F;
    this->m_gx = this->m_gy = this->m_gz = 0.0F;

    this->m_posX = this->m_posY = this->m_posZ = 0.0F;
    this->m_velX = this->m_velY = this->m_velZ = 0.0F;

    this->m_accelNorm = 0.0F;
    this->m_gyroNorm  = 0.0F;

    this->m_noiseFlag = 0U;

    this->tlmWrite_IMU_ACCEL_X(this->m_ax);
    this->tlmWrite_IMU_ACCEL_Y(this->m_ay);
    this->tlmWrite_IMU_ACCEL_Z(this->m_az);

    this->tlmWrite_IMU_GYRO_X(this->m_gx);
    this->tlmWrite_IMU_GYRO_Y(this->m_gy);
    this->tlmWrite_IMU_GYRO_Z(this->m_gz);

    this->tlmWrite_IMU_POS_X(this->m_posX);
    this->tlmWrite_IMU_POS_Y(this->m_posY);
    this->tlmWrite_IMU_POS_Z(this->m_posZ);

    this->tlmWrite_IMU_VEL_X(this->m_velX);
    this->tlmWrite_IMU_VEL_Y(this->m_velY);
    this->tlmWrite_IMU_VEL_Z(this->m_velZ);

    this->tlmWrite_IMU_ACCEL_NORM(this->m_accelNorm);
    this->tlmWrite_IMU_GYRO_NORM(this->m_gyroNorm);
    this->tlmWrite_IMU_NOISE_FLAG(this->m_noiseFlag);
  }

  void ImuDriver::updateHealthStatus() {
    U8 newStatus = 0U;

    // Logic:
    // 0 -> Stopped (F)
    // 1 -> Running (T)
    // 2 -> Warning (N or W)
    // 3 -> Error (E)

    // Check critical config errors (Parse Error)
    if (this->m_configStatus == 2U) { 
         newStatus = 3U; // Error -> E
    }
    // Check workflow status
    else if (this->m_workflowStatus == 2U) {
        // Distinguish between Warnings and Errors
        U32 err = this->m_workflowErrorCode;
        if (err == WF_CONFIG_RANGE_ERROR || 
            err == WF_ALREADY_STARTED || 
            err == WF_ALREADY_STOPPED || 
            err == WF_NEED_START ||
            err == WF_DOUBLE_RESET) {
            newStatus = 2U; // Warning -> N/W (Not E)
        } else {
            newStatus = 3U; // Unknown/Severe Error -> E
        }
    } 
    else if (!this->m_enabled) {
      newStatus = 0U;    // stopped -> F
    } else if (this->m_noiseFlag != 0U) {
      newStatus = 2U;    // noisy   -> N/W
    } else {
      newStatus = 1U;    // running -> T
    }

    if (newStatus != this->m_imuStatus) {
      this->m_imuStatus = newStatus;

      if (this->isConnected_imuStatusOut_OutputPort(0)) {
        this->imuStatusOut_out(0, this->m_imuStatus);
      }
    }
  }

  void ImuDriver::registerDuplicateCommand(
      LastCommand cmd,
      WorkflowError& outError,
      const char*& outText
  ) {
    if (cmd == CMD_START) {
      outError = WF_ALREADY_STARTED;
      outText  = "ALREADY_STARTED";
    } else if (cmd == CMD_STOP) {
      outError = WF_ALREADY_STOPPED;
      outText  = "ALREADY_STOPPED";
    } else {
      outError = WF_DOUBLE_RESET;
      outText  = "DOUBLE_RESET";
    }
  }

  void ImuDriver::handleConfigError(
      WorkflowError error,
      const char* text
  ) {
    // Config status
    this->publishConfigStatus(
        2U,
        static_cast<U32>(error),
        text
    );

    // Workflow status
    this->publishWorkflowStatus(
        2U,
        error,
        text
    );

    // Stop sampling on any config error
    this->m_enabled = false;
    this->m_state   = IMU_STATE_STOPPED;
    this->log_ACTIVITY_HI_ImuStopped();

    this->updateHealthStatus();
  }

  // ----------------------------------------------------------------------
  // Command handlers
  // ----------------------------------------------------------------------

  void ImuDriver::IMU_START_cmdHandler(
      FwOpcodeType opCode,
      U32 cmdSeq
  ) {
    // ----------------------------------------------------------------------
    // 1. Duplicate Command Check
    // ----------------------------------------------------------------------
    if (this->m_state == IMU_STATE_RUNNING) {
      // Just telemetry warning, NO health update (prevents LED blink change)
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U); 
      this->tlmWrite_IMU_WORKFLOW_ERROR(WF_ALREADY_STARTED);
      Fw::String s("ALREADY_STARTED");
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);

      this->cmdResponse_out(
          opCode,
          cmdSeq,
          Fw::CmdResponse::VALIDATION_ERROR
      );
      return;
    }

    // ----------------------------------------------------------------------
    // 2. State Update
    // ----------------------------------------------------------------------
    this->m_enabled     = true;
    this->m_state       = IMU_STATE_RUNNING;
    this->m_lastCommand = CMD_START;

    const char* statusText = (this->m_sampleCount > 0) ? "RESUMING" : "STARTED";

    // ----------------------------------------------------------------------
    // 3. Telemetry & Events
    // ----------------------------------------------------------------------
    this->tlmWrite_IMU_ENABLED(1U);
    
    // Updates health -> triggers LED 'T'
    this->publishWorkflowStatus(1U, WF_OK, statusText);
    
    this->log_ACTIVITY_HI_ImuStarted();

    // ----------------------------------------------------------------------
    // 4. Command Response
    // ----------------------------------------------------------------------
    this->cmdResponse_out(
        opCode,
        cmdSeq,
        Fw::CmdResponse::OK
    );
  }

  void ImuDriver::IMU_STOP_cmdHandler(
      FwOpcodeType opCode,
      U32 cmdSeq
  ) {
    if (this->m_state != IMU_STATE_RUNNING) {
      WorkflowError err;
      const char* text = nullptr;

      if (this->m_state == IMU_STATE_NEVER_STARTED) {
        err  = WF_NEED_START;
        text = "NEED_START";
      } else {
        // ALREADY_STOPPED
        this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
        this->tlmWrite_IMU_WORKFLOW_ERROR(WF_ALREADY_STOPPED);
        Fw::String s("ALREADY_STOPPED");
        this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);
        
        // Return error WITHOUT updating LED (keep current state 'F' or off)
        this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
        return;
      }

      // If NEVER STARTED, we might want to just warn via TLM too, to avoid LED E
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
      this->tlmWrite_IMU_WORKFLOW_ERROR(err);
      if (text) { Fw::String s(text); this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s); }

      this->cmdResponse_out(
          opCode,
          cmdSeq,
          Fw::CmdResponse::VALIDATION_ERROR
      );
      return;
    }

    this->m_enabled     = false;
    this->m_state       = IMU_STATE_STOPPED;
    this->m_lastCommand = CMD_STOP;

    this->tlmWrite_IMU_ENABLED(0U);
    this->publishWorkflowStatus(1U, WF_OK, "STOPPED");
    this->log_ACTIVITY_HI_ImuStopped();

    this->cmdResponse_out(
        opCode,
        cmdSeq,
        Fw::CmdResponse::OK
    );
  }

  void ImuDriver::IMU_RESET_cmdHandler(
      FwOpcodeType opCode,
      U32 cmdSeq
  ) {
    // Must be stopped before reset
    if (this->m_state == IMU_STATE_RUNNING) {
      // Just TLM warning, no LED blink
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
      this->tlmWrite_IMU_WORKFLOW_ERROR(WF_NEED_STOP_BEFORE_RESET);
      Fw::String s("NEED_STOP_BEFORE_RESET");
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);

      this->cmdResponse_out(
          opCode,
          cmdSeq,
          Fw::CmdResponse::VALIDATION_ERROR
      );
      return;
    }

    // Avoid double reset
    if (this->m_state == IMU_STATE_STOPPED &&
        this->m_lastCommand == CMD_RESET) {
      
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
      this->tlmWrite_IMU_WORKFLOW_ERROR(WF_DOUBLE_RESET);
      Fw::String s("DOUBLE_RESET");
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);

      this->cmdResponse_out(
          opCode,
          cmdSeq,
          Fw::CmdResponse::VALIDATION_ERROR
      );
      return;
    }

    // Cannot reset if there is no config to clear
    if (!this->m_hasValidConfig) {
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
      this->tlmWrite_IMU_WORKFLOW_ERROR(WF_RESET_EMPTY_CONFIG);
      Fw::String s("RESET_EMPTY_CONFIG");
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);

      this->cmdResponse_out(
          opCode,
          cmdSeq,
          Fw::CmdResponse::VALIDATION_ERROR
      );
      return;
    }

    this->resetState();

    this->m_state          = IMU_STATE_RESET;
    this->m_lastCommand    = CMD_RESET;
    this->m_hasValidConfig = false;

    this->publishWorkflowStatus(1U, WF_OK, "RESET");
    this->log_ACTIVITY_HI_ImuReset();

    this->cmdResponse_out(
        opCode,
        cmdSeq,
        Fw::CmdResponse::OK
    );
  }

  void ImuDriver::IMU_SET_CONFIG_cmdHandler(
      FwOpcodeType opCode,
      U32 cmdSeq,
      const Fw::CmdStringArg& config
  ) {
    // 1. Clear previous errors
    this->m_configStatus = 0U;
    this->m_configError = 0U;

    // 2. DEBUG: Log exactly what we received 
    this->log_ACTIVITY_HI_ImuConfigReceived(config);

    if (this->m_state != IMU_STATE_RUNNING) {
      // Don't change LED status, just report to GDS
      this->tlmWrite_IMU_WORKFLOW_STATUS(2U);
      this->tlmWrite_IMU_WORKFLOW_ERROR(WF_NEED_START);
      Fw::String s("NEED_START");
      this->tlmWrite_IMU_WORKFLOW_STATUS_TEXT(s);

      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
      return;
    }

    U32 parseError = 0U;
    F32 newDt   = this->m_dt;
    F32 newATh  = this->m_accelNoiseThresh;
    F32 newGTh  = this->m_gyroNoiseThresh;

    // 3. Attempt Partial Parsing
    const bool ok = this->parseConfig(config, parseError, newDt, newATh, newGTh);

    if (!ok) {
      this->handleConfigError(WF_CONFIG_PARSE_ERROR, "CONFIG_PARSE_ERROR");
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::FORMAT_ERROR);
      return;
    }

    // 4. Range Check
    const bool inRange =
        (newDt  > 0.0F && newDt  <= 0.1F) &&
        (newATh > 0.0F && newATh <= 100.0F) &&
        (newGTh > 0.0F && newGTh <= 10.0F);

    if (!inRange) {
      // Range Warning -> Send Warning but DO NOT stop
      this->publishWorkflowStatus(2U, WF_CONFIG_RANGE_ERROR, "CONFIG_RANGE_WARNING");
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::VALIDATION_ERROR);
      return;
    }

    this->applyConfig(newDt, newATh, newGTh);
    this->m_hasValidConfig = true;

    this->publishConfigStatus(1U, 0U, "CONFIG_OK");
    this->publishWorkflowStatus(1U, WF_OK, "CONFIG_OK");
    this->log_ACTIVITY_HI_ImuConfigAccepted();

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  // ----------------------------------------------------------------------
  // Scheduler
  // ----------------------------------------------------------------------

  void ImuDriver::schedIn_handler(
      FwIndexType portNum,
      U32 context
  ) {
    (void) portNum;
    (void) context;

    this->tlmWrite_IMU_ENABLED(this->m_enabled ? 1U : 0U);

    if (!this->m_enabled) {
      this->updateHealthStatus();
      return;
    }

    this->m_sampleCount++;

    const F32 t = static_cast<F32>(this->m_sampleCount) * this->m_dt;

    this->m_ax = 0.1F * std::sinf(t);
    this->m_ay = 0.1F * std::cosf(t);
    this->m_az = 9.81F;

    this->m_gx = 0.01F * std::sinf(0.5F * t);
    this->m_gy = 0.01F * std::cosf(0.5F * t);
    this->m_gz = 0.02F;

    this->m_velX += this->m_ax * this->m_dt;
    this->m_velY += this->m_ay * this->m_dt;
    this->m_velZ += this->m_az * this->m_dt;

    this->m_posX += this->m_velX * this->m_dt;
    this->m_posY += this->m_velY * this->m_dt;
    this->m_posZ += this->m_velZ * this->m_dt;

    this->m_accelNorm =
      std::sqrt(this->m_ax*this->m_ax +
                this->m_ay*this->m_ay +
                this->m_az*this->m_az);

    this->m_gyroNorm =
      std::sqrt(this->m_gx*this->m_gx +
                this->m_gy*this->m_gy +
                this->m_gz*this->m_gz);

    this->m_noiseFlag = 0U;
    if (this->m_accelNorm > this->m_accelNoiseThresh ||
        this->m_gyroNorm  > this->m_gyroNoiseThresh) {
      this->m_noiseFlag = 1U;
    }

    this->tlmWrite_IMU_ACCEL_X(this->m_ax);
    this->tlmWrite_IMU_ACCEL_Y(this->m_ay);
    this->tlmWrite_IMU_ACCEL_Z(this->m_az);

    this->tlmWrite_IMU_GYRO_X(this->m_gx);
    this->tlmWrite_IMU_GYRO_Y(this->m_gy);
    this->tlmWrite_IMU_GYRO_Z(this->m_gz);

    this->tlmWrite_IMU_POS_X(this->m_posX);
    this->tlmWrite_IMU_POS_Y(this->m_posY);
    this->tlmWrite_IMU_POS_Z(this->m_posZ);

    this->tlmWrite_IMU_VEL_X(this->m_velX);
    this->tlmWrite_IMU_VEL_Y(this->m_velY);
    this->tlmWrite_IMU_VEL_Z(this->m_velZ);

    this->tlmWrite_IMU_ACCEL_NORM(this->m_accelNorm);
    this->tlmWrite_IMU_GYRO_NORM(this->m_gyroNorm);
    this->tlmWrite_IMU_NOISE_FLAG(this->m_noiseFlag);

    this->updateHealthStatus();
  }

  // ----------------------------------------------------------------------
  // Config parsing / application
  // ----------------------------------------------------------------------

 // ----------------------------------------------------------------------
  // Strict Config Parser Implementation
  // ----------------------------------------------------------------------
  bool ImuDriver::parseConfig(
    const Fw::CmdStringArg& config,
    U32& errorCode,
    F32& outDt,
    F32& outAccelThresh,
    F32& outGyroThresh
) {
  errorCode = 0U;
  const char* p = config.toChar();

  // 1. Basic Validation
  if (p == nullptr || *p == '\0') {
      errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
      return false;
  }

  // Temporary storage to hold values until we are sure everything is valid
  F32 tempDt = outDt;
  F32 tempAcc = outAccelThresh;
  F32 tempGyro = outGyroThresh;

  // Flags to detect duplicates
  bool seenDt = false;
  bool seenAcc = false;
  bool seenGyro = false;

  // Helper to skip whitespace (spaces, tabs, newlines)
  auto skipSpace = [&](const char*& ptr) {
      while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
          ptr++;
      }
  };

  // 2. Expect opening brace '{'
  skipSpace(p);
  if (*p != '{') {
      errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
      return false;
  }
  p++; // Consume '{'

  // 3. Loop through Key-Value pairs
  while (true) {
      skipSpace(p);

      // Check for end of JSON '}'
      if (*p == '}') {
          p++; // Consume '}'
          break; // Done successfully
      }

      // Expect Key Start '"'
      if (*p != '"') {
           errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
           return false;
      }
      p++; // Consume opening quote

      // Read Key
      const char* keyStart = p;
      while (*p != '"' && *p != '\0') {
          p++;
      }
      if (*p != '"') { // String ended without closing quote (Truncated?)
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
          return false;
      }
      
      // Calculate key length
      size_t keyLen = p - keyStart;
      p++; // Consume closing quote

      // 4. Validate Key & Check Duplicates
      // We compare the extracted string with known keys
      bool isDt   = (keyLen == 2 && std::strncmp(keyStart, "dt", 2) == 0);
      bool isAcc  = (keyLen == 16 && std::strncmp(keyStart, "accelNoiseThresh", 16) == 0);
      bool isGyro = (keyLen == 15 && std::strncmp(keyStart, "gyroNoiseThresh", 15) == 0);

      if (!isDt && !isAcc && !isGyro) {
          // UNKNOWN KEY FOUND ("aa", "dtt", etc.) -> Error
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR); 
          return false;
      }

      if ((isDt && seenDt) || (isAcc && seenAcc) || (isGyro && seenGyro)) {
          // DUPLICATE KEY FOUND -> Error
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
          return false;
      }

      // 5. Expect Colon ':'
      skipSpace(p);
      if (*p != ':') {
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
          return false;
      }
      p++; // Consume ':'

      // 6. Parse Value
      skipSpace(p);
      char* endPtr = nullptr;
      F32 val = std::strtof(p, &endPtr);

      if (p == endPtr) { // No number found
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
          return false;
      }
      p = endPtr; // Advance pointer

      // Store value in temp variables
      if (isDt) { tempDt = val; seenDt = true; }
      if (isAcc) { tempAcc = val; seenAcc = true; }
      if (isGyro) { tempGyro = val; seenGyro = true; }

      // 7. Expect Comma ',' or End '}'
      skipSpace(p);
      if (*p == ',') {
          p++; // Consume comma and continue loop
          continue;
      } else if (*p == '}') {
          p++; // Consume closing brace
          break; // Done
      } else {
          // Unexpected character
          errorCode = static_cast<U32>(WF_CONFIG_PARSE_ERROR);
          return false;
      }
  }

  // Validation passed, update output variables
  outDt = tempDt;
  outAccelThresh = tempAcc;
  outGyroThresh = tempGyro;

  return true;
}

  void ImuDriver::applyConfig(
      F32 dt,
      F32 accelThresh,
      F32 gyroThresh
  ) {
    this->m_dt               = dt;
    this->m_accelNoiseThresh = accelThresh;
    this->m_gyroNoiseThresh  = gyroThresh;
  }

}  // namespace Components
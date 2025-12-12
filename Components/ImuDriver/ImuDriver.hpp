#ifndef COMPONENTS_IMUDRIVER_IMUDRIVER_HPP
#define COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

#include <Components/ImuDriver/ImuDriverComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>

namespace Components {

  class ImuDriver final : public ImuDriverComponentBase {
   public:
    ImuDriver(const char* compName);
    ~ImuDriver() override;

   private:
    // ------------------------------------------------------------------
    // Configuration constants (simple LEO-like orbit model)
    // ------------------------------------------------------------------
    static constexpr F32 MU_EARTH          = 3.986004418e14F;  // [m^3/s^2]
    static constexpr F32 R_EARTH           = 6378.0e3F;        // [m]
    static constexpr F32 ORBIT_ALTITUDE    = 400.0e3F;         // [m]
    static constexpr F32 SAMPLE_PERIOD_SEC = 0.01F;            // [s]

    // Small fake maneuver / motion scale (for "nice" plots)
    static constexpr F32 ACCEL_LATERAL_SCALE = 0.1F;           // [m/s^2]
    static constexpr F32 GYRO_SCALE          = 0.05F;          // [rad/s]

    // ------------------------------------------------------------------
    // Internal state
    // ------------------------------------------------------------------

    // Whether IMU sampling is enabled
    bool m_enabled;

    // Number of samples since IMU_START
    U32 m_sampleCount;

    // "Simulation time" since IMU_START [s]
    F32 m_timeSec;

    // Orbital state (very simple circular orbit in ECI)
    // r_ECI and v_ECI are stored in meters and m/s
    F32 m_rEci[3];
    F32 m_vEci[3];

    // Last simulated accelerometer readings [m/s^2]
    F32 m_ax;
    F32 m_ay;
    F32 m_az;

    // Last simulated gyroscope readings [rad/s]
    F32 m_gx;
    F32 m_gy;
    F32 m_gz;

    // Simple constant biases for IMU (could later be parameters)
    F32 m_accelBias[3];  // [m/s^2]
    F32 m_gyroBias[3];   // [rad/s]

    // ------------------------------------------------------------------
    // Command handlers
    // ------------------------------------------------------------------
    void IMU_START_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    void IMU_STOP_cmdHandler(
        FwOpcodeType opCode,
        U32 cmdSeq
    ) override;

    // ------------------------------------------------------------------
    // Scheduler tick
    // ------------------------------------------------------------------
    void schedIn_handler(
        FwIndexType portNum,
        U32 context
    ) override;

    // ------------------------------------------------------------------
    // Helper methods
    // ------------------------------------------------------------------

    //! Reset orbital and IMU state when IMU_START is called
    void resetState();

    //! Propagate a very simple circular orbit in the equatorial plane
    void propagateOrbit(const F32 dt);

    //! Compute "ideal" IMU signals from orbital motion + small maneuvers
    void computeIdealImu(
        F32 accel[3],  // [m/s^2] in body frame
        F32 gyro[3]    // [rad/s] in body frame
    ) const;

    //! Add simple deterministic pseudo-noise + biases
    void applyBiasAndNoise(
        const F32 accelIdeal[3],
        const F32 gyroIdeal[3]
    );
  };

}  // namespace Components

#endif  // COMPONENTS_IMUDRIVER_IMUDRIVER_HPP

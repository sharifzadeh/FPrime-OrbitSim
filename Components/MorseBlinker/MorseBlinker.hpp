#ifndef COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP
#define COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP

#include <Components/MorseBlinker/MorseBlinkerComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <string>

namespace Components {

  class MorseBlinker :
    public MorseBlinkerComponentBase
  {
    public:
      // Construction and destruction
      MorseBlinker(const char* const compName);
      ~MorseBlinker();

    private:
      // Command handler implementation
      void BLINK_STRING_cmdHandler(
          FwOpcodeType opCode,
          U32 cmdSeq,
          const Fw::CmdStringArg& message
      ) override;

      // New handler for IMU status port (0 = stopped, 1 = started)
      void imuStatusIn_handler(
          FwIndexType portNum,
          U8 status
      ) override;
  };

} // namespace Components

#endif // COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP

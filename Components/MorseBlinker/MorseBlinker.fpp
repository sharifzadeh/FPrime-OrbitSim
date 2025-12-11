module Components {

  @ Morse LED Blinker component
  active component MorseBlinker {

    # ----------------------------------------------------------------------
    # Commands
    # ----------------------------------------------------------------------

    @ Command to receive a string and blink it in Morse code
    async command BLINK_STRING(
      message: string size 80 @< The text to blink
    )

    # ----------------------------------------------------------------------
    # Events
    # ----------------------------------------------------------------------

    @ Event to report the text being blinked
    event Blinking(
      message: string size 80 @< The text being blinked
    ) severity activity high format "Morse Blinking: {}"

    # ----------------------------------------------------------------------
    # Standard AC Ports
    # ----------------------------------------------------------------------

    @ Port for requesting the current time
    time get port timeCaller

    @ Port for sending command registrations
    command reg port cmdRegOut

    @ Port for receiving commands
    command recv port cmdIn

    @ Port for sending command responses
    command resp port cmdResponseOut

    @ Port for sending textual representation of events
    text event port logTextOut

    @ Port for sending events to downlink
    event port logOut

    @ Port for sending telemetry channels
    telemetry port tlmOut

    # ----------------------------------------------------------------------
    # IMU status input
    # ----------------------------------------------------------------------

    @ IMU status from ImuDriver (0=stopped/F, 1=started/T)
    async input port imuStatusIn: ImuStatusPort

  }

}

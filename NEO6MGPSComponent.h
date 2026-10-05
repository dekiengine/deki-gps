#pragma once

#include <cstdint>
#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "nmea/NMEAGPS.h"

namespace DekiGps
{

/// Boot-scene component for a u-blox NEO-6M GPS.
///
/// Uses the generic NMEA-over-UART driver (NMEAGPS), so it works with any
/// NMEA receiver; the name is the module it was written for.
DEKI_CATEGORY("Sensors")
DEKI_DISPLAY_NAME("NEO-6M GPS")
DEKI_DESCRIPTION("Reads location from a u-blox NEO-6M GPS over UART.")
class NEO6MGPSComponent : public Deki::SetupComponent
{
public:
    DEKI_EXPORT
    DEKI_TOOLTIP(
        "GPIO the board transmits on, wired to the module's RX. Crossed over: this pin talks, the module listens.")
    DEKI_RANGE(0, 48)
    int32_t txPin = 17;

    DEKI_EXPORT
    DEKI_TOOLTIP("GPIO the board receives on, wired to the module's TX. This is the one that carries position data, so "
                 "a fix that never arrives usually means this pin.")
    DEKI_RANGE(0, 48)
    int32_t rxPin = 16;

    // NEO-6M defaults to 9600. A u-blox M8 can be configured up to 115200.
    DEKI_EXPORT
    DEKI_TOOLTIP("Serial speed, which must match how the module is configured. NEO-6M ships at 9600; a module "
                 "reconfigured to 38400 and left at 9600 here reads as garbage rather than silence.")
    DEKI_RANGE(2400, 921600)
    int32_t baudRate = 9600;

    DEKI_EXPORT
    DEKI_TOOLTIP("Which of the chip's serial controllers to use. Port 0 is usually the USB console, so a GPS normally "
                 "wants 1 or 2.")
    DEKI_RANGE(0, 2)
    int32_t uartPort = 1;

    NEO6MGPSComponent() = default;
    virtual ~NEO6MGPSComponent() = default;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "NEO-6M GPS"; }
};

}  // namespace DekiGps

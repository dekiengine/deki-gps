#pragma once

#include <cstdint>
#include "SetupComponent.h"
#include "reflection/DekiProperty.h"
#include "nmea/NMEAGPS.h"

/**
 * @brief Boot-prefab component for the u-blox NEO-6M GPS module.
 *
 * Uses the generic NMEA-over-UART driver (NMEAGPS). Works with any
 * NMEA-compliant GPS receiver; named after the specific hardware on the
 * Go-Hero board.
 */
class NEO6MGPSComponent : public SetupComponent
{
public:
    DEKI_COMPONENT(NEO6MGPSComponent, SetupComponent, "Sensors", "7c3a5e12-8b49-4f26-a735-d089e4b1c6f2", "")
    DEKI_DISPLAY_NAME("NEO-6M GPS")

    DEKI_EXPORT
    DEKI_RANGE(0, 48)
    int32_t tx_pin = 17;

    DEKI_EXPORT
    DEKI_RANGE(0, 48)
    int32_t rx_pin = 16;

    /** @brief NEO-6M default is 9600. u-blox M8 can go up to 115200 once configured. */
    DEKI_EXPORT
    DEKI_RANGE(2400, 921600)
    int32_t baud = 9600;

    DEKI_EXPORT
    DEKI_RANGE(0, 2)
    int32_t uart_port = 1;

    NEO6MGPSComponent() = default;
    virtual ~NEO6MGPSComponent() = default;

    void        Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "NEO-6M GPS"; }
};

#include "generated/NEO6MGPSComponent.gen.h"

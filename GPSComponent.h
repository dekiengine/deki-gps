#pragma once

#include <cstdint>
#include "SetupComponent.h"
#include "reflection/DekiProperty.h"
#include "IDekiGPS.h"

class GPSComponent : public SetupComponent
{
public:
    DEKI_COMPONENT(GPSComponent, SetupComponent, "System", "e7f2c5d8-9b34-4a87-a012-5c8d7e6f9b21", "")

    DEKI_EXPORT
    DEKI_RANGE(0, 48)
    int32_t tx_pin = 17;

    DEKI_EXPORT
    DEKI_RANGE(0, 48)
    int32_t rx_pin = 16;

    DEKI_EXPORT
    DEKI_RANGE(2400, 921600)
    int32_t baud = 9600;

    DEKI_EXPORT
    DEKI_RANGE(0, 2)
    int32_t uart_port = 1;

    GPSComponent() = default;
    virtual ~GPSComponent() = default;

    void        Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "GPS"; }

    static IDekiGPS* GetGPSModule();
};

#include "generated/GPSComponent.gen.h"

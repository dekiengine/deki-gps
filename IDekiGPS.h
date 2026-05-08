#pragma once

#include "providers/IDekiModule.h"
#include <cstdint>

struct DekiGPSLocation
{
    double latitude  = 0.0;
    double longitude = 0.0;
};

class IDekiGPS : public IDekiModule
{
public:
    const char* GetModuleCategory() const override { return "gps"; }

    virtual DekiGPSLocation Current() const = 0;
    virtual bool            HasLiveFix() const = 0;

    /// UTC seconds since 1970-01-01 00:00 UTC at the moment of the most recent
    /// time fix. Returns 0 when HasUTC() is false. Backends that have a system
    /// clock can return that; backends that get UTC out-of-band (NMEA $GPRMC)
    /// return the timestamp parsed from the sentence.
    virtual int64_t CurrentUTCEpochSeconds() const = 0;
    virtual bool    HasUTC() const = 0;
};

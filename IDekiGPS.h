#pragma once

#include <deki/providers/IPackage.h>
#include <cstdint>

namespace DekiGps
{

struct DekiGPSLocation
{
    double latitude = 0.0;
    double longitude = 0.0;
};

class IDekiGPS : public Deki::IPackage
{
public:
    const char* GetPackageCategory() const override { return "gps"; }

    virtual DekiGPSLocation Current() const = 0;
    virtual bool HasLiveFix() const = 0;

    /// UTC seconds since 1970-01-01 at the most recent time fix, or 0 when
    /// HasUTC() is false. A backend with a system clock may return that; an
    /// NMEA backend returns the time parsed from the $GPRMC sentence.
    virtual int64_t CurrentUTCEpochSeconds() const = 0;
    virtual bool HasUTC() const = 0;
};

}  // namespace DekiGps

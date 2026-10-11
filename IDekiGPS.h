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

/// What the receiver reports besides the position, for showing progress while
/// there is no fix yet (indoors there may never be one).
struct DekiGPSStatus
{
    // Valid NMEA sentences read so far. 0 means nothing is talking on the
    // wire: wrong pins, wrong speed, or the module is not powered.
    uint32_t sentences = 0;

    // Bytes read from the receiver, valid or not. Bytes but no sentences is a
    // wrong speed; no bytes at all, a wrong pin or a receiver without power.
    uint32_t bytes = 0;

    // Satellites the receiver can see, over every constellation it reports,
    // and how many of them it is using for the fix. -1 until reported.
    int satellitesInView = -1;
    int satellitesUsed = -1;

    // Height above mean sea level, once there is a fix.
    bool hasAltitude = false;
    double altitudeMeters = 0.0;

    // The serial speed in use; a backend set to find it reports it once it
    // has. 0 when not applicable or not found yet.
    uint32_t baudRate = 0;
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

    /// The receiver's state besides the position. A backend that is not a
    /// receiver (the desktop one) leaves it empty.
    virtual DekiGPSStatus Status() const { return {}; }
};

}  // namespace DekiGps

#pragma once

#include "IDekiGPS.h"

namespace DekiGps
{

/// Holds the active GPS driver.
///
/// A SetupComponent (NEO6MGPSComponent on a device, DesktopGPSComponent on
/// desktop and in the editor) registers its driver with SetCurrent() during
/// Setup(). Game and editor code read the location through
/// GetCurrent()->Current() and HasLiveFix().
class DekiGPS
{
public:
    static void SetCurrent(IDekiGPS* gps);
    static IDekiGPS* GetCurrent();

private:
    static IDekiGPS* s_Current;
};

}  // namespace DekiGps

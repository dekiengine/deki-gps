#pragma once

#include "IDekiGPS.h"

/**
 * @brief Active-driver registry for GPS.
 *
 * A SetupComponent (NEO6MGPSComponent on embedded, DesktopGPSComponent on
 * desktop / editor) registers its IDekiGPS driver via SetCurrent() during
 * Setup(). Game / editor code reads the current location via
 * GetCurrent()->Current() / HasLiveFix().
 */
class DekiGPS
{
public:
    static void      SetCurrent(IDekiGPS* gps);
    static IDekiGPS* GetCurrent();

private:
    static IDekiGPS* s_Current;
};

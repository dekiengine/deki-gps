#pragma once

#include "IDekiGPS.h"

/**
 * @brief Published-instance registry for the currently active GPS driver.
 *
 * Chip-specific SetupComponents (e.g., NEO6MComponent) call SetCurrent() in
 * their Setup() once the driver is configured and initialized. Game code reads
 * the current GPS via GetCurrent().
 */
class DekiGPSProvider
{
public:
    static void      SetCurrent(IDekiGPS* gps);
    static IDekiGPS* GetCurrent();

private:
    static IDekiGPS* s_Current;
};

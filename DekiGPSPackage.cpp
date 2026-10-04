/**
 * @file DekiGPSPackage.cpp
 * @brief Package entry point for deki-gps
 */
#include "DekiGPSPackage.h"
#include <deki/interop/Plugin.h>
#include <deki/LogSystem.h>
#include "DekiGPS.h"

extern void DekiGPS_RegisterComponents();
extern int DekiGPS_GetAutoComponentCount();
extern const Deki::ComponentMeta* DekiGPS_GetAutoComponentMeta(int index);

namespace DekiGps
{

#ifdef DEKI_EDITOR

static bool s_GPSRegistered = false;

// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiGps;

extern "C"
{
    DEKI_GPS_API int DekiGPS_EnsureRegistered(void)
    {
        if (s_GPSRegistered)
        {
            return ::DekiGPS_GetAutoComponentCount();
        }
        s_GPSRegistered = true;
        ::DekiGPS_RegisterComponents();
        return ::DekiGPS_GetAutoComponentCount();
    }

    DEKI_PLUGIN_API const char* DekiPlugin_GetName(void)
    {
        return "Deki GPS Package";
    }
    DEKI_PLUGIN_API const char* DekiPlugin_GetVersion(void)
    {
#ifdef DEKI_PACKAGE_VERSION
        return DEKI_PACKAGE_VERSION;
#else
        return "0.0.0-dev";
#endif
    }
    DEKI_PLUGIN_API int DekiPlugin_Init(void)
    {
        return 0;
    }
    DEKI_PLUGIN_API void DekiPlugin_Shutdown(void)
    {
        s_GPSRegistered = false;
        // Stop the driver first: the desktop one asks a web service for the
        // location on its own thread, which can take up to 30 s, and returning
        // into this DLL's code after it was unloaded crashed the editor.
        if (IDekiGPS* driver = DekiGPS::GetCurrent())
        {
            driver->Shutdown();
        }
        // Null the provider so a hot-reload doesn't leave a dangling pointer to a
        // driver instance whose .text is about to be unloaded with the DLL. The
        // driver itself (DesktopGPSComponent's s_Driver) is intentionally leaked,
        // matching the embedded NEO6MGPSComponent pattern.
        DekiGPS::SetCurrent(nullptr);
    }
    DEKI_PLUGIN_API int DekiPlugin_GetComponentCount(void)
    {
        return ::DekiGPS_GetAutoComponentCount();
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPlugin_GetComponentMeta(int index)
    {
        return ::DekiGPS_GetAutoComponentMeta(index);
    }
    DEKI_PLUGIN_API void DekiPlugin_RegisterComponents(void)
    {
        DekiGPS_EnsureRegistered();
    }

}  // extern "C"

#endif  // DEKI_EDITOR
}  // namespace DekiGps

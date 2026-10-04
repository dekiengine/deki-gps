// Package entry point for deki-gps.
#include "DekiGPSPackage.h"
#include <deki/interop/Plugin.h>
#include <deki/LogSystem.h>
#include "DekiGPS.h"

extern void DekiGPSRegisterComponents();
extern int DekiGPSGetAutoComponentCount();
extern const Deki::ComponentMeta* DekiGPSGetAutoComponentMeta(int index);

namespace DekiGps
{

#ifdef DEKI_EDITOR

static bool s_GPSRegistered = false;

// The exports below are C symbols at global scope; the package's own
// registration helpers and statics live in its namespace.
using namespace DekiGps;

extern "C"
{
    DEKI_GPS_API int DekiGPSEnsureRegistered(void)
    {
        if (s_GPSRegistered)
        {
            return ::DekiGPSGetAutoComponentCount();
        }
        s_GPSRegistered = true;
        ::DekiGPSRegisterComponents();
        return ::DekiGPSGetAutoComponentCount();
    }

    DEKI_PLUGIN_API const char* DekiPluginGetName(void)
    {
        return "Deki GPS Package";
    }
    DEKI_PLUGIN_API const char* DekiPluginGetVersion(void)
    {
#ifdef DEKI_PACKAGE_VERSION
        return DEKI_PACKAGE_VERSION;
#else
        return "0.0.0-dev";
#endif
    }
    DEKI_PLUGIN_API int DekiPluginInit(void)
    {
        return 0;
    }
    DEKI_PLUGIN_API void DekiPluginShutdown(void)
    {
        s_GPSRegistered = false;
        // Stop the driver before the DLL unloads: the desktop one asks a web
        // service for the location on its own thread, which can take up to
        // 30 s, and that thread must not return into unloaded code.
        if (IDekiGPS* driver = DekiGPS::GetCurrent())
        {
            driver->Shutdown();
        }
        // Clear the provider so a hot reload leaves no pointer to a driver
        // whose code unloads with the DLL. The driver object itself is leaked
        // on purpose, as NEO6MGPSComponent does.
        DekiGPS::SetCurrent(nullptr);
    }
    DEKI_PLUGIN_API int DekiPluginGetComponentCount(void)
    {
        return ::DekiGPSGetAutoComponentCount();
    }
    DEKI_PLUGIN_API const Deki::ComponentMeta* DekiPluginGetComponentMeta(int index)
    {
        return ::DekiGPSGetAutoComponentMeta(index);
    }
    DEKI_PLUGIN_API void DekiPluginRegisterComponents(void)
    {
        DekiGPSEnsureRegistered();
    }

}  // extern "C"

#endif  // DEKI_EDITOR
}  // namespace DekiGps

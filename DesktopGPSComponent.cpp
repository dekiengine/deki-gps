#include "DesktopGPSComponent.h"
#include "DekiGPS.h"
#include <deki/PackageConfig.h>
#include <deki/LogSystem.h>

namespace DekiGps
{

static DesktopGPS* s_DesktopDriver = nullptr;

void DesktopGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_DesktopDriver)
        s_DesktopDriver = new DesktopGPS();

    Deki::PackageConfig cfg;
    cfg.packageId = "gps";
    cfg.enabled  = true;

    s_DesktopDriver->Configure(cfg);

    const bool success = s_DesktopDriver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_DesktopDriver);
    }
    else
    {
        DEKI_LOG_ERROR("DesktopGPSComponent: Initialize() failed");
    }

    if (onComplete) onComplete(success);
}

// Play, not project open. This asks a third party where the machine is, so
// it waits for the user to press Play rather than firing every time a
// project is opened. The answer is cached for an hour either way.
DEKI_REGISTER_PLAY_AUTO_SETUP(DesktopGPSComponent);

}  // namespace DekiGps

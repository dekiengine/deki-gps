#include "DesktopGPSComponent.h"
#include "DekiGPS.h"
#include "PackageConfig.h"
#include "DekiLogSystem.h"

static DesktopGPS* s_DesktopDriver = nullptr;

void DesktopGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_DesktopDriver)
        s_DesktopDriver = new DesktopGPS();

    PackageConfig cfg;
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

DEKI_REGISTER_EDITOR_AUTO_SETUP(DesktopGPSComponent);

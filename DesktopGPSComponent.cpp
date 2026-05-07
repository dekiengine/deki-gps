#include "DesktopGPSComponent.h"
#include "DekiGPS.h"
#include "ModuleConfig.h"
#include "DekiLogSystem.h"

static DesktopGPS* s_Driver = nullptr;

void DesktopGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_Driver)
        s_Driver = new DesktopGPS();

    ModuleConfig cfg;
    cfg.moduleId = "gps";
    cfg.enabled  = true;

    s_Driver->Configure(cfg);

    const bool success = s_Driver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_Driver);
    }
    else
    {
        DEKI_LOG_ERROR("DesktopGPSComponent: Initialize() failed");
    }

    if (onComplete) onComplete(success);
}

DEKI_REGISTER_EDITOR_AUTO_SETUP(DesktopGPSComponent);

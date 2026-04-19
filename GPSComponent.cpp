#include "GPSComponent.h"
#include "DekiGPSProvider.h"
#include "modules/ModuleConfig.h"
#include "DekiLogSystem.h"
#include <string>

static IDekiGPS* s_GPSModule = nullptr;

void GPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_GPSModule)
    {
        s_GPSModule = DekiGPSProvider::Create();
        if (!s_GPSModule)
        {
            DEKI_LOG_ERROR("GPSComponent: No GPS backend registered");
            if (onComplete) onComplete(false);
            return;
        }
    }

    ModuleConfig config;
    config.moduleId = "gps";
    config.enabled  = true;
    config.pins["TX"] = tx_pin;
    config.pins["RX"] = rx_pin;
    config.settings["baud"]      = std::to_string(baud);
    config.settings["uart_port"] = std::to_string(uart_port);

    s_GPSModule->Configure(config);

    bool success = s_GPSModule->Initialize();
    if (success)
    {
        DekiGPSProvider::SetCurrent(s_GPSModule);
    }
    else
    {
        DEKI_LOG_ERROR("GPSComponent: Failed to initialize GPS backend");
    }

    if (onComplete) onComplete(success);
}

IDekiGPS* GPSComponent::GetGPSModule()
{
    return s_GPSModule;
}

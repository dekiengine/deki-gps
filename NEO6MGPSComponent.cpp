#include "NEO6MGPSComponent.h"
#include "DekiGPS.h"
#include <deki/PackageConfig.h>
#include <deki/LogSystem.h>
#include <string>

namespace DekiGps
{

static NMEAGPS* s_NEO6MDriver = nullptr;

void NEO6MGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_NEO6MDriver)
        s_NEO6MDriver = new NMEAGPS();

    Deki::PackageConfig cfg;
    cfg.packageId = "gps";
    cfg.enabled  = true;
    cfg.pins["TX"] = txPin;
    cfg.pins["RX"] = rxPin;
    cfg.settings["baudRate"]      = std::to_string(baudRate);
    cfg.settings["uartPort"] = std::to_string(uartPort);

    s_NEO6MDriver->Configure(cfg);

    const bool success = s_NEO6MDriver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_NEO6MDriver);
    }
    else
    {
        DEKI_LOG_ERROR("NEO6MGPSComponent: Failed to initialize NEO-6M on UART %d (TX=%d RX=%d @ %d baudRate)",
                       (int)uartPort, (int)txPin, (int)rxPin, (int)baudRate);
    }

    if (onComplete) onComplete(success);
}

}  // namespace DekiGps

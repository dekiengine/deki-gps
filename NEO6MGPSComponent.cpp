#include "NEO6MGPSComponent.h"
#include "DekiGPS.h"
#include "PackageConfig.h"
#include "DekiLogSystem.h"
#include <string>

static NMEAGPS* s_NEO6MDriver = nullptr;

void NEO6MGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_NEO6MDriver)
        s_NEO6MDriver = new NMEAGPS();

    PackageConfig cfg;
    cfg.packageId = "gps";
    cfg.enabled  = true;
    cfg.pins["TX"] = txPin;
    cfg.pins["RX"] = rxPin;
    cfg.settings["baud"]      = std::to_string(baud);
    cfg.settings["uartPort"] = std::to_string(uartPort);

    s_NEO6MDriver->Configure(cfg);

    const bool success = s_NEO6MDriver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_NEO6MDriver);
    }
    else
    {
        DEKI_LOG_ERROR("NEO6MGPSComponent: Failed to initialize NEO-6M on UART %d (TX=%d RX=%d @ %d baud)",
                       (int)uartPort, (int)txPin, (int)rxPin, (int)baud);
    }

    if (onComplete) onComplete(success);
}

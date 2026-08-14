#include "NEO6MGPSComponent.h"
#include "DekiGPS.h"
#include "ModuleConfig.h"
#include "DekiLogSystem.h"
#include <string>

static NMEAGPS* s_NEO6MDriver = nullptr;

void NEO6MGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_NEO6MDriver)
        s_NEO6MDriver = new NMEAGPS();

    ModuleConfig cfg;
    cfg.moduleId = "gps";
    cfg.enabled  = true;
    cfg.pins["TX"] = tx_pin;
    cfg.pins["RX"] = rx_pin;
    cfg.settings["baud"]      = std::to_string(baud);
    cfg.settings["uart_port"] = std::to_string(uart_port);

    s_NEO6MDriver->Configure(cfg);

    const bool success = s_NEO6MDriver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_NEO6MDriver);
    }
    else
    {
        DEKI_LOG_ERROR("NEO6MGPSComponent: Failed to initialize NEO-6M on UART %d (TX=%d RX=%d @ %d baud)",
                       (int)uart_port, (int)tx_pin, (int)rx_pin, (int)baud);
    }

    if (onComplete) onComplete(success);
}

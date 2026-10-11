#include "NEO6MGPSComponent.h"
#include "DekiGPS.h"
#include <deki/PackageConfig.h>
#include <deki/Engine.h>
#include <deki/LogSystem.h>
#include <cstddef>
#include <string>

namespace DekiGps
{

static NMEAGPS* s_NEO6MDriver = nullptr;

// The engine's per-frame update that reads the module. Without it nothing
// called the driver's Update, so its UART was never read and a fix never
// came.
constexpr size_t kNoUpdate = static_cast<size_t>(-1);
static size_t s_UpdateId = kNoUpdate;

void NEO6MGPSComponent::Setup(SetupCallback onComplete)
{
    if (!s_NEO6MDriver)
    {
        s_NEO6MDriver = new NMEAGPS();
    }

    Deki::PackageConfig cfg;
    cfg.packageId = "gps";
    cfg.enabled = true;
    cfg.pins["TX"] = txPin;
    cfg.pins["RX"] = rxPin;
    cfg.settings["baudRate"] = std::to_string(baudRate);
    cfg.settings["uartPort"] = std::to_string(uartPort);

    s_NEO6MDriver->Configure(cfg);

    const bool success = s_NEO6MDriver->Initialize();
    if (success)
    {
        DekiGPS::SetCurrent(s_NEO6MDriver);
        if (s_UpdateId == kNoUpdate)
        {
            s_UpdateId = Deki::Engine::GetInstance().RegisterUpdate(
                [](uint32_t deltaMs)
                {
                    if (s_NEO6MDriver)
                    {
                        s_NEO6MDriver->Update(static_cast<float>(deltaMs) * 0.001f);
                    }
                });
        }
    }
    else
    {
        DEKI_LOG_ERROR("NEO6MGPSComponent: Failed to initialize NEO-6M on UART %d (TX=%d RX=%d @ %d baudRate)",
                       (int)uartPort, (int)txPin, (int)rxPin, (int)baudRate);
    }

    if (onComplete)
    {
        onComplete(success);
    }
}

void NEO6MGPSComponent::StopUpdates()
{
    if (s_UpdateId != kNoUpdate)
    {
        Deki::Engine::GetInstance().UnregisterUpdate(s_UpdateId);
        s_UpdateId = kNoUpdate;
    }
}

}  // namespace DekiGps

#pragma once

#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "DesktopGPS.h"

namespace DekiGps
{

/// Desktop and editor counterpart of NEO6MGPSComponent: registers a
/// DesktopGPS with DekiGPS. It has no properties because the desktop driver
/// has no pins or UART to set.
///
/// Run automatically when Play starts.
DEKI_CATEGORY("System")
DEKI_DISPLAY_NAME("Desktop GPS")
DEKI_DESCRIPTION("Stands in for the GPS receiver on desktop, so location works without hardware.")
class DesktopGPSComponent : public Deki::SetupComponent
{
public:
    DesktopGPSComponent() = default;
    virtual ~DesktopGPSComponent() = default;

    void Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "Desktop GPS"; }
};

}  // namespace DekiGps

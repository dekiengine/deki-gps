#pragma once

#include <deki/SetupComponent.h>
#include <deki/reflection/Property.h>
#include "DesktopGPS.h"

/**
 * @brief Editor / desktop SetupComponent that registers a DesktopGPS with
 * DekiGPS. Mirrors NEO6MGPSComponent (embedded) — same SetupComponent
 * shape, no editable hardware properties because the desktop driver has no
 * pins/UART config.
 *
 * Auto-fired by SetupComponent::RunEditorAutoSetups() after package load.
 */
DEKI_CATEGORY("System")
DEKI_DISPLAY_NAME("Desktop GPS")
DEKI_DESCRIPTION("Stands in for the GPS receiver on desktop, so location works without hardware.")
class DesktopGPSComponent : public Deki::SetupComponent
{
public:

    DesktopGPSComponent() = default;
    virtual ~DesktopGPSComponent() = default;

    void        Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "Desktop GPS"; }
};


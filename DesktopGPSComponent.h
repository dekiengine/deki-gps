#pragma once

#include "SetupComponent.h"
#include "reflection/DekiProperty.h"
#include "DesktopGPS.h"

/**
 * @brief Editor / desktop SetupComponent that registers a DesktopGPS with
 * DekiGPS. Mirrors NEO6MGPSComponent (embedded) — same SetupComponent
 * shape, no editable hardware properties because the desktop driver has no
 * pins/UART config.
 *
 * Auto-fired by SetupComponent::RunEditorAutoSetups() after module load.
 */
class DesktopGPSComponent : public SetupComponent
{
public:
    DEKI_COMPONENT(DesktopGPSComponent, SetupComponent, "System", "5e2a9d61-4f8c-4b73-a902-1d6b7e4c8a3f", "")
    DEKI_DISPLAY_NAME("Desktop GPS")

    DesktopGPSComponent() = default;
    virtual ~DesktopGPSComponent() = default;

    void        Setup(SetupCallback onComplete) override;
    const char* GetSetupName() const override { return "Desktop GPS"; }
};

#include "generated/DesktopGPSComponent.gen.h"

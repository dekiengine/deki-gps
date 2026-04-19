#pragma once

#include "providers/IDekiModule.h"

struct DekiGPSLocation
{
    double latitude  = 0.0;
    double longitude = 0.0;
};

class IDekiGPS : public IDekiModule
{
public:
    const char* GetModuleCategory() const override { return "gps"; }

    virtual DekiGPSLocation Current() const = 0;
    virtual bool            HasLiveFix() const = 0;
};

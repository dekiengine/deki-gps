#include "DekiGPSProvider.h"
#include "DekiLogSystem.h"

DekiGPSProvider::Factory DekiGPSProvider::s_Factory = nullptr;
IDekiGPS*                DekiGPSProvider::s_Current = nullptr;

void DekiGPSProvider::SetFactory(Factory factory)
{
    s_Factory = factory;
    DEKI_LOG_INTERNAL("DekiGPSProvider: Factory registered");
}

IDekiGPS* DekiGPSProvider::Create()
{
    if (!s_Factory)
    {
        DEKI_LOG_ERROR("DekiGPSProvider: No factory registered - module must call SetFactory()");
        return nullptr;
    }
    return s_Factory();
}

bool DekiGPSProvider::HasFactory()
{
    return s_Factory != nullptr;
}

void DekiGPSProvider::SetCurrent(IDekiGPS* gps)
{
    s_Current = gps;
}

IDekiGPS* DekiGPSProvider::GetCurrent()
{
    return s_Current;
}

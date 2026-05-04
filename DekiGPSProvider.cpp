#include "DekiGPSProvider.h"

IDekiGPS* DekiGPSProvider::s_Current = nullptr;

void DekiGPSProvider::SetCurrent(IDekiGPS* gps)
{
    s_Current = gps;
}

IDekiGPS* DekiGPSProvider::GetCurrent()
{
    return s_Current;
}

#include "DekiGPS.h"

IDekiGPS* DekiGPS::s_Current = nullptr;

void DekiGPS::SetCurrent(IDekiGPS* gps)
{
    s_Current = gps;
}

IDekiGPS* DekiGPS::GetCurrent()
{
    return s_Current;
}

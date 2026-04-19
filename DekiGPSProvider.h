#pragma once

#include "IDekiGPS.h"

class DekiGPSProvider
{
public:
    using Factory = IDekiGPS* (*)();

    static void      SetFactory(Factory factory);
    static IDekiGPS* Create();
    static bool      HasFactory();

    static void      SetCurrent(IDekiGPS* gps);
    static IDekiGPS* GetCurrent();

private:
    static Factory   s_Factory;
    static IDekiGPS* s_Current;
};

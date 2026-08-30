#pragma once

#ifdef _WIN32
    #ifdef DEKI_GPS_EXPORTS
        #define DEKI_GPS_API __declspec(dllexport)
    #else
        #define DEKI_GPS_API __declspec(dllimport)
    #endif
#else
    #define DEKI_GPS_API __attribute__((visibility("default")))
#endif

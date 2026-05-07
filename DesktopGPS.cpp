#include "DesktopGPS.h"
#include "providers/DekiHttp.h"
#include "DekiLogSystem.h"

#include <cstdlib>
#include <cstring>

void DesktopGPS::Configure(const ModuleConfig&)
{
}

bool DesktopGPS::Initialize()
{
    m_Cancel.store(false);
    m_HasFix.store(false);
    m_LastError.clear();

    m_Thread = std::thread(&DesktopGPS::FetchLocation, this);

    m_State = ModuleState::Initialized;
    return true;
}

void DesktopGPS::Shutdown()
{
    m_Cancel.store(true);
    if (m_Thread.joinable())
        m_Thread.join();
    m_State = ModuleState::Uninitialized;
}

DekiGPSLocation DesktopGPS::Current() const
{
    DekiGPSLocation loc;
    loc.latitude  = m_Lat.load();
    loc.longitude = m_Lon.load();
    return loc;
}

bool DesktopGPS::HasLiveFix() const
{
    return m_HasFix.load();
}

namespace
{
    bool ParseDoubleAfter(const std::string& body, const char* key, double& out)
    {
        size_t pos = body.find(key);
        if (pos == std::string::npos) return false;
        pos += std::strlen(key);
        while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':' || body[pos] == '\t')) ++pos;
        if (pos >= body.size()) return false;

        const char* start = body.c_str() + pos;
        char* end = nullptr;
        double v = std::strtod(start, &end);
        if (end == start) return false;
        out = v;
        return true;
    }

    bool ResponseIsSuccess(const std::string& body)
    {
        size_t pos = body.find("\"status\"");
        if (pos == std::string::npos) return false;
        pos = body.find("\"success\"", pos);
        return pos != std::string::npos;
    }
}

void DesktopGPS::FetchLocation()
{
    DEKI_LOG_INFO("[deki-gps] DesktopGPS: querying ip-api.com for approximate location");

    std::string body = DekiHttp::FetchUrl("http://ip-api.com/json/");

    if (m_Cancel.load() || body.empty())
    {
        if (body.empty())
        {
            m_LastError = "DekiHttp returned empty body (no client registered or fetch failed)";
            DEKI_LOG_WARNING("[deki-gps] DesktopGPS: %s", m_LastError.c_str());
        }
        return;
    }

    if (!ResponseIsSuccess(body))
    {
        m_LastError = "ip-api response status was not success";
        DEKI_LOG_WARNING("[deki-gps] DesktopGPS: %s", m_LastError.c_str());
        return;
    }

    double lat = 0.0;
    double lon = 0.0;
    if (!ParseDoubleAfter(body, "\"lat\"", lat) || !ParseDoubleAfter(body, "\"lon\"", lon))
    {
        m_LastError = "ip-api response missing lat/lon";
        DEKI_LOG_WARNING("[deki-gps] DesktopGPS: %s", m_LastError.c_str());
        return;
    }

    m_Lat.store(lat);
    m_Lon.store(lon);
    m_HasFix.store(true);
    DEKI_LOG_INFO("[deki-gps] DesktopGPS: live fix acquired at %.4f, %.4f", lat, lon);
}

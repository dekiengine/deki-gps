#include "DesktopGPS.h"
#include "DekiHttp.h"
#include <deki/LogSystem.h>

#include <cstdlib>
#include <cstring>
#include <ctime>

namespace DekiGps
{

void DesktopGPS::Configure(const Deki::PackageConfig&)
{
}

bool DesktopGPS::Initialize()
{
    m_Cancel.store(false);
    m_HasFix.store(false);
    m_LastError.clear();

    m_Thread = std::thread(&DesktopGPS::FetchLocation, this);

    m_State = Deki::PackageState::Initialized;
    return true;
}

void DesktopGPS::Shutdown()
{
    m_Cancel.store(true);
    if (m_Thread.joinable())
        m_Thread.join();
    m_State = Deki::PackageState::Uninitialized;
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

int64_t DesktopGPS::CurrentUTCEpochSeconds() const
{
    return (int64_t)std::time(nullptr);
}

bool DesktopGPS::HasUTC() const
{
    // The desktop's system clock is the UTC source — std::time() is always UTC
    // regardless of the OS timezone setting. Available for the lifetime of the process.
    return true;
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

    // Plain HTTP, deliberately and unavoidably: ip-api.com serves HTTPS only
    // on its paid tier. This sends the machine's IP address to a third party
    // in the clear, and it runs from Initialize(), so simply having this
    // package active in a desktop build is enough to do it. Called out in the
    // package README and in the editor's SECURITY.md rather than left to be
    // discovered. Moving to a provider with free HTTPS would end the
    // exception and is the right fix when one is chosen.
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

}  // namespace DekiGps

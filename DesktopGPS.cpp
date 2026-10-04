#include "DesktopGPS.h"
#include "DekiHttp.h"
#include <deki/LogSystem.h>
#include <deki/providers/FileSystem.h>

#include <cstdio>
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
    {
        m_Thread.join();
    }
    m_State = Deki::PackageState::Uninitialized;
}

DekiGPSLocation DesktopGPS::Current() const
{
    DekiGPSLocation loc;
    loc.latitude = m_Lat.load();
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
    // The system clock is the UTC source: std::time() is UTC whatever the OS
    // time zone.
    return true;
}

namespace
{
bool ParseDoubleAfter(const std::string& body, const char* key, double& out)
{
    size_t pos = body.find(key);
    if (pos == std::string::npos)
    {
        return false;
    }
    pos += std::strlen(key);
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':' || body[pos] == '\t'))
    {
        ++pos;
    }
    if (pos >= body.size())
    {
        return false;
    }

    const char* start = body.c_str() + pos;
    char* end = nullptr;
    double v = std::strtod(start, &end);
    if (end == start)
    {
        return false;
    }
    out = v;
    return true;
}

// ipwho.is answers errors with HTTP 200 and "success": false, so this field
// is the only reliable check.
bool ResponseIsSuccess(const std::string& body)
{
    size_t pos = body.find("\"success\"");
    if (pos == std::string::npos)
    {
        return false;
    }
    pos += std::strlen("\"success\"");
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == ':' || body[pos] == '\t'))
    {
        ++pos;
    }
    if (pos + 4 > body.size())
    {
        return false;
    }
    return std::memcmp(body.data() + pos, "true", 4) == 0;
}

// Where the last fix is kept between runs. S:/ is the writable storage
// partition on every platform (./storage/ beside the executable on desktop),
// so the cache belongs to the game, not the machine.
const char* const kCachePath = "S:/deki-gps-location.txt";

// One hour. A city-level answer from an IP address does not change within
// that, and the cache outlives the process so that each test run of a game
// is not another request to a third party.
constexpr int64_t kCacheSeconds = 60 * 60;

// "<unix seconds> <lat> <lon>" on one line. Nothing else reads it, and a
// parse failure costs no more than a cache miss.
bool ReadCache(int64_t now, double& lat, double& lon)
{
    Deki::IFileSystem* fs = Deki::FileSystem::GetFileSystemForPath(kCachePath);
    if (!fs || !fs->FileExists(kCachePath))
    {
        return false;
    }

    Deki::IFileSystem::FileHandle f = fs->OpenFile(kCachePath, Deki::IFileSystem::OpenMode::ReadText);
    if (!f)
    {
        return false;
    }

    char buf[128] = {};
    const size_t read = fs->ReadFile(f, buf, sizeof(buf) - 1);
    fs->CloseFile(f);
    if (read == 0)
    {
        return false;
    }
    buf[read] = '\0';

    char* end = nullptr;
    const long long stamp = std::strtoll(buf, &end, 10);
    if (end == buf)
    {
        return false;
    }

    // A stamp in the future means the clock moved backwards since it was
    // written; treat it as a miss, or it would be trusted forever.
    if (stamp <= 0 || now < stamp || now - stamp >= kCacheSeconds)
    {
        return false;
    }

    char* p = end;
    const double cachedLat = std::strtod(p, &end);
    if (end == p)
    {
        return false;
    }
    p = end;
    const double cachedLon = std::strtod(p, &end);
    if (end == p)
    {
        return false;
    }

    lat = cachedLat;
    lon = cachedLon;
    return true;
}

void WriteCache(int64_t now, double lat, double lon)
{
    Deki::IFileSystem* fs = Deki::FileSystem::GetFileSystemForPath(kCachePath);
    if (!fs)
    {
        return;
    }

    Deki::IFileSystem::FileHandle f = fs->OpenFile(kCachePath, Deki::IFileSystem::OpenMode::WriteText);
    if (!f)
    {
        return;
    }

    char buf[128];
    const int n = std::snprintf(buf, sizeof(buf), "%lld %.6f %.6f\n", static_cast<long long>(now), lat, lon);
    if (n > 0)
    {
        fs->WriteFile(f, buf, static_cast<size_t>(n));
    }
    fs->CloseFile(f);
}
}  // namespace

void DesktopGPS::FetchLocation()
{
    const int64_t now = static_cast<int64_t>(std::time(nullptr));

    // Every request tells the service where the machine is, so send as few as
    // possible: at city-level accuracy an hour-old answer is as good as a new
    // one.
    double lat = 0.0;
    double lon = 0.0;
    if (ReadCache(now, lat, lon))
    {
        m_Lat.store(lat);
        m_Lon.store(lon);
        m_HasFix.store(true);
        DEKI_LOG_INFO("[deki-gps] DesktopGPS: using the cached fix at %.4f, %.4f (under an hour old)", lat, lon);
        return;
    }

    DEKI_LOG_INFO("[deki-gps] DesktopGPS: querying ipwho.is for approximate location");

    // ipwho.is is used because it serves HTTPS and allows commercial use on
    // its keyless free tier, and games built with this engine are sold. Keep
    // both when choosing another service.
    //
    // The request tells a third party the machine's address, and it runs from
    // Initialize(), so having this package active in a desktop build is
    // enough to send it. The package README says so.
    std::string body = DekiHttp::FetchUrl("https://ipwho.is/");

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
        m_LastError = "ipwho.is response was not a success";
        DEKI_LOG_WARNING("[deki-gps] DesktopGPS: %s", m_LastError.c_str());
        return;
    }

    if (!ParseDoubleAfter(body, "\"latitude\"", lat) || !ParseDoubleAfter(body, "\"longitude\"", lon))
    {
        m_LastError = "ipwho.is response missing latitude/longitude";
        DEKI_LOG_WARNING("[deki-gps] DesktopGPS: %s", m_LastError.c_str());
        return;
    }

    m_Lat.store(lat);
    m_Lon.store(lon);
    m_HasFix.store(true);
    WriteCache(now, lat, lon);
    DEKI_LOG_INFO("[deki-gps] DesktopGPS: live fix acquired at %.4f, %.4f", lat, lon);
}

}  // namespace DekiGps

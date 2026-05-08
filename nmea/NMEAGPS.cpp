#include "NMEAGPS.h"
#include "providers/DekiUART.h"
#include "DekiLogSystem.h"

#include <cstdlib>
#include <cstring>
#include <cmath>

namespace
{
    constexpr float kFixStaleAfterSeconds = 5.0f;

    // Hinnant's days_from_civil — converts a civil (Y, M, D) UTC date to days
    // since 1970-01-01. Pure arithmetic, no platform clock dependencies.
    int64_t DaysFromCivilUTC(int y, unsigned m, unsigned d)
    {
        y -= (m <= 2);
        const int era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = (unsigned)(y - era * 400);
        const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return (int64_t)era * 146097 + (int64_t)doe - 719468;
    }
}

void NMEAGPS::Configure(const ModuleConfig& config)
{
    m_PinTX     = config.GetPin("TX", -1);
    m_PinRX     = config.GetPin("RX", -1);
    m_Baud      = (uint32_t)config.GetInt("baud", 9600);
    m_UartPort  = config.GetInt("uart_port", 1);
}

bool NMEAGPS::Initialize()
{
    if (!m_UART)
    {
        m_UART = DekiUART::Create();
        if (!m_UART)
        {
            m_LastError = "NMEAGPS: no UART backend registered";
            m_State = ModuleState::Error;
            return false;
        }
    }

    ModuleConfig uartCfg;
    uartCfg.moduleId = "uart";
    uartCfg.enabled  = true;
    uartCfg.pins["TX"] = m_PinTX;
    uartCfg.pins["RX"] = m_PinRX;
    uartCfg.settings["baud"]      = std::to_string(m_Baud);
    uartCfg.settings["uart_port"] = std::to_string(m_UartPort);

    m_UART->Configure(uartCfg);

    if (!m_UART->Initialize())
    {
        m_LastError = "NMEAGPS: UART initialize failed";
        m_State = ModuleState::Error;
        return false;
    }

    m_State = ModuleState::Running;
    return true;
}

void NMEAGPS::Shutdown()
{
    if (m_UART)
    {
        m_UART->Shutdown();
        delete m_UART;
        m_UART = nullptr;
    }
    m_State = ModuleState::Uninitialized;
}

void NMEAGPS::Update(float deltaTime)
{
    if (m_State != ModuleState::Running) return;

    float t = m_SecondsSinceFix.load(std::memory_order_relaxed);
    if (t < 1e8f) m_SecondsSinceFix.store(t + deltaTime, std::memory_order_relaxed);

    PumpUart();
}

void NMEAGPS::PumpUart()
{
    if (!m_UART) return;

    uint8_t buf[128];
    int n = m_UART->Read(buf, sizeof(buf), 0);
    for (int i = 0; i < n; ++i)
    {
        char c = (char)buf[i];
        if (c == '\n' || m_LinePos >= sizeof(m_LineBuffer) - 1)
        {
            m_LineBuffer[m_LinePos] = 0;
            HandleLine(m_LineBuffer);
            m_LinePos = 0;
        }
        else if (c != '\r')
        {
            m_LineBuffer[m_LinePos++] = c;
        }
    }
}

bool NMEAGPS::ChecksumValid(const char* line)
{
    if (!line || line[0] != '$') return false;
    const char* star = std::strchr(line, '*');
    if (!star) return false;
    uint8_t cs = 0;
    for (const char* p = line + 1; p < star; ++p) cs ^= (uint8_t)*p;
    unsigned want = 0;
    if (std::sscanf(star + 1, "%2x", &want) != 1) return false;
    return (uint8_t)want == cs;
}

double NMEAGPS::NMEACoordToDeg(const char* coord, char hemi)
{
    if (!coord || !*coord) return 0.0;
    double raw = std::atof(coord);
    double deg = std::floor(raw / 100.0);
    double min = raw - deg * 100.0;
    double val = deg + min / 60.0;
    if (hemi == 'S' || hemi == 'W') val = -val;
    return val;
}

bool NMEAGPS::ParseRMC(const char* line, double& lat, double& lon,
                       int64_t& utc_epoch, bool& utc_valid) const
{
    // $GPRMC,hhmmss.sss,status,lat,N/S,lon,E/W,speed,course,ddmmyy,...
    char buf[96];
    std::strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char* fields[13] = {};
    int n = 0;
    for (char* tok = std::strtok(buf, ",*"); tok && n < 13; tok = std::strtok(nullptr, ",*"))
    {
        fields[n++] = tok;
    }
    if (n < 7) return false;
    if (!fields[2] || fields[2][0] != 'A') return false;     // status A = valid

    lat = NMEACoordToDeg(fields[3], fields[4] ? fields[4][0] : 'N');
    lon = NMEACoordToDeg(fields[5], fields[6] ? fields[6][0] : 'E');

    utc_valid = false;
    utc_epoch = 0;
    if (n >= 10 && fields[1] && fields[9] && std::strlen(fields[1]) >= 6 && std::strlen(fields[9]) == 6)
    {
        auto two = [](const char* p) { return (p[0] - '0') * 10 + (p[1] - '0'); };
        const char* t = fields[1];
        const char* d = fields[9];
        int hh = two(t);
        int mm = two(t + 2);
        int ss = two(t + 4);
        int dd = two(d);
        int mo = two(d + 2);
        int yy = two(d + 4);
        if (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59 && ss >= 0 && ss <= 60 &&
            dd >= 1 && dd <= 31 && mo >= 1 && mo <= 12)
        {
            int year = 2000 + yy;
            utc_epoch = DaysFromCivilUTC(year, (unsigned)mo, (unsigned)dd) * 86400
                      + hh * 3600 + mm * 60 + ss;
            utc_valid = true;
        }
    }
    return true;
}

void NMEAGPS::HandleLine(const char* line)
{
    if (!ChecksumValid(line)) return;

    if (std::strncmp(line, "$GPRMC", 6) == 0 || std::strncmp(line, "$GNRMC", 6) == 0)
    {
        double lat, lon;
        int64_t utc_epoch;
        bool utc_valid;
        if (ParseRMC(line, lat, lon, utc_epoch, utc_valid))
        {
            m_LastLat.store(lat, std::memory_order_relaxed);
            m_LastLon.store(lon, std::memory_order_relaxed);
            m_SecondsSinceFix.store(0.0f, std::memory_order_relaxed);
            if (utc_valid)
            {
                m_UTCEpoch.store(utc_epoch, std::memory_order_relaxed);
                m_HasUTC.store(true, std::memory_order_relaxed);
            }
        }
    }
}

DekiGPSLocation NMEAGPS::Current() const
{
    DekiGPSLocation loc;
    loc.latitude  = m_LastLat.load(std::memory_order_relaxed);
    loc.longitude = m_LastLon.load(std::memory_order_relaxed);
    return loc;
}

bool NMEAGPS::HasLiveFix() const
{
    return m_SecondsSinceFix.load(std::memory_order_relaxed) < kFixStaleAfterSeconds;
}

int64_t NMEAGPS::CurrentUTCEpochSeconds() const
{
    return m_UTCEpoch.load(std::memory_order_relaxed);
}

bool NMEAGPS::HasUTC() const
{
    return m_HasUTC.load(std::memory_order_relaxed);
}

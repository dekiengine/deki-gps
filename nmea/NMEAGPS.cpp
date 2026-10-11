#include "NMEAGPS.h"
#include "DekiUART.h"  // from deki-uart
#include <deki/LogSystem.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace DekiGps
{

namespace
{
constexpr float kFixStaleAfterSeconds = 5.0f;

// The speeds tried when the configured one is 0, most likely first: 9600 is
// what most modules ship at (NEO-6M, L76K), 38400 the u-blox M10's.
constexpr uint32_t kBaudCandidates[] = { 9600, 38400, 115200, 57600, 4800 };
constexpr int kBaudCandidateCount = sizeof(kBaudCandidates) / sizeof(kBaudCandidates[0]);

// A module sends at least one sentence a second, so two seconds of silence
// or garbage at a speed means it is not that one.
constexpr float kSecondsPerBaud = 2.0f;

// The talkers counted for satellites in view, by the two letters after '$'.
constexpr const char* kTalkerIds[] = { "GP", "GL", "GA", "GB", "BD", "GQ", "GI", "QZ" };

// Howard Hinnant's days_from_civil: days from 1970-01-01 to a UTC date.
// Plain arithmetic, so it needs no platform clock.
int64_t DaysFromCivilUTC(int y, unsigned m, unsigned d)
{
    y -= (m <= 2);
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

// Splits a sentence into its fields in place, keeping empty ones: ",," is an
// empty field, which strtok would skip, shifting every field after it (an
// RMC with no course, as when standing still, lost its date). Field 0 is the
// sentence id ("$GNRMC"); the checksum is cut off.
int SplitFields(char* sentence, char** fields, int maxFields)
{
    if (char* star = std::strchr(sentence, '*'))
    {
        *star = 0;
    }
    int count = 0;
    char* p = sentence;
    while (count < maxFields)
    {
        fields[count++] = p;
        char* comma = std::strchr(p, ',');
        if (!comma)
        {
            break;
        }
        *comma = 0;
        p = comma + 1;
    }
    return count;
}

bool HasField(char** fields, int count, int index)
{
    return index < count && fields[index] && fields[index][0] != 0;
}
}  // namespace

void NMEAGPS::Configure(const Deki::PackageConfig& config)
{
    m_PinTX = config.GetPin("TX", -1);
    m_PinRX = config.GetPin("RX", -1);
    m_Baud = (uint32_t)config.GetInt("baudRate", 9600);
    m_UartPort = config.GetInt("uartPort", 1);
}

bool NMEAGPS::OpenUart(uint32_t baud)
{
    Deki::PackageConfig uartCfg;
    uartCfg.packageId = "uart";
    uartCfg.enabled = true;
    uartCfg.pins["TX"] = m_PinTX;
    uartCfg.pins["RX"] = m_PinRX;
    uartCfg.settings["baudRate"] = std::to_string(baud);
    uartCfg.settings["uartPort"] = std::to_string(m_UartPort);

    m_UART->Configure(uartCfg);
    if (!m_UART->Initialize())
    {
        return false;
    }
    m_LinePos = 0;
    return true;
}

bool NMEAGPS::Initialize()
{
    if (!m_UART)
    {
        m_UART = DekiUart::DekiUART::Create();
        if (!m_UART)
        {
            m_LastError = "NMEAGPS: no UART backend registered";
            m_State = Deki::PackageState::Error;
            return false;
        }
    }

    m_SearchingBaud = (m_Baud == 0);
    m_BaudCandidate = 0;
    m_SecondsAtBaud = 0.0f;
    const uint32_t baud = m_SearchingBaud ? kBaudCandidates[0] : m_Baud;
    if (!OpenUart(baud))
    {
        m_LastError = "NMEAGPS: UART initialize failed";
        m_State = Deki::PackageState::Error;
        return false;
    }
    m_BaudInUse.store(m_SearchingBaud ? 0 : baud, std::memory_order_relaxed);

    m_State = Deki::PackageState::Running;
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
    m_State = Deki::PackageState::Uninitialized;
}

void NMEAGPS::Update(float deltaTime)
{
    if (m_State != Deki::PackageState::Running)
    {
        return;
    }

    float t = m_SecondsSinceFix.load(std::memory_order_relaxed);
    if (t < 1e8f)
    {
        m_SecondsSinceFix.store(t + deltaTime, std::memory_order_relaxed);
    }

    PumpUart();

    // Nothing valid at this speed for a while: try the next one.
    if (m_SearchingBaud)
    {
        m_SecondsAtBaud += deltaTime;
        if (m_SecondsAtBaud >= kSecondsPerBaud)
        {
            m_SecondsAtBaud = 0.0f;
            m_BaudCandidate = (m_BaudCandidate + 1) % kBaudCandidateCount;
            m_UART->Shutdown();
            if (!OpenUart(kBaudCandidates[m_BaudCandidate]))
            {
                m_LastError = "NMEAGPS: UART initialize failed while looking for the speed";
                m_State = Deki::PackageState::Error;
            }
        }
    }
}

void NMEAGPS::PumpUart()
{
    if (!m_UART)
    {
        return;
    }

    uint8_t buf[128];
    int n = m_UART->Read(buf, sizeof(buf), 0);
    if (n > 0)
    {
        m_Bytes.fetch_add(static_cast<uint32_t>(n), std::memory_order_relaxed);
    }
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
    if (!line || line[0] != '$')
    {
        return false;
    }
    const char* star = std::strchr(line, '*');
    if (!star)
    {
        return false;
    }
    uint8_t cs = 0;
    for (const char* p = line + 1; p < star; ++p)
    {
        cs ^= (uint8_t)*p;
    }
    unsigned want = 0;
    if (std::sscanf(star + 1, "%2x", &want) != 1)
    {
        return false;
    }
    return (uint8_t)want == cs;
}

double NMEAGPS::NMEACoordToDeg(const char* coord, char hemi)
{
    if (!coord || !*coord)
    {
        return 0.0;
    }
    double raw = std::atof(coord);
    double deg = std::floor(raw / 100.0);
    double min = raw - deg * 100.0;
    double val = deg + min / 60.0;
    if (hemi == 'S' || hemi == 'W')
    {
        val = -val;
    }
    return val;
}

void NMEAGPS::HandleRMC(char** f, int n)
{
    // $--RMC,hhmmss.ss,status,lat,N/S,lon,E/W,speed,course,ddmmyy,...
    if (!HasField(f, n, 2) || f[2][0] != 'A' || !HasField(f, n, 3) || !HasField(f, n, 5))
    {
        return;  // status A = a valid fix
    }

    m_LastLat.store(NMEACoordToDeg(f[3], HasField(f, n, 4) ? f[4][0] : 'N'), std::memory_order_relaxed);
    m_LastLon.store(NMEACoordToDeg(f[5], HasField(f, n, 6) ? f[6][0] : 'E'), std::memory_order_relaxed);
    m_SecondsSinceFix.store(0.0f, std::memory_order_relaxed);

    if (HasField(f, n, 1) && HasField(f, n, 9) && std::strlen(f[1]) >= 6 && std::strlen(f[9]) == 6)
    {
        auto two = [](const char* p) { return (p[0] - '0') * 10 + (p[1] - '0'); };
        const char* t = f[1];
        const char* d = f[9];
        const int hh = two(t);
        const int mm = two(t + 2);
        const int ss = two(t + 4);
        const int dd = two(d);
        const int mo = two(d + 2);
        const int yy = two(d + 4);
        if (hh >= 0 && hh <= 23 && mm >= 0 && mm <= 59 && ss >= 0 && ss <= 60 && dd >= 1 && dd <= 31 && mo >= 1 &&
            mo <= 12)
        {
            const int64_t utcEpoch =
                DaysFromCivilUTC(2000 + yy, (unsigned)mo, (unsigned)dd) * 86400 + hh * 3600 + mm * 60 + ss;
            m_UTCEpoch.store(utcEpoch, std::memory_order_relaxed);
            m_HasUTC.store(true, std::memory_order_relaxed);
        }
    }
}

void NMEAGPS::HandleGGA(char** f, int n)
{
    // $--GGA,hhmmss.ss,lat,N/S,lon,E/W,quality,satellites,hdop,altitude,M,...
    if (HasField(f, n, 7))
    {
        m_SatellitesUsed.store(std::atoi(f[7]), std::memory_order_relaxed);
    }
    const bool fix = HasField(f, n, 6) && f[6][0] != '0';
    if (fix && HasField(f, n, 9))
    {
        m_Altitude.store(std::atof(f[9]), std::memory_order_relaxed);
        m_HasAltitude.store(true, std::memory_order_relaxed);
    }
    else
    {
        m_HasAltitude.store(false, std::memory_order_relaxed);
    }
}

void NMEAGPS::HandleGSV(const char* talker, char** f, int n)
{
    // $--GSV,messages,message number,satellites in view,...
    // Each constellation reports its own; the first message carries the count.
    if (!HasField(f, n, 2) || std::atoi(f[2]) != 1 || !HasField(f, n, 3))
    {
        return;
    }
    for (int i = 0; i < kTalkers; ++i)
    {
        if (std::strncmp(talker, kTalkerIds[i], 2) == 0)
        {
            m_InView[i] = std::atoi(f[3]);
            int total = 0;
            bool any = false;
            for (int view : m_InView)
            {
                if (view >= 0)
                {
                    total += view;
                    any = true;
                }
            }
            m_SatellitesInView.store(any ? total : -1, std::memory_order_relaxed);
            return;
        }
    }
}

void NMEAGPS::HandleLine(const char* line)
{
    if (!ChecksumValid(line) || std::strlen(line) < 6)
    {
        return;
    }

    m_Sentences.fetch_add(1, std::memory_order_relaxed);
    if (m_SearchingBaud)
    {
        // A sentence with a valid checksum: this is the speed.
        m_SearchingBaud = false;
        m_BaudInUse.store(kBaudCandidates[m_BaudCandidate], std::memory_order_relaxed);
        DEKI_LOG_INFO("NMEAGPS: the receiver talks at %u baud", (unsigned)kBaudCandidates[m_BaudCandidate]);
    }

    char sentence[sizeof(m_LineBuffer)];
    std::strncpy(sentence, line, sizeof(sentence) - 1);
    sentence[sizeof(sentence) - 1] = 0;
    char talker[3] = { line[1], line[2], 0 };
    const char* type = line + 3;

    char* fields[24] = {};
    const int count = SplitFields(sentence, fields, 24);

    if (std::strncmp(type, "RMC", 3) == 0)
    {
        HandleRMC(fields, count);
    }
    else if (std::strncmp(type, "GGA", 3) == 0)
    {
        HandleGGA(fields, count);
    }
    else if (std::strncmp(type, "GSV", 3) == 0)
    {
        HandleGSV(talker, fields, count);
    }
}

DekiGPSLocation NMEAGPS::Current() const
{
    DekiGPSLocation loc;
    loc.latitude = m_LastLat.load(std::memory_order_relaxed);
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

DekiGPSStatus NMEAGPS::Status() const
{
    DekiGPSStatus status;
    status.sentences = m_Sentences.load(std::memory_order_relaxed);
    status.bytes = m_Bytes.load(std::memory_order_relaxed);
    status.satellitesInView = m_SatellitesInView.load(std::memory_order_relaxed);
    status.satellitesUsed = m_SatellitesUsed.load(std::memory_order_relaxed);
    status.hasAltitude = m_HasAltitude.load(std::memory_order_relaxed);
    status.altitudeMeters = m_Altitude.load(std::memory_order_relaxed);
    status.baudRate = m_BaudInUse.load(std::memory_order_relaxed);
    return status;
}

}  // namespace DekiGps

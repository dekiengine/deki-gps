#include "NMEAGPS.h"
#include "DekiLogSystem.h"

#include <cstdlib>
#include <cstring>
#include <cmath>

#if defined(ESP32)
#include "driver/uart.h"
#endif

namespace
{
    constexpr float kFixStaleAfterSeconds = 5.0f;
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
#if defined(ESP32)
    if (m_PinRX < 0)
    {
        m_LastError = "NMEAGPS: RX pin not configured";
        m_State = ModuleState::Error;
        return false;
    }

    uart_config_t cfg = {};
    cfg.baud_rate = (int)m_Baud;
    cfg.data_bits = UART_DATA_8_BITS;
    cfg.parity    = UART_PARITY_DISABLE;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    if (uart_param_config((uart_port_t)m_UartPort, &cfg) != ESP_OK ||
        uart_set_pin((uart_port_t)m_UartPort, m_PinTX, m_PinRX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK ||
        uart_driver_install((uart_port_t)m_UartPort, 1024, 0, 0, nullptr, 0) != ESP_OK)
    {
        m_LastError = "NMEAGPS: uart driver install failed";
        m_State = ModuleState::Error;
        return false;
    }

    m_State = ModuleState::Running;
    return true;
#else
    m_LastError = "NMEAGPS: hardware path only built for ESP32";
    m_State = ModuleState::Error;
    return false;
#endif
}

void NMEAGPS::Shutdown()
{
#if defined(ESP32)
    uart_driver_delete((uart_port_t)m_UartPort);
#endif
    m_State = ModuleState::Uninitialized;
}

void NMEAGPS::Update(float deltaTime)
{
    if (m_State != ModuleState::Running) return;

    float t = m_SecondsSinceFix.load(std::memory_order_relaxed);
    if (t < 1e8f) m_SecondsSinceFix.store(t + deltaTime, std::memory_order_relaxed);

    PumpUart(deltaTime);
}

void NMEAGPS::PumpUart(float)
{
#if defined(ESP32)
    uint8_t buf[128];
    int n = uart_read_bytes((uart_port_t)m_UartPort, buf, sizeof(buf), 0);
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
#endif
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

bool NMEAGPS::ParseRMC(const char* line, double& lat, double& lon) const
{
    // $GPRMC,utc,status,lat,N/S,lon,E/W,...
    char buf[96];
    std::strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    char* fields[13] = {};
    int n = 0;
    char* save = nullptr;
    for (char* tok = std::strtok(buf, ",*"); tok && n < 13; tok = std::strtok(nullptr, ",*"))
    {
        fields[n++] = tok;
        (void)save;
    }
    if (n < 7) return false;
    if (!fields[2] || fields[2][0] != 'A') return false;     // status A = valid

    lat = NMEACoordToDeg(fields[3], fields[4] ? fields[4][0] : 'N');
    lon = NMEACoordToDeg(fields[5], fields[6] ? fields[6][0] : 'E');
    return true;
}

void NMEAGPS::HandleLine(const char* line)
{
    if (!ChecksumValid(line)) return;

    if (std::strncmp(line, "$GPRMC", 6) == 0 || std::strncmp(line, "$GNRMC", 6) == 0)
    {
        double lat, lon;
        if (ParseRMC(line, lat, lon))
        {
            m_LastLat.store(lat, std::memory_order_relaxed);
            m_LastLon.store(lon, std::memory_order_relaxed);
            m_SecondsSinceFix.store(0.0f, std::memory_order_relaxed);
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

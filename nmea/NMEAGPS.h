#pragma once

#include "../IDekiGPS.h"
#include "ModuleConfig.h"
#include "IDekiUART.h"  // from deki-uart
#include <string>
#include <atomic>

class NMEAGPS : public IDekiGPS
{
public:
    NMEAGPS() = default;
    ~NMEAGPS() override = default;

    const char* GetModuleId() const override   { return "gps"; }
    const char* GetModuleName() const override { return "NMEA GPS (UART)"; }
    void        Configure(const ModuleConfig& config) override;
    bool        Initialize() override;
    void        Shutdown() override;
    void        Update(float deltaTime) override;
    ModuleState GetState() const override      { return m_State; }
    const char* GetLastError() const override  { return m_LastError.c_str(); }

    DekiGPSLocation Current() const override;
    bool            HasLiveFix() const override;
    int64_t         CurrentUTCEpochSeconds() const override;
    bool            HasUTC() const override;

private:
    int        m_PinTX = -1;
    int        m_PinRX = -1;
    int        m_UartPort = 1;
    uint32_t   m_Baud = 9600;
    IDekiUART* m_UART = nullptr;

    ModuleState m_State = ModuleState::Uninitialized;
    std::string m_LastError;

    mutable std::atomic<double>  m_LastLat{0.0};
    mutable std::atomic<double>  m_LastLon{0.0};
    mutable std::atomic<float>   m_SecondsSinceFix{1e9f};
    mutable std::atomic<int64_t> m_UTCEpoch{0};
    mutable std::atomic<bool>    m_HasUTC{false};

    char    m_LineBuffer[96] = {};
    size_t  m_LinePos = 0;

    void  PumpUart();
    void  HandleLine(const char* line);
    bool  ParseRMC(const char* line, double& lat, double& lon, int64_t& utc_epoch, bool& utc_valid) const;
    static bool ChecksumValid(const char* line);
    static double NMEACoordToDeg(const char* coord, char hemi);
};

#pragma once

#include "../IDekiGPS.h"
#include <deki/PackageConfig.h>
#include "IDekiUART.h"  // from deki-uart
#include <string>
#include <atomic>

namespace DekiGps
{

/// GPS over a UART speaking NMEA 0183. Reads $--RMC (position, time), $--GGA
/// (satellites used, altitude) and $--GSV (satellites in view) from any
/// talker (GP, GN, GL, GA, GB, BD, ...).
///
/// A baudRate of 0 finds the speed itself: it listens at 9600, 38400, 115200,
/// 57600 and 4800 in turn, two seconds each, until a sentence with a valid
/// checksum arrives, and keeps that speed.
class NMEAGPS : public IDekiGPS
{
public:
    NMEAGPS() = default;
    ~NMEAGPS() override = default;

    const char* GetPackageId() const override { return "gps"; }
    const char* GetPackageName() const override { return "NMEA GPS (UART)"; }
    void Configure(const Deki::PackageConfig& config) override;
    bool Initialize() override;
    void Shutdown() override;
    void Update(float deltaTime) override;
    Deki::PackageState GetState() const override { return m_State; }
    const char* GetLastError() const override { return m_LastError.c_str(); }

    DekiGPSLocation Current() const override;
    bool HasLiveFix() const override;
    int64_t CurrentUTCEpochSeconds() const override;
    bool HasUTC() const override;
    DekiGPSStatus Status() const override;

private:
    // Talkers whose $--GSV counts are added up for satellites in view.
    static constexpr int kTalkers = 8;

    int m_PinTX = -1;
    int m_PinRX = -1;
    int m_UartPort = 1;
    uint32_t m_Baud = 9600;  // 0: find it
    DekiUart::IDekiUART* m_UART = nullptr;

    // Finding the speed (m_Baud 0).
    bool m_SearchingBaud = false;
    int m_BaudCandidate = 0;
    float m_SecondsAtBaud = 0.0f;
    std::atomic<uint32_t> m_BaudInUse{ 0 };

    Deki::PackageState m_State = Deki::PackageState::Uninitialized;
    std::string m_LastError;

    mutable std::atomic<double> m_LastLat{ 0.0 };
    mutable std::atomic<double> m_LastLon{ 0.0 };
    mutable std::atomic<float> m_SecondsSinceFix{ 1e9f };
    mutable std::atomic<int64_t> m_UTCEpoch{ 0 };
    mutable std::atomic<bool> m_HasUTC{ false };

    std::atomic<uint32_t> m_Sentences{ 0 };
    std::atomic<uint32_t> m_Bytes{ 0 };
    std::atomic<int> m_SatellitesUsed{ -1 };
    std::atomic<bool> m_HasAltitude{ false };
    std::atomic<double> m_Altitude{ 0.0 };
    int m_InView[kTalkers] = { -1, -1, -1, -1, -1, -1, -1, -1 };
    std::atomic<int> m_SatellitesInView{ -1 };

    char m_LineBuffer[96] = {};
    size_t m_LinePos = 0;

    bool OpenUart(uint32_t baud);
    void PumpUart();
    void HandleLine(const char* line);
    void HandleRMC(char** fields, int count);
    void HandleGGA(char** fields, int count);
    void HandleGSV(const char* talker, char** fields, int count);
    static bool ChecksumValid(const char* line);
    static double NMEACoordToDeg(const char* coord, char hemi);
};

}  // namespace DekiGps

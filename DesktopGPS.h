#pragma once

#include "IDekiGPS.h"
#include <deki/PackageConfig.h>
#include <atomic>
#include <string>
#include <thread>

namespace DekiGps
{

/// GPS driver for desktop, where there is no GPS chip. Asks an IP
/// geolocation service (ipwho.is) for an approximate, city-level location
/// once at startup and reports it as the live fix.
///
/// The lookup runs on a background thread so Setup() does not wait on the
/// network. HasLiveFix() is false until the response is parsed, and stays
/// false if the lookup fails.
///
/// Requests go through DekiHttp::GetCurrent(), which the deki-http package
/// sets before deki-gps loads (packages load in alphabetical order).
class DesktopGPS : public IDekiGPS
{
public:
    DesktopGPS() = default;
    ~DesktopGPS() override = default;

    const char* GetPackageId() const override { return "gps"; }
    const char* GetPackageName() const override { return "Desktop IP Geolocation"; }
    void Configure(const Deki::PackageConfig& config) override;
    bool Initialize() override;
    void Shutdown() override;
    void Update(float) override {}
    Deki::PackageState GetState() const override { return m_State; }
    const char* GetLastError() const override { return m_LastError.c_str(); }

    DekiGPSLocation Current() const override;
    bool HasLiveFix() const override;
    int64_t CurrentUTCEpochSeconds() const override;
    bool HasUTC() const override;

private:
    void FetchLocation();

    Deki::PackageState m_State = Deki::PackageState::Uninitialized;
    std::string m_LastError;

    std::atomic<double> m_Lat{ 0.0 };
    std::atomic<double> m_Lon{ 0.0 };
    std::atomic<bool> m_HasFix{ false };
    std::atomic<bool> m_Cancel{ false };

    std::thread m_Thread;
};

}  // namespace DekiGps

#pragma once

#include "IDekiGPS.h"
#include "PackageConfig.h"
#include <atomic>
#include <string>
#include <thread>

/**
 * GPS driver used on desktop where there is no GPS chip. Asks an HTTP IP
 * geolocation service (ip-api.com) for an approximate location once at
 * startup and exposes that as the live fix. City-level accuracy.
 *
 * The lookup runs on a background thread so the SetupComponent's Setup() is
 * not blocked by network latency. HasLiveFix() returns false until the
 * response has been parsed; it stays false on any failure (no fallback).
 *
 * Network is performed via DekiHttp::GetCurrent(), which is set by
 * the deki-http package before deki-gps loads (load order is alphabetical).
 */
class DesktopGPS : public IDekiGPS
{
public:
    DesktopGPS()  = default;
    ~DesktopGPS() override = default;

    const char* GetPackageId() const override   { return "gps"; }
    const char* GetPackageName() const override { return "Desktop IP Geolocation"; }
    void        Configure(const Deki::PackageConfig& config) override;
    bool        Initialize() override;
    void        Shutdown() override;
    void        Update(float) override {}
    Deki::PackageState GetState() const override      { return m_State; }
    const char* GetLastError() const override  { return m_LastError.c_str(); }

    DekiGPSLocation Current() const override;
    bool            HasLiveFix() const override;
    int64_t         CurrentUTCEpochSeconds() const override;
    bool            HasUTC() const override;

private:
    void FetchLocation();

    Deki::PackageState m_State = Deki::PackageState::Uninitialized;
    std::string m_LastError;

    std::atomic<double> m_Lat{0.0};
    std::atomic<double> m_Lon{0.0};
    std::atomic<bool>   m_HasFix{false};
    std::atomic<bool>   m_Cancel{false};

    std::thread m_Thread;
};

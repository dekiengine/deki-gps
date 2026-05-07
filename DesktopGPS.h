#pragma once

#include "IDekiGPS.h"
#include "ModuleConfig.h"
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
 * the deki-http module before deki-gps loads (load order is alphabetical).
 */
class DesktopGPS : public IDekiGPS
{
public:
    DesktopGPS()  = default;
    ~DesktopGPS() override = default;

    const char* GetModuleId() const override   { return "gps"; }
    const char* GetModuleName() const override { return "Desktop IP Geolocation"; }
    void        Configure(const ModuleConfig& config) override;
    bool        Initialize() override;
    void        Shutdown() override;
    void        Update(float) override {}
    ModuleState GetState() const override      { return m_State; }
    const char* GetLastError() const override  { return m_LastError.c_str(); }

    DekiGPSLocation Current() const override;
    bool            HasLiveFix() const override;

private:
    void FetchLocation();

    ModuleState m_State = ModuleState::Uninitialized;
    std::string m_LastError;

    std::atomic<double> m_Lat{0.0};
    std::atomic<double> m_Lon{0.0};
    std::atomic<bool>   m_HasFix{false};
    std::atomic<bool>   m_Cancel{false};

    std::thread m_Thread;
};

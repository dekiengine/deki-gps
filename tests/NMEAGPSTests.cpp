// NMEAGPS end to end, from bytes on a UART to what the game reads: lines
// assembled across reads, checksums, fields split with their empty ones kept,
// RMC position and UTC time, GGA satellites and altitude, GSV satellites in
// view over every constellation, a fix going stale, and finding the speed.
//
// The UART is a fake installed through DekiUART::SetFactory, as a platform
// package installs its real one, so the driver runs unchanged. Sentences are
// written without their checksum and the helper adds it, so each test reads
// as the data a receiver sends.

#include <gtest/gtest.h>

#include "nmea/NMEAGPS.h"
#include "DekiUART.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace DekiGps;

namespace
{

// What the receiver side of the wire holds. The fake UART reads from it; a
// test writes to it. A receiver at a speed other than the one the UART is
// opened at reads as garbage, as it does on real hardware.
struct Wire
{
    std::string pending;
    uint32_t receiverBaud = 9600;
    std::vector<uint32_t> opened;  // each speed the UART was opened at
    bool failInitialize = false;
};
Wire g_Wire;

class FakeUART : public DekiUart::IDekiUART
{
public:
    const char* GetPackageId() const override { return "uart"; }
    const char* GetPackageName() const override { return "Fake UART"; }
    void Configure(const Deki::PackageConfig& config) override
    {
        m_Baud = static_cast<uint32_t>(config.GetInt("baudRate", 0));
    }
    bool Initialize() override
    {
        if (g_Wire.failInitialize)
        {
            return false;
        }
        g_Wire.opened.push_back(m_Baud);
        return true;
    }
    void Shutdown() override {}
    void Update(float) override {}
    Deki::PackageState GetState() const override { return Deki::PackageState::Running; }
    const char* GetLastError() const override { return ""; }

    int Read(uint8_t* dst, size_t maxLen, uint32_t) override
    {
        const size_t n = std::min(maxLen, g_Wire.pending.size());
        for (size_t i = 0; i < n; ++i)
        {
            // Wrong speed: the bytes arrive, but not as the receiver sent them.
            dst[i] = m_Baud == g_Wire.receiverBaud ? static_cast<uint8_t>(g_Wire.pending[i]) : 0xF0;
        }
        g_Wire.pending.erase(0, n);
        return static_cast<int>(n);
    }
    int Write(const uint8_t*, size_t len) override { return static_cast<int>(len); }

private:
    uint32_t m_Baud = 0;
};

DekiUart::IDekiUART* CreateFakeUART()
{
    return new FakeUART();
}

// "$<body>*<checksum>\r\n", the checksum being the XOR of the body.
std::string Sentence(const std::string& body)
{
    uint8_t cs = 0;
    for (char c : body)
    {
        cs ^= static_cast<uint8_t>(c);
    }
    char tail[8];
    std::snprintf(tail, sizeof(tail), "*%02X\r\n", cs);
    return "$" + body + tail;
}

class NMEAGPSTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_Wire = Wire{};
        DekiUart::DekiUART::SetFactory(&CreateFakeUART);
    }

    void TearDown() override
    {
        gps.Shutdown();
        DekiUart::DekiUART::SetFactory(nullptr);
    }

    void Start(int baud = 9600)
    {
        Deki::PackageConfig config;
        config.pins["TX"] = 17;
        config.pins["RX"] = 16;
        config.settings["baudRate"] = std::to_string(baud);
        gps.Configure(config);
        ASSERT_TRUE(gps.Initialize());
    }

    // Puts bytes on the wire and lets the driver read them.
    void Receive(const std::string& bytes, float seconds = 0.1f)
    {
        g_Wire.pending += bytes;
        while (!g_Wire.pending.empty())
        {
            gps.Update(seconds);
            seconds = 0.0f;
        }
    }

    NMEAGPS gps;
};

}  // namespace

// --- RMC: position and time ---

TEST_F(NMEAGPSTest, ReadsPositionAndTimeFromRMC)
{
    Start();
    Receive(Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,131026,003.1,W"));
    EXPECT_TRUE(gps.HasLiveFix());
    EXPECT_NEAR(gps.Current().latitude, 48.1173, 1e-9);
    EXPECT_NEAR(gps.Current().longitude, 11.516666666666667, 1e-9);
    ASSERT_TRUE(gps.HasUTC());
    EXPECT_EQ(gps.CurrentUTCEpochSeconds(), 1791894919);  // 2026-10-13 12:35:19 UTC
}

TEST_F(NMEAGPSTest, SouthAndWestAreNegative)
{
    Start();
    Receive(Sentence("GNRMC,000000,A,3352.000,S,15112.000,W,0,0,010126,,"));
    EXPECT_LT(gps.Current().latitude, 0.0);
    EXPECT_LT(gps.Current().longitude, 0.0);
    EXPECT_NEAR(gps.Current().latitude, -(33 + 52.0 / 60.0), 1e-9);
}

TEST_F(NMEAGPSTest, AnEmptyCourseKeepsTheDateInItsField)
{
    // Standing still, a receiver leaves the course empty (",,"). Splitting
    // with strtok skipped empty fields and read the date from the wrong one.
    Start();
    Receive(Sentence("GNRMC,083000.00,A,5230.000,N,01323.000,E,0.0,,131026,,,A"));
    ASSERT_TRUE(gps.HasUTC());
    EXPECT_EQ(gps.CurrentUTCEpochSeconds(), 1791880200);  // 2026-10-13 08:30:00 UTC
    EXPECT_NEAR(gps.Current().latitude, 52.5, 1e-9);
    EXPECT_NEAR(gps.Current().longitude, 13.383333333333333, 1e-9);
}

TEST_F(NMEAGPSTest, LeapDayConvertsToEpoch)
{
    Start();
    Receive(Sentence("GPRMC,235959,A,0000.000,N,00000.000,E,0,0,290228,,"));
    EXPECT_EQ(gps.CurrentUTCEpochSeconds(), 1835481599);  // 2028-02-29 23:59:59 UTC
}

TEST_F(NMEAGPSTest, AVoidFixMovesNothing)
{
    // Status V: the receiver is talking but has no position yet.
    Start();
    Receive(Sentence("GPRMC,123519,V,4807.038,N,01131.000,E,,,131026,,"));
    EXPECT_FALSE(gps.HasLiveFix());
    EXPECT_FALSE(gps.HasUTC());
    EXPECT_DOUBLE_EQ(gps.Current().latitude, 0.0);
    EXPECT_EQ(gps.Status().sentences, 1u);
}

TEST_F(NMEAGPSTest, AFixGoesStaleAfterFiveSeconds)
{
    Start();
    Receive(Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,"));
    ASSERT_TRUE(gps.HasLiveFix());
    gps.Update(4.0f);
    EXPECT_TRUE(gps.HasLiveFix());
    gps.Update(1.5f);
    EXPECT_FALSE(gps.HasLiveFix());
    // The last position is still there to show.
    EXPECT_NEAR(gps.Current().latitude, 48.1173, 1e-9);
}

TEST_F(NMEAGPSTest, AnImpossibleTimeLeavesUTCUnset)
{
    Start();
    Receive(Sentence("GPRMC,253519,A,4807.038,N,01131.000,E,0,0,131026,,"));
    EXPECT_TRUE(gps.HasLiveFix());
    EXPECT_FALSE(gps.HasUTC());
}

// --- Checksums and line assembly ---

TEST_F(NMEAGPSTest, ABadChecksumIsIgnoredButItsBytesAreCounted)
{
    Start();
    std::string s = Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,");
    s[s.find('*') + 1] = s[s.find('*') + 1] == '0' ? '1' : '0';
    Receive(s);
    EXPECT_FALSE(gps.HasLiveFix());
    EXPECT_EQ(gps.Status().sentences, 0u);
    EXPECT_EQ(gps.Status().bytes, s.size());
}

TEST_F(NMEAGPSTest, LinesWithoutChecksumOrDollarAreIgnored)
{
    Start();
    Receive("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,\r\n");
    Receive("$GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,\r\n");
    EXPECT_EQ(gps.Status().sentences, 0u);
}

TEST_F(NMEAGPSTest, ASentenceSplitAcrossReadsIsAssembled)
{
    Start();
    const std::string s = Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,");
    Receive(s.substr(0, 20));
    EXPECT_EQ(gps.Status().sentences, 0u);
    Receive(s.substr(20));
    EXPECT_EQ(gps.Status().sentences, 1u);
    EXPECT_TRUE(gps.HasLiveFix());
}

TEST_F(NMEAGPSTest, AnOverlongLineDoesNotStopTheNextOne)
{
    // Noise with no newline fills the 96-byte line buffer; the driver must
    // drop it and still read the sentence after.
    Start();
    Receive(std::string(300, 'x'));
    Receive("\r\n" + Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,"));
    EXPECT_EQ(gps.Status().sentences, 1u);
    EXPECT_TRUE(gps.HasLiveFix());
}

// --- GGA and GSV ---

TEST_F(NMEAGPSTest, GGAGivesSatellitesUsedAndAltitude)
{
    Start();
    EXPECT_EQ(gps.Status().satellitesUsed, -1);
    Receive(Sentence("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,"));
    EXPECT_EQ(gps.Status().satellitesUsed, 8);
    EXPECT_TRUE(gps.Status().hasAltitude);
    EXPECT_DOUBLE_EQ(gps.Status().altitudeMeters, 545.4);

    // Quality 0: no fix, so no altitude, though the count still reads.
    Receive(Sentence("GPGGA,123520,,,,,0,03,,,M,,M,,"));
    EXPECT_EQ(gps.Status().satellitesUsed, 3);
    EXPECT_FALSE(gps.Status().hasAltitude);
}

TEST_F(NMEAGPSTest, GSVAddsUpEveryConstellation)
{
    Start();
    EXPECT_EQ(gps.Status().satellitesInView, -1);
    Receive(Sentence("GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00"));
    EXPECT_EQ(gps.Status().satellitesInView, 11);
    Receive(Sentence("GLGSV,2,1,05,65,10,100,,66,20,200,,67,30,300,,68,40,040,"));
    EXPECT_EQ(gps.Status().satellitesInView, 16);

    // Only the first message of a group carries the count; later ones
    // repeat it and must not be added again.
    Receive(Sentence("GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00"));
    EXPECT_EQ(gps.Status().satellitesInView, 16);

    // A new count from a constellation replaces its old one.
    Receive(Sentence("GPGSV,3,1,09,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00"));
    EXPECT_EQ(gps.Status().satellitesInView, 14);
}

// --- Speed ---

TEST_F(NMEAGPSTest, AConfiguredSpeedIsReportedAtOnce)
{
    Start(38400);
    EXPECT_EQ(gps.Status().baudRate, 38400u);
    ASSERT_EQ(g_Wire.opened.size(), 1u);
    EXPECT_EQ(g_Wire.opened[0], 38400u);
}

TEST_F(NMEAGPSTest, SpeedZeroFindsTheReceiversSpeed)
{
    g_Wire.receiverBaud = 38400;
    Start(0);
    EXPECT_EQ(gps.Status().baudRate, 0u);  // not found yet

    // 9600 first: two seconds of garbage, then the next speed.
    const std::string rmc = Sentence("GPRMC,123519,A,4807.038,N,01131.000,E,0,0,131026,,");
    Receive(rmc, 1.0f);
    Receive(rmc, 1.0f);
    ASSERT_GE(g_Wire.opened.size(), 2u);
    EXPECT_EQ(g_Wire.opened[0], 9600u);
    EXPECT_EQ(g_Wire.opened[1], 38400u);
    EXPECT_EQ(gps.Status().sentences, 0u);

    Receive(rmc, 0.5f);
    EXPECT_EQ(gps.Status().baudRate, 38400u);
    EXPECT_TRUE(gps.HasLiveFix());

    // Found: it stays at that speed however long the receiver is quiet.
    gps.Update(10.0f);
    EXPECT_EQ(g_Wire.opened.size(), 2u);
    EXPECT_EQ(gps.Status().baudRate, 38400u);
}

// --- Failure ---

TEST_F(NMEAGPSTest, NoUARTBackendIsAnError)
{
    DekiUart::DekiUART::SetFactory(nullptr);
    gps.Configure(Deki::PackageConfig{});
    EXPECT_FALSE(gps.Initialize());
    EXPECT_EQ(gps.GetState(), Deki::PackageState::Error);
    EXPECT_STRNE(gps.GetLastError(), "");
}

TEST_F(NMEAGPSTest, AUARTThatWillNotOpenIsAnError)
{
    g_Wire.failInitialize = true;
    gps.Configure(Deki::PackageConfig{});
    EXPECT_FALSE(gps.Initialize());
    EXPECT_EQ(gps.GetState(), Deki::PackageState::Error);
}

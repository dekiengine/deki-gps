# Deki GPS

Docs: https://dekiengine.github.io/deki-gps/ (components and properties, generated from the code)

GPS peripheral interface for the Deki Engine, with an NMEA-over-UART backend.

Part of [Deki Engine](https://github.com/dekiengine/deki-engine).

## Desktop backend and your IP address

There is no GPS on a desktop machine, so the desktop backend asks
[ipwho.is](https://ipwho.is) where it is. The answer comes from the address the
request arrives from, so the request itself is the disclosure.

In practice:

- **It runs when you press Play**, and when the package starts in a built
  game. Just opening a project does not send it. Nothing prompts you, so it is
  worth knowing that Play does.
- **Cached for an hour** in `S:/deki-gps-location.txt`. Restarting inside that
  hour sends nothing. Delete the file to force a fresh lookup.
- **HTTPS, no API key**, and ipwho.is allows commercial use on the free
  endpoint. The limit is 1,000 requests a day per address, counted against the
  machine that asks, so players never share a quota.
- **Accuracy is city-level.** It is a stand-in so a GPS-driven scene does
  something on a desktop, not a position fix.

Leave the package out of a desktop target if you would rather it never asked.
Device builds use the NMEA-over-UART backend and make no network request at all.

Before 0.16.0 this used ip-api.com over plain **HTTP** (they sell HTTPS), and
its free tier was non-commercial only, which every game shipping the engine
inherited without being told. That is why it changed.

## Namespace

Types live in `DekiGps`. Scene files store the qualified name, and so does code:

```cpp
using namespace DekiGps;
obj->AddComponent<SomeComponent>();
```

Scenes saved before 0.16.0 used bare names and still load; saving writes the current one.

## Dependencies

| Dependency | Type |
|---|---|
| `deki-http` | Deki package |
| `deki-uart` | Deki package |

## Install

Package Manager in the Deki Editor, or `DekiEditor --packages-add deki-gps <project>`.

## License

Apache 2.0. See [LICENSE](LICENSE).

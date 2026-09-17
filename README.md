# Deki GPS

Documentation: https://dekiengine.github.io/deki-gps/ (components and properties, generated from the code)

GPS peripheral interface for the Deki Engine, with an NMEA-over-UART backend.

Part of the [Deki Engine](https://github.com/dekiengine/deki-engine) package ecosystem.

## Desktop backend and your IP address

There is no GPS on a desktop machine, so the desktop backend asks
[ipwho.is](https://ipwho.is) where the machine is. It answers from the address
the request arrives from, which means the request itself is the disclosure.

What that means in practice:

- **It runs from the backend's `Initialize()`.** Having this package active in
  a desktop build is enough to make it happen. Nothing asks first.
- **The answer is cached for an hour**, in `S:/deki-gps-location.txt` beside the
  game's other storage. A second run inside that hour makes no request at all,
  which matters while you are testing and restarting repeatedly. Delete the file
  to force a fresh lookup.
- **Over HTTPS**, with no API key, and ipwho.is permits commercial use on that
  free endpoint. Its published limit is 1,000 requests a day per address, which
  is counted against the machine that asks, so your players never share a quota.
- **Accuracy is city-level.** It is a stand-in so a GPS-driven scene does
  something on a desktop, not a position fix.

Leave the package out of a desktop target if you would rather it never asked.
Device builds use the NMEA-over-UART backend and make no network request at all.

Before 0.16.0 this used ip-api.com over plain **HTTP**, because that service
sells HTTPS as a paid feature. Its free tier also limited use to "a
non-commercial purpose and in a non-commercial environment", which every
developer shipping a game inherited from the engine without being told. Both
problems are why it changed.

## Namespace

This package's types live in `DekiGps`. Scene files store the qualified
name, so a component is `DekiGps::SomeComponent` there, and code naming one
needs the namespace:

```cpp
using namespace DekiGps;
obj->AddComponent<SomeComponent>();
```

Scenes saved before 0.16.0 used bare names and still load: every component
records what it used to be called, and a save writes the current name.

## Dependencies

| Dependency | Type |
|---|---|
| `deki-http` | Deki package |
| `deki-uart` | Deki package |

## Installation

Install via the Package Manager inside the Deki Editor.

## License

Licensed under the Apache License, Version 2.0. See [LICENSE](LICENSE) for details.

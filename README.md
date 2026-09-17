# Deki GPS

Documentation: https://dekiengine.github.io/deki-gps/ (components and properties, generated from the code)

GPS peripheral interface for the Deki Engine, with an NMEA-over-UART backend.

Part of the [Deki Engine](https://github.com/dekiengine/deki-engine) package ecosystem.

## Desktop backend and your IP address

There is no GPS on a desktop machine, so the desktop backend asks
[ip-api.com](https://ip-api.com) where the machine is, which it answers from
the IP address the request comes from. Two things to know:

- The request is plain HTTP. ip-api.com serves HTTPS only on its paid tier.
- It runs from the backend's `Initialize()`, so having this package active in
  a desktop build is enough to make it happen. Nothing asks first.

So a simulator run with this package sends your IP address to a third party
unencrypted. Leave the package out of a desktop target if that is not what you
want. Device builds use the NMEA-over-UART backend and make no network request.

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

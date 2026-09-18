# Changelog

Notable changes to `deki-gps`. Engine and editor changes are in the
[engine changelog](https://github.com/dekiengine/deki-engine/blob/master/CHANGELOG.md).

A package's `minEngine` names the engine version it needs. Before 1.0 a
breaking change bumps the minor across the editor, the engine and every
package together, so a package with no changes of its own is still released
alongside one that has them.

## 0.16.0

### Changed
- **The location lookup waits for Play.** It was registered to run when the
  editor finished loading packages, so merely opening a project sent a request
  to a third party. It is registered for the Play phase now, which is the point
  at which the user has asked for the game to run. A built game is unchanged:
  the setup runs when the package starts.
- **The desktop backend asks ipwho.is, over HTTPS, instead of ip-api.com.** The
  old endpoint was plain HTTP, because that service sells encryption as a paid
  feature, so the machine's address travelled in the clear. Its free tier also
  restricted use to "a non-commercial purpose and in a non-commercial
  environment", a restriction every developer shipping a game inherited from
  the engine without ever being told. ipwho.is permits commercial use on its
  keyless free endpoint, and its quota is counted per calling address, so one
  game's players never share a budget.
- **The answer is cached for an hour**, in `S:/deki-gps-location.txt`. The
  lookup runs once per process, so without a cache that outlives the process
  every restart while testing was another request and another disclosure. A
  missing, unreadable, truncated or future-dated file is a cache miss, never an
  error. Delete the file to force a fresh lookup.
- **Moved into the `DekiGps` namespace.** Every component was declared at global
  scope, which made its identity a bare class name — the name a scene file
  stores and the name the registry keys on — so two packages defining one name
  collided there with nothing to tell them apart. Each component carries
  `DEKI_FORMER_NAME` with the name it was saved under before, so existing
  scenes load unchanged and are written back qualified on the next save.
  Code naming these types needs the namespace: `using namespace DekiGps;` or a
  qualified name.
- Enum properties are stored by name rather than by number, so appending to an
  enum or reordering one no longer changes what a saved scene means. Files
  written before this still read.
- `minEngine` 0.16.0. Reflection ABI 17: the package must be rebuilt.

## 0.15.0

### Changed
- No changes of its own. Released alongside engine 0.15.0 so `minEngine`
  tracks the engine version.

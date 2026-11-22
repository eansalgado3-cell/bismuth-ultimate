# Bismuth Ultimate

Independent Geode mod project for Geometry Dash 2.2081 (Windows x64).

## Current status — v0.1.0-alpha.1

This first build is a safe bootstrap that confirms Geode can load the mod. It logs a startup message and deliberately does not alter gameplay, physics, frame timing, rendering, particles, or input.

## Build

GitHub Actions builds a Windows x64 .geode package using the official geode-sdk/build-geode-mod action. Open Actions, select Build Bismuth Ultimate, open the latest successful run, and download the Bismuth-Ultimate-Windows-x64 artifact.

## Roadmap

- [x] Geode project and Windows CI bootstrap
- [ ] In-game performance diagnostics (frame-time percentiles and sampling)
- [ ] Opt-in, independently configurable optimizations
- [ ] Stress-test matrix and before/after benchmarks
- [ ] Validate timing, input, replay and physics behavior

No FPS improvement is claimed until it is benchmarked in Geometry Dash.

# NEXUS Ultimate

Windows x64 performance diagnostics and optimization project for Geometry Dash 2.2081 using Geode 5.11.2.

## Current development direction

NEXUS is being refocused on **stable frame pacing and engine-level performance**, with a target such as holding 90 FPS on modest hardware where the game can otherwise suffer large frame-time spikes.

The current safe baseline preserves the game's original rendering path. Legacy features that filtered sprites or batched level geometry, culled particles, suppressed particle simulation, capped particle pools, or culled editor content are hard-disabled in code. This applies to existing installations too, even if an old saved setting enabled one of those options.

NEXUS does not intentionally remove visual effects, particles, objects, or level content. It does not modify gameplay physics, object timing, music synchronization, or input handling.

## Measurement before optimization

The next development phase is to use frame-time percentiles, 1% lows, CPU-side timings, non-blocking GPU timing where supported, and repeatable test scenes to identify actual bottlenecks. Changes will be kept only when they improve frame pacing without changing the game's output.

No universal FPS gain is promised. Results must be compared against the unmodified game on the same hardware, level, resolution, and frame-rate target.

## Credits and license

NEXUS Ultimate incorporates and adapts MIT-licensed code from Minimum Reborn by ziyyhhk. Attribution and the upstream MIT license are retained in LICENSE. The name, icon and project metadata are original to this project.

## Builds

The Windows x64 .geode artifact is produced by the GitHub Actions build workflow when a build succeeds.

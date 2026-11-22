# NEXUS Ultimate

Windows x64 performance mod for Geometry Dash 2.2081 using Geode 5.11.2.

This alpha includes sprite fast-outs, particle pool controls, off-screen/idle particle draw culling, CPU per-quad filtering for eligible batched level geometry, optional low-latency GPU queue draining, Windows frame pacing controls and an FPS/frame-time diagnostics overlay.

No universal FPS gain is promised. Geometry Dash batches level objects; gains depend on level, driver and hardware. Use the overlay counters and compare the same section with features enabled and disabled. Physics and game input code are not modified. Some particle caps can thin visual effects; GPU queue draining can trade average FPS for responsiveness.

## Credits and license
NEXUS Ultimate incorporates and adapts MIT-licensed code from Minimum Reborn by ziyyhhk. Attribution and the upstream MIT license are retained in LICENSE. The name, icon and project metadata are original to this project.

Download the Windows `.geode` artifact from GitHub Actions.

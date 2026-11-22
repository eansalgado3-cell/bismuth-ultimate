#include <Geode/Geode.hpp>

using namespace geode::prelude;

// Safe bootstrap: no gameplay, physics, rendering, or input hooks are modified.
$on_mod(Loaded) {
    log::info("Bismuth Ultimate v0.1.0-alpha.1 loaded.");
    log::info("Diagnostics bootstrap active; no gameplay or physics hooks are modified.");
}

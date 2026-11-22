#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/OverlayManager.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include "config.hpp"
#include "frame_stats.hpp"
#include "gpu_cull.hpp"
#include "hud.hpp"
#include "rendering.hpp"

using namespace geode::prelude;

// Settings are opened with the configurable F10 keybind. No pause-menu button is installed.
// The performance overlay remains independent of the settings popup.

namespace {

    char const* bottleneckLabel(nexus::Bottleneck hint) {
        switch (hint) {
            case nexus::Bottleneck::DrawBound: return "draw path busy";
            case nexus::Bottleneck::GpuOrOther: return "GPU or wait";
            default: return "not measured";
        }
    }

    // Performance overlay. Numbers only; no color semantics.
    class StatsHud : public CCNode {
    protected:
        CCLabelBMFont* m_label = nullptr;
        std::string m_lastText;
        bool m_right = false;
        bool m_bottom = false;

        bool init() override {
            if (!CCNode::init()) return false;
            m_label = CCLabelBMFont::create("NEXUS Ultimate", "chatFont.fnt");
            if (!m_label) return false;
            this->addChild(m_label);
            this->schedule(schedule_selector(StatsHud::tick), 0.25f);
            this->applyLayout();
            return true;
        }

        void applyLayout() {
            auto const& cfg = nexus::config();
            auto const win = CCDirector::get()->getWinSize();
            bool const right = cfg.overlayCorner == nexus::Corner::TopRight
                            || cfg.overlayCorner == nexus::Corner::BottomRight;
            bool const bottom = cfg.overlayCorner == nexus::Corner::BottomLeft
                             || cfg.overlayCorner == nexus::Corner::BottomRight;

            if (right != m_right) {
                m_label->setAlignment(right ? kCCTextAlignmentRight : kCCTextAlignmentLeft);
                m_right = right;
            }
            m_bottom = bottom;
            m_label->setAnchorPoint({right ? 1.f : 0.f, bottom ? 0.f : 1.f});
            m_label->setPosition({
                right ? win.width - 6.f : 6.f,
                bottom ? 6.f : win.height - 6.f
            });
            m_label->setScale(cfg.overlayScale);
            m_label->setOpacity(cfg.overlayOpacity);
            m_label->setColor(ccc3(238, 241, 248));
            m_label->setVisible(cfg.showOverlay && cfg.enabled);
        }

        void tick(float) {
            auto const& cfg = nexus::config();
            this->applyLayout();
            if (!cfg.showOverlay || !cfg.enabled) return;

            auto& counters = nexus::counters();
            auto& stats = nexus::frameStats();
            std::uint32_t const fps = counters.fps.load(std::memory_order_relaxed);
            std::uint32_t const logic = counters.logicFps.load(std::memory_order_relaxed);
            double const avgMs = stats.averageFrameMs(120);
            double const p95Ms = stats.percentileFrameMs(120, 0.95);
            double const lowFps = stats.onePercentLowFps(240);
            double const workMs = stats.averageWorkMs(60);

            char text[1024];
            if (cfg.overlayDetailed) {
                char lowText[32];
                if (lowFps > 0.5) std::snprintf(lowText, sizeof(lowText), "%.0f", lowFps);
                else std::snprintf(lowText, sizeof(lowText), "--");
                char gpuText[32];
                float const gpuMs = counters.gpuFrameMs.load(std::memory_order_relaxed);
                if (gpuMs >= 0.01f) std::snprintf(gpuText, sizeof(gpuText), "%.2f ms", gpuMs);
                else std::snprintf(gpuText, sizeof(gpuText), "--");
                std::snprintf(
                    text, sizeof(text),
                    "%s%u FPS%s | avg %.1f ms | p95 %.1f ms | 1%% low %s\n"
                    "logic %u/s | draw %.2f ms (worst %.2f ms) | gpu %s | %s | renderer %s\n"
                    "scene %u/%u/s | sprite fast-out %u/s | particles %u/%u/s | editor %u/s | capped %u/s | batch culls %llu\n"
                    "quads culled %llu | batch scans %llu | fully culled %llu | sims supp %llu | trace binds %llu\n"
                    "gpu batches %llu | gpu quads %llu",
                    cfg.enabled ? "" : "(OFF) ", fps,
                    counters.throttled.load(std::memory_order_relaxed) ? " (background)" : "",
                    avgMs, p95Ms, lowText,
                    logic, workMs, stats.worstWorkMs(240), gpuText,
                    bottleneckLabel(static_cast<nexus::Bottleneck>(counters.bottleneck.load(std::memory_order_relaxed))),
                    cfg.rendererOptimized ? "optimized" : "engine",
                    counters.sceneCulledPerSec.load(std::memory_order_relaxed),
                    counters.sceneExaminedPerSec.load(std::memory_order_relaxed),
                    counters.spriteSkipsPerSec.load(std::memory_order_relaxed),
                    counters.particleSkipsPerSec.load(std::memory_order_relaxed),
                    counters.particleExaminedPerSec.load(std::memory_order_relaxed),
                    counters.editorCulledPerSec.load(std::memory_order_relaxed),
                    counters.cappedPerSec.load(std::memory_order_relaxed),
                    static_cast<unsigned long long>(
                        counters.batchCulls.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(
                        counters.batchQuadsCulled.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(
                        counters.batchScans.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(
                        counters.batchFullyCulled.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(
                        counters.particleSimsSuppressed.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(
                        counters.shaderTraceBinds.load(std::memory_order_relaxed)),
                    static_cast<unsigned long long>(nexus::gpuCullBatchesSubmitted()),
                    static_cast<unsigned long long>(nexus::gpuCullQuadsScanned())
                );
            }
            else {
                std::snprintf(
                    text, sizeof(text), "%s%u FPS | %.1f ms | 1%% low %.0f",
                    cfg.enabled ? "" : "(OFF) ", fps, avgMs, lowFps
                );
            }

            if (m_lastText != text) {
                m_lastText = text;
                m_label->setString(text);
            }
        }

    public:
        static StatsHud* create() {
            auto* ret = new StatsHud();
            if (ret && ret->init()) {
                ret->autorelease();
                return ret;
            }
            CC_SAFE_DELETE(ret);
            return nullptr;
        }
    };

    bool g_hudCreated = false;
}

void openNexusSettings() {
    openSettingsPopup(Mod::get(), true);
}


namespace nexus {
    void ensureHud(unsigned int totalFrames) {
        // Create early and let applyLayout control visibility; an overlay created only
        // after frame 300 never appears if the setting is flipped later.
        if (g_hudCreated || totalFrames < 120) return;
        auto* overlay = OverlayManager::get();
        if (!overlay) return;
        if (auto* hud = StatsHud::create()) {
            hud->setID("nexus-ultimate-hud"_spr);
            overlay->addChild(hud, 99999);
            g_hudCreated = true;
        }
    }
}

$on_game(Loaded) {
    listenForKeybindSettingPresses(
        "open-settings",
        [](Keybind const&, bool down, bool repeat, double) {
            if (down && !repeat) {
                openNexusSettings();
            }
        }
    );
}

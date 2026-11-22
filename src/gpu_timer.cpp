#include <Geode/Geode.hpp>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#include "config.hpp"
#include "gpu_timer.hpp"

namespace nexus {

    namespace {
        // glew32.dll exports GLEW's function-pointer tables (double-underscore data symbols),
        // not an import library this toolchain can link, so the entry points come from
        // GetProcAddress at runtime - the same pattern ModImpl uses for the game's own
        // symbols. GD initializes GLEW at startup, which fills these in.
        using GenQueries = void (APIENTRY*)(unsigned int n, unsigned int* ids);
        using BeginQuery = void (APIENTRY*)(unsigned int target, unsigned int id);
        using EndQuery = void (APIENTRY*)(unsigned int target);
        using GetQueryObjectuiv = void (APIENTRY*)(unsigned int id, unsigned int pname, unsigned int* params);

        GenQueries p_genQueries = nullptr;
        BeginQuery p_beginQuery = nullptr;
        EndQuery p_endQuery = nullptr;
        GetQueryObjectuiv p_getQueryObjectuiv = nullptr;

        // cocos OGLES headers #define these GL constants, so the mod's own are renamed.
        unsigned int const kTimeElapsedQueryTarget = 0x88BF;
        unsigned int const kQueryResult = 0x87CB;
        unsigned int const kQueryResultAvailable = 0x8866;

        bool resolveGl() {
            if (p_genQueries) return true;
            HMODULE mod = GetModuleHandleA("glew32.dll");
            if (!mod) mod = LoadLibraryA("glew32.dll");
            if (!mod) return false;
            auto const genVar = reinterpret_cast<GenQueries*>(GetProcAddress(mod, "__glewGenQueries"));
            auto const beginVar = reinterpret_cast<BeginQuery*>(GetProcAddress(mod, "__glewBeginQuery"));
            auto const endVar = reinterpret_cast<EndQuery*>(GetProcAddress(mod, "__glewEndQuery"));
            auto const getObjVar = reinterpret_cast<GetQueryObjectuiv*>(GetProcAddress(mod, "__glewGetQueryObjectuiv"));
            if (!genVar || !beginVar || !endVar || !getObjVar) return false;
            // GLEW leaves these null when the context lacks the feature (anything below
            // GL 1.5). Null here means no GL query support: the timer stays off.
            if (!*genVar || !*beginVar || !*endVar || !*getObjVar) return false;
            p_genQueries = *genVar;
            p_beginQuery = *beginVar;
            p_endQuery = *endVar;
            p_getQueryObjectuiv = *getObjVar;
            return true;
        }

        constexpr unsigned int kQueryCount = 4;
        struct QuerySlot {
            unsigned int id = 0;
            bool pending = false;
            bool active = false;
        };
        QuerySlot g_queries[kQueryCount];
        bool g_initialized = false;
        int g_activeSlot = -1;

        bool timerReady() {
            if (g_initialized) return g_queries[0].id != 0;
            g_initialized = true;
            if (!resolveGl()) return false;
            unsigned int ids[kQueryCount]{};
            p_genQueries(kQueryCount, ids);
            for (unsigned int i = 0; i < kQueryCount; ++i) {
                g_queries[i].id = ids[i];
            }
            return g_queries[0].id != 0;
        }

        void collectResults() {
            for (auto& slot : g_queries) {
                if (!slot.pending) continue;
                // Availability is checked first. If the GPU is still working, keep this
                // query pending and use another free slot; never wait for a result.
                unsigned int available = 0;
                p_getQueryObjectuiv(slot.id, kQueryResultAvailable, &available);
                if (!available) continue;

                unsigned int nanoseconds = 0;
                p_getQueryObjectuiv(slot.id, kQueryResult, &nanoseconds);
                slot.pending = false;
                double const ms = static_cast<double>(nanoseconds) / 1e6;
                if (ms > 0.0 && ms < 1000.0) {
                    nexus::counters().gpuFrameMs.store(static_cast<float>(ms));
                }
            }
        }
    }

    void gpuTimerBegin() {
        auto const& cfg = config();
        if (!cfg.enabled || !cfg.gpuFrameTime) return;
        if (!timerReady()) return;
        collectResults();

        // A small query ring tolerates several frames of GPU latency. If every slot is
        // still pending, skip this sample rather than reusing a query or blocking.
        for (unsigned int i = 0; i < kQueryCount; ++i) {
            auto& slot = g_queries[i];
            if (slot.pending || slot.active || slot.id == 0) continue;
            p_beginQuery(kTimeElapsedQueryTarget, slot.id);
            slot.active = true;
            g_activeSlot = static_cast<int>(i);
            return;
        }
    }

    void gpuTimerEnd() {
        if (g_activeSlot < 0) return;
        auto& slot = g_queries[static_cast<unsigned int>(g_activeSlot)];
        p_endQuery(kTimeElapsedQueryTarget);
        slot.active = false;
        slot.pending = true;
        g_activeSlot = -1;
    }

}

#else

namespace nexus {
    void gpuTimerBegin() {}
    void gpuTimerEnd() {}
}

#endif

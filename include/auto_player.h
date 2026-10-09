#pragma once
#include "engine.h"

namespace hc {
// One finite state transition of an already initialized match.
// Only Engine-validated actions are attempted; no fabricated card effects.
struct AutoStep {
    Result result;
    Phase phaseBefore;
    int actor;
    AutoStep(const Result& value, Phase phase, int player)
        : result(value), phaseBefore(phase), actor(player) {}
};
class AutoPlayer {
public:
    static AutoStep step(Engine& engine);
};
}

#include "core/Timeline.hpp"

#include <algorithm>

namespace xeldi {

const char* toString(LoopMode m) {
    switch (m) {
        case LoopMode::Forward: return "forward";
        case LoopMode::Reverse: return "reverse";
        case LoopMode::PingPong: return "pingpong";
    }
    return "forward";
}

LoopMode loopModeFromString(const std::string& s) {
    if (s == "reverse") return LoopMode::Reverse;
    if (s == "pingpong" || s == "ping-pong") return LoopMode::PingPong;
    return LoopMode::Forward;
}

const AnimationTag* Timeline::findTag(const std::string& name) const {
    for (const auto& t : tags)
        if (t.name == name) return &t;
    return nullptr;
}

const AnimationTag* Timeline::previewTag() const {
    for (const auto& t : tags)
        if (t.autoPreview) return &t;
    return findTag("idle");
}

void Timeline::onFrameInserted(int at) {
    for (auto& t : tags) {
        if (at <= t.from) { ++t.from; ++t.to; }
        else if (at <= t.to) ++t.to;
    }
}

void Timeline::onFrameRemoved(int at) {
    for (auto& t : tags) {
        if (at < t.from) { --t.from; --t.to; }
        else if (at <= t.to) --t.to;
    }
    tags.erase(std::remove_if(tags.begin(), tags.end(), [](const AnimationTag& t) { return t.to < t.from; }),
               tags.end());
}

}  // namespace xeldi

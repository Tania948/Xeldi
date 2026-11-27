#include "engine2d/AnimationPlayer.hpp"

#include <algorithm>

namespace xeldi {

void AnimationPlayer::setRange(int from, int to, LoopMode mode, int fpsOverride) {
    if (to < from) std::swap(from, to);
    bool changed = from != from_ || to != to_ || mode != mode_;
    from_ = from;
    to_ = to;
    mode_ = mode;
    fps_ = fpsOverride;
    if (changed || current_ < from_ || current_ > to_) reset();
}

void AnimationPlayer::reset() {
    accum_ = 0;
    if (mode_ == LoopMode::Reverse) {
        current_ = to_;
        dir_ = -1;
    } else {
        current_ = from_;
        dir_ = 1;
    }
}

double AnimationPlayer::frameDurationMs(const Timeline& tl, int f) const {
    if (fps_ > 0) return 1000.0 / fps_;
    if (f >= 0 && f < tl.frameCount() && tl.frames[f].durationMs > 0) return tl.frames[f].durationMs;
    return 1000.0 / std::max(1, tl.fps);
}

void AnimationPlayer::step() {
    if (from_ == to_) { current_ = from_; return; }
    switch (mode_) {
        case LoopMode::Forward:
            current_ = current_ >= to_ ? from_ : current_ + 1;
            break;
        case LoopMode::Reverse:
            current_ = current_ <= from_ ? to_ : current_ - 1;
            break;
        case LoopMode::PingPong:
            if (current_ + dir_ > to_ || current_ + dir_ < from_) dir_ = -dir_;
            current_ += dir_;
            break;
    }
}

bool AnimationPlayer::advance(const Timeline& tl, double ms) {
    int start = current_;
    accum_ += ms;
    int guard = 0;
    for (double d = frameDurationMs(tl, current_); accum_ >= d && guard < 10000; d = frameDurationMs(tl, current_)) {
        accum_ -= d;
        step();
        ++guard;
    }
    return current_ != start;
}

}  // namespace xeldi

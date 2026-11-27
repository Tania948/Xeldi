#pragma once
// Reproductor de animacion independiente del reloj: la UI le pasa los
// milisegundos transcurridos. Sirve para la preview idle en bucle y el play
// del lienzo principal.
#include "core/Timeline.hpp"

namespace xeldi {

class AnimationPlayer {
public:
    void setRange(int from, int to, LoopMode mode = LoopMode::Forward, int fpsOverride = 0);
    void setTag(const AnimationTag& t) { setRange(t.from, t.to, t.loop, t.fpsOverride); }
    void setWholeTimeline(const Timeline& tl) { setRange(0, tl.frameCount() - 1, LoopMode::Forward, 0); }
    void reset();

    int frame() const { return current_; }
    int from() const { return from_; }
    int to() const { return to_; }
    LoopMode mode() const { return mode_; }

    /// Duracion en ms del frame dado (respeta fpsOverride).
    double frameDurationMs(const Timeline& tl, int frame) const;
    /// Avanza `ms`. Devuelve true si cambio el frame mostrado.
    bool advance(const Timeline& tl, double ms);
    /// Frame siguiente segun el modo de bucle (sin tiempo).
    void step();

private:
    int from_ = 0, to_ = 0, current_ = 0, dir_ = 1, fps_ = 0;
    LoopMode mode_ = LoopMode::Forward;
    double accum_ = 0;
};

}  // namespace xeldi

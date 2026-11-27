#pragma once
#include <string>
#include <vector>

#include "core/Color.hpp"

namespace xeldi {

struct Frame {
    int durationMs = 100;
};

enum class LoopMode { Forward, Reverse, PingPong };

const char* toString(LoopMode m);
LoopMode loopModeFromString(const std::string& s);

/// Rango con nombre de la linea de tiempo (idle, walk, attack...).
struct AnimationTag {
    std::string name;
    int from = 0;
    int to = 0;
    LoopMode loop = LoopMode::Forward;
    int fpsOverride = 0;       ///< 0 = usar duraciones por frame
    bool autoPreview = false;  ///< se reproduce solo en la ventana de preview
    Rgba color = 0xF29BB8FFu;
};

struct Timeline {
    int fps = 12;
    std::vector<Frame> frames{Frame{}};
    std::vector<AnimationTag> tags;

    int frameCount() const { return int(frames.size()); }
    const AnimationTag* findTag(const std::string& name) const;
    /// Primer tag con autoPreview, o el primero llamado "idle", o nullptr.
    const AnimationTag* previewTag() const;
    /// Ajusta los rangos de los tags al insertar/quitar frames.
    void onFrameInserted(int at);
    void onFrameRemoved(int at);
};

}  // namespace xeldi

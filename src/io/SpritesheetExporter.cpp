#include "io/SpritesheetExporter.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "engine2d/Compositor.hpp"
#include "io/Png.hpp"

namespace xeldi::io {

Spritesheet buildSpritesheet(const Document& doc, const SpritesheetOptions& opt) {
    int from = std::clamp(opt.from, 0, doc.frameCount() - 1);
    int to = opt.to < 0 ? doc.frameCount() - 1 : std::clamp(opt.to, from, doc.frameCount() - 1);
    int n = to - from + 1;
    int cols = opt.columns > 0 ? opt.columns : int(std::ceil(std::sqrt(double(n))));
    int rows = (n + cols - 1) / cols;
    int s = std::max(1, opt.scale);
    int fw = doc.width * s, fh = doc.height * s, pad = std::max(0, opt.padding);
    Spritesheet out;
    out.image = Image(cols * fw + (cols - 1) * pad, rows * fh + (rows - 1) * pad);
    nlohmann::json frames = nlohmann::json::array();
    for (int i = 0; i < n; ++i) {
        int f = from + i;
        Image img = composeFrame(doc, f);
        if (s > 1) img = img.scaledNearest(fw, fh);
        int x = (i % cols) * (fw + pad), y = (i / cols) * (fh + pad);
        out.image.paste(img, x, y);
        frames.push_back({{"frame", f}, {"x", x}, {"y", y}, {"w", fw}, {"h", fh},
                          {"duration_ms", doc.timeline.frames[f].durationMs}});
    }
    nlohmann::json tags = nlohmann::json::array();
    for (const auto& t : doc.timeline.tags)
        if (t.to >= from && t.from <= to)
            tags.push_back({{"name", t.name}, {"from", std::max(t.from, from) - from},
                            {"to", std::min(t.to, to) - from}, {"loop_mode", toString(t.loop)}});
    nlohmann::json meta = {{"app", "Xeldi"},
                           {"version", XELDI_VERSION},
                           {"size", {{"w", out.image.width()}, {"h", out.image.height()}}},
                           {"frame_size", {{"w", fw}, {"h", fh}}},
                           {"frames", frames},
                           {"tags", tags}};
    out.json = meta.dump(2);
    return out;
}

bool exportSpritesheet(const Document& doc, const std::string& pngPath, const SpritesheetOptions& opt,
                       std::string* error) {
    Spritesheet sheet = buildSpritesheet(doc, opt);
    if (!writePngFile(sheet.image, pngPath, error)) return false;
    std::filesystem::path jp(pngPath);
    jp.replace_extension(".json");
    std::ofstream f(jp);
    if (!f) {
        if (error) *error = "no se pudo escribir " + jp.string();
        return false;
    }
    f << sheet.json;
    return true;
}

}  // namespace xeldi::io

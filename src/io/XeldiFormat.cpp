#include "io/XeldiFormat.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <map>
#include <tuple>

#include "engine2d/Compositor.hpp"
#include "io/Binary.hpp"
#include "io/Png.hpp"

namespace xeldi::io {

using nlohmann::json;

namespace {

json vec3(Vec3 v) { return json::array({v.x, v.y, v.z}); }
Vec3 toVec3(const json& j, Vec3 def = {}) {
    if (!j.is_array() || j.size() != 3) return def;
    return {j[0].get<float>(), j[1].get<float>(), j[2].get<float>()};
}
std::string hex(Rgba c) { return toHex(c); }
Rgba color(const json& j, Rgba def) {
    if (!j.is_string()) return def;
    auto c = fromHex(j.get<std::string>());
    return c ? *c : def;
}

struct ChunkInfo {
    std::string tag;
    std::uint64_t offset = 0, size = 0;
};

void writeChunk(ByteWriter& w, const char tag[4], const std::vector<std::uint8_t>& raw, bool compress, int level,
                std::vector<ChunkInfo>& index) {
    std::vector<std::uint8_t> stored = compress ? zlibCompress(raw.data(), raw.size(), level) : raw;
    // Si comprimir no ayuda, se guarda en crudo.
    Codec codec = compress ? Codec::Zlib : Codec::Raw;
    if (compress && stored.size() >= raw.size()) {
        stored = raw;
        codec = Codec::Raw;
    }
    std::size_t start = w.size();
    w.tag(tag);
    w.u32(std::uint32_t(codec));
    w.u64(raw.size());
    w.u64(stored.size());
    w.u32(crc32(stored.data(), stored.size()));
    w.bytes(stored);
    index.push_back({std::string(tag, 4), start, w.size() - start});
}

std::vector<std::uint8_t> readChunk(ByteReader& r, const std::string& expectTag) {
    std::string tag = r.tag();
    if (tag != expectTag) throw FormatError("se esperaba el bloque " + expectTag + " y hay " + tag);
    auto codec = Codec(r.u32());
    std::uint64_t rawSize = r.u64(), storedSize = r.u64();
    std::uint32_t crc = r.u32();
    if (storedSize > r.remaining()) throw FormatError("bloque " + tag + " truncado");
    const std::uint8_t* p = r.ptr();
    if (crc32(p, storedSize) != crc) throw FormatError("CRC incorrecto en el bloque " + tag);
    r.seek(r.pos() + storedSize);
    if (codec == Codec::Raw) return std::vector<std::uint8_t>(p, p + storedSize);
    if (codec == Codec::Zlib) {
        if (rawSize > (std::uint64_t(1) << 33)) throw FormatError("bloque demasiado grande");
        return zlibDecompress(p, storedSize, std::size_t(rawSize));
    }
    throw FormatError("codec no soportado en el bloque " + tag);
}

json metaFor(const Document& d) {
    json j;
    json pal = json::array();
    for (Rgba c : d.palette.colors) pal.push_back(hex(c));
    j["project"] = {{"name", d.name}, {"unit_scale", d.unitScale}, {"palette_name", d.palette.name}, {"palette", pal},
                    {"generator", std::string("Xeldi ") + XELDI_VERSION}};
    j["canvas_2d"] = {{"width", d.width}, {"height", d.height}, {"pixel_format", "RGBA8"}};
    json frames = json::array();
    for (int i = 0; i < d.frameCount(); ++i)
        frames.push_back({{"index", i}, {"duration_ms", d.timeline.frames[i].durationMs}});
    json tags = json::array();
    for (const auto& t : d.timeline.tags)
        tags.push_back({{"name", t.name}, {"from", t.from}, {"to", t.to}, {"loop_mode", toString(t.loop)},
                        {"fps_override", t.fpsOverride}, {"auto_preview", t.autoPreview}, {"color", hex(t.color)}});
    j["timeline"] = {{"fps", d.timeline.fps}, {"frame_count", d.frameCount()}, {"frames", frames}, {"tags", tags}};
    const auto& o = d.onion;
    j["onion_skin"] = {{"enabled", o.enabled},          {"prev_frames", o.prevFrames},
                       {"next_frames", o.nextFrames},   {"prev_tint", hex(o.prevTint)},
                       {"next_tint", hex(o.nextTint)},  {"base_opacity", o.baseOpacity},
                       {"falloff", o.falloff},          {"only_current_layer", o.onlyCurrentLayer},
                       {"loop_within_tag", o.loopWithinTag}};
    json layers = json::array();
    for (const auto& l : d.layers)
        layers.push_back({{"id", l.id},
                          {"name", l.name},
                          {"type", toString(l.type)},
                          {"visible", l.visible},
                          {"locked", l.locked},
                          {"opacity", l.opacity},
                          {"blend_mode", toString(l.blend)},
                          {"parent_id", l.parentId.empty() ? json(nullptr) : json(l.parentId)}});
    j["layers"] = layers;
    const auto& cam = d.scene.camera;
    json planes = json::array();
    for (const auto& p : d.scene.workplanes)
        planes.push_back({{"id", p.id},
                          {"name", p.name},
                          {"origin", vec3(p.origin)},
                          {"normal", vec3(p.normal)},
                          {"right", vec3(p.right)},
                          {"up", vec3(p.up)},
                          {"width_px", p.widthPx},
                          {"height_px", p.heightPx},
                          {"snap_to_grid", p.snapToGrid},
                          {"visible", p.visible}});
    j["scene_3d"] = {{"chunk_size", {kChunkSize, kChunkSize, kChunkSize}},
                     {"camera",
                      {{"position", vec3(cam.position())},
                       {"target", vec3(cam.target)},
                       {"yaw_deg", cam.yawDeg},
                       {"pitch_deg", cam.pitchDeg},
                       {"distance", cam.distance},
                       {"fov_deg", cam.fovDeg},
                       {"ortho", cam.ortho}}},
                     {"workplanes", planes},
                     {"active_workplane", d.scene.activeWorkplane}};
    return j;
}

void applyMeta(const json& j, Document& d, std::vector<std::string>& warnings) {
    const json& pr = j.at("project");
    d.name = pr.value("name", std::string("Sin titulo"));
    d.unitScale = pr.value("unit_scale", 1.0f);
    d.palette.name = pr.value("palette_name", std::string("Paleta"));
    d.palette.colors.clear();
    if (pr.contains("palette"))
        for (const auto& c : pr["palette"]) d.palette.colors.push_back(color(c, 0));
    const json& cv = j.at("canvas_2d");
    d.width = cv.at("width").get<int>();
    d.height = cv.at("height").get<int>();
    if (d.width <= 0 || d.height <= 0 || d.width > 8192 || d.height > 8192)
        throw FormatError("tamano de lienzo invalido");
    if (cv.value("pixel_format", std::string("RGBA8")) != "RGBA8") throw FormatError("formato de pixel no soportado");

    const json& tl = j.at("timeline");
    d.timeline.fps = tl.value("fps", 12);
    d.timeline.frames.clear();
    int frameCount = tl.value("frame_count", 1);
    if (frameCount < 1 || frameCount > 100000) throw FormatError("numero de frames invalido");
    d.timeline.frames.assign(std::size_t(frameCount), Frame{});
    if (tl.contains("frames"))
        for (const auto& f : tl["frames"]) {
            int i = f.value("index", -1);
            if (i >= 0 && i < frameCount) d.timeline.frames[i].durationMs = f.value("duration_ms", 100);
        }
    d.timeline.tags.clear();
    if (tl.contains("tags"))
        for (const auto& t : tl["tags"]) {
            AnimationTag tag;
            tag.name = t.value("name", std::string("tag"));
            tag.from = std::clamp(t.value("from", 0), 0, frameCount - 1);
            tag.to = std::clamp(t.value("to", 0), tag.from, frameCount - 1);
            tag.loop = loopModeFromString(t.value("loop_mode", std::string("forward")));
            tag.fpsOverride = t.value("fps_override", 0);
            tag.autoPreview = t.value("auto_preview", false);
            tag.color = color(t.value("color", json()), tag.color);
            d.timeline.tags.push_back(tag);
        }
    if (j.contains("onion_skin")) {
        const json& o = j["onion_skin"];
        auto& s = d.onion;
        s.enabled = o.value("enabled", s.enabled);
        s.prevFrames = o.value("prev_frames", s.prevFrames);
        s.nextFrames = o.value("next_frames", s.nextFrames);
        s.prevTint = color(o.value("prev_tint", json()), s.prevTint);
        s.nextTint = color(o.value("next_tint", json()), s.nextTint);
        s.baseOpacity = o.value("base_opacity", s.baseOpacity);
        s.falloff = o.value("falloff", s.falloff);
        s.onlyCurrentLayer = o.value("only_current_layer", s.onlyCurrentLayer);
        s.loopWithinTag = o.value("loop_within_tag", s.loopWithinTag);
    }
    d.layers.clear();
    for (const auto& l : j.at("layers")) {
        Layer layer;
        layer.id = l.at("id").get<std::string>();
        layer.name = l.value("name", layer.id);
        layer.type = layerTypeFromString(l.value("type", std::string("raster2d")));
        layer.visible = l.value("visible", true);
        layer.locked = l.value("locked", false);
        layer.opacity = l.value("opacity", 1.0f);
        layer.blend = blendModeFromString(l.value("blend_mode", std::string("normal")));
        if (l.contains("parent_id") && l["parent_id"].is_string()) layer.parentId = l["parent_id"].get<std::string>();
        layer.cels.resize(std::size_t(frameCount));
        d.layers.push_back(std::move(layer));
    }
    if (j.contains("scene_3d")) {
        const json& s = j["scene_3d"];
        if (s.contains("chunk_size") && s["chunk_size"].is_array() && s["chunk_size"][0].get<int>() != kChunkSize)
            warnings.push_back("chunk_size distinto de 16: se reempaqueta al cargar");
        if (s.contains("camera")) {
            const json& c = s["camera"];
            auto& cam = d.scene.camera;
            cam.target = toVec3(c.value("target", json()), cam.target);
            cam.fovDeg = c.value("fov_deg", cam.fovDeg);
            cam.ortho = c.value("ortho", cam.ortho);
            if (c.contains("yaw_deg")) {
                cam.yawDeg = c.value("yaw_deg", cam.yawDeg);
                cam.pitchDeg = c.value("pitch_deg", cam.pitchDeg);
                cam.distance = c.value("distance", cam.distance);
            } else if (c.contains("position")) {  // archivos que solo traen posicion
                Vec3 off = toVec3(c["position"]) - cam.target;
                cam.distance = std::max(1.0f, length(off));
                cam.pitchDeg = std::asin(std::clamp(off.y / cam.distance, -1.0f, 1.0f)) * 180.0f / kPi;
                cam.yawDeg = std::atan2(off.x, off.z) * 180.0f / kPi;
            }
        }
        d.scene.workplanes.clear();
        if (s.contains("workplanes"))
            for (const auto& p : s["workplanes"]) {
                Workplane w;
                w.id = p.value("id", d.newId("plane"));
                w.name = p.value("name", w.id);
                w.origin = toVec3(p.value("origin", json()), w.origin);
                w.normal = toVec3(p.value("normal", json()), w.normal);
                w.right = toVec3(p.value("right", json()), w.right);
                w.up = toVec3(p.value("up", json()), w.up);
                w.widthPx = std::clamp(p.value("width_px", 32), 1, 4096);
                w.heightPx = std::clamp(p.value("height_px", 32), 1, 4096);
                w.snapToGrid = p.value("snap_to_grid", true);
                w.visible = p.value("visible", true);
                w.ensurePixels();
                d.scene.workplanes.push_back(std::move(w));
            }
        d.scene.activeWorkplane = s.value("active_workplane", 0);
    }
}

}  // namespace

Image makeThumbnail(const Document& doc, int maxSize) {
    Image frame = composeFrame(doc, 0);
    int scale = std::max(1, maxSize / std::max(doc.width, doc.height));
    int w = doc.width * scale, h = doc.height * scale;
    if (w > maxSize || h > maxSize) {
        float f = float(maxSize) / float(std::max(doc.width, doc.height));
        w = std::max(1, int(doc.width * f));
        h = std::max(1, int(doc.height * f));
    }
    return frame.scaledNearest(w, h);
}

std::vector<std::uint8_t> serializeDocument(const Document& doc, const SaveOptions& opt) {
    std::string err;
    if (!doc.validate(&err)) throw FormatError("documento invalido: " + err);
    ByteWriter w;
    std::vector<ChunkInfo> index;
    // Cabecera (se completa al final).
    w.tag(kMagic);
    w.u16(kVersionMajor);
    w.u16(kVersionMinor);
    std::uint32_t flags = FlagCompressed;
    if (!doc.scene.voxels.empty() || !doc.scene.workplanes.empty()) flags |= FlagHas3D;
    if (opt.thumbnail) flags |= FlagHasThumbnail;
    w.u32(flags);
    w.u64(0);  // index_offset
    w.u64(0);  // index_size
    w.u32(0);  // crc32 de la cabecera
    const int L = opt.compressionLevel;

    // META
    std::string meta = metaFor(doc).dump(2);
    writeChunk(w, "META", std::vector<std::uint8_t>(meta.begin(), meta.end()), true, L, index);

    // C2D_: u32 cuenta; por cel: u32 capa, u32 frame, u32 flags(0), RGBA8
    {
        ByteWriter c;
        std::uint32_t count = 0;
        c.u32(0);
        for (int l = 0; l < int(doc.layers.size()); ++l)
            for (int f = 0; f < doc.frameCount(); ++f) {
                const Image* img = doc.cel(l, f);
                if (!img || img->isFullyTransparent()) continue;  // celdas vacias se omiten
                c.u32(std::uint32_t(l));
                c.u32(std::uint32_t(f));
                c.u32(0);
                c.bytes(img->toRgba8());
                ++count;
            }
        c.patchU32(0, count);
        writeChunk(w, "C2D_", c.data(), true, L, index);
    }
    // WPLN: bitmaps de planos en el orden de META
    {
        ByteWriter c;
        c.u32(std::uint32_t(doc.scene.workplanes.size()));
        for (const auto& p : doc.scene.workplanes) {
            Image px = p.pixels;
            if (px.width() != p.widthPx || px.height() != p.heightPx) {
                Image n(p.widthPx, p.heightPx);
                n.paste(px, 0, 0);
                px = n;
            }
            c.u32(std::uint32_t(p.widthPx));
            c.u32(std::uint32_t(p.heightPx));
            c.bytes(px.toRgba8());
        }
        writeChunk(w, "WPLN", c.data(), true, L, index);
    }
    // VX3D: chunks dispersos. Por chunk: i32 cx,cy,cz, i32 frame(-1), u8 modo, ...
    //   modo 0 (disperso): u16 n; n x (u8 lx, u8 ly, u8 lz, u32 rgba, u8 flags)
    //   modo 1 (denso):    4096 x (u32 rgba, u8 flags)
    {
        ByteWriter c;
        c.u32(std::uint32_t(doc.scene.voxels.chunkCount()));
        // Orden estable para que el archivo sea reproducible.
        std::vector<IVec3> keys;
        for (const auto& [k, ch] : doc.scene.voxels.chunks()) keys.push_back(k);
        std::sort(keys.begin(), keys.end(), [](IVec3 a, IVec3 b) {
            return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
        });
        for (const IVec3& k : keys) {
            const VoxelChunk& ch = *doc.scene.voxels.chunks().at(k);
            c.i32(k.x);
            c.i32(k.y);
            c.i32(k.z);
            c.i32(-1);
            bool dense = ch.count > kChunkVolume / 6;
            c.u8(dense ? 1 : 0);
            if (dense) {
                for (const Voxel& v : ch.voxels) {
                    c.u32(v.rgba);
                    c.u8(v.flags);
                }
            } else {
                c.u16(std::uint16_t(ch.count));
                for (int z = 0; z < kChunkSize; ++z)
                    for (int y = 0; y < kChunkSize; ++y)
                        for (int x = 0; x < kChunkSize; ++x) {
                            const Voxel& v = ch.voxels[VoxelChunk::index(x, y, z)];
                            if (v.empty()) continue;
                            c.u8(std::uint8_t(x));
                            c.u8(std::uint8_t(y));
                            c.u8(std::uint8_t(z));
                            c.u32(v.rgba);
                            c.u8(v.flags);
                        }
            }
        }
        writeChunk(w, "VX3D", c.data(), true, L, index);
    }
    if (opt.thumbnail) writeChunk(w, "THMB", encodePng(makeThumbnail(doc)), false, L, index);

    // IDX_: u32 n; n x (tag[4], u64 offset, u64 size)
    std::size_t idxOffset = w.size();
    {
        ByteWriter c;
        c.u32(std::uint32_t(index.size()));
        for (const auto& e : index) {
            c.tag(e.tag.c_str());
            c.u64(e.offset);
            c.u64(e.size);
        }
        std::vector<ChunkInfo> dummy;
        writeChunk(w, "IDX_", c.data(), false, L, dummy);
    }
    w.patchU64(12, idxOffset);
    w.patchU64(20, w.size() - idxOffset);
    w.patchU32(28, crc32(w.data().data(), 28));
    return std::move(w.data());
}

LoadResult deserializeDocument(const std::uint8_t* data, std::size_t size) {
    LoadResult res;
    try {
        ByteReader r(data, size);
        if (size < 32 || r.tag() != std::string(kMagic, 4)) throw FormatError("no es un archivo .xeldi");
        res.versionMajor = r.u16();
        res.versionMinor = r.u16();
        if (res.versionMajor != kVersionMajor)
            throw FormatError("version de formato " + std::to_string(res.versionMajor) + " no soportada");
        r.u32();  // flags
        std::uint64_t idxOffset = r.u64(), idxSize = r.u64();
        std::uint32_t headerCrc = r.u32();
        if (crc32(data, 28) != headerCrc) throw FormatError("cabecera corrupta (CRC)");
        if (idxOffset + idxSize > size) throw FormatError("indice fuera del archivo");

        // Indice -> posiciones de cada bloque.
        r.seek(std::size_t(idxOffset));
        auto idxData = readChunk(r, "IDX_");
        ByteReader ir(idxData.data(), idxData.size());
        std::map<std::string, std::uint64_t> where;
        std::uint32_t n = ir.u32();
        for (std::uint32_t i = 0; i < n; ++i) {
            std::string tag = ir.tag();
            std::uint64_t off = ir.u64();
            ir.u64();
            where[tag] = off;
        }
        auto load = [&](const std::string& tag) -> std::optional<std::vector<std::uint8_t>> {
            auto it = where.find(tag);
            if (it == where.end()) return std::nullopt;
            r.seek(std::size_t(it->second));
            return readChunk(r, tag);
        };

        Document& d = res.document;
        auto meta = load("META");
        if (!meta) throw FormatError("falta el bloque META");
        json j = json::parse(meta->begin(), meta->end());
        applyMeta(j, d, res.warnings);

        if (auto c2d = load("C2D_")) {
            ByteReader cr(c2d->data(), c2d->size());
            std::uint32_t count = cr.u32();
            std::size_t bytes = std::size_t(d.width) * d.height * 4;
            for (std::uint32_t i = 0; i < count; ++i) {
                std::uint32_t l = cr.u32(), f = cr.u32();
                cr.u32();
                if (cr.remaining() < bytes) throw FormatError("bloque C2D_ truncado");
                if (l >= d.layers.size() || f >= std::uint32_t(d.frameCount())) {
                    res.warnings.push_back("cel fuera de rango ignorada");
                    cr.seek(cr.pos() + bytes);
                    continue;
                }
                d.layers[l].cels[f] = std::make_shared<Image>(Image::fromRgba8(d.width, d.height, cr.ptr()));
                cr.seek(cr.pos() + bytes);
            }
        }
        if (auto wp = load("WPLN")) {
            ByteReader pr(wp->data(), wp->size());
            std::uint32_t count = pr.u32();
            for (std::uint32_t i = 0; i < count; ++i) {
                int pw = int(pr.u32()), ph = int(pr.u32());
                std::size_t bytes = std::size_t(pw) * ph * 4;
                if (pw <= 0 || ph <= 0 || pr.remaining() < bytes) throw FormatError("bloque WPLN corrupto");
                if (i < d.scene.workplanes.size()) {
                    auto& p = d.scene.workplanes[i];
                    p.pixels = Image::fromRgba8(pw, ph, pr.ptr());
                    p.widthPx = pw;
                    p.heightPx = ph;
                }
                pr.seek(pr.pos() + bytes);
            }
        }
        if (auto vx = load("VX3D")) {
            ByteReader vr(vx->data(), vx->size());
            std::uint32_t count = vr.u32();
            for (std::uint32_t i = 0; i < count; ++i) {
                IVec3 cc{vr.i32(), vr.i32(), vr.i32()};
                vr.i32();  // frame (reservado, -1 = estatico)
                auto chunk = std::make_unique<VoxelChunk>();
                if (vr.u8() == 1) {
                    for (auto& v : chunk->voxels) {
                        v.rgba = vr.u32();
                        v.flags = vr.u8();
                    }
                } else {
                    std::uint16_t nv = vr.u16();
                    for (std::uint16_t k = 0; k < nv; ++k) {
                        int x = vr.u8(), y = vr.u8(), z = vr.u8();
                        Voxel v{vr.u32(), vr.u8()};
                        if (x >= kChunkSize || y >= kChunkSize || z >= kChunkSize)
                            throw FormatError("voxel fuera de su chunk");
                        chunk->voxels[VoxelChunk::index(x, y, z)] = v;
                    }
                }
                d.scene.voxels.setChunk(cc, std::move(chunk));
            }
        }
        if (d.layers.empty()) d.insertRasterLayer(0, "Capa 1");
        d.scene.activeWorkplane =
            std::clamp(d.scene.activeWorkplane, 0, std::max(0, int(d.scene.workplanes.size()) - 1));
        std::string err;
        if (!d.validate(&err)) throw FormatError(err);
        res.ok = true;
    } catch (const json::exception& e) {
        res.error = std::string("META invalido: ") + e.what();
    } catch (const std::exception& e) {
        res.error = e.what();
    }
    return res;
}

bool saveDocument(const Document& doc, const std::string& path, std::string* error, const SaveOptions& opt) {
    try {
        writeFileAtomic(path, serializeDocument(doc, opt));
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

LoadResult loadDocument(const std::string& path) {
    try {
        auto bytes = readFile(path);
        return deserializeDocument(bytes.data(), bytes.size());
    } catch (const std::exception& e) {
        LoadResult r;
        r.error = e.what();
        return r;
    }
}

std::optional<Image> readThumbnail(const std::string& path) {
    try {
        auto bytes = readFile(path);
        ByteReader r(bytes.data(), bytes.size());
        if (r.tag() != std::string(kMagic, 4)) return std::nullopt;
        r.seek(12);
        std::uint64_t idx = r.u64();
        r.seek(std::size_t(idx));
        auto idxData = readChunk(r, "IDX_");
        ByteReader ir(idxData.data(), idxData.size());
        std::uint32_t n = ir.u32();
        for (std::uint32_t i = 0; i < n; ++i) {
            std::string tag = ir.tag();
            std::uint64_t off = ir.u64();
            ir.u64();
            if (tag == "THMB") {
                r.seek(std::size_t(off));
                auto png = readChunk(r, "THMB");
                return decodePng(png.data(), png.size());
            }
        }
    } catch (...) {
    }
    return std::nullopt;
}

}  // namespace xeldi::io

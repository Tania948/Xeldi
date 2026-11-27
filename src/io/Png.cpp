#include "io/Png.hpp"

#include <cstdlib>
#include <cstring>

#include "io/Binary.hpp"

namespace xeldi::io {

namespace {
const std::uint8_t kSig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

void putBE32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(std::uint8_t(x >> 24));
    v.push_back(std::uint8_t(x >> 16));
    v.push_back(std::uint8_t(x >> 8));
    v.push_back(std::uint8_t(x));
}
std::uint32_t getBE32(const std::uint8_t* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3];
}
void chunk(std::vector<std::uint8_t>& out, const char* type, const std::vector<std::uint8_t>& data) {
    putBE32(out, std::uint32_t(data.size()));
    std::size_t start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    putBE32(out, crc32(out.data() + start, out.size() - start));
}
int paeth(int a, int b, int c) {
    int p = a + b - c, pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}
}  // namespace

std::vector<std::uint8_t> encodePng(const Image& img, int level) {
    std::vector<std::uint8_t> out(kSig, kSig + 8);
    std::vector<std::uint8_t> ihdr;
    putBE32(ihdr, std::uint32_t(img.width()));
    putBE32(ihdr, std::uint32_t(img.height()));
    ihdr.insert(ihdr.end(), {8, 6, 0, 0, 0});  // 8 bits, RGBA
    chunk(out, "IHDR", ihdr);
    std::vector<std::uint8_t> raw;
    raw.reserve(std::size_t(img.height()) * (img.width() * 4 + 1));
    auto rgba = img.toRgba8();
    const std::size_t stride = std::size_t(img.width()) * 4;
    for (int y = 0; y < img.height(); ++y) {
        // Filtro "Sub" (1): suele comprimir bien el pixel art.
        raw.push_back(1);
        const std::uint8_t* row = rgba.data() + y * stride;
        for (std::size_t i = 0; i < stride; ++i) raw.push_back(std::uint8_t(row[i] - (i >= 4 ? row[i - 4] : 0)));
    }
    chunk(out, "IDAT", zlibCompress(raw.data(), raw.size(), level));
    chunk(out, "IEND", {});
    return out;
}

std::optional<Image> decodePng(const std::uint8_t* data, std::size_t size, std::string* error) {
    auto fail = [&](const char* m) -> std::optional<Image> {
        if (error) *error = m;
        return std::nullopt;
    };
    if (size < 8 || std::memcmp(data, kSig, 8) != 0) return fail("no es un PNG");
    std::size_t pos = 8;
    std::uint32_t w = 0, h = 0;
    int depth = 0, ctype = 0, interlace = 0;
    std::vector<std::uint8_t> idat, plte, trns;
    while (pos + 12 <= size) {
        std::uint32_t len = getBE32(data + pos);
        if (len > size - pos - 12) return fail("chunk PNG truncado");
        const char* type = reinterpret_cast<const char*>(data + pos + 4);
        const std::uint8_t* body = data + pos + 8;
        if (!std::memcmp(type, "IHDR", 4) && len >= 13) {
            w = getBE32(body);
            h = getBE32(body + 4);
            depth = body[8];
            ctype = body[9];
            interlace = body[12];
        } else if (!std::memcmp(type, "PLTE", 4)) {
            plte.assign(body, body + len);
        } else if (!std::memcmp(type, "tRNS", 4)) {
            trns.assign(body, body + len);
        } else if (!std::memcmp(type, "IDAT", 4)) {
            idat.insert(idat.end(), body, body + len);
        } else if (!std::memcmp(type, "IEND", 4)) {
            break;
        }
        pos += 12 + len;
    }
    if (w == 0 || h == 0 || w > 16384 || h > 16384) return fail("dimensiones PNG invalidas");
    if (depth != 8 || interlace != 0) return fail("solo PNG de 8 bits sin entrelazado");
    int channels = 0;
    switch (ctype) {
        case 0: channels = 1; break;
        case 2: channels = 3; break;
        case 3: channels = 1; break;
        case 4: channels = 2; break;
        case 6: channels = 4; break;
        default: return fail("tipo de color PNG no soportado");
    }
    const std::size_t stride = std::size_t(w) * channels;
    std::vector<std::uint8_t> raw;
    try {
        raw = zlibDecompress(idat.data(), idat.size(), (stride + 1) * h);
    } catch (const FormatError&) {
        return fail("datos PNG corruptos");
    }
    std::vector<std::uint8_t> cur(stride), prev(stride, 0);
    Image img{int(w), int(h)};
    for (std::uint32_t y = 0; y < h; ++y) {
        const std::uint8_t* src = raw.data() + y * (stride + 1);
        int filter = src[0];
        ++src;
        for (std::size_t i = 0; i < stride; ++i) {
            int a = i >= std::size_t(channels) ? cur[i - channels] : 0;
            int b = prev[i];
            int c = i >= std::size_t(channels) ? prev[i - channels] : 0;
            int x = src[i];
            switch (filter) {
                case 0: break;
                case 1: x += a; break;
                case 2: x += b; break;
                case 3: x += (a + b) / 2; break;
                case 4: x += paeth(a, b, c); break;
                default: return fail("filtro PNG invalido");
            }
            cur[i] = std::uint8_t(x);
        }
        for (std::uint32_t xI = 0; xI < w; ++xI) {
            const std::uint8_t* p = cur.data() + std::size_t(xI) * channels;
            Rgba col = 0;
            switch (ctype) {
                case 0: col = makeRgba(p[0], p[0], p[0]); break;
                case 4: col = makeRgba(p[0], p[0], p[0], p[1]); break;
                case 2: col = makeRgba(p[0], p[1], p[2]); break;
                case 6: col = makeRgba(p[0], p[1], p[2], p[3]); break;
                case 3: {
                    std::size_t k = p[0];
                    if (k * 3 + 2 >= plte.size()) return fail("indice de paleta PNG invalido");
                    col = makeRgba(plte[k * 3], plte[k * 3 + 1], plte[k * 3 + 2], k < trns.size() ? trns[k] : 255);
                    break;
                }
            }
            img.setPixel(int(xI), int(y), col);
        }
        std::swap(cur, prev);
    }
    return img;
}

bool writePngFile(const Image& img, const std::string& path, std::string* error) {
    try {
        writeFileAtomic(path, encodePng(img));
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::optional<Image> readPngFile(const std::string& path, std::string* error) {
    try {
        auto bytes = readFile(path);
        return decodePng(bytes.data(), bytes.size(), error);
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

}  // namespace xeldi::io

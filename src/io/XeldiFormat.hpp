#pragma once
// Formato nativo de proyecto ".xeldi". Ver docs/FORMATO.md.
//
//   [Cabecera 32 B] [chunk META] [chunk C2D_] [chunk WPLN] [chunk VX3D] [chunk THMB] [chunk IDX_]
//
// Cada chunk: tag[4] | u32 codec | u64 tamano_original | u64 tamano_guardado | u32 crc32 | datos
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/Document.hpp"

namespace xeldi::io {

constexpr char kMagic[4] = {'X', 'E', 'L', 'D'};
constexpr std::uint16_t kVersionMajor = 1;
constexpr std::uint16_t kVersionMinor = 0;

enum FileFlags : std::uint32_t {
    FlagHas3D = 1u << 0,
    FlagCompressed = 1u << 1,
    FlagHasThumbnail = 1u << 2,
};

enum class Codec : std::uint32_t { Raw = 0, Zlib = 1 /* reservado: Zstd = 2 */ };

struct SaveOptions {
    bool thumbnail = true;
    int compressionLevel = 6;
};

struct LoadResult {
    bool ok = false;
    std::string error;
    Document document;
    std::uint16_t versionMajor = 0, versionMinor = 0;
    std::vector<std::string> warnings;
};

std::vector<std::uint8_t> serializeDocument(const Document& doc, const SaveOptions& opt = {});
LoadResult deserializeDocument(const std::uint8_t* data, std::size_t size);

bool saveDocument(const Document& doc, const std::string& path, std::string* error = nullptr,
                  const SaveOptions& opt = {});
LoadResult loadDocument(const std::string& path);
/// Lee solo la miniatura (para dialogos de abrir / gestores de archivos).
std::optional<Image> readThumbnail(const std::string& path);
/// Genera la miniatura (frame 0 compuesto, escalado al vecino mas cercano).
Image makeThumbnail(const Document& doc, int maxSize = 128);

}  // namespace xeldi::io

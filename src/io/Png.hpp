#pragma once
// Codificador/decodificador PNG minimo (zlib). RGBA8 sin entrelazado.
#include <optional>
#include <string>
#include <vector>

#include "core/Image.hpp"

namespace xeldi::io {

std::vector<std::uint8_t> encodePng(const Image& img, int compressionLevel = 6);
/// Soporta PNG de 8 bits: gris, gris+alfa, RGB, RGBA e indexado; sin entrelazado.
std::optional<Image> decodePng(const std::uint8_t* data, std::size_t size, std::string* error = nullptr);
bool writePngFile(const Image& img, const std::string& path, std::string* error = nullptr);
std::optional<Image> readPngFile(const std::string& path, std::string* error = nullptr);

}  // namespace xeldi::io

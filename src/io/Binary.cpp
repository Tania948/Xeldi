#include "io/Binary.hpp"

#include <zlib.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace xeldi::io {

void ByteWriter::u16(std::uint16_t v) {
    u8(std::uint8_t(v));
    u8(std::uint8_t(v >> 8));
}
void ByteWriter::u32(std::uint32_t v) {
    for (int i = 0; i < 4; ++i) u8(std::uint8_t(v >> (8 * i)));
}
void ByteWriter::u64(std::uint64_t v) {
    for (int i = 0; i < 8; ++i) u8(std::uint8_t(v >> (8 * i)));
}
void ByteWriter::bytes(const void* p, std::size_t n) {
    auto* b = static_cast<const std::uint8_t*>(p);
    buf_.insert(buf_.end(), b, b + n);
}
void ByteWriter::patchU32(std::size_t at, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) buf_.at(at + i) = std::uint8_t(v >> (8 * i));
}
void ByteWriter::patchU64(std::size_t at, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) buf_.at(at + i) = std::uint8_t(v >> (8 * i));
}

void ByteReader::need(std::size_t n) const {
    if (n > n_ - pos_) throw FormatError("archivo truncado o corrupto");
}
std::uint8_t ByteReader::u8() {
    need(1);
    return p_[pos_++];
}
std::uint16_t ByteReader::u16() {
    need(2);
    std::uint16_t v = std::uint16_t(p_[pos_] | (p_[pos_ + 1] << 8));
    pos_ += 2;
    return v;
}
std::uint32_t ByteReader::u32() {
    need(4);
    std::uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= std::uint32_t(p_[pos_ + i]) << (8 * i);
    pos_ += 4;
    return v;
}
std::uint64_t ByteReader::u64() {
    need(8);
    std::uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= std::uint64_t(p_[pos_ + i]) << (8 * i);
    pos_ += 8;
    return v;
}
void ByteReader::bytes(void* out, std::size_t n) {
    need(n);
    std::memcpy(out, p_ + pos_, n);
    pos_ += n;
}
std::string ByteReader::tag() {
    char t[4];
    bytes(t, 4);
    return std::string(t, 4);
}
void ByteReader::seek(std::size_t pos) {
    if (pos > n_) throw FormatError("desplazamiento fuera del archivo");
    pos_ = pos;
}

std::uint32_t crc32(const void* data, std::size_t n, std::uint32_t seed) {
    return std::uint32_t(::crc32(seed, static_cast<const Bytef*>(data), uInt(n)));
}

std::vector<std::uint8_t> zlibCompress(const std::uint8_t* data, std::size_t n, int level) {
    uLongf cap = compressBound(uLong(n));
    std::vector<std::uint8_t> out(cap);
    if (compress2(out.data(), &cap, data, uLong(n), level) != Z_OK) throw FormatError("error de compresion zlib");
    out.resize(cap);
    return out;
}

std::vector<std::uint8_t> zlibDecompress(const std::uint8_t* data, std::size_t n, std::size_t expected) {
    std::vector<std::uint8_t> out(expected);
    uLongf len = uLongf(expected);
    int r = uncompress(out.data(), &len, data, uLong(n));
    if (r != Z_OK || len != expected) throw FormatError("datos comprimidos corruptos");
    return out;
}

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) throw FormatError("no se pudo abrir " + path);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(f), {});
}

void writeFileAtomic(const std::string& path, const std::vector<std::uint8_t>& data) {
    const std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) throw FormatError("no se pudo escribir " + tmp);
        f.write(reinterpret_cast<const char*>(data.data()), std::streamsize(data.size()));
        f.flush();
        if (!f) throw FormatError("escritura incompleta en " + tmp);
    }
#if defined(__unix__) || defined(__APPLE__)
    int fd = ::open(tmp.c_str(), O_RDONLY);
    if (fd >= 0) {
        ::fsync(fd);
        ::close(fd);
    }
#endif
    std::error_code ec;
    if (std::filesystem::file_size(tmp, ec) != data.size() || ec) throw FormatError("verificacion fallida de " + tmp);
    std::filesystem::rename(tmp, path, ec);
    if (ec) throw FormatError("no se pudo reemplazar " + path + ": " + ec.message());
}

}  // namespace xeldi::io

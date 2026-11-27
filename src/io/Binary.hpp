#pragma once
// Lectura/escritura binaria little-endian y compresion zlib.
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace xeldi::io {

struct FormatError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class ByteWriter {
public:
    void u8(std::uint8_t v) { buf_.push_back(v); }
    void u16(std::uint16_t v);
    void u32(std::uint32_t v);
    void u64(std::uint64_t v);
    void i32(std::int32_t v) { u32(std::uint32_t(v)); }
    void bytes(const void* p, std::size_t n);
    void bytes(const std::vector<std::uint8_t>& v) { bytes(v.data(), v.size()); }
    void tag(const char t[4]) { bytes(t, 4); }
    void patchU32(std::size_t at, std::uint32_t v);
    void patchU64(std::size_t at, std::uint64_t v);
    std::size_t size() const { return buf_.size(); }
    std::vector<std::uint8_t>& data() { return buf_; }

private:
    std::vector<std::uint8_t> buf_;
};

class ByteReader {
public:
    ByteReader(const std::uint8_t* p, std::size_t n) : p_(p), n_(n) {}
    std::uint8_t u8();
    std::uint16_t u16();
    std::uint32_t u32();
    std::uint64_t u64();
    std::int32_t i32() { return std::int32_t(u32()); }
    void bytes(void* out, std::size_t n);
    std::string tag();
    void seek(std::size_t pos);
    std::size_t pos() const { return pos_; }
    std::size_t size() const { return n_; }
    std::size_t remaining() const { return n_ - pos_; }
    const std::uint8_t* ptr() const { return p_ + pos_; }

private:
    void need(std::size_t n) const;
    const std::uint8_t* p_;
    std::size_t n_, pos_ = 0;
};

std::uint32_t crc32(const void* data, std::size_t n, std::uint32_t seed = 0);
std::vector<std::uint8_t> zlibCompress(const std::uint8_t* data, std::size_t n, int level = 6);
std::vector<std::uint8_t> zlibDecompress(const std::uint8_t* data, std::size_t n, std::size_t expectedSize);

std::vector<std::uint8_t> readFile(const std::string& path);
/// Escritura atomica: escribe en path.tmp, sincroniza y renombra.
void writeFileAtomic(const std::string& path, const std::vector<std::uint8_t>& data);

}  // namespace xeldi::io

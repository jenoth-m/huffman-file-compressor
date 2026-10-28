#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace huffman {

using Byte = std::uint8_t;
using Bytes = std::vector<Byte>;

// A file is kept in memory while it is processed.
constexpr std::uint64_t kMaxFileSize = 64ULL * 1024 * 1024;

// Four signature bytes, followed by 256 four-byte frequency counts.
constexpr std::size_t kHeaderSize = 4 + 256 * 4;
constexpr std::uint64_t kMaxArchiveSize = kMaxFileSize + kHeaderSize;

// Return a new byte buffer. Invalid input is reported with std::runtime_error.
Bytes compress(const Bytes& input);
Bytes decompress(const Bytes& archive);

}  // namespace huffman

#endif

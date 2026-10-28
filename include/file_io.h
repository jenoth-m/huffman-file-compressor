#ifndef FILE_IO_H
#define FILE_IO_H

#include "huffman.h"

#include <filesystem>

namespace huffman {

Bytes read_file(const std::filesystem::path& path, std::uint64_t size_limit);

// Exclusively creates a new file. Existing files, including aliases of the input,
// are never opened for writing. A failed write attempts to remove the partial file.
void write_new_file(const std::filesystem::path& path, const Bytes& bytes);

}  // namespace huffman

#endif

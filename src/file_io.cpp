#include "file_io.h"

#include <cerrno>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace huffman {

Bytes read_file(const std::filesystem::path& path, std::uint64_t size_limit) {
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Input must be an existing regular file: " + path.string());
    }

    std::ifstream file(path, std::ios::binary | std::ios::ate);

    if (!file) {
        throw std::runtime_error("Could not open input: " + path.string());
    }

    const auto end = file.tellg();

    if (end < 0 || static_cast<std::uint64_t>(end) > size_limit) {
        throw std::runtime_error("Input is too large or its size could not be read.");
    }

    Bytes bytes(static_cast<std::size_t>(end));
    file.seekg(0);

    if (!bytes.empty()) {
        file.read(reinterpret_cast<char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    }

    if (!file) {
        throw std::runtime_error("Input read failed or the file changed during reading.");
    }

    if (file.peek() != std::char_traits<char>::eof() || file.bad()) {
        throw std::runtime_error("Input grew or an I/O error occurred during reading.");
    }

    return bytes;
}

void write_new_file(const std::filesystem::path& path, const Bytes& bytes) {
    // The x mode requests exclusive creation, avoiding an exists-then-open race.
    // Windows uses wide paths; the codec itself remains platform-independent.
#ifdef _WIN32
    std::FILE* file = _wfopen(path.c_str(), L"wbx");
#else
    std::FILE* file = std::fopen(path.c_str(), "wbx");
#endif

    if (file == nullptr) {
        if (errno == EEXIST) {
            throw std::runtime_error("Output already exists; choose a new filename: " + path.string());
        }

        throw std::runtime_error("Could not create output: " + path.string());
    }

    std::size_t written = 0;

    if (!bytes.empty()) {
        written = std::fwrite(bytes.data(), 1, bytes.size(), file);
    }

    const bool closed = std::fclose(file) == 0;

    if (written != bytes.size() || !closed) {
        std::error_code ignored;
        std::filesystem::remove(path, ignored);
        throw std::runtime_error("Output write failed; an incomplete file was removed if possible.");
    }
}

}  // namespace huffman

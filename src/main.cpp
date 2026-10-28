#include "file_io.h"
#include "huffman.h"

#include <iostream>
#include <stdexcept>
#include <string>

void print_help() {
    std::cout
        << "Huffman File Compressor\n\n"
        << "Usage:\n"
        << "  huff compress   <input> <archive.hfc>\n"
        << "  huff decompress <archive.hfc> <output>\n\n"
        << "Choose a new output filename; existing files are not overwritten.\n"
        << "Use quotes around paths containing spaces.\n";
}

int main(int argc, char* argv[]) {
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) {
        print_help();
        return 0;
    }

    const std::string command = argv[1];

    if (argc != 4 || (command != "compress" && command != "decompress")) {
        print_help();
        return 2;
    }

    try {
        const bool compressing = command == "compress";
        const auto limit = compressing ? huffman::kMaxFileSize
                                       : huffman::kMaxArchiveSize;

        const auto input = huffman::read_file(argv[2], limit);
        const auto output = compressing ? huffman::compress(input)
                                        : huffman::decompress(input);

        // Finish processing before creating the output file.
        huffman::write_new_file(argv[3], output);

        std::cout << (compressing ? "Compressed: " : "Restored: ")
                  << argv[3] << '\n';
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

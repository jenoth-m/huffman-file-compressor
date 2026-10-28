#include "huffman.h"

#include <iostream>
#include <stdexcept>
#include <string>

using huffman::Byte;
using huffman::Bytes;

void check(bool condition, const std::string& message) {
    // Unlike assert(), this also checks results in Release builds.
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void round_trip(const Bytes& input, const std::string& name) {
    const Bytes archive = huffman::compress(input);
    const Bytes restored = huffman::decompress(archive);

    check(restored == input, name + ": recovered bytes differ");
    std::cout << "PASS: " << name << '\n';
}

void expect_rejected(const Bytes& archive, const std::string& name) {
    bool rejected = false;

    try {
        huffman::decompress(archive);
    }
    catch (const std::runtime_error&) {
        rejected = true;
    }

    check(rejected, name + ": invalid archive was accepted");
    std::cout << "PASS: " << name << '\n';
}

int main() {
    try {
        round_trip({}, "empty input");
        round_trip({'Z'}, "one byte");
        round_trip(Bytes(4097, 'Z'), "repeated byte and partial final byte");

        const std::string text = "Huffman coding keeps frequent bytes short.\n"
                                 "Repeated letters receive shorter codes.\n";
        round_trip(Bytes(text.begin(), text.end()), "text and unequal frequencies");

        Bytes binary;

        for (int value = 0; value < 256; ++value) {
            binary.push_back(static_cast<Byte>(value));
        }

        round_trip(binary, "all 256 byte values");

        // Independent example: four equal counts give A=00, B=01, C=10, D=11.
        // ABCD therefore packs into 00011011, or 0x1B.
        Bytes expected(huffman::kHeaderSize, 0);
        expected[0] = 'H';
        expected[1] = 'F';
        expected[2] = 'C';
        expected[3] = '2';

        for (Byte symbol : Bytes{'A', 'B', 'C', 'D'}) {
            expected[4 + 4 * symbol] = 1;
        }

        expected.push_back(0x1B);

        const Bytes letters = {'A', 'B', 'C', 'D'};
        check(huffman::compress(letters) == expected, "Known encoding differs");
        check(huffman::decompress(expected) == letters, "Known decoding differs");
        std::cout << "PASS: known header, tie-breaking, and packed bits\n";

        Bytes bad_magic = expected;
        bad_magic[0] = 'X';
        expect_rejected(bad_magic, "incorrect signature");
        expect_rejected(Bytes(10, 0), "truncated header");

        Bytes truncated = expected;
        truncated.pop_back();
        expect_rejected(truncated, "truncated payload");

        Bytes trailing = expected;
        trailing.push_back(0);
        expect_rejected(trailing, "extra payload byte");

        Bytes oversized = expected;
        oversized[4 + 4 * 'A' + 3] = 0xFF;
        expect_rejected(oversized, "decoded size limit");

        Bytes bad_padding = huffman::compress(Bytes(9, 'Z'));
        bad_padding.back() |= 1;
        expect_rejected(bad_padding, "nonzero padding");

        std::cout << "\nAll directed tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}

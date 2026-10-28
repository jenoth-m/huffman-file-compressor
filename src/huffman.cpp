#include "huffman.h"

#include <algorithm>
#include <array>
#include <queue>
#include <stdexcept>

namespace huffman {
namespace {

using Frequencies = std::array<std::uint32_t, 256>;
using Code = std::vector<bool>;
using CodeTable = std::array<Code, 256>;

constexpr std::array<Byte, 4> kMagic = {'H', 'F', 'C', '2'};

struct Node {
    std::uint64_t frequency;
    int smallest_symbol;
    int symbol;                 // 0..255 for a leaf; -1 for a parent.
    int left = -1;
    int right = -1;
};

struct Tree {
    std::vector<Node> nodes;
    int root = -1;              // An empty input has no root.
};

// Store each count low byte first, regardless of the computer's byte order.
void append_u32(Bytes& output, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) {
        output.push_back(static_cast<Byte>((value >> (i * 8)) & 0xFF));
    }
}

// The caller checks that the complete fixed-size header exists first.
std::uint32_t read_u32(const Bytes& input, std::size_t& position) {
    std::uint32_t value = 0;

    for (unsigned i = 0; i < 4; ++i) {
        value |= static_cast<std::uint32_t>(input[position++]) << (i * 8);
    }

    return value;
}

// Fill each payload byte from bit 7 down to bit 0. Unused bits stay zero.
void write_bit(Bytes& output, bool bit, unsigned& used_bits) {
    if (used_bits == 0) {
        output.push_back(0);
    }

    if (bit) {
        output.back() |= static_cast<Byte>(1U << (7 - used_bits));
    }

    used_bits = (used_bits + 1) % 8;
}

bool read_bit(const Bytes& archive, std::uint64_t position) {
    const auto index = kHeaderSize + static_cast<std::size_t>(position / 8);
    const auto shift = 7U - static_cast<unsigned>(position % 8);

    return ((archive[index] >> shift) & 1U) != 0;
}

// priority_queue normally returns the largest item. Reversing the comparison
// puts the lowest frequency first. The queue stores indexes into tree.nodes.
struct CompareNodes {
    const std::vector<Node>* nodes;

    bool operator()(int first, int second) const {
        const Node& a = (*nodes)[static_cast<std::size_t>(first)];
        const Node& b = (*nodes)[static_cast<std::size_t>(second)];

        if (a.frequency != b.frequency) {
            return a.frequency > b.frequency;
        }

        // Equal frequencies need a fixed rule so decoding rebuilds the same tree.
        return a.smallest_symbol > b.smallest_symbol;
    }
};

Tree build_tree(const Frequencies& frequencies) {
    Tree tree;
    tree.nodes.reserve(511);    // Up to 256 leaves and 255 parent nodes.

    CompareNodes compare{&tree.nodes};
    std::priority_queue<int, std::vector<int>, CompareNodes> queue(compare);

    for (int symbol = 0; symbol < 256; ++symbol) {
        const auto frequency = frequencies[static_cast<std::size_t>(symbol)];

        if (frequency != 0) {
            tree.nodes.push_back({frequency, symbol, symbol});
            queue.push(static_cast<int>(tree.nodes.size() - 1));
        }
    }

    // Join the two lightest subtrees until one root remains.
    while (queue.size() > 1) {
        const int left = queue.top();
        queue.pop();

        const int right = queue.top();
        queue.pop();

        const Node& a = tree.nodes[static_cast<std::size_t>(left)];
        const Node& b = tree.nodes[static_cast<std::size_t>(right)];

        tree.nodes.push_back({a.frequency + b.frequency,
                              std::min(a.smallest_symbol, b.smallest_symbol),
                              -1, left, right});
        queue.push(static_cast<int>(tree.nodes.size() - 1));
    }

    if (!queue.empty()) {
        tree.root = queue.top();
    }

    return tree;
}

void assign_codes(const Tree& tree, int index, Code& path, CodeTable& codes) {
    const Node& node = tree.nodes[static_cast<std::size_t>(index)];

    if (node.symbol >= 0) {
        // A one-symbol file uses code 0, even though its tree has no branches.
        codes[static_cast<std::size_t>(node.symbol)] = path.empty() ? Code{false} : path;
        return;
    }

    path.push_back(false);
    assign_codes(tree, node.left, path, codes);

    path.back() = true;
    assign_codes(tree, node.right, path, codes);
    path.pop_back();
}

CodeTable make_codes(const Tree& tree) {
    CodeTable codes;
    Code path;

    if (tree.root >= 0) {
        assign_codes(tree, tree.root, path, codes);
    }

    return codes;
}

std::uint64_t count_bits(const Frequencies& frequencies, const CodeTable& codes) {
    std::uint64_t bits = 0;

    for (std::size_t symbol = 0; symbol < frequencies.size(); ++symbol) {
        bits += static_cast<std::uint64_t>(frequencies[symbol]) * codes[symbol].size();
    }

    return bits;
}

}  // namespace

Bytes compress(const Bytes& input) {
    if (input.size() > kMaxFileSize) {
        throw std::runtime_error("Input exceeds the 64 MiB file limit.");
    }

    Frequencies frequencies{};

    for (Byte byte : input) {
        ++frequencies[byte];
    }

    const Tree tree = build_tree(frequencies);
    const CodeTable codes = make_codes(tree);
    const auto payload_bytes = (count_bits(frequencies, codes) + 7) / 8;

    Bytes archive;
    archive.reserve(kHeaderSize + static_cast<std::size_t>(payload_bytes));

    for (Byte byte : kMagic) {
        archive.push_back(byte);
    }

    // Every possible byte gets one count, including bytes that never appeared.
    for (auto frequency : frequencies) {
        append_u32(archive, frequency);
    }

    unsigned used_bits = 0;

    for (Byte byte : input) {
        for (bool bit : codes[byte]) {
            write_bit(archive, bit, used_bits);
        }
    }

    return archive;
}

Bytes decompress(const Bytes& archive) {
    if (archive.size() < kHeaderSize || archive.size() > kMaxArchiveSize) {
        throw std::runtime_error("Archive size is outside the supported range.");
    }

    if (!std::equal(kMagic.begin(), kMagic.end(), archive.begin())) {
        throw std::runtime_error("Not an HFC2 archive.");
    }

    Frequencies frequencies{};
    std::size_t position = kMagic.size();
    std::uint64_t original_size = 0;

    for (auto& frequency : frequencies) {
        frequency = read_u32(archive, position);
        original_size += frequency;
    }

    if (original_size > kMaxFileSize) {
        throw std::runtime_error("Decoded file would exceed the 64 MiB limit.");
    }

    const Tree tree = build_tree(frequencies);
    const CodeTable codes = make_codes(tree);
    const auto payload_bits = count_bits(frequencies, codes);
    const auto payload_bytes = (payload_bits + 7) / 8;

    if (archive.size() - kHeaderSize != payload_bytes) {
        throw std::runtime_error("Payload is truncated or has extra bytes.");
    }

    // The expected bit count also tells us which final bits are only padding.
    if (payload_bits % 8 != 0) {
        const auto padding = 8U - static_cast<unsigned>(payload_bits % 8);

        if ((archive.back() & ((1U << padding) - 1U)) != 0) {
            throw std::runtime_error("Invalid padding bits.");
        }
    }

    if (tree.root < 0) {
        return {};             // All frequency counts are zero: an empty file.
    }

    const Node& root = tree.nodes[static_cast<std::size_t>(tree.root)];

    if (root.symbol >= 0) {
        for (std::uint64_t bit = 0; bit < payload_bits; ++bit) {
            if (read_bit(archive, bit)) {
                throw std::runtime_error("A one-symbol code must be zero.");
            }
        }

        return Bytes(static_cast<std::size_t>(original_size),
                     static_cast<Byte>(root.symbol));
    }

    Bytes output;
    output.reserve(static_cast<std::size_t>(original_size));
    int current = tree.root;

    for (std::uint64_t bit = 0; bit < payload_bits; ++bit) {
        const Node& node = tree.nodes[static_cast<std::size_t>(current)];
        current = read_bit(archive, bit) ? node.right : node.left;
        const Node& next = tree.nodes[static_cast<std::size_t>(current)];

        if (next.symbol >= 0) {
            const auto symbol = static_cast<std::size_t>(next.symbol);

            if (frequencies[symbol] == 0) {
                throw std::runtime_error("Decoded counts do not match the header.");
            }

            --frequencies[symbol];
            output.push_back(static_cast<Byte>(symbol));
            current = tree.root;    // Start the next code from the root.
        }
    }

    if (current != tree.root || output.size() != original_size) {
        throw std::runtime_error("Payload ends before all bytes are decoded.");
    }

    return output;
}

}  // namespace huffman

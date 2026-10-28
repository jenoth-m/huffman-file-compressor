# Code walkthrough

The program compresses a file using Huffman coding. It stores the byte frequencies with the encoded data so the decoder can rebuild the same tree and restore the original bytes.

## Source files

| File | Purpose |
| --- | --- |
| `src/main.cpp` | Handles command-line arguments and calls the compressor or decoder. |
| `src/huffman.cpp` | Builds the tree, generates codes, packs bits, and decodes archives. |
| `src/file_io.cpp` | Reads input files and creates output files. |
| `include/huffman.h` | Defines the byte types, size limits, and codec interface. |
| `tests/test_codec.cpp` | Checks round trips, a known encoding, and selected invalid archives. |

## Compression

`compress()` counts each byte value using a 256-entry frequency array. `build_tree()` creates a leaf for every byte that appears and adds its index to a priority queue.

The two nodes with the lowest frequencies are removed and joined under a parent. The parent’s frequency is their sum. This repeats until one root remains.

Equal frequencies are ordered by the smallest byte value in each subtree. This makes the result deterministic: the encoder and decoder must build identical trees from the stored counts.

The nodes are stored in a vector, with child indexes connecting them. The vector reserves 511 entries, enough for 256 leaves and 255 parents.

`assign_codes()` walks the tree recursively. A left edge adds `0`; a right edge adds `1`. The path to each leaf becomes that byte’s code. These codes are prefix-free, so the decoder can identify where each encoded byte ends.

`write_bit()` packs the codes into bytes, starting at bit 7 and working down to bit 0. Any unused bits in the final byte remain zero.

## Archive format

| Field | Size |
| --- | --- |
| `HFC2` signature | 4 bytes |
| Frequency table | 256 × 4 bytes |
| Encoded payload | Variable |

The fixed header occupies 1,028 bytes. Each frequency is stored as a little-endian 32-bit unsigned integer. The original file size is the sum of those frequencies.

For `ABCD`, all four frequencies are equal. The tie-breaking rule produces:

| Byte | Code |
| --- | --- |
| A | `00` |
| B | `01` |
| C | `10` |
| D | `11` |

The payload is `00011011`, or `0x1B`. The complete archive occupies 1,029 bytes, so this four-byte input becomes larger. Compression only reduces the total size when payload savings exceed the header cost.

## Decompression

`decompress()` checks the signature and size limits, reads the frequency table, and rebuilds the tree. It calculates the expected payload length from the frequencies and code lengths.

Each payload bit selects a child node. When a leaf is reached, its byte is appended to the output and decoding returns to the root. Remaining frequency counts are tracked to catch inconsistent results.

Empty input produces a header with zero counts and no payload. For input containing one unique byte, that byte uses the code `0`, repeated once per occurrence.

The decoder rejects incorrect payload lengths, nonzero padding, and inconsistent decoded counts. There is no checksum, so these checks cannot detect every possible alteration.

## File handling and tests

Files are opened in binary mode. Input size is checked before allocating the buffer, and processing finishes before the output file is created.

`write_new_file()` uses exclusive creation (`wbx`) to protect existing files. If writing fails, it attempts to remove the incomplete output.

The codec tests cover empty input, repeated bytes, text, all 256 byte values, an independently specified `ABCD` archive, and selected malformed archives. Checks use exceptions so they remain active in Release builds.

Files are processed in memory, with a maximum original size of 64 MiB. Archives contain file contents only; filenames, permissions, and other metadata are not stored.
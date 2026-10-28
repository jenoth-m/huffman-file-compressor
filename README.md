# Huffman File Compressor

A C++17 command-line tool that compresses and restores text and binary files using Huffman coding. It uses a custom `.hfc` format and the C++ standard library.

The compressor counts byte frequencies, builds a Huffman tree, and packs the resulting codes into bytes. The archive stores the frequency table so the decoder can rebuild the same tree.

## Build

Requires CMake 3.20+ and a C++17 compiler. On Windows, install Visual Studio’s **Desktop development with C++** workload and CMake tools, then open **Developer PowerShell for Visual Studio** in the project folder.

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Usage

On Windows with the Visual Studio generator:

```powershell
.\build\Release\huff.exe compress README.md build\readme.hfc
.\build\Release\huff.exe decompress build\readme.hfc build\readme-restored.md
fc.exe /b README.md build\readme-restored.md
```

`fc.exe` checks that the restored file matches the original. Existing output files are never overwritten, so choose fresh filenames when repeating the commands.

On Linux/macOS, build and test with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Use `./build/huff` with the same `compress` and `decompress` arguments.

## Tests

The 12 directed checks cover empty input, repeated bytes, text, all 256 byte values, a known encoding, and selected invalid archives.

The build and tests were verified on Linux. The Windows instructions have not yet been verified for this version.

## Limitations

- The fixed 1,028-byte header can make small files larger.
- Files are processed in memory, with a 64 MiB original-file limit.
- Archive validation does not include a checksum.
- Each archive stores one file’s contents without its name or other metadata.

See the [code walkthrough](docs/WALKTHROUGH.md) for implementation details.
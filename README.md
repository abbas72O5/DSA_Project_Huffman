Huffman Multi-Module Compressor 🗜️

A Huffman-coding–based compression tool with GUI visualization, built as a Data Structures and Algorithms (DSA) project.

## 👥 Group Members

| Name            | Roll Number | Contribution                                           |
|-----------------|-------------|--------------------------------------------------------|
| Syed Abbas Raza | 517626      | Core Huffman Algorithm, Tree Visualization, Basic GUI Setup |
| Shuja Naveed    | 502379      | GUI Development, File I/O, Multi-Module System, Integration of Modules |


Course: Data Structures and Algorithms
Class: BSCS 14-A

📌 Table of Contents

Project Overview

Features

Installation

Usage

Technical Details

Project Structure

Performance

Limitations

Contributing

License


🎯 Project Overview

This project implements lossless data compression using the Huffman algorithm, combined with a GUI built in SFML. It highlights core DSA concepts:

Greedy algorithms (optimal prefix codes)

Priority queues (custom min-heap)

Tree structures

Hash maps for O(1) code lookup

Bit-level encoding

Key Highlights

Multi-module compression: Text, Audio, Video

Interactive Huffman tree visualization (zoom, pan, scroll)

Lossless compression/decompression

Detailed compression statistics


<img width="994" height="688" alt="image" src="https://github.com/user-attachments/assets/bfb61960-bffa-45f9-9e87-7f2222e2b656" />

<img width="1001" height="725" alt="image" src="https://github.com/user-attachments/assets/2ed2a61a-ebe5-478d-9c93-cc3cd8ed3fc0" />


✨ Features
1. Compression Modules

Text (.txt)

Audio (.wav, .mp3, .flac, .aac)

Video (.mp4, .avi, .mkv, .mov)
Each module displays hex/character visualization where applicable.

2. Tree Visualization

Zoom (0.1×–5×), pan, scroll

Color-coded nodes (green = leaves, blue = internal nodes)

Automatic node spacing

Separate trees for compression & decompression

3. Statistics

Original vs compressed size

Compression ratio & space saved

Unique bytes & total nodes

4. User-Friendly GUI

Simple module selection

Native file dialogs

Save/load compressed (.huff) files

Status messages & error handling

🚀 Installation Requirements

C++17 compiler

SFML 2.6.x

Windows OS (for native dialogs)

arial.ttf in project directory

Steps

Install SFML (via vcpkg or manual download)

Clone repo

Place arial.ttf in project root

Compile with MinGW or Visual Studio (link SFML libraries)

Run huffman_compressor.exe

📖 Usage
Compression

Launch the app

Select Compress Files

Choose module → choose input file

View tree + statistics

Save the .huff file

Decompression

Select Decompress Files

Choose .huff file

View statistics/tree

Save restored file

Navigation Controls

Mouse wheel: Zoom

Click + drag / arrow keys: Pan

Scrollbars: Navigation

+ / – / Reset: View controls

🔧 Technical Details
Huffman Encoding

Count byte frequencies

Build min-heap

Construct Huffman tree

Generate binary codes

Encode data & write header + bitstream

Decoding

Read header

Rebuild tree

Decode bitstream to original file

Core Data Structures

Binary Min-Heap – O(log k) operations

Huffman Node Tree

Hash map for fast byte → code lookup


Complexity
Operation	Time	Space
Frequency Counting	O(n)	O(1)
Heap & Tree Build	O(k log k)	O(k)
Encoding/Decoding	O(n)	O(n)
Overall: O(n + k log k)		


.huff File Structure
Includes:

Unique byte count

Symbol–frequency table

Total compressed bits

Bit-packed encoded data

📊 Performance Summary

Text files: 40–60% reduction
Uncompressed audio: Small gains
Compressed formats (MP3/MP4): May increase size (expected)

Benchmarks (Intel i5):

Size	Compress	Decompress
1 KB	<0.1s	<0.1s
1 MB	~2s	~1s
10 MB	~15s	~8s

⚠️ Known Limitations
Inefficient for Small files as header info can be greater than the file itself
 
Loads entire file into memory (large files may fail)

Windows-only due to native dialogs

Already-compressed files may grow

Requires arial.ttf

Single-threaded processing

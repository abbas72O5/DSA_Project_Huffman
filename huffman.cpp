// main.cpp
// Advanced Huffman Multimedia Compressor with SFML GUI
// Features: Universal Huffman Compression (Text, Audio, Video binary processing), Tree Visualization, Detailed Stats
// Build: C++17, SFML 2.6.0 (link sfml-graphics, sfml-window, sfml-system)

#define _CRT_SECURE_NO_WARNINGS

#include <SFML/Graphics.hpp>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

using namespace std;

// ==========================================
// DATA STRUCTURES
// ==========================================

class HuffmanNode {
public:
    unsigned char data;
    uint64_t freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(unsigned char data, uint64_t freq) {
        this->data = data;
        this->freq = freq;
        left = right = nullptr;
    }

    HuffmanNode(HuffmanNode* l, HuffmanNode* r) {
        data = 0;
        freq = l->freq + r->freq;
        left = l;
        right = r;
    }

    bool isLeaf() { return left == nullptr && right == nullptr; }
};

struct CompressionStats {
    uint64_t originalSize;
    uint64_t compressedSize;
    double compressionRatio;
    double entropy;
    double avgCodeLength;
    double efficiency;
    string fileName;
    string fileType;
    string status; // Success/Error message
    string outputPath; // Where the file was saved
};

// ==========================================
// HUFFMAN LOGIC
// ==========================================

class BinaryHeap {
    HuffmanNode** arr;
    int rear;
    int capacity;

    void swapNodes(int i, int j) {
        HuffmanNode* temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }

public:
    BinaryHeap(int capacity) {
        this->capacity = capacity;
        arr = new HuffmanNode * [capacity + 2];
        rear = 0;
    }

    ~BinaryHeap() { delete[] arr; }

    int size() { return rear; }

    void push(HuffmanNode* node) {
        if (rear + 1 >= capacity) return;
        arr[++rear] = node;
        int i = rear;
        while (i > 1 && arr[i]->freq < arr[i / 2]->freq) {
            swapNodes(i, i / 2);
            i /= 2;
        }
    }

    HuffmanNode* pop() {
        if (rear == 0) return nullptr;
        HuffmanNode* minNode = arr[1];
        arr[1] = arr[rear--];
        int i = 1;
        while (true) {
            int left = 2 * i, right = 2 * i + 1;
            int smallest = i;
            if (left <= rear && arr[left]->freq < arr[smallest]->freq) smallest = left;
            if (right <= rear && arr[right]->freq < arr[smallest]->freq) smallest = right;
            if (smallest == i) break;
            swapNodes(i, smallest);
            i = smallest;
        }
        return minNode;
    }
};

HuffmanNode* buildHuffmanTree(uint64_t* freqs) {
    BinaryHeap minHeap(256 + 5);
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            minHeap.push(new HuffmanNode((unsigned char)i, freqs[i]));
        }
    }

    if (minHeap.size() == 0) return nullptr;
    if (minHeap.size() == 1) {
        HuffmanNode* only = minHeap.pop();
        HuffmanNode* dummy = new HuffmanNode((unsigned char)0, 0);
        return new HuffmanNode(only, dummy);
    }

    while (minHeap.size() > 1) {
        HuffmanNode* l = minHeap.pop();
        HuffmanNode* r = minHeap.pop();
        minHeap.push(new HuffmanNode(l, r));
    }
    return minHeap.pop();
}

void generateCodes(HuffmanNode* root, unordered_map<unsigned char, string>& codeMap, string path = "") {
    if (!root) return;
    if (root->isLeaf()) {
        codeMap[root->data] = path.empty() ? "0" : path;
        return;
    }
    generateCodes(root->left, codeMap, path + "0");
    generateCodes(root->right, codeMap, path + "1");
}

void freeTree(HuffmanNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// ==========================================
// FILE I/O HELPERS
// ==========================================

class BitWriter {
public:
    vector<unsigned char> bytes;
    unsigned char currentByte = 0;
    int bitCount = 0;

    void writeBit(char bit) { // '0' or '1'
        if (bit == '1') {
            currentByte |= (1 << (7 - bitCount));
        }
        bitCount++;
        if (bitCount == 8) {
            bytes.push_back(currentByte);
            currentByte = 0;
            bitCount = 0;
        }
    }

    void flush() {
        if (bitCount > 0) {
            bytes.push_back(currentByte);
        }
    }
};

// ==========================================
// GUI & UTILS
// ==========================================

struct VizNode {
    HuffmanNode* n;
    int x;
    int depth;
    float screenX, screenY;
};

void assignPositions(HuffmanNode* root, int& currentX, int depth, vector<VizNode>& viz) {
    if (!root) return;
    assignPositions(root->left, currentX, depth + 1, viz);
    viz.push_back({ root, currentX++, depth, 0, 0 });
    assignPositions(root->right, currentX, depth + 1, viz);
}

string openFileDialog(const char* filter) {
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) return string(szFile);
    return "";
}

void drawButton(sf::RenderWindow& window, const string& text, float x, float y, float w, float h, sf::Color color, sf::Font& font, bool active = false) {
    sf::RectangleShape btn(sf::Vector2f(w, h));
    btn.setPosition(x, y);
    btn.setFillColor(active ? sf::Color(color.r + 40, color.g + 40, color.b + 40) : color);
    btn.setOutlineThickness(active ? 2 : 1);
    btn.setOutlineColor(active ? sf::Color::White : sf::Color(150, 150, 150));
    window.draw(btn);

    sf::Text txt(text, font, 16);
    txt.setFillColor(sf::Color::White);
    sf::FloatRect bounds = txt.getLocalBounds();
    txt.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
    txt.setPosition(x + w / 2.0f, y + h / 2.0f);
    window.draw(txt);
}

void drawCard(sf::RenderWindow& window, const string& title, const string& sub, const string& icon, float x, float y, float w, float h, sf::Color baseColor, sf::Font& font, bool hover) {
    sf::RectangleShape card(sf::Vector2f(w, h));
    card.setPosition(x, y);
    card.setFillColor(hover ? sf::Color(baseColor.r + 20, baseColor.g + 20, baseColor.b + 20) : baseColor);
    card.setOutlineThickness(hover ? 3 : 1);
    card.setOutlineColor(hover ? sf::Color::White : sf::Color(100, 100, 100));
    window.draw(card);

    sf::Text iconTxt(icon, font, 48);
    iconTxt.setFillColor(sf::Color(255, 255, 255, 100));
    sf::FloatRect ib = iconTxt.getLocalBounds();
    iconTxt.setOrigin(ib.width / 2, ib.height / 2);
    iconTxt.setPosition(x + w / 2, y + h / 3);
    window.draw(iconTxt);

    sf::Text tTxt(title, font, 22);
    tTxt.setFillColor(sf::Color::White);
    tTxt.setStyle(sf::Text::Bold);
    sf::FloatRect tb = tTxt.getLocalBounds();
    tTxt.setOrigin(tb.width / 2, tb.height / 2);
    tTxt.setPosition(x + w / 2, y + h * 0.65f);
    window.draw(tTxt);

    sf::Text sTxt(sub, font, 14);
    sTxt.setFillColor(sf::Color(200, 200, 200));
    sf::FloatRect sb = sTxt.getLocalBounds();
    sTxt.setOrigin(sb.width / 2, sb.height / 2);
    sTxt.setPosition(x + w / 2, y + h * 0.8f);
    window.draw(sTxt);
}

bool isHovered(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

// ==========================================
// COMPRESSION & DECOMPRESSION
// ==========================================

void compressFile(const string& path, HuffmanNode*& root, vector<VizNode>& vizNodes, CompressionStats& stats, unordered_map<unsigned char, string>& codeMap) {
    ifstream f(path, ios::binary);
    if (!f) {
        stats.status = "Error: Cannot open file";
        return;
    }

    // Get file size
    f.seekg(0, ios::end);
    size_t size = f.tellg();
    f.seekg(0, ios::beg);

    // Read data
    vector<char> buffer(size);
    if (f.read(buffer.data(), size)) {
        // Clean up old tree
        if (root) freeTree(root);
        root = nullptr;
        vizNodes.clear();
        codeMap.clear();

        // Frequency Analysis
        uint64_t freqs[256] = { 0 };
        for (unsigned char c : buffer) freqs[c]++;

        // Build Tree
        root = buildHuffmanTree(freqs);
        generateCodes(root, codeMap);

        // Calculate Compressed Size & Generate Bit Stream
        BitWriter bw;
        for (unsigned char c : buffer) {
            string code = codeMap[c];
            for (char bit : code) bw.writeBit(bit);
        }
        bw.flush();

        // SAVE TO FILE
        string outPath = path + ".huff";
        ofstream out(outPath, ios::binary);
        if (out) {
            // Header: MAGIC (4 bytes), Original Size (8 bytes)
            out.write("HUFF", 4);
            uint64_t origSize = size;
            out.write(reinterpret_cast<const char*>(&origSize), sizeof(origSize));

            // Frequency Table
            out.write(reinterpret_cast<const char*>(freqs), sizeof(freqs));

            // Compressed Data
            out.write(reinterpret_cast<const char*>(bw.bytes.data()), bw.bytes.size());
            out.close();

            stats.status = "Success! Compressed.";
            stats.outputPath = outPath;
        }
        else {
            stats.status = "Error: Cannot write output";
        }

        // Stats
        stats.originalSize = size * 8; // bits
        stats.compressedSize = bw.bytes.size() * 8;
        stats.entropy = 0;
        stats.avgCodeLength = 0;

        uint64_t totalFreq = 0;
        for (int i = 0; i < 256; ++i) totalFreq += freqs[i];

        if (totalFreq > 0) {
            for (auto& pair : codeMap) {
                double p = (double)freqs[pair.first] / totalFreq;
                if (p > 0) {
                    stats.entropy -= p * log2(p);
                }
                stats.avgCodeLength += p * pair.second.length();
            }
            stats.compressionRatio = (1.0 - (double)stats.compressedSize / stats.originalSize) * 100.0;
            stats.efficiency = (stats.entropy / stats.avgCodeLength) * 100.0;
        }

        // Viz Layout
        int cx = 0;
        assignPositions(root, cx, 0, vizNodes);
    }
}

void decompressFile(const string& path, HuffmanNode*& root, vector<VizNode>& vizNodes, CompressionStats& stats, unordered_map<unsigned char, string>& codeMap) {
    ifstream f(path, ios::binary);
    if (!f) {
        stats.status = "Error: Cannot open file";
        return;
    }

    // Check Header
    char magic[5] = { 0 };
    f.read(magic, 4);
    if (strcmp(magic, "HUFF") != 0) {
        stats.status = "Error: Not a HUFF file (Cannot Decompress)";
        return;
    }

    uint64_t originalSize = 0;
    f.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));

    // Read Frequency Table
    uint64_t freqs[256];
    f.read(reinterpret_cast<char*>(freqs), sizeof(freqs));

    // Rebuild Tree
    if (root) freeTree(root);
    root = buildHuffmanTree(freqs);
    vizNodes.clear();
    codeMap.clear();
    generateCodes(root, codeMap);

    // Viz Layout
    int cx = 0;
    assignPositions(root, cx, 0, vizNodes);

    // Read Compressed Data & Decode
    string outPath = path + ".decoded";
    size_t extPos = path.find(".huff");
    if (extPos != string::npos) {
        outPath = path.substr(0, extPos); // Restore name
        outPath = "decoded_" + outPath.substr(outPath.find_last_of("/\\") + 1);
    }
    else {
        outPath = "decoded_" + path.substr(path.find_last_of("/\\") + 1);
    }

    ofstream out(outPath, ios::binary);
    if (!out) {
        stats.status = "Error: Cannot create output";
        return;
    }

    HuffmanNode* curr = root;
    uint64_t bytesDecoded = 0;
    char byte;

    // Read rest of file
    while (f.get(byte) && bytesDecoded < originalSize) {
        for (int i = 7; i >= 0; --i) {
            int bit = (byte >> i) & 1;
            if (bit == 0) curr = curr->left;
            else curr = curr->right;

            if (curr->isLeaf()) {
                out.put(curr->data);
                bytesDecoded++;
                curr = root;
                if (bytesDecoded == originalSize) break;
            }
        }
    }
    out.close();

    stats.status = "Success! Decompressed.";
    stats.outputPath = outPath;
    stats.originalSize = originalSize * 8;
    stats.compressedSize = (uint64_t)f.tellg() * 8; // approx
    stats.fileType = "DECOMPRESSED";

    stats.compressionRatio = 0;
    stats.entropy = 0;
    stats.efficiency = 0;
}

// ==========================================
// MAIN APPLICATION
// ==========================================

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Nexus Huffman Compressor", sf::Style::Close | sf::Style::Titlebar);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        // Handle error in production
    }

    enum Mode { HOME, TEXT_MODE, AUDIO_MODE, VIDEO_MODE, TREE_VIEW };
    Mode currentMode = HOME;
    Mode returnMode = HOME;

    // Global State
    HuffmanNode* root = nullptr;
    unordered_map<unsigned char, string> codeMap;
    CompressionStats stats = { 0 };
    vector<VizNode> vizNodes;
    string selectedFilePath = "";

    auto clearStats = [&]() {
        if (root) freeTree(root);
        root = nullptr;
        codeMap.clear();
        stats = { 0 };
        vizNodes.clear();
        selectedFilePath = "";
        stats.status = "Ready";
        stats.outputPath = "";
        };

    while (window.isOpen()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        float mx = (float)mousePos.x;
        float my = (float)mousePos.y;

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                if (root) freeTree(root);
                window.close();
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {

                    // NAV: Back Button
                    if (currentMode != HOME) {
                        if (isHovered(mx, my, 20, 20, 80, 40)) {
                            if (currentMode == TREE_VIEW) {
                                currentMode = returnMode;
                            }
                            else {
                                currentMode = HOME;
                                clearStats();
                            }
                        }
                    }

                    if (currentMode == HOME) {
                        if (isHovered(mx, my, 150, 300, 250, 300)) currentMode = TEXT_MODE;
                        if (isHovered(mx, my, 475, 300, 250, 300)) currentMode = AUDIO_MODE;
                        if (isHovered(mx, my, 800, 300, 250, 300)) currentMode = VIDEO_MODE;
                    }
                    else if (currentMode == TEXT_MODE || currentMode == AUDIO_MODE || currentMode == VIDEO_MODE) {

                        // 1. SELECT FILE BUTTON
                        if (isHovered(mx, my, 50, 100, 250, 50)) {
                            string path = openFileDialog("All Files\0*.*\0");
                            if (!path.empty()) {
                                clearStats(); // Reset previous run
                                selectedFilePath = path;

                                // Extract filename for display
                                size_t lastSlash = path.find_last_of("\\/");
                                if (lastSlash != string::npos) stats.fileName = path.substr(lastSlash + 1);
                                else stats.fileName = path;
                                stats.fileType = (currentMode == TEXT_MODE ? "TEXT" : (currentMode == AUDIO_MODE ? "AUDIO" : "VIDEO"));
                                stats.status = "File Selected. Choose Action.";
                            }
                        }

                        // 2. ACTION BUTTONS (Only if file selected)
                        if (!selectedFilePath.empty()) {
                            // COMPRESS BUTTON
                            if (isHovered(mx, my, 50, 170, 120, 40)) {
                                compressFile(selectedFilePath, root, vizNodes, stats, codeMap);
                            }
                            // DECOMPRESS BUTTON
                            if (isHovered(mx, my, 180, 170, 120, 40)) {
                                decompressFile(selectedFilePath, root, vizNodes, stats, codeMap);
                            }
                        }

                        // 3. VIEW TREE BUTTON
                        if (root && isHovered(mx, my, 50, 250, 250, 50)) {
                            returnMode = currentMode;
                            currentMode = TREE_VIEW;
                        }
                    }
                }
            }
        }

        // RENDERING
        window.clear(sf::Color(20, 20, 25));

        // HEADER
        sf::RectangleShape header(sf::Vector2f(1200, 80));
        header.setFillColor(sf::Color(30, 30, 35));
        window.draw(header);

        sf::Text title("NEXUS COMPRESSOR", font, 28);
        title.setStyle(sf::Text::Bold);
        title.setPosition(130, 22);
        title.setFillColor(sf::Color::White);
        window.draw(title);

        sf::Text subtitle("v2.3 PRO", font, 14);
        subtitle.setPosition(470, 35);
        subtitle.setFillColor(sf::Color(0, 255, 128));
        window.draw(subtitle);

        if (currentMode != HOME) {
            drawButton(window, "< BACK", 20, 20, 80, 40, sf::Color(60, 60, 70), font, isHovered(mx, my, 20, 20, 80, 40));
        }
        else {
            sf::CircleShape logo(20);
            logo.setPosition(40, 20);
            logo.setFillColor(sf::Color(0, 255, 128));
            window.draw(logo);
        }

        if (currentMode == HOME) {
            sf::Text welcome("SELECT MODULE", font, 20);
            welcome.setPosition(520, 200);
            welcome.setFillColor(sf::Color(150, 150, 160));
            window.draw(welcome);

            drawCard(window, "TEXT", "Huffman Lossless", "Tx", 150, 300, 250, 300, sf::Color(40, 40, 50), font, isHovered(mx, my, 150, 300, 250, 300));
            drawCard(window, "AUDIO", "Binary Lossless", "Au", 475, 300, 250, 300, sf::Color(40, 40, 50), font, isHovered(mx, my, 475, 300, 250, 300));
            drawCard(window, "VIDEO", "Binary Lossless", "Vi", 800, 300, 250, 300, sf::Color(40, 40, 50), font, isHovered(mx, my, 800, 300, 250, 300));
        }
        else if (currentMode == TEXT_MODE || currentMode == AUDIO_MODE || currentMode == VIDEO_MODE) {
            // SIDEBAR
            sf::RectangleShape sidebar(sf::Vector2f(300, 720));
            sidebar.setPosition(0, 80);
            sidebar.setFillColor(sf::Color(25, 25, 30));
            window.draw(sidebar);

            sf::Text sbTitle("CONTROLS", font, 18);
            sbTitle.setPosition(20, 100);
            sbTitle.setFillColor(sf::Color::White);
            window.draw(sbTitle);

            // 1. SELECT FILE
            drawButton(window, "SELECT FILE", 50, 100, 250, 50, sf::Color(0, 120, 210), font, isHovered(mx, my, 50, 100, 250, 50));

            // 2. ACTION BUTTONS (Conditional)
            if (!selectedFilePath.empty()) {
                drawButton(window, "COMPRESS", 50, 170, 120, 40, sf::Color(0, 150, 100), font, isHovered(mx, my, 50, 170, 120, 40));
                drawButton(window, "DECOMPRESS", 180, 170, 120, 40, sf::Color(200, 100, 0), font, isHovered(mx, my, 180, 170, 120, 40));
            }
            else {
                sf::Text hint("No file selected", font, 14);
                hint.setPosition(50, 180);
                hint.setFillColor(sf::Color(100, 100, 100));
                window.draw(hint);
            }

            // 3. TREE VIEW
            if (root) {
                drawButton(window, "VIEW TREE MAP", 50, 250, 250, 50, sf::Color(80, 80, 150), font, isHovered(mx, my, 50, 250, 250, 50));
            }

            // MAIN PANEL
            sf::Text panelTitle("ANALYSIS REPORT", font, 20);
            panelTitle.setPosition(350, 110);
            panelTitle.setFillColor(sf::Color::White);
            window.draw(panelTitle);

            sf::RectangleShape panel(sf::Vector2f(800, 500));
            panel.setPosition(350, 150);
            panel.setFillColor(sf::Color(35, 35, 40));
            panel.setOutlineThickness(1);
            panel.setOutlineColor(sf::Color(60, 60, 70));
            window.draw(panel);

            // STATUS MESSAGES
            if (!stats.status.empty() && stats.status != "Ready") {
                sf::Text statusTxt(stats.status, font, 18);
                statusTxt.setPosition(370, 170);
                statusTxt.setFillColor(stats.status.find("Error") != string::npos ? sf::Color::Red : sf::Color::Green);
                window.draw(statusTxt);

                if (!stats.outputPath.empty()) {
                    sf::Text pathTxt("Saved to: " + stats.outputPath, font, 14);
                    pathTxt.setPosition(370, 200);
                    pathTxt.setFillColor(sf::Color(200, 200, 200));
                    window.draw(pathTxt);
                }
            }
            else {
                sf::Text info("1. Select a file\n2. Choose Compress or Decompress", font, 20);
                sf::FloatRect ib = info.getLocalBounds();
                info.setOrigin(ib.width / 2, ib.height / 2);
                info.setPosition(750, 400);
                info.setFillColor(sf::Color(80, 80, 90));
                window.draw(info);
            }

            // STATS DISPLAY (Only if we have processed data)
            if (root && !stats.status.empty() && stats.status.find("Success") != string::npos) {
                float startY = 250;
                float barHeight = 40;
                float gap = 20;
                float barX = 380;
                float maxW = 740;

                auto drawStatRow = [&](string label, string val, sf::Color c, float y) {
                    sf::RectangleShape row(sf::Vector2f(maxW, barHeight));
                    row.setPosition(barX, y);
                    row.setFillColor(sf::Color(45, 45, 50));
                    window.draw(row);

                    sf::RectangleShape accent(sf::Vector2f(5, barHeight));
                    accent.setPosition(barX, y);
                    accent.setFillColor(c);
                    window.draw(accent);

                    sf::Text l(label, font, 16);
                    l.setPosition(barX + 20, y + 10);
                    l.setFillColor(sf::Color(200, 200, 200));
                    window.draw(l);

                    sf::Text v(val, font, 16);
                    sf::FloatRect vb = v.getLocalBounds();
                    v.setPosition(barX + maxW - vb.width - 20, y + 10);
                    v.setFillColor(sf::Color::White);
                    window.draw(v);
                    };

                drawStatRow("File Name", stats.fileName, sf::Color::White, startY);

                stringstream ss;
                ss << stats.originalSize << " bits";
                drawStatRow("Original Size", ss.str(), sf::Color::Red, startY + barHeight + gap);

                ss.str(""); ss << stats.compressedSize << " bits";
                drawStatRow("Compressed Size", ss.str(), sf::Color::Green, startY + (barHeight + gap) * 2);
            }
        }
        else if (currentMode == TREE_VIEW) {
            // Tree Visualization (Rendering Logic)
            if (!vizNodes.empty()) {
                int maxX = 0;
                for (auto& n : vizNodes) if (n.x > maxX) maxX = n.x;
                float scaleX = 1100.0f / (maxX + 2);

                for (auto& n : vizNodes) {
                    n.screenX = 50 + n.x * scaleX;
                    n.screenY = 100 + n.depth * 80;

                    if (n.n->left) {
                        for (auto& child : vizNodes) {
                            if (child.n == n.n->left) {
                                sf::Vertex line[] = {
                                    sf::Vertex(sf::Vector2f(n.screenX, n.screenY), sf::Color(100, 100, 100)),
                                    sf::Vertex(sf::Vector2f(child.screenX, child.screenY), sf::Color(100, 100, 100))
                                };
                                window.draw(line, 2, sf::Lines);
                                break;
                            }
                        }
                    }
                    if (n.n->right) {
                        for (auto& child : vizNodes) {
                            if (child.n == n.n->right) {
                                sf::Vertex line[] = {
                                    sf::Vertex(sf::Vector2f(n.screenX, n.screenY), sf::Color(100, 100, 100)),
                                    sf::Vertex(sf::Vector2f(child.screenX, child.screenY), sf::Color(100, 100, 100))
                                };
                                window.draw(line, 2, sf::Lines);
                                break;
                            }
                        }
                    }
                }

                for (auto& n : vizNodes) {
                    sf::CircleShape c(15);
                    c.setOrigin(15, 15);
                    c.setPosition(n.screenX, n.screenY);
                    c.setFillColor(n.n->isLeaf() ? sf::Color(0, 200, 100) : sf::Color(200, 200, 0));
                    window.draw(c);

                    // FIXED: Character rendering
                    if (n.n->isLeaf()) {
                        string s = "";
                        unsigned char ch = n.n->data;
                        if (ch == '\n') s = "\\n";
                        else if (ch == '\t') s = "\\t";
                        else if (ch == ' ') s = "SPC";
                        else if (isprint(ch)) s += (char)ch;
                        else {
                            char buf[10];
                            sprintf(buf, "0x%02X", ch);
                            s = buf;
                        }

                        sf::Text t(s, font, 12);
                        sf::FloatRect b = t.getLocalBounds();
                        t.setOrigin(b.width / 2, b.height / 2);
                        t.setPosition(n.screenX, n.screenY);
                        t.setFillColor(sf::Color::Black);
                        window.draw(t);
                    }
                }
            }
        }

        window.display();
    }
    return 0;
}


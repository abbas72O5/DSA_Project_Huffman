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
// DATA STRUCTURES (Unchanged)
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
};

// ==========================================
// HUFFMAN LOGIC (Unchanged from previous fix)
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

HuffmanNode* buildHuffmanTree(const unsigned char* bytes, const uint64_t* freqs, int uniqueCount) {
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
        codeMap[root->data] = path.empty() ? "0" : path; // Handle single character case
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
// GUI & UTILS (Headers and helpers, mostly unchanged)
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

// Helpers for UI (drawButton, drawCard, isHovered - Unchanged)
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

// Universal File Processor (Unchanged from previous fix)
void processFile(const string& path, HuffmanNode*& root, vector<VizNode>& vizNodes, CompressionStats& stats, unordered_map<unsigned char, string>& codeMap, string& fileData) {
    ifstream f(path, ios::binary);
    if (!f) return;

    // Get file size
    f.seekg(0, ios::end);
    size_t size = f.tellg();
    f.seekg(0, ios::beg);

    // Read data
    vector<char> buffer(size);
    if (f.read(buffer.data(), size)) {
        fileData.assign(buffer.begin(), buffer.end());

        // Clean up old tree
        if (root) freeTree(root);
        root = nullptr;
        vizNodes.clear();
        codeMap.clear();

        // Frequency Analysis
        uint64_t freqs[256] = { 0 };
        for (unsigned char c : fileData) freqs[c]++;

        // Build Tree
        root = buildHuffmanTree(nullptr, freqs, 256);
        generateCodes(root, codeMap);

        // Stats
        stats.originalSize = size * 8; // bits
        stats.compressedSize = 0;
        stats.entropy = 0;
        stats.avgCodeLength = 0;

        uint64_t totalFreq = 0;
        for (int i = 0; i < 256; ++i) totalFreq += freqs[i];

        if (totalFreq > 0) {
            for (auto& pair : codeMap) {
                stats.compressedSize += freqs[pair.first] * pair.second.length();
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

// ==========================================
// MAIN APPLICATION
// ==========================================

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 800), "Nexus Huffman Compressor", sf::Style::Close | sf::Style::Titlebar);
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cerr << "Error: arial.ttf not found." << endl;
    }

    enum Mode { HOME, TEXT_MODE, AUDIO_MODE, VIDEO_MODE, TREE_VIEW };
    Mode currentMode = HOME;
    Mode returnMode = HOME; // To know where to go back from Tree View

    // Global State
    HuffmanNode* root = nullptr;
    string currentFileData = "";
    unordered_map<unsigned char, string> codeMap;
    CompressionStats stats = { 0 };
    vector<VizNode> vizNodes;
    string statusMsg = "Ready";

    // Helper function to clear all stats and reset the state
    auto clearStats = [&]() {
        if (root) freeTree(root);
        root = nullptr;
        currentFileData.clear();
        codeMap.clear();
        stats = { 0 };
        vizNodes.clear();
        statusMsg = "Ready";
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

                    // NAV: Back Button Logic
                    if (currentMode != HOME) {
                        if (isHovered(mx, my, 20, 20, 80, 40)) {
                            if (currentMode == TREE_VIEW) {
                                currentMode = returnMode;
                                // DO NOT clear stats when returning from tree view, keep them to show on the main analysis panel
                            }
                            else { // Returning from a mode (TEXT/AUDIO/VIDEO) to HOME
                                currentMode = HOME;
                                // ADDED: Clear all data and stats when returning to HOME
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
                        // Import Button
                        if (isHovered(mx, my, 50, 140, 200, 50)) {
                            const char* filter = "All Files\0*.*\0";
                            if (currentMode == TEXT_MODE) filter = "Text Files\0*.txt\0All Files\0*.*\0";
                            else if (currentMode == AUDIO_MODE) filter = "Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0";
                            else if (currentMode == VIDEO_MODE) filter = "Video Files\0*.mp4;*.avi;*.mkv;*.mov\0All Files\0*.*\0";

                            string path = openFileDialog(filter);
                            if (!path.empty()) {
                                processFile(path, root, vizNodes, stats, codeMap, currentFileData);
                                statusMsg = "Compression Complete!";
                                stats.fileType = (currentMode == TEXT_MODE ? "TEXT" : (currentMode == AUDIO_MODE ? "AUDIO" : "VIDEO"));

                                // Extract filename
                                size_t lastSlash = path.find_last_of("\\/");
                                if (lastSlash != string::npos) stats.fileName = path.substr(lastSlash + 1);
                                else stats.fileName = path;
                            }
                        }

                        // View Tree Button
                        if (root && isHovered(mx, my, 50, 210, 200, 50)) {
                            returnMode = currentMode;
                            currentMode = TREE_VIEW;
                        }
                    }
                }
            }
        }

        // RENDERING (Unchanged from previous fix)
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

        sf::Text subtitle("v2.0 PRO", font, 14);
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
            // Shared Layout for all compression modes
            sf::RectangleShape sidebar(sf::Vector2f(300, 720));
            sidebar.setPosition(0, 80);
            sidebar.setFillColor(sf::Color(25, 25, 30));
            window.draw(sidebar);

            sf::Text sbTitle("CONTROLS", font, 18);
            sbTitle.setPosition(20, 100);
            sbTitle.setFillColor(sf::Color::White);
            window.draw(sbTitle);

            string btnLabel = "IMPORT FILE";
            if (currentMode == AUDIO_MODE) btnLabel = "IMPORT AUDIO";
            if (currentMode == VIDEO_MODE) btnLabel = "IMPORT VIDEO";

            drawButton(window, btnLabel, 50, 140, 200, 50, sf::Color(0, 120, 210), font, isHovered(mx, my, 50, 140, 200, 50));

            if (root) {
                drawButton(window, "VIEW TREE MAP", 50, 210, 200, 50, sf::Color(0, 180, 100), font, isHovered(mx, my, 50, 210, 200, 50));
            }
            else {
                drawButton(window, "VIEW TREE MAP", 50, 210, 200, 50, sf::Color(60, 60, 70), font, false);
            }

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

            if (root) {
                float startY = 180;
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
                ss << stats.originalSize << " bits (" << fixed << setprecision(2) << stats.originalSize / 8.0 / 1024.0 << " KB)";
                drawStatRow("Original Size", ss.str(), sf::Color::Red, startY + barHeight + gap);

                ss.str(""); ss << stats.compressedSize << " bits (" << fixed << setprecision(2) << stats.compressedSize / 8.0 / 1024.0 << " KB)";
                drawStatRow("Compressed Size", ss.str(), sf::Color::Green, startY + (barHeight + gap) * 2);

                ss.str(""); ss << fixed << setprecision(2) << stats.compressionRatio << " %";
                drawStatRow("Ratio", ss.str(), sf::Color::Cyan, startY + (barHeight + gap) * 3);

                ss.str(""); ss << fixed << setprecision(3) << stats.entropy << " bits/sym";
                drawStatRow("Entropy", ss.str(), sf::Color::Magenta, startY + (barHeight + gap) * 4);

                ss.str(""); ss << fixed << setprecision(3) << stats.efficiency << " %";
                drawStatRow("Efficiency", ss.str(), sf::Color::Yellow, startY + (barHeight + gap) * 5);

            }
            else {
                sf::Text info("Waiting for input stream...", font, 24);
                sf::FloatRect ib = info.getLocalBounds();
                info.setOrigin(ib.width / 2, ib.height / 2);
                info.setPosition(750, 400);
                info.setFillColor(sf::Color(80, 80, 90));
                window.draw(info);
            }
        }
        else if (currentMode == TREE_VIEW) {
            // Tree Visualization
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

                                sf::Text zero("0", font, 12);
                                zero.setPosition((n.screenX + child.screenX) / 2, (n.screenY + child.screenY) / 2);
                                zero.setFillColor(sf::Color::Yellow);
                                window.draw(zero);
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

                                sf::Text one("1", font, 12);
                                one.setPosition((n.screenX + child.screenX) / 2, (n.screenY + child.screenY) / 2);
                                one.setFillColor(sf::Color::Yellow);
                                window.draw(one);
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
                    c.setOutlineThickness(1);
                    c.setOutlineColor(sf::Color::White);
                    window.draw(c);

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

                        sf::Text t(s, font, 10);
                        t.setOrigin(t.getLocalBounds().width / 2, t.getLocalBounds().height / 2);
                        t.setPosition(n.screenX, n.screenY - 5);
                        t.setFillColor(sf::Color::Black);
                        window.draw(t);
                    }
                    else {
                        sf::Text freq(to_string(n.n->freq), font, 9);
                        freq.setOrigin(freq.getLocalBounds().width / 2, freq.getLocalBounds().height / 2);
                        freq.setPosition(n.screenX, n.screenY - 2);
                        freq.setFillColor(sf::Color::Black);
                        window.draw(freq);
                    }

                    if (n.n->isLeaf()) {
                        sf::Text f(to_string(n.n->freq), font, 10);
                        f.setOrigin(f.getLocalBounds().width / 2, 0);
                        f.setPosition(n.screenX, n.screenY + 18);
                        f.setFillColor(sf::Color(200, 200, 200));
                        window.draw(f);
                    }
                }
            }
        }

        window.display();
    }

    return 0;
}



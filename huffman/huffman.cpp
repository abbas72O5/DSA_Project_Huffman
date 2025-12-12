// main.cpp - Complete Huffman Multi-Module Compressor
// Build: C++17, SFML 2.6.x (link sfml-graphics, sfml-window, sfml-system)

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

using namespace std;

// ===========================
// MODULE CONFIGURATION
// ===========================
enum ModuleType {
    MODULE_NONE,
    MODULE_TEXT,
    MODULE_AUDIO,
    MODULE_VIDEO
};

struct ModuleConfig {
    ModuleType type;
    string name;
    string fileFilter;

    string getUnitLabel(unsigned char byte) const {
        if (type == MODULE_TEXT) {
            if (byte == '\n') return "\\n";
            if (byte == '\t') return "\\t";
            if (byte == ' ') return "SPC";
            if (isprint(byte)) return string(1, (char)byte);
            char buf[16];
            sprintf_s(buf, sizeof(buf), "0x%02X", byte);
            return buf;
        }
        else {
            // Audio/Video: show hex
            char buf[16];
            sprintf_s(buf, sizeof(buf), "0x%02X", byte);
            return buf;
        }
    }
};

ModuleConfig modules[3] = {
    {MODULE_TEXT, "Text Files", "Text Files\0*.txt\0All Files\0*.*\0"},
    {MODULE_AUDIO, "Audio Files", "Audio Files\0*.wav;*.mp3;*.flac;*.aac\0All Files\0*.*\0"},
    {MODULE_VIDEO, "Video Files", "Video Files\0*.mp4;*.avi;*.mkv;*.mov\0All Files\0*.*\0"}
};

// ===========================
// HUFFMAN CORE LOGIC
// ===========================
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

    bool isLeaf() const { return left == nullptr && right == nullptr; }
};

class BinaryHeap {
    HuffmanNode** arr;
    int rear;
    int capacity;
    int h;

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
        h = -1;
    }
    ~BinaryHeap() { delete[] arr; }

    void updateHeight() {
        if (rear == 0) h = -1;
        else h = (int)floor(log2((double)rear));
    }

    bool isEmpty() { return rear == 0; }
    int size() { return rear; }

    void push(HuffmanNode* node) {
        if (rear + 1 >= capacity) return;
        arr[++rear] = node;
        int i = rear;
        while (i > 1 && arr[i]->freq < arr[i / 2]->freq) {
            swapNodes(i, i / 2);
            i /= 2;
        }
        updateHeight();
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
        updateHeight();
        return minNode;
    }
};

HuffmanNode* buildHuffmanTree(unsigned char bytes[], uint64_t freqs[], int uniqueCount) {
    BinaryHeap minHeap(uniqueCount + 5);
    for (int i = 0; i < uniqueCount; ++i) {
        if (freqs[bytes[i]] > 0) {
            minHeap.push(new HuffmanNode(bytes[i], freqs[bytes[i]]));
        }
    }

    if (minHeap.size() == 0) return nullptr;

    if (minHeap.size() == 1) {
        HuffmanNode* only = minHeap.pop();
        HuffmanNode* dummy = new HuffmanNode((unsigned char)0, 0);
        HuffmanNode* parent = new HuffmanNode(only, dummy);
        minHeap.push(parent);
    }

    while (minHeap.size() > 1) {
        HuffmanNode* l = minHeap.pop();
        HuffmanNode* r = minHeap.pop();
        HuffmanNode* m = new HuffmanNode(l, r);
        minHeap.push(m);
    }
    return minHeap.pop();
}

void storeCodesHashMap(HuffmanNode* root, unordered_map<unsigned char, string>& codeMap, string path = "") {
    if (!root) return;

    if (root->isLeaf()) {
        codeMap[root->data] = path;
        return;
    }

    storeCodesHashMap(root->left, codeMap, path + "0");
    storeCodesHashMap(root->right, codeMap, path + "1");
}

void writeCompressedText(const string& text, const string& outPath,
    unordered_map<unsigned char, string>& codeMap,
    unsigned char bytesPresent[256], uint64_t freqs[256]) {

    uint64_t totalBits = 0;
    for (unordered_map<unsigned char, string>::const_iterator it = codeMap.begin(); it != codeMap.end(); ++it) {
        unsigned char sym = it->first;
        totalBits += freqs[sym] * it->second.length();
    }

    ofstream out(outPath, ios::binary);
    if (!out) { cerr << "Cannot open output file\n"; return; }

    uint16_t uniq = 0;
    for (int i = 0; i < 256; i++) if (bytesPresent[i]) ++uniq;

    out.write(reinterpret_cast<const char*>(&uniq), sizeof(uniq));
    for (int i = 0; i < 256; i++) {
        if (bytesPresent[i]) {
            uint8_t s = (uint8_t)i;
            uint64_t f = freqs[i];
            out.write(reinterpret_cast<const char*>(&s), sizeof(s));
            out.write(reinterpret_cast<const char*>(&f), sizeof(f));
        }
    }
    out.write(reinterpret_cast<const char*>(&totalBits), sizeof(totalBits));

    uint8_t outByte = 0;
    int outBits = 0;
    for (size_t p = 0; p < text.size(); ++p) {
        unsigned char ch = (unsigned char)text[p];

        unordered_map<unsigned char, string>::const_iterator it = codeMap.find(ch);
        if (it == codeMap.end()) continue;

        const string& code = it->second;
        for (size_t k = 0; k < code.length(); ++k) {
            outByte <<= 1;
            if (code[k] == '1') outByte |= 1;
            ++outBits;
            if (outBits == 8) {
                out.write(reinterpret_cast<const char*>(&outByte), 1);
                outByte = 0; outBits = 0;
            }
        }
    }
    if (outBits > 0) {
        outByte <<= (8 - outBits);
        out.write(reinterpret_cast<const char*>(&outByte), 1);
    }
    out.close();
}

bool readCompressedAndDecode(const string& inPath, const string& outPath) {
    ifstream in(inPath, ios::binary);
    if (!in) return false;

    uint16_t uniq = 0;
    in.read(reinterpret_cast<char*>(&uniq), sizeof(uniq));
    if (!in) return false;

    uint64_t freqs[256] = { 0 };
    unsigned char bytesPresent[256] = { 0 };
    for (int i = 0; i < uniq; i++) {
        uint8_t s; uint64_t f;
        in.read(reinterpret_cast<char*>(&s), sizeof(s));
        in.read(reinterpret_cast<char*>(&f), sizeof(f));
        bytesPresent[s] = 1;
        freqs[s] = f;
    }
    uint64_t totalBits = 0;
    in.read(reinterpret_cast<char*>(&totalBits), sizeof(totalBits));

    unsigned char bytesList[256];
    int uniqueCount = 0;
    for (int i = 0; i < 256; i++) if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

    HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
    if (!root) return false;

    ofstream out(outPath, ios::binary);
    if (!out) return false;

    uint64_t bitsRead = 0;
    HuffmanNode* node = root;
    char byteBuf;
    while (bitsRead < totalBits && in.get(byteBuf)) {
        uint8_t b = (uint8_t)byteBuf;
        for (int bit = 7; bit >= 0 && bitsRead < totalBits; --bit) {
            int val = (b >> bit) & 1;
            if (val == 0) node = node->left; else node = node->right;
            if (!node) return false;
            if (node->isLeaf()) {
                out.put((char)node->data);
                node = root;
            }
            ++bitsRead;
        }
    }
    out.close();
    return true;
}

// PART 2: Visualization & Helper Functions
// Add this after Part 1

struct VizNode {
    HuffmanNode* n;
    int x;
    int depth;
    float screenX, screenY;
};

void assignPositionsInorder(HuffmanNode* root, int& currentX, int depth, VizNode viz[], int& idx) {
    if (!root) return;
    assignPositionsInorder(root->left, currentX, depth + 1, viz, idx);
    viz[idx].n = root;
    viz[idx].x = currentX++;
    viz[idx].depth = depth;
    ++idx;
    assignPositionsInorder(root->right, currentX, depth + 1, viz, idx);
}

void drawTreeSFML(sf::RenderWindow& window, VizNode viz[], int vizCount,
    float nodeRadius, sf::Font& font,
    float zoomLevel, float scrollX, float scrollY,
    float maxScrollX, float maxScrollY, const ModuleConfig& module) {

    float viewportWidth = 760.f;
    float viewportHeight = 640.f;

    sf::View treeView(sf::FloatRect(0, 0, viewportWidth / zoomLevel, viewportHeight / zoomLevel));
    treeView.setViewport(sf::FloatRect(10.f / 1000.f, 10.f / 700.f, 760.f / 1000.f, 640.f / 700.f));

    float offsetX = scrollX * maxScrollX;
    float offsetY = scrollY * maxScrollY;
    treeView.setCenter(viewportWidth / (2 * zoomLevel) + offsetX,
        viewportHeight / (2 * zoomLevel) + offsetY);

    window.setView(treeView);

    // Draw edges
    for (int i = 0; i < vizCount; i++) {
        HuffmanNode* node = viz[i].n;
        if (!node) continue;

        if (node->left) {
            for (int j = 0; j < vizCount; j++) {
                if (viz[j].n == node->left) {
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(viz[i].screenX, viz[i].screenY), sf::Color::Black),
                        sf::Vertex(sf::Vector2f(viz[j].screenX, viz[j].screenY), sf::Color::Black)
                    };
                    window.draw(line, 2, sf::Lines);
                    break;
                }
            }
        }

        if (node->right) {
            for (int j = 0; j < vizCount; j++) {
                if (viz[j].n == node->right) {
                    sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(viz[i].screenX, viz[i].screenY), sf::Color::Black),
                        sf::Vertex(sf::Vector2f(viz[j].screenX, viz[j].screenY), sf::Color::Black)
                    };
                    window.draw(line, 2, sf::Lines);
                    break;
                }
            }
        }
    }

    // Draw nodes
    for (int i = 0; i < vizCount; i++) {
        if (!viz[i].n) continue;

        sf::Color nodeColor = viz[i].n->isLeaf() ?
            sf::Color(144, 238, 144) : sf::Color(173, 216, 230);

        sf::CircleShape circle(nodeRadius);
        circle.setOrigin(nodeRadius, nodeRadius);
        circle.setPosition(viz[i].screenX, viz[i].screenY);
        circle.setOutlineColor(sf::Color::Black);
        circle.setOutlineThickness(2.f);
        circle.setFillColor(nodeColor);
        window.draw(circle);

        // Use module-specific label
        string label;
        if (viz[i].n->isLeaf()) {
            label = module.getUnitLabel(viz[i].n->data);
        }
        else {
            label = to_string((uint32_t)viz[i].n->freq);
        }

        sf::Text txt(label, font, 11);
        txt.setFillColor(sf::Color::Black);
        sf::FloatRect bounds = txt.getLocalBounds();
        txt.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        txt.setPosition(viz[i].screenX, viz[i].screenY - 3.f);
        window.draw(txt);

        sf::Text freqTxt(to_string((uint32_t)viz[i].n->freq), font, 9);
        freqTxt.setFillColor(sf::Color(40, 40, 40));
        sf::FloatRect freqBounds = freqTxt.getLocalBounds();
        freqTxt.setOrigin(freqBounds.width / 2.f, 0);
        freqTxt.setPosition(viz[i].screenX, viz[i].screenY + nodeRadius + 2.f);
        window.draw(freqTxt);
    }

    window.setView(window.getDefaultView());
}

string openFileDialogWin(const char* filter) {
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn)) {
        return string(szFile);
    }
    return string();
}

string saveFileDialogWin(const char* filter, const char* defaultName = "compressed.huff") {
    OPENFILENAMEA ofn;
    CHAR szFile[MAX_PATH] = { 0 };
    strcpy_s(szFile, MAX_PATH, defaultName);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = "huff";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn)) {
        return string(szFile);
    }
    return string();
}

void freeTree(HuffmanNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// CONTINUE TO PART 3 (Main Function)...
// PART 3: Main Function (Beginning)
// Add this after Part 2
// PART 3: Main Function (Beginning)
// Add this after Part 2

int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Huffman Multi-Module Compressor");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cerr << "Warning: could not load arial.ttf\n";
    }

    enum AppState {
        MENU,
        MODULE_SELECTION_COMPRESS,
        MODULE_SELECTION_DECOMPRESS,
        SELECTING,
        PROCESSING,
        SHOW_RESULT,
        DECOMPRESSING,
        DECOMPRESS_RESULT,
        SHOW_DECOMPRESS_TREE
    };
    AppState state = MENU;

    // Current module selection
    ModuleConfig currentModule = modules[0]; // Default: text

    // Main menu buttons
    sf::RectangleShape startBtn(sf::Vector2f(260, 60));
    startBtn.setFillColor(sf::Color(120, 180, 255));
    startBtn.setPosition(370, 220);
    sf::Text startTxt("Compress Files", font, 20);
    startTxt.setFillColor(sf::Color::Black);
    startTxt.setPosition(405, 235);

    sf::RectangleShape decompressBtn(sf::Vector2f(260, 60));
    decompressBtn.setFillColor(sf::Color(255, 180, 120));
    decompressBtn.setPosition(370, 300);
    sf::Text decompressTxt("Decompress Files", font, 20);
    decompressTxt.setFillColor(sf::Color::Black);
    decompressTxt.setPosition(395, 315);

    // Module selection buttons
    sf::RectangleShape textModuleBtn(sf::Vector2f(200, 80));
    textModuleBtn.setFillColor(sf::Color(120, 180, 255));
    textModuleBtn.setPosition(150, 250);

    sf::RectangleShape audioModuleBtn(sf::Vector2f(200, 80));
    audioModuleBtn.setFillColor(sf::Color(255, 150, 150));
    audioModuleBtn.setPosition(400, 250);

    sf::RectangleShape videoModuleBtn(sf::Vector2f(200, 80));
    videoModuleBtn.setFillColor(sf::Color(150, 255, 150));
    videoModuleBtn.setPosition(650, 250);

    sf::RectangleShape backBtn(sf::Vector2f(200, 40));
    backBtn.setFillColor(sf::Color(200, 200, 200));
    backBtn.setPosition(400, 450);

    sf::RectangleShape selectBtn(sf::Vector2f(220, 40));
    selectBtn.setFillColor(sf::Color(160, 220, 160));
    selectBtn.setPosition(60, 580);
    sf::Text selectTxt("Select File", font, 18);
    selectTxt.setPosition(105, 588);
    selectTxt.setFillColor(sf::Color::Black);

    sf::Text statusTxt("", font, 16);
    statusTxt.setFillColor(sf::Color::Black);
    statusTxt.setPosition(60, 520);

    // Compression/decompression storage
    string inputPath;
    string compressedPath = "output.huff";
    uint64_t origBytes = 0, compBytes = 0;
    double ratio = 0.0;
    bool processed = false;

    string decompressInputPath;
    string decompressOutputPath = "decompressed.txt";
    uint64_t decompressedBytes = 0;
    bool decompressSuccess = false;

    // Tree visualization
    VizNode viz[1024];
    int vizCount = 0;
    float nodeRadius = 22.f;
    HuffmanNode* savedRoot = nullptr;

    VizNode decompressViz[1024];
    int decompressVizCount = 0;
    HuffmanNode* decompressRoot = nullptr;

    // Camera controls
    float zoomLevel = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    bool isDragging = false;
    sf::Vector2i lastMousePos;

    // Scrollbars
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float maxScrollX = 0.0f;
    float maxScrollY = 0.0f;
    float totalTreeWidth = 0.0f;
    float totalTreeHeight = 0.0f;
    bool isDraggingHScroll = false;
    bool isDraggingVScroll = false;

    sf::RectangleShape hScrollTrack(sf::Vector2f(740, 12));
    hScrollTrack.setPosition(15, 653);
    hScrollTrack.setFillColor(sf::Color(200, 200, 200));
    hScrollTrack.setOutlineThickness(1.f);
    hScrollTrack.setOutlineColor(sf::Color(150, 150, 150));

    sf::RectangleShape hScrollThumb(sf::Vector2f(100, 12));
    hScrollThumb.setFillColor(sf::Color(100, 150, 200));
    hScrollThumb.setOutlineThickness(1.f);
    hScrollThumb.setOutlineColor(sf::Color(70, 100, 150));

    sf::RectangleShape vScrollTrack(sf::Vector2f(12, 620));
    vScrollTrack.setPosition(758, 15);
    vScrollTrack.setFillColor(sf::Color(200, 200, 200));
    vScrollTrack.setOutlineThickness(1.f);
    vScrollTrack.setOutlineColor(sf::Color(150, 150, 150));

    sf::RectangleShape vScrollThumb(sf::Vector2f(12, 100));
    vScrollThumb.setFillColor(sf::Color(100, 150, 200));
    vScrollThumb.setOutlineThickness(1.f);
    vScrollThumb.setOutlineColor(sf::Color(70, 100, 150));

    // Zoom buttons
    sf::RectangleShape zoomInBtn(sf::Vector2f(50, 30));
    zoomInBtn.setFillColor(sf::Color(100, 200, 100));
    zoomInBtn.setPosition(15, 670);

    sf::Text zoomInTxt("+", font, 20);
    zoomInTxt.setFillColor(sf::Color::Black);
    zoomInTxt.setPosition(32, 672);

    sf::RectangleShape zoomOutBtn(sf::Vector2f(50, 30));
    zoomOutBtn.setFillColor(sf::Color(200, 100, 100));
    zoomOutBtn.setPosition(70, 670);

    sf::Text zoomOutTxt("-", font, 20);
    zoomOutTxt.setFillColor(sf::Color::Black);
    zoomOutTxt.setPosition(88, 672);

    sf::RectangleShape resetViewBtn(sf::Vector2f(80, 30));
    resetViewBtn.setFillColor(sf::Color(150, 150, 200));
    resetViewBtn.setPosition(125, 670);

    sf::Text resetViewTxt("Reset", font, 16);
    resetViewTxt.setFillColor(sf::Color::Black);
    resetViewTxt.setPosition(138, 675);

    sf::Text zoomDisplay("", font, 14);
    zoomDisplay.setFillColor(sf::Color::Black);
    zoomDisplay.setPosition(220, 678);

    // Save buttons
    sf::RectangleShape saveCompressedBtn(sf::Vector2f(180, 35));
    saveCompressedBtn.setFillColor(sf::Color(100, 180, 100));
    saveCompressedBtn.setPosition(790, 320);

    sf::Text saveCompressedTxt("Save Compressed File", font, 14);
    saveCompressedTxt.setFillColor(sf::Color::Black);
    saveCompressedTxt.setPosition(800, 328);

    sf::Text saveStatusTxt("", font, 12);
    saveStatusTxt.setFillColor(sf::Color(0, 120, 0));
    saveStatusTxt.setPosition(790, 365);

    sf::RectangleShape saveDecompressedBtn(sf::Vector2f(200, 40));
    saveDecompressedBtn.setFillColor(sf::Color(100, 180, 100));
    saveDecompressedBtn.setPosition(220, 500);

    sf::Text saveDecompressedTxt("Save Decompressed File", font, 14);
    saveDecompressedTxt.setFillColor(sf::Color::Black);
    saveDecompressedTxt.setPosition(235, 510);

    sf::RectangleShape backToMenuBtn(sf::Vector2f(200, 40));
    backToMenuBtn.setFillColor(sf::Color(150, 150, 200));
    backToMenuBtn.setPosition(450, 500);

    sf::Text backToMenuTxt("Back to Menu", font, 14);
    backToMenuTxt.setFillColor(sf::Color::Black);
    backToMenuTxt.setPosition(495, 510);

    sf::RectangleShape showTreeBtn(sf::Vector2f(260, 40));
    showTreeBtn.setFillColor(sf::Color(120, 150, 220));
    showTreeBtn.setPosition(270, 560);

    sf::Text showTreeTxt("Show Huffman Tree", font, 14);
    showTreeTxt.setFillColor(sf::Color::Black);
    showTreeTxt.setPosition(310, 570);

    sf::Text decompressSaveStatus("", font, 12);
    decompressSaveStatus.setFillColor(sf::Color(0, 120, 0));
    decompressSaveStatus.setPosition(300, 555);

    // Main event loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                if (decompressRoot) { freeTree(decompressRoot); decompressRoot = nullptr; }
                window.close();
            }

            // PART 4: Event Handling
// Add after Part 3 (inside the while loop that was started)

            // Mouse wheel zoom
            if (event.type == sf::Event::MouseWheelScrolled && (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE)) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    float delta = event.mouseWheelScroll.delta;
                    if (delta > 0) zoomLevel *= 1.1f;
                    else zoomLevel *= 0.9f;
                    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
                    if (zoomLevel > 5.0f) zoomLevel = 5.0f;
                }
            }

            // Scrollbar dragging
            if (event.type == sf::Event::MouseButtonPressed && (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE)) {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                sf::Vector2f mfp((float)mp.x, (float)mp.y);

                if (hScrollThumb.getGlobalBounds().contains(mfp)) {
                    isDraggingHScroll = true;
                }
                else if (vScrollThumb.getGlobalBounds().contains(mfp)) {
                    isDraggingVScroll = true;
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                isDraggingHScroll = false;
                isDraggingVScroll = false;
                isDragging = false;
            }

            if (event.type == sf::Event::MouseMoved && (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE)) {
                sf::Vector2i mp = sf::Mouse::getPosition(window);

                if (isDraggingHScroll) {
                    float trackLeft = hScrollTrack.getPosition().x;
                    float trackWidth = hScrollTrack.getSize().x;
                    float thumbWidth = hScrollThumb.getSize().x;
                    float newX = mp.x - thumbWidth / 2.f;
                    newX = max(trackLeft, min(newX, trackLeft + trackWidth - thumbWidth));
                    scrollX = (newX - trackLeft) / (trackWidth - thumbWidth);
                    scrollX = max(0.0f, min(1.0f, scrollX));
                }
                else if (isDraggingVScroll) {
                    float trackTop = vScrollTrack.getPosition().y;
                    float trackHeight = vScrollTrack.getSize().y;
                    float thumbHeight = vScrollThumb.getSize().y;
                    float newY = mp.y - thumbHeight / 2.f;
                    newY = max(trackTop, min(newY, trackTop + trackHeight - thumbHeight));
                    scrollY = (newY - trackTop) / (trackHeight - thumbHeight);
                    scrollY = max(0.0f, min(1.0f, scrollY));
                }
            }

            // Arrow key scrolling
            if (event.type == sf::Event::KeyPressed && (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE)) {
                float scrollSpeed = 0.05f;
                if (event.key.code == sf::Keyboard::Left) scrollX = max(0.0f, scrollX - scrollSpeed);
                else if (event.key.code == sf::Keyboard::Right) scrollX = min(1.0f, scrollX + scrollSpeed);
                else if (event.key.code == sf::Keyboard::Up) scrollY = max(0.0f, scrollY - scrollSpeed);
                else if (event.key.code == sf::Keyboard::Down) scrollY = min(1.0f, scrollY + scrollSpeed);
            }

            // Button click events
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                sf::Vector2f mfp((float)mp.x, (float)mp.y);

                // MENU state
                if (state == MENU) {
                    if (startBtn.getGlobalBounds().contains(mfp)) {
                        state = MODULE_SELECTION_COMPRESS;
                    }
                    else if (decompressBtn.getGlobalBounds().contains(mfp)) {
                        state = MODULE_SELECTION_DECOMPRESS;
                    }
                }
                // MODULE SELECTION FOR COMPRESSION
                else if (state == MODULE_SELECTION_COMPRESS) {
                    if (textModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[0];
                        state = SELECTING;
                    }
                    else if (audioModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[1];
                        state = SELECTING;
                    }
                    else if (videoModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[2];
                        state = SELECTING;
                    }
                    else if (backBtn.getGlobalBounds().contains(mfp)) {
                        state = MENU;
                    }
                }
                // MODULE SELECTION FOR DECOMPRESSION
                else if (state == MODULE_SELECTION_DECOMPRESS) {
                    if (textModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[0];
                    }
                    else if (audioModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[1];
                    }
                    else if (videoModuleBtn.getGlobalBounds().contains(mfp)) {
                        currentModule = modules[2];
                    }
                    else if (backBtn.getGlobalBounds().contains(mfp)) {
                        state = MENU;
                        continue; // Skip the decompression logic below
                    }

                    // IMPORTANT: If we clicked a module button (text/audio/video), 
                    // then perform decompression. Otherwise, skip.
                    if (textModuleBtn.getGlobalBounds().contains(mfp) ||
                        audioModuleBtn.getGlobalBounds().contains(mfp) ||
                        videoModuleBtn.getGlobalBounds().contains(mfp)) {

                        // Start decompression
                        string picked = openFileDialogWin("Huffman Compressed\0*.huff\0All Files\0*.*\0");
                        if (picked.size()) {
                            decompressInputPath = picked;
                            state = DECOMPRESSING;

                            // Determine output filename with appropriate extension
                            size_t lastSlash = decompressInputPath.find_last_of("\\/");
                            size_t lastDot = decompressInputPath.find_last_of(".");
                            if (lastSlash != string::npos && lastDot != string::npos && lastDot > lastSlash) {
                                string baseName = decompressInputPath.substr(lastSlash + 1, lastDot - lastSlash - 1);

                                // Use module-specific extension
                                switch (currentModule.type) {
                                case MODULE_TEXT:
                                    decompressOutputPath = baseName + "_decompressed.txt";
                                    break;
                                case MODULE_AUDIO:
                                    decompressOutputPath = baseName + "_decompressed.wav";
                                    break;
                                case MODULE_VIDEO:
                                    decompressOutputPath = baseName + "_decompressed.mp4";
                                    break;
                                default:
                                    decompressOutputPath = baseName + "_decompressed";
                                    break;
                                }
                            }
                            else {
                                // Fallback
                                decompressOutputPath = "decompressed_output";
                            }

                            // Read header and rebuild tree
                            ifstream in(decompressInputPath, ios::binary);
                            if (in) {
                                uint16_t uniq = 0;
                                in.read(reinterpret_cast<char*>(&uniq), sizeof(uniq));
                                uint64_t decompressFreqs[256] = { 0 };
                                unsigned char decompressBytesPresent[256] = { 0 };
                                for (int i = 0; i < uniq; i++) {
                                    uint8_t s; uint64_t f;
                                    in.read(reinterpret_cast<char*>(&s), sizeof(s));
                                    in.read(reinterpret_cast<char*>(&f), sizeof(f));
                                    decompressBytesPresent[s] = 1;
                                    decompressFreqs[s] = f;
                                }
                                in.close();

                                unsigned char bytesList[256];
                                int uniqueCount = 0;
                                for (int i = 0; i < 256; i++)
                                    if (decompressBytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

                                if (decompressRoot) {
                                    freeTree(decompressRoot);
                                    decompressRoot = nullptr;
                                }
                                decompressRoot = buildHuffmanTree(bytesList, decompressFreqs, uniqueCount);

                                if (decompressRoot) {
                                    decompressVizCount = 0;
                                    int curX = 0;
                                    assignPositionsInorder(decompressRoot, curX, 0, decompressViz, decompressVizCount);
                                    int maxDepth = 0, maxX = 0;
                                    for (int i = 0; i < decompressVizCount; i++) {
                                        if (decompressViz[i].depth > maxDepth) maxDepth = decompressViz[i].depth;
                                        if (decompressViz[i].x > maxX) maxX = decompressViz[i].x;
                                    }
                                    float minSpacingX = 30.f, minSpacingY = 120.f;
                                    totalTreeWidth = (maxX + 1) * minSpacingX;
                                    totalTreeHeight = (maxDepth + 1) * minSpacingY;
                                    float offsetX = 50.f, offsetY = 50.f;
                                    for (int i = 0; i < decompressVizCount; i++) {
                                        decompressViz[i].screenX = offsetX + decompressViz[i].x * minSpacingX;
                                        decompressViz[i].screenY = offsetY + decompressViz[i].depth * minSpacingY;
                                    }
                                    maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                                    maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                                    scrollX = scrollY = 0.0f;
                                    zoomLevel = 1.0f;
                                }
                            }

                            decompressSuccess = readCompressedAndDecode(decompressInputPath, decompressOutputPath);
                            if (decompressSuccess) {
                                ifstream fin(decompressOutputPath, ios::binary | ios::ate);
                                if (fin) {
                                    decompressedBytes = (uint64_t)fin.tellg();
                                    fin.close();
                                }
                                state = DECOMPRESS_RESULT;
                            }
                            else {
                                statusTxt.setString("Decompression failed!");
                                state = MENU;
                            }
                        }
                    }
                }

                // PART 5: Compression Event Handling
 // Continues from Part 4

                 // SELECTING state - file selection for compression
                else if (state == SELECTING) {
                     if (selectBtn.getGlobalBounds().contains(mfp)) {
                         string picked = openFileDialogWin(currentModule.fileFilter.c_str());
                         if (picked.size()) {
                             inputPath = picked;
                             state = PROCESSING;

                             ifstream fin(inputPath, ios::binary);
                             if (!fin) {
                                 statusTxt.setString("Error reading file.");
                                 state = SELECTING;
                                 break;
                             }
                             string text((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
                             fin.close();
                             origBytes = text.size();

                             uint64_t freqs[256] = { 0 };
                             unsigned char bytesList[256];
                             unsigned char bytesPresent[256] = { 0 };
                             for (size_t i = 0; i < text.size(); ++i) {
                                 unsigned char c = (unsigned char)text[i];
                                 freqs[c]++;
                                 bytesPresent[c] = 1;
                             }
                             int uniqueCount = 0;
                             for (int i = 0; i < 256; i++)
                                 if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

                             HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
                             if (!root) {
                                 statusTxt.setString("File empty or unreadable.");
                                 state = SELECTING;
                                 break;
                             }

                             unordered_map<unsigned char, string> codeMap;
                             storeCodesHashMap(root, codeMap);
                             writeCompressedText(text, compressedPath, codeMap, bytesPresent, freqs);

                             compBytes = 0;
                             {
                                 ifstream fin2(compressedPath, ios::binary | ios::ate);
                                 if (fin2) compBytes = (uint64_t)fin2.tellg();
                             }
                             ratio = (origBytes > 0) ? ((double)compBytes / (double)origBytes) : 0.0;

                             vizCount = 0;
                             int curX = 0;
                             assignPositionsInorder(root, curX, 0, viz, vizCount);

                             int maxDepth = 0;
                             int maxX = 0;
                             for (int i = 0; i < vizCount; i++) {
                                 if (viz[i].depth > maxDepth) maxDepth = viz[i].depth;
                                 if (viz[i].x > maxX) maxX = viz[i].x;
                             }

                             float minSpacingX = 30.f;
                             float minSpacingY = 120.f;
                             totalTreeWidth = (maxX + 1) * minSpacingX;
                             totalTreeHeight = (maxDepth + 1) * minSpacingY;
                             float offsetX = 50.f;
                             float offsetY = 50.f;

                             for (int i = 0; i < vizCount; i++) {
                                 viz[i].screenX = offsetX + viz[i].x * minSpacingX;
                                 viz[i].screenY = offsetY + viz[i].depth * minSpacingY;
                             }

                             nodeRadius = 22.f;
                             maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                             maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                             scrollX = 0.0f;
                             scrollY = 0.0f;
                             zoomLevel = 1.0f;

                             processed = true;
                             if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                             savedRoot = root;

                             state = SHOW_RESULT;
                         }
                     }
                     }
                     // SHOW_RESULT state - compression results
                else if (state == SHOW_RESULT) {
                         if (zoomInBtn.getGlobalBounds().contains(mfp)) {
                             zoomLevel *= 1.2f;
                             if (zoomLevel > 5.0f) zoomLevel = 5.0f;
                             maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                             maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                         }
                         else if (zoomOutBtn.getGlobalBounds().contains(mfp)) {
                             zoomLevel *= 0.8f;
                             if (zoomLevel < 0.1f) zoomLevel = 0.1f;
                             maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                             maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                         }
                         else if (resetViewBtn.getGlobalBounds().contains(mfp)) {
                             zoomLevel = 1.0f;
                             scrollX = 0.0f;
                             scrollY = 0.0f;
                             maxScrollX = max(0.0f, totalTreeWidth - 760.f);
                             maxScrollY = max(0.0f, totalTreeHeight - 640.f);
                         }
                         else if (saveCompressedBtn.getGlobalBounds().contains(mfp)) {
                             saveStatusTxt.setString("");
                             string suggestedName = "compressed.huff";
                             if (!inputPath.empty()) {
                                 size_t lastSlash = inputPath.find_last_of("\\/");
                                 size_t lastDot = inputPath.find_last_of(".");
                                 if (lastSlash != string::npos && lastDot != string::npos && lastDot > lastSlash) {
                                     string baseName = inputPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
                                     suggestedName = baseName + ".huff";
                                 }
                             }
                             string savePath = saveFileDialogWin("Huffman Compressed\0*.huff\0All Files\0*.*\0",
                                 suggestedName.c_str());
                             if (!savePath.empty()) {
                                 ifstream src("output.huff", ios::binary);
                                 ofstream dst(savePath, ios::binary);
                                 if (src && dst) {
                                     dst << src.rdbuf();
                                     src.close();
                                     dst.close();
                                     saveStatusTxt.setString("Saved successfully!");
                                     saveStatusTxt.setFillColor(sf::Color(0, 120, 0));
                                     compressedPath = savePath;
                                 }
                                 else {
                                     saveStatusTxt.setString("Error: Could not save.");
                                     saveStatusTxt.setFillColor(sf::Color(200, 0, 0));
                                 }
                             }
                         }
                         }

                         // PART 6: Continue Event Handling

                   // DECOMPRESS_RESULT state
                else if (state == DECOMPRESS_RESULT) {
                    if (saveDecompressedBtn.getGlobalBounds().contains(mfp)) {
                        string defaultName = "decompressed.txt";
                        if (!decompressInputPath.empty()) {
                            size_t lastSlash = decompressInputPath.find_last_of("\\/");
                            size_t lastDot = decompressInputPath.find_last_of(".");
                            if (lastSlash != string::npos && lastDot != string::npos && lastDot > lastSlash) {
                                string baseName = decompressInputPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
                                defaultName = baseName + "_decompressed.txt";
                            }
                        }
                        string savePath = saveFileDialogWin("All Files\0*.*\0", defaultName.c_str());
                        if (!savePath.empty()) {
                            ifstream src(decompressOutputPath, ios::binary);
                            ofstream dst(savePath, ios::binary);
                            if (src && dst) {
                                dst << src.rdbuf();
                                src.close();
                                dst.close();
                                decompressSaveStatus.setString("Saved successfully!");
                                decompressSaveStatus.setFillColor(sf::Color(0, 120, 0));
                                decompressOutputPath = savePath;
                            }
                            else {
                                decompressSaveStatus.setString("Error: Could not save.");
                                decompressSaveStatus.setFillColor(sf::Color(200, 0, 0));
                            }
                        }
                    }
                    else if (backToMenuBtn.getGlobalBounds().contains(mfp)) {
                        state = MENU;
                    }
                    else if (showTreeBtn.getGlobalBounds().contains(mfp)) {
                        state = SHOW_DECOMPRESS_TREE;
                    }
}

// SHOW_DECOMPRESS_TREE state - similar to SHOW_RESULT but for decompression tree
                else if (state == SHOW_DECOMPRESS_TREE) {
                    if (zoomInBtn.getGlobalBounds().contains(mfp)) {
                        zoomLevel *= 1.2f;
                        if (zoomLevel > 5.0f) zoomLevel = 5.0f;
                        maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                        maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                    }
                    else if (zoomOutBtn.getGlobalBounds().contains(mfp)) {
                        zoomLevel *= 0.8f;
                        if (zoomLevel < 0.1f) zoomLevel = 0.1f;
                        maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                        maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                    }
                    else if (resetViewBtn.getGlobalBounds().contains(mfp)) {
                        zoomLevel = 1.0f;
                        scrollX = 0.0f;
                        scrollY = 0.0f;
                        maxScrollX = max(0.0f, totalTreeWidth - 760.f);
                        maxScrollY = max(0.0f, totalTreeHeight - 640.f);
                    }
                    else if (backToMenuBtn.getGlobalBounds().contains(mfp)) {
                        state = DECOMPRESS_RESULT;
                    }
                    }
} // End of mouse button pressed

// Mouse drag for panning (only in tree view states)
if ((state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE) && event.type == sf::Event::MouseButtonPressed) {
    sf::Vector2i mp = sf::Mouse::getPosition(window);
    sf::Vector2f mfp((float)mp.x, (float)mp.y);

    // Check if mouse is in tree view area and not on scrollbars or buttons
    if (mfp.x >= 10 && mfp.x <= 770 && mfp.y >= 10 && mfp.y <= 650 &&
        !hScrollThumb.getGlobalBounds().contains(mfp) &&
        !vScrollThumb.getGlobalBounds().contains(mfp) &&
        !zoomInBtn.getGlobalBounds().contains(mfp) &&
        !zoomOutBtn.getGlobalBounds().contains(mfp) &&
        !resetViewBtn.getGlobalBounds().contains(mfp)) {
        isDragging = true;
        lastMousePos = mp;
    }
}
if (event.type == sf::Event::MouseMoved && isDragging) {
    sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
    int deltaX = currentMousePos.x - lastMousePos.x;
    int deltaY = currentMousePos.y - lastMousePos.y;

    // Adjust scroll based on drag
    if (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE) {
        float dragFactor = 0.5f;
        scrollX -= (deltaX * dragFactor) / (maxScrollX * zoomLevel + 1.0f);
        scrollY -= (deltaY * dragFactor) / (maxScrollY * zoomLevel + 1.0f);
        scrollX = max(0.0f, min(1.0f, scrollX));
        scrollY = max(0.0f, min(1.0f, scrollY));
    }

    lastMousePos = currentMousePos;
}
} // End of event polling

// Update UI elements
zoomDisplay.setString("Zoom: " + to_string((int)(zoomLevel * 100)) + "%");
hScrollThumb.setPosition(15 + scrollX * (hScrollTrack.getSize().x - hScrollThumb.getSize().x), 653);
vScrollThumb.setPosition(758, 15 + scrollY * (vScrollTrack.getSize().y - vScrollThumb.getSize().y));

// Clear window
window.clear(sf::Color(240, 240, 245));

// Draw based on state
if (state == MENU) {
    window.draw(startBtn);
    window.draw(startTxt);
    window.draw(decompressBtn);
    window.draw(decompressTxt);

    // Draw title
    sf::Text title("Huffman Multi-Module Compressor", font, 28);
    title.setFillColor(sf::Color::Black);
    title.setPosition(250, 120);
    window.draw(title);

    sf::Text subtitle("Select an option below", font, 18);
    subtitle.setFillColor(sf::Color(80, 80, 80));
    subtitle.setPosition(400, 180);
    window.draw(subtitle);
}
else if (state == MODULE_SELECTION_COMPRESS) {
    sf::Text header("Select Module for Compression", font, 24);
    header.setFillColor(sf::Color::Black);
    header.setPosition(320, 150);
    window.draw(header);

    window.draw(textModuleBtn);
    sf::Text textTxt("Text\nFiles", font, 18);
    textTxt.setFillColor(sf::Color::Black);
    textTxt.setPosition(215, 270);
    window.draw(textTxt);

    window.draw(audioModuleBtn);
    sf::Text audioTxt("Audio\nFiles", font, 18);
    audioTxt.setFillColor(sf::Color::Black);
    audioTxt.setPosition(465, 270);
    window.draw(audioTxt);

    window.draw(videoModuleBtn);
    sf::Text videoTxt("Video\nFiles", font, 18);
    videoTxt.setFillColor(sf::Color::Black);
    videoTxt.setPosition(715, 270);
    window.draw(videoTxt);

    window.draw(backBtn);
    sf::Text backTxt("Back to Menu", font, 16);
    backTxt.setFillColor(sf::Color::Black);
    backTxt.setPosition(450, 460);
    window.draw(backTxt);
}
else if (state == MODULE_SELECTION_DECOMPRESS) {
    sf::Text header("Select Module for Decompression", font, 24);
    header.setFillColor(sf::Color::Black);
    header.setPosition(300, 150);
    window.draw(header);

    window.draw(textModuleBtn);
    sf::Text textTxt("Text\nFiles", font, 18);
    textTxt.setFillColor(sf::Color::Black);
    textTxt.setPosition(215, 270);
    window.draw(textTxt);

    window.draw(audioModuleBtn);
    sf::Text audioTxt("Audio\nFiles", font, 18);
    audioTxt.setFillColor(sf::Color::Black);
    audioTxt.setPosition(465, 270);
    window.draw(audioTxt);

    window.draw(videoModuleBtn);
    sf::Text videoTxt("Video\nFiles", font, 18);
    videoTxt.setFillColor(sf::Color::Black);
    videoTxt.setPosition(715, 270);
    window.draw(videoTxt);

    window.draw(backBtn);
    sf::Text backTxt("Back to Menu", font, 16);
    backTxt.setFillColor(sf::Color::Black);
    backTxt.setPosition(450, 460);
    window.draw(backTxt);
}
else if (state == SELECTING) {
    sf::RectangleShape bg(sf::Vector2f(600, 200));
    bg.setFillColor(sf::Color(255, 255, 255));
    bg.setOutlineColor(sf::Color(180, 180, 180));
    bg.setOutlineThickness(2.f);
    bg.setPosition(200, 200);
    window.draw(bg);

    sf::Text prompt("Selected Module: " + currentModule.name, font, 20);
    prompt.setFillColor(sf::Color::Black);
    prompt.setPosition(250, 220);
    window.draw(prompt);

    sf::Text desc("Click below to select a file", font, 16);
    desc.setFillColor(sf::Color(80, 80, 80));
    desc.setPosition(320, 260);
    window.draw(desc);

    window.draw(selectBtn);
    window.draw(selectTxt);

    window.draw(statusTxt);

    window.draw(backBtn);
    sf::Text backTxt("Back", font, 16);
    backTxt.setFillColor(sf::Color::Black);
    backTxt.setPosition(470, 460);
    window.draw(backTxt);
}
else if (state == SHOW_RESULT) {
    // Draw tree area background
    sf::RectangleShape treeArea(sf::Vector2f(760, 640));
    treeArea.setFillColor(sf::Color(255, 255, 255));
    treeArea.setOutlineColor(sf::Color(180, 180, 180));
    treeArea.setOutlineThickness(2.f);
    treeArea.setPosition(10, 10);
    window.draw(treeArea);

    // Draw tree with current module's labeling
    drawTreeSFML(window, viz, vizCount, nodeRadius, font, zoomLevel,
        scrollX, scrollY, maxScrollX, maxScrollY, currentModule);

    // Draw info panel on right
    sf::RectangleShape infoPanel(sf::Vector2f(220, 300));
    infoPanel.setFillColor(sf::Color(245, 245, 250));
    infoPanel.setOutlineColor(sf::Color(200, 200, 210));
    infoPanel.setOutlineThickness(2.f);
    infoPanel.setPosition(780, 10);
    window.draw(infoPanel);

    // Info text
    sf::Text infoTitle("Compression Results", font, 18);
    infoTitle.setFillColor(sf::Color::Black);
    infoTitle.setPosition(810, 20);
    window.draw(infoTitle);

    string moduleStr = "Module: " + currentModule.name;
    sf::Text moduleTxt(moduleStr, font, 14);
    moduleTxt.setFillColor(sf::Color(80, 80, 80));
    moduleTxt.setPosition(810, 60);
    window.draw(moduleTxt);

    string origStr = "Original size: " + to_string(origBytes) + " bytes";
    sf::Text origTxt(origStr, font, 14);
    origTxt.setFillColor(sf::Color::Black);
    origTxt.setPosition(810, 90);
    window.draw(origTxt);

    string compStr = "Compressed size: " + to_string(compBytes) + " bytes";
    sf::Text compTxt(compStr, font, 14);
    compTxt.setFillColor(sf::Color::Black);
    compTxt.setPosition(810, 115);
    window.draw(compTxt);

    string ratioStr = "Compression ratio: " + to_string(ratio * 100) + "%";
    sf::Text ratioTxt(ratioStr, font, 14);
    ratioTxt.setFillColor(sf::Color::Black);
    ratioTxt.setPosition(810, 140);
    window.draw(ratioTxt);

    string savedStr = "Space saved: " + to_string((int)((1.0 - ratio) * 100)) + "%";
    sf::Text savedTxt(savedStr, font, 14);
    savedTxt.setFillColor(sf::Color(0, 120, 0));
    savedTxt.setPosition(810, 165);
    window.draw(savedTxt);

    window.draw(saveCompressedBtn);
    window.draw(saveCompressedTxt);
    window.draw(saveStatusTxt);

    // Draw scrollbars
    if (maxScrollX > 0.0f) {
        window.draw(hScrollTrack);
        window.draw(hScrollThumb);
    }
    if (maxScrollY > 0.0f) {
        window.draw(vScrollTrack);
        window.draw(vScrollThumb);
    }

    // Draw zoom controls
    window.draw(zoomInBtn);
    window.draw(zoomInTxt);
    window.draw(zoomOutBtn);
    window.draw(zoomOutTxt);
    window.draw(resetViewBtn);
    window.draw(resetViewTxt);
    window.draw(zoomDisplay);
}
else if (state == DECOMPRESS_RESULT) {
    sf::RectangleShape bg(sf::Vector2f(700, 400));
    bg.setFillColor(sf::Color(255, 255, 255));
    bg.setOutlineColor(sf::Color(180, 180, 180));
    bg.setOutlineThickness(2.f);
    bg.setPosition(150, 150);
    window.draw(bg);

    sf::Text header("Decompression Complete", font, 24);
    header.setFillColor(sf::Color::Black);
    header.setPosition(300, 170);
    window.draw(header);

    sf::Text moduleTxt("Module: " + currentModule.name, font, 16);
    moduleTxt.setFillColor(sf::Color(80, 80, 80));
    moduleTxt.setPosition(200, 230);
    window.draw(moduleTxt);

    string inputStr = "Input file: " + decompressInputPath;
    sf::Text inputTxt(inputStr, font, 14);
    inputTxt.setFillColor(sf::Color::Black);
    inputTxt.setPosition(200, 260);
    window.draw(inputTxt);

    string outputStr = "Output file: " + decompressOutputPath;
    sf::Text outputTxt(outputStr, font, 14);
    outputTxt.setFillColor(sf::Color::Black);
    outputTxt.setPosition(200, 285);
    window.draw(outputTxt);

    string sizeStr = "Decompressed size: " + to_string(decompressedBytes) + " bytes";
    sf::Text sizeTxt(sizeStr, font, 14);
    sizeTxt.setFillColor(sf::Color::Black);
    sizeTxt.setPosition(200, 310);
    window.draw(sizeTxt);

    window.draw(saveDecompressedBtn);
    window.draw(saveDecompressedTxt);
    window.draw(backToMenuBtn);
    window.draw(backToMenuTxt);
    window.draw(showTreeBtn);
    window.draw(showTreeTxt);
    window.draw(decompressSaveStatus);
}
else if (state == SHOW_DECOMPRESS_TREE) {
    // Similar to SHOW_RESULT but for decompression tree
    sf::RectangleShape treeArea(sf::Vector2f(760, 640));
    treeArea.setFillColor(sf::Color(255, 255, 255));
    treeArea.setOutlineColor(sf::Color(180, 180, 180));
    treeArea.setOutlineThickness(2.f);
    treeArea.setPosition(10, 10);
    window.draw(treeArea);

    drawTreeSFML(window, decompressViz, decompressVizCount, nodeRadius, font, zoomLevel,
        scrollX, scrollY, maxScrollX, maxScrollY, currentModule);

    // Draw info panel
    sf::RectangleShape infoPanel(sf::Vector2f(220, 200));
    infoPanel.setFillColor(sf::Color(245, 245, 250));
    infoPanel.setOutlineColor(sf::Color(200, 200, 210));
    infoPanel.setOutlineThickness(2.f);
    infoPanel.setPosition(780, 10);
    window.draw(infoPanel);

    sf::Text infoTitle("Decompression Tree", font, 18);
    infoTitle.setFillColor(sf::Color::Black);
    infoTitle.setPosition(810, 20);
    window.draw(infoTitle);

    string moduleStr = "Module: " + currentModule.name;
    sf::Text moduleTxt(moduleStr, font, 14);
    moduleTxt.setFillColor(sf::Color(80, 80, 80));
    moduleTxt.setPosition(810, 60);
    window.draw(moduleTxt);

    string fileStr = "File: " + decompressInputPath;
    sf::Text fileTxt(fileStr, font, 12);
    fileTxt.setFillColor(sf::Color::Black);
    fileTxt.setPosition(810, 90);
    window.draw(fileTxt);

    // Draw scrollbars
    if (maxScrollX > 0.0f) {
        window.draw(hScrollTrack);
        window.draw(hScrollThumb);
    }
    if (maxScrollY > 0.0f) {
        window.draw(vScrollTrack);
        window.draw(vScrollThumb);
    }

    // Draw zoom controls
    window.draw(zoomInBtn);
    window.draw(zoomInTxt);
    window.draw(zoomOutBtn);
    window.draw(zoomOutTxt);
    window.draw(resetViewBtn);
    window.draw(resetViewTxt);
    window.draw(zoomDisplay);

    // Back button
    window.draw(backToMenuBtn);
    window.draw(backToMenuTxt);
}

// Display everything
window.display();
} // End of main loop

// Cleanup
if (savedRoot) {
    freeTree(savedRoot);
    savedRoot = nullptr;
}
if (decompressRoot) {
    freeTree(decompressRoot);
    decompressRoot = nullptr;
}

return 0;
}
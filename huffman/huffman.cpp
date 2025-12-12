// main.cpp - Complete Huffman Multi-Module Compressor
// Build: C++17, SFML 2.6.x (link sfml-graphics, sfml-window, sfml-system)
////CLASS: BSCS 14-A
//Group Members:
//Syed Abbas Raza (517626)
//Shuja Naveed (502379)
//github: https://github.com/abbas72O5/DSA_Project_Huffman

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

// MODULE CONFIGURATION
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
    {MODULE_AUDIO, "Audio Files", "Audio Files\0*.wav;.mp3;.flac;.aac\0All Files\0.*\0"},
    {MODULE_VIDEO, "Video Files", "Video Files\0*.mp4;.avi;.mkv;.mov\0All Files\0.*\0"}
};


// HUFFMAN CORE LOGIC

// Huffman node & BinaryHeap

class HuffmanNode {
public:
    unsigned char data;
    uint64_t freq;// using uint64 instead of int because frequencies can be much larger than 2 billion (max int range)
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(unsigned char data, uint64_t freq) {
        this->data = data;
        this->freq = freq;
        left = right = nullptr;
    }

    HuffmanNode(HuffmanNode* l, HuffmanNode* r) {
        data = 0; // internal node
        freq = l->freq + r->freq;
        left = l;
        right = r;
    }

    bool isLeaf()
    {
        return left == nullptr && right == nullptr;
    }
};

class BinaryHeap {
    HuffmanNode** arr;//pointer to array of pointers to HuffmanNodes
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
        arr = new HuffmanNode * [capacity + 2]; // 1-based indexing
        rear = 0;
        h = -1;
    }

    void updateHeight() {
        if (rear == 0) h = -1;
        else h = (int)floor(log2((double)rear));
    }

    bool isEmpty()
    {
        return rear == 0;
    }
    int size()
    {
        return rear;
    }
    int getHeight()
    {
        return h;
    }

    HuffmanNode* top() { // min element
        if (rear == 0) return nullptr;
        return arr[1];
    }

    void push(HuffmanNode* node) { //pushing new node into the binary heap
        if (rear + 1 >= capacity) return;
        arr[++rear] = node;
        int i = rear;
        while (i > 1 && arr[i]->freq < arr[i / 2]->freq) {//heapify up
            swapNodes(i, i / 2);
            i /= 2;
        }
        updateHeight();
    }

    HuffmanNode* pop() {
        if (rear == 0) return nullptr;
        HuffmanNode* minNode = arr[1]; //root node popped 
        arr[1] = arr[rear--];// move last node to root
        int i = 1;
        while (true) {//heapify down
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

/*
      Fuctional Module 1: Frequency & Tree
*/
HuffmanNode* buildHuffmanTree(unsigned char bytes[], uint64_t freqs[], int uniqueCount) {
    BinaryHeap minHeap(uniqueCount + 5); // create minheap with extra space
    for (int i = 0; i < uniqueCount; ++i) {
        if (freqs[bytes[i]] > 0) {
            minHeap.push(new HuffmanNode(bytes[i], freqs[bytes[i]]));
        }
    }

    if (minHeap.size() == 0) return nullptr;
    // edge-case: only one unique symbol: create dummy sibling
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
// storing codes using hashmaps for faster O(1) lookup
void storeCodesHashMap(HuffmanNode* root, unordered_map<unsigned char, string>& codeMap, string path = "") {
    if (!root) return;

    if (root->isLeaf()) {
        codeMap[root->data] = path;// Direct insertion - O(1)
        return;
    }

    storeCodesHashMap(root->left, codeMap, path + "0");
    storeCodesHashMap(root->right, codeMap, path + "1");
}

/*
 Functional Module 2: Encoding/Decoding & File I/O
 */

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
    /* writing header information in output file
       1. number of unique symbols (2 bytes)
       2. for each unique symbol: symbol (1 byte) + frequency (8 bytes)
       3. total bits in compressed data (8 bytes)
    */

    /*
    * For small files, the header may take up a significant portion of the compressed file.
    * This causes the compressed file to be larger than the original.
    * This is a known limitation of this simple implementation.
    */

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
    //writing compressed data in the form of bits packed into bytes in output file
    // pack bits into bytes (MSB-first) - WITH O(1) LOOKUP using hashmap!
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
    /*Reading header information:
    * including number of unique symbols, their frequencies, and total bits
    * This information is used to reconstruct the Huffman tree for decoding
    */
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
    // collect bytes list
    unsigned char bytesList[256];
    int uniqueCount = 0;
    for (int i = 0; i < 256; i++) if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

    HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
    if (!root) return false;

    ofstream out(outPath, ios::binary);
    if (!out) return false;
    // decoding bits into original symbols using the Huffman tree
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

/*
 Functional Module 3: Tree layout & SFML visualization
 */
struct VizNode {//tree visualization node
    HuffmanNode* n;
    int x;
    int depth;
    float screenX, screenY;
};
// assigning x positions using inorder traversal
void assignPositionsInorder(HuffmanNode* root, int& currentX, int depth, VizNode viz[], int& idx) {
    if (!root) return;
    assignPositionsInorder(root->left, currentX, depth + 1, viz, idx);
    viz[idx].n = root;
    viz[idx].x = currentX++;
    viz[idx].depth = depth;
    ++idx;
    assignPositionsInorder(root->right, currentX, depth + 1, viz, idx);
}
// Draw function with scrollbar support
void drawTreeSFML(sf::RenderWindow& window, VizNode viz[], int vizCount,
    float nodeRadius, sf::Font& font,
    float zoomLevel, float scrollX, float scrollY,
    float maxScrollX, float maxScrollY, const ModuleConfig& module) {

    // Viewport dimensions for the tree area (white box)
    // Matches the size used in SHOW_RESULT and SHOW_DECOMPRESS_TREE: 740x600
    float viewportWidth = 740.f;
    float viewportHeight = 600.f;

    // Position of the viewport on screen (x=10, y=50)
    float viewportX = 10.f;
    float viewportY = 50.f;

    sf::View treeView(sf::FloatRect(0, 0, viewportWidth / zoomLevel, viewportHeight / zoomLevel));

    // SFML Viewport uses normalized coordinates (0..1) relative to window size (1000x700)
    treeView.setViewport(sf::FloatRect(viewportX / 1000.f, viewportY / 700.f, viewportWidth / 1000.f, viewportHeight / 700.f));

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
        // Different colors for leaf vs internal nodes
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
// native Windows file dialog
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

// GUI CLASS (Simple Button Helper)
class Button {
public:
    sf::RectangleShape shape;
    sf::Text text;
    sf::Color idleColor;
    sf::Color hoverColor;
    bool visible;

    Button(sf::Vector2f size, sf::Vector2f position, string btnText, sf::Font& font, sf::Color idle, sf::Color hover) {
        shape.setSize(size);
        shape.setPosition(position);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color(200, 200, 200));
        idleColor = idle;
        hoverColor = hover;
        shape.setFillColor(idleColor);
        visible = true;

        text.setFont(font);
        text.setString(btnText);
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::White);
        text.setStyle(sf::Text::Bold);

        centerText();
    }

    void centerText() {
        sf::FloatRect bounds = text.getLocalBounds();
        text.setOrigin(bounds.left + bounds.width / 2.0f, bounds.top + bounds.height / 2.0f);
        text.setPosition(shape.getPosition().x + shape.getSize().x / 2.0f, shape.getPosition().y + shape.getSize().y / 2.0f);
    }

    void setPosition(float x, float y) {
        shape.setPosition(x, y);
        centerText();
    }

    void update(sf::Vector2f mousePos) {
        if (!visible) return;
        if (shape.getGlobalBounds().contains(mousePos)) {
            shape.setFillColor(hoverColor);
            shape.setOutlineColor(sf::Color::White);
        }
        else {
            shape.setFillColor(idleColor);
            shape.setOutlineColor(sf::Color(200, 200, 200));
        }
    }

    void draw(sf::RenderWindow& window) {
        if (!visible) return;
        window.draw(shape);
        window.draw(text);
    }

    bool isClicked(sf::Vector2f mousePos) {
        return visible && shape.getGlobalBounds().contains(mousePos);
    }
};

/*
Main: SFML application + integration
 */
int main() {
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Huffman Multi-Module Compressor");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        // Fallback or just print error, application might crash if font missing
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

    // COLORS
    sf::Color bgCol(30, 30, 35);
    sf::Color primaryCol(0, 122, 204);
    sf::Color primaryHover(28, 151, 234);
    sf::Color dangerCol(204, 0, 0);
    sf::Color dangerHover(234, 28, 28);
    sf::Color successCol(40, 167, 69);
    sf::Color successHover(60, 187, 89);
    sf::Color neutralCol(108, 117, 125);
    sf::Color neutralHover(128, 137, 145);

    // BUTTONS
    Button startBtn({ 280, 60 }, { 360, 220 }, "Compress Files", font, primaryCol, primaryHover);
    Button decompressBtn({ 280, 60 }, { 360, 300 }, "Decompress Files", font, successCol, successHover);

    // Module Selection
    Button textModuleBtn({ 200, 80 }, { 150, 250 }, "Text Files", font, primaryCol, primaryHover);
    Button audioModuleBtn({ 200, 80 }, { 400, 250 }, "Audio Files", font, dangerCol, dangerHover);
    Button videoModuleBtn({ 200, 80 }, { 650, 250 }, "Video Files", font, successCol, successHover);

    // Navigation
    Button backBtn({ 150, 40 }, { 425, 450 }, "Back", font, neutralCol, neutralHover);
    Button selectBtn({ 220, 50 }, { 60, 580 }, "Select File", font, primaryCol, primaryHover);

    // Result View Buttons
    Button saveCompressedBtn({ 220, 40 }, { 770, 320 }, "Save Compressed", font, successCol, successHover);
    Button backToMenuBtn({ 180, 40 }, { 800, 640 }, "Main Menu", font, neutralCol, neutralHover);
    Button showTreeBtn({ 220, 40 }, { 270, 560 }, "Show Tree", font, primaryCol, primaryHover);

    // Decompress View Buttons
    Button saveDecompressedBtn({ 220, 40 }, { 220, 550 }, "Save Output", font, successCol, successHover);
    Button backToMenuSmallBtn({ 180, 40 }, { 460, 550 }, "Back to Menu", font, neutralCol, neutralHover);

    // Zoom Controls
    Button zoomInBtn({ 40, 30 }, { 15, 660 }, "+", font, neutralCol, neutralHover);
    Button zoomOutBtn({ 40, 30 }, { 65, 660 }, "-", font, neutralCol, neutralHover);
    Button resetViewBtn({ 80, 30 }, { 115, 660 }, "Reset", font, neutralCol, neutralHover);

    // Text labels
    sf::Text statusTxt("", font, 16);
    statusTxt.setFillColor(sf::Color::White);
    statusTxt.setPosition(60, 530);

    sf::Text titleTxt("Huffman Compressor", font, 32);
    titleTxt.setFillColor(sf::Color::White);
    titleTxt.setStyle(sf::Text::Bold);
    titleTxt.setPosition(340, 50);

    // Compression/decompression storage
    string inputPath;
    string compressedPath = "output.huff";
    uint64_t origBytes = 0, compBytes = 0;
    double ratio = 0.0;
    bool processed = false;

    string decompressInputPath;
    string decompressOutputPath = "decompressed.txt";
    uint64_t decompressedBytes = 0;
    uint64_t decompressInputBytes = 0; // Added for stats
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
    hScrollTrack.setFillColor(sf::Color(60, 60, 65));
    hScrollTrack.setOutlineThickness(1.f);
    hScrollTrack.setOutlineColor(sf::Color(100, 100, 100));

    sf::RectangleShape hScrollThumb(sf::Vector2f(100, 12));
    hScrollThumb.setFillColor(sf::Color(100, 150, 200));

    sf::RectangleShape vScrollTrack(sf::Vector2f(12, 620));
    vScrollTrack.setPosition(758, 15);
    vScrollTrack.setFillColor(sf::Color(60, 60, 65));
    vScrollTrack.setOutlineThickness(1.f);
    vScrollTrack.setOutlineColor(sf::Color(100, 100, 100));

    sf::RectangleShape vScrollThumb(sf::Vector2f(12, 100));
    vScrollThumb.setFillColor(sf::Color(100, 150, 200));

    sf::Text zoomDisplay("", font, 14);
    zoomDisplay.setFillColor(sf::Color::White);
    zoomDisplay.setPosition(220, 668);

    sf::Text saveStatusTxt("", font, 14);
    saveStatusTxt.setFillColor(sf::Color::Green);
    saveStatusTxt.setPosition(770, 370);

    // Main event loop
    while (window.isOpen()) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

        // Update Button Hover States
        startBtn.update(mousePos);
        decompressBtn.update(mousePos);
        textModuleBtn.update(mousePos);
        audioModuleBtn.update(mousePos);
        videoModuleBtn.update(mousePos);
        backBtn.update(mousePos);
        selectBtn.update(mousePos);
        saveCompressedBtn.update(mousePos);
        backToMenuBtn.update(mousePos);
        showTreeBtn.update(mousePos);
        saveDecompressedBtn.update(mousePos);
        backToMenuSmallBtn.update(mousePos);
        zoomInBtn.update(mousePos);
        zoomOutBtn.update(mousePos);
        resetViewBtn.update(mousePos);

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                if (decompressRoot) { freeTree(decompressRoot); decompressRoot = nullptr; }
                window.close();
            }

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
                if (hScrollThumb.getGlobalBounds().contains(mousePos)) isDraggingHScroll = true;
                else if (vScrollThumb.getGlobalBounds().contains(mousePos)) isDraggingVScroll = true;
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                isDraggingHScroll = false;
                isDraggingVScroll = false;
            }

            if (event.type == sf::Event::MouseMoved && (state == SHOW_RESULT || state == SHOW_DECOMPRESS_TREE)) {
                if (isDraggingHScroll) {
                    float trackLeft = hScrollTrack.getPosition().x;
                    float trackWidth = hScrollTrack.getSize().x;
                    float thumbWidth = hScrollThumb.getSize().x;
                    float newX = mousePos.x - thumbWidth / 2.f;
                    newX = max(trackLeft, min(newX, trackLeft + trackWidth - thumbWidth));
                    scrollX = (newX - trackLeft) / (trackWidth - thumbWidth);
                    scrollX = max(0.0f, min(1.0f, scrollX));
                }
                else if (isDraggingVScroll) {
                    float trackTop = vScrollTrack.getPosition().y;
                    float trackHeight = vScrollTrack.getSize().y;
                    float thumbHeight = vScrollThumb.getSize().y;
                    float newY = mousePos.y - thumbHeight / 2.f;
                    newY = max(trackTop, min(newY, trackTop + trackHeight - thumbHeight));
                    scrollY = (newY - trackTop) / (trackHeight - thumbHeight);
                    scrollY = max(0.0f, min(1.0f, scrollY));
                }
            }

            // Button Click Handling
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

                // GLOBAL BACK/HOME (available in result screens)
                if ((state == SHOW_RESULT || state == DECOMPRESS_RESULT || state == SHOW_DECOMPRESS_TREE) && backToMenuBtn.isClicked(mousePos)) {
                    state = MENU;
                    continue;
                }

                if (state == MENU) {
                    if (startBtn.isClicked(mousePos)) state = MODULE_SELECTION_COMPRESS;
                    else if (decompressBtn.isClicked(mousePos)) state = MODULE_SELECTION_DECOMPRESS;
                }
                else if (state == MODULE_SELECTION_COMPRESS) {
                    if (backBtn.isClicked(mousePos)) state = MENU;
                    else if (textModuleBtn.isClicked(mousePos)) { currentModule = modules[0]; state = SELECTING; statusTxt.setString(""); }
                    else if (audioModuleBtn.isClicked(mousePos)) { currentModule = modules[1]; state = SELECTING; statusTxt.setString(""); }
                    else if (videoModuleBtn.isClicked(mousePos)) { currentModule = modules[2]; state = SELECTING; statusTxt.setString(""); }
                }
                else if (state == MODULE_SELECTION_DECOMPRESS) {
                    if (backBtn.isClicked(mousePos)) state = MENU;
                    else if (textModuleBtn.isClicked(mousePos) || audioModuleBtn.isClicked(mousePos) || videoModuleBtn.isClicked(mousePos)) {
                        if (textModuleBtn.isClicked(mousePos)) currentModule = modules[0];
                        if (audioModuleBtn.isClicked(mousePos)) currentModule = modules[1];
                        if (videoModuleBtn.isClicked(mousePos)) currentModule = modules[2];

                        // Decompression Logic
                        string picked = openFileDialogWin("Huffman Compressed\0*.huff\0All Files\0*.*\0");
                        if (picked.size()) {
                            decompressInputPath = picked;
                            // Measure input file size for stats
                            ifstream inSize(decompressInputPath, ios::binary | ios::ate);
                            if (inSize) {
                                decompressInputBytes = (uint64_t)inSize.tellg();
                                inSize.close();
                            }
                            else {
                                decompressInputBytes = 0;
                            }

                            state = DECOMPRESSING;
                            // ... (Decompression logic same as original, simplified for brevity in this view)
                            // NOTE: Copied core logic below for completeness
                            size_t lastSlash = decompressInputPath.find_last_of("\\/");
                            size_t lastDot = decompressInputPath.find_last_of(".");
                            string baseName = "output";
                            if (lastSlash != string::npos && lastDot != string::npos && lastDot > lastSlash)
                                baseName = decompressInputPath.substr(lastSlash + 1, lastDot - lastSlash - 1);

                            switch (currentModule.type) {
                            case MODULE_TEXT: decompressOutputPath = baseName + "_decompressed.txt"; break;
                            case MODULE_AUDIO: decompressOutputPath = baseName + "_decompressed.wav"; break;
                            case MODULE_VIDEO: decompressOutputPath = baseName + "_decompressed.mp4"; break;
                            default: decompressOutputPath = baseName + "_decompressed"; break;
                            }

                            ifstream in(decompressInputPath, ios::binary);
                            if (in) {
                                uint16_t uniq = 0; in.read(reinterpret_cast<char*>(&uniq), sizeof(uniq));
                                uint64_t decompressFreqs[256] = { 0 };
                                unsigned char decompressBytesPresent[256] = { 0 };
                                for (int i = 0; i < uniq; i++) {
                                    uint8_t s; uint64_t f;
                                    in.read(reinterpret_cast<char*>(&s), sizeof(s)); in.read(reinterpret_cast<char*>(&f), sizeof(f));
                                    decompressBytesPresent[s] = 1; decompressFreqs[s] = f;
                                }
                                in.close();
                                unsigned char bytesList[256]; int uniqueCount = 0;
                                for (int i = 0; i < 256; i++) if (decompressBytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;
                                if (decompressRoot) { freeTree(decompressRoot); decompressRoot = nullptr; }
                                decompressRoot = buildHuffmanTree(bytesList, decompressFreqs, uniqueCount);
                                if (decompressRoot) {
                                    decompressVizCount = 0; int curX = 0;
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
                                    scrollX = scrollY = 0.0f; zoomLevel = 1.0f;
                                }
                            }
                            decompressSuccess = readCompressedAndDecode(decompressInputPath, decompressOutputPath);
                            if (decompressSuccess) {
                                ifstream fin(decompressOutputPath, ios::binary | ios::ate);
                                if (fin) { decompressedBytes = (uint64_t)fin.tellg(); fin.close(); }
                                state = DECOMPRESS_RESULT;
                            }
                            else {
                                statusTxt.setString("Decompression failed!");
                                state = MENU;
                            }
                        }
                    }
                }
                else if (state == SELECTING) {
                    if (backBtn.isClicked(mousePos)) state = MODULE_SELECTION_COMPRESS;
                    else if (selectBtn.isClicked(mousePos)) {
                        string picked = openFileDialogWin(currentModule.fileFilter.c_str());
                        if (picked.size()) {
                            inputPath = picked;
                            state = PROCESSING; // Logic continues in main loop update

                            ifstream fin(inputPath, ios::binary);
                            if (!fin) { statusTxt.setString("Error reading file."); state = SELECTING; }
                            else {
                                string text((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
                                fin.close();
                                origBytes = text.size();
                                uint64_t freqs[256] = { 0 };
                                unsigned char bytesList[256];
                                unsigned char bytesPresent[256] = { 0 };
                                for (size_t i = 0; i < text.size(); ++i) {
                                    unsigned char c = (unsigned char)text[i];
                                    freqs[c]++; bytesPresent[c] = 1;
                                }
                                int uniqueCount = 0;
                                for (int i = 0; i < 256; i++) if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;
                                HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
                                if (!root) { statusTxt.setString("File empty or unreadable."); state = SELECTING; }
                                else {
                                    if (savedRoot) freeTree(savedRoot);
                                    savedRoot = root;
                                    unordered_map<unsigned char, string> codeMap;
                                    storeCodesHashMap(root, codeMap);
                                    writeCompressedText(text, compressedPath, codeMap, bytesPresent, freqs);

                                    // Viz setup
                                    vizCount = 0; int curX = 0;
                                    assignPositionsInorder(root, curX, 0, viz, vizCount);
                                    int maxDepth = 0, maxX = 0;
                                    for (int i = 0; i < vizCount; i++) {
                                        if (viz[i].depth > maxDepth) maxDepth = viz[i].depth;
                                        if (viz[i].x > maxX) maxX = viz[i].x;
                                    }
                                    float minSpacingX = 30.f, minSpacingY = 120.f;
                                    totalTreeWidth = (maxX + 1) * minSpacingX;
                                    totalTreeHeight = (maxDepth + 1) * minSpacingY;
                                    float offsetX = 50.f, offsetY = 50.f;
                                    for (int i = 0; i < vizCount; i++) {
                                        viz[i].screenX = offsetX + viz[i].x * minSpacingX;
                                        viz[i].screenY = offsetY + viz[i].depth * minSpacingY;
                                    }
                                    maxScrollX = max(0.0f, totalTreeWidth - 760.f / zoomLevel);
                                    maxScrollY = max(0.0f, totalTreeHeight - 640.f / zoomLevel);
                                    scrollX = scrollY = 0.0f; zoomLevel = 1.0f;

                                    ifstream cfin(compressedPath, ios::binary | ios::ate);
                                    compBytes = (uint64_t)cfin.tellg(); cfin.close();
                                    ratio = 100.0 * (1.0 - (double)compBytes / (double)origBytes);
                                    state = SHOW_RESULT;
                                }
                            }
                        }
                    }
                }
                else if (state == SHOW_RESULT) {
                    if (backToMenuBtn.isClicked(mousePos)) state = MENU;
                    else if (showTreeBtn.isClicked(mousePos)) { /* Toggle tree view or stays in same state but draws differently? */ }
                    else if (saveCompressedBtn.isClicked(mousePos)) {
                        string savePath = saveFileDialogWin("Huffman Compressed\0*.huff\0All Files\0*.*\0");
                        if (savePath.size()) {
                            // Copy file
                            ifstream src(compressedPath, ios::binary);
                            ofstream dst(savePath, ios::binary);
                            dst << src.rdbuf();
                            saveStatusTxt.setString("Saved to " + savePath);
                        }
                    }
                }
                else if (state == DECOMPRESS_RESULT) {
                    if (backToMenuSmallBtn.isClicked(mousePos)) state = MENU;
                    else if (showTreeBtn.isClicked(mousePos)) state = SHOW_DECOMPRESS_TREE;
                    else if (saveDecompressedBtn.isClicked(mousePos)) {
                        string savePath = saveFileDialogWin("All Files\0*.*\0");
                        if (savePath.size()) {
                            ifstream src(decompressOutputPath, ios::binary);
                            ofstream dst(savePath, ios::binary);
                            dst << src.rdbuf();
                            saveStatusTxt.setString("Saved!");
                        }
                    }
                }
                else if (state == SHOW_DECOMPRESS_TREE) {
                    if (backToMenuBtn.isClicked(mousePos)) state = DECOMPRESS_RESULT; // Go back to result view first
                    else if (zoomInBtn.isClicked(mousePos)) zoomLevel = min(5.0f, zoomLevel * 1.1f);
                    else if (zoomOutBtn.isClicked(mousePos)) zoomLevel = max(0.1f, zoomLevel * 0.9f);
                    else if (resetViewBtn.isClicked(mousePos)) { zoomLevel = 1.0f; scrollX = 0; scrollY = 0; }
                }
            }
        }

        // RENDER
        window.clear(bgCol);

        if (state == MENU) {
            titleTxt.setString("Huffman Multi-Module Compressor");
            titleTxt.setPosition(260, 100);
            window.draw(titleTxt);
            startBtn.draw(window);
            decompressBtn.draw(window);
        }
        else if (state == MODULE_SELECTION_COMPRESS || state == MODULE_SELECTION_DECOMPRESS) {
            titleTxt.setString(state == MODULE_SELECTION_COMPRESS ? "Select Module to Compress" : "Select Module to Decompress");
            titleTxt.setPosition(280, 100);
            window.draw(titleTxt);
            textModuleBtn.draw(window);
            audioModuleBtn.draw(window);
            videoModuleBtn.draw(window);
            backBtn.draw(window);
        }
        else if (state == SELECTING) {
            titleTxt.setString("Select " + currentModule.name);
            titleTxt.setPosition(350, 100);
            window.draw(titleTxt);
            statusTxt.setPosition(350, 400);
            window.draw(statusTxt);
            selectBtn.setPosition(390, 300);
            selectBtn.draw(window);
            backBtn.setPosition(425, 450);
            backBtn.draw(window);
        }
        else if (state == SHOW_RESULT) {
            // Layout: Tree on Left/Center, Info Panel on Right

            // Draw Tree Area
            sf::RectangleShape treeArea(sf::Vector2f(740, 600));
            treeArea.setFillColor(sf::Color::White);
            treeArea.setPosition(10, 50);
            window.draw(treeArea);

            drawTreeSFML(window, viz, vizCount, nodeRadius, font, zoomLevel, scrollX, scrollY, maxScrollX, maxScrollY, currentModule);

            // Side Panel
            sf::RectangleShape sidePanel(sf::Vector2f(240, 600));
            sidePanel.setFillColor(sf::Color(50, 50, 55));
            sidePanel.setPosition(760, 50);
            window.draw(sidePanel);

            // Stats
            sf::Text stats("Statistics", font, 20);
            stats.setPosition(780, 70); stats.setStyle(sf::Text::Bold);
            window.draw(stats);

            sf::Text detail("Original: " + to_string(origBytes) + " B\nCompressed: " + to_string(compBytes) + " B\nRatio: " + to_string(ratio).substr(0, 4) + "%", font, 14);
            detail.setPosition(780, 120);
            window.draw(detail);

            saveCompressedBtn.setPosition(770, 250);
            saveCompressedBtn.draw(window);
            window.draw(saveStatusTxt);

            backToMenuBtn.setPosition(770, 580);
            backToMenuBtn.draw(window);

            // Zoom controls
            zoomInBtn.setPosition(20, 660); zoomInBtn.draw(window);
            zoomOutBtn.setPosition(70, 660); zoomOutBtn.draw(window);
            resetViewBtn.setPosition(120, 660); resetViewBtn.draw(window);

            // Scrollbars
            if (maxScrollX > 0.0f) {
                float thumbWidth = hScrollTrack.getSize().x * (hScrollTrack.getSize().x / totalTreeWidth);
                if (thumbWidth < 30) thumbWidth = 30;
                hScrollThumb.setSize(sf::Vector2f(thumbWidth, 12));
                hScrollThumb.setPosition(hScrollTrack.getPosition().x + scrollX * (hScrollTrack.getSize().x - thumbWidth), hScrollTrack.getPosition().y);
                window.draw(hScrollTrack); window.draw(hScrollThumb);
            }
            if (maxScrollY > 0.0f) {
                float thumbHeight = vScrollTrack.getSize().y * (vScrollTrack.getSize().y / totalTreeHeight);
                if (thumbHeight < 30) thumbHeight = 30;
                vScrollThumb.setSize(sf::Vector2f(12, thumbHeight));
                vScrollThumb.setPosition(vScrollTrack.getPosition().x, vScrollTrack.getPosition().y + scrollY * (vScrollTrack.getSize().y - thumbHeight));
                window.draw(vScrollTrack); window.draw(vScrollThumb);
            }
        }
        else if (state == DECOMPRESS_RESULT) {
            titleTxt.setString("Decompression Successful!");
            titleTxt.setPosition(300, 100);
            window.draw(titleTxt);

            // Stats Panel for Decompression
            sf::RectangleShape statsPanel(sf::Vector2f(400, 200));
            statsPanel.setFillColor(sf::Color(50, 50, 55));
            statsPanel.setOutlineColor(sf::Color(100, 100, 100));
            statsPanel.setOutlineThickness(2.f);
            statsPanel.setPosition(300, 180);
            window.draw(statsPanel);

            sf::Text statsTitle("Decompression Statistics", font, 20);
            statsTitle.setStyle(sf::Text::Bold);
            statsTitle.setPosition(320, 200);
            window.draw(statsTitle);

            string statStr = "Compressed Size: " + to_string(decompressInputBytes) + " Bytes\n";
            statStr += "Decompressed Size: " + to_string(decompressedBytes) + " Bytes\n\n";
            statStr += "File: " + decompressOutputPath;

            sf::Text statsBody(statStr, font, 16);
            statsBody.setPosition(320, 240);
            window.draw(statsBody);

            saveDecompressedBtn.draw(window);
            showTreeBtn.setPosition(460, 450); showTreeBtn.draw(window);
            backToMenuSmallBtn.draw(window);
            window.draw(saveStatusTxt);
        }
        else if (state == SHOW_DECOMPRESS_TREE) {
            // Layout: Tree on Left/Center, Info Panel on Right (Consistent with SHOW_RESULT)

            // Draw Tree Area
            sf::RectangleShape treeArea(sf::Vector2f(740, 600));
            treeArea.setFillColor(sf::Color::White);
            treeArea.setPosition(10, 50);
            window.draw(treeArea);

            drawTreeSFML(window, decompressViz, decompressVizCount, nodeRadius, font, zoomLevel, scrollX, scrollY, maxScrollX, maxScrollY, currentModule);

            // Side Panel
            sf::RectangleShape sidePanel(sf::Vector2f(240, 600));
            sidePanel.setFillColor(sf::Color(50, 50, 55));
            sidePanel.setPosition(760, 50);
            window.draw(sidePanel);

            // Stats in side panel
            sf::Text stats("Info", font, 20);
            stats.setPosition(780, 70); stats.setStyle(sf::Text::Bold);
            window.draw(stats);

            sf::Text detail("Viewing Decompression Tree\n\nScroll/Zoom to navigate.", font, 14);
            detail.setPosition(780, 120);
            window.draw(detail);

            backToMenuBtn.setPosition(770, 580);
            backToMenuBtn.draw(window);

            // Zoom controls
            zoomInBtn.setPosition(20, 660); zoomInBtn.draw(window);
            zoomOutBtn.setPosition(70, 660); zoomOutBtn.draw(window);
            resetViewBtn.setPosition(120, 660); resetViewBtn.draw(window);

            // Scrollbars (logic reuse)
            if (maxScrollX > 0.0f) {
                float thumbWidth = hScrollTrack.getSize().x * (hScrollTrack.getSize().x / totalTreeWidth);
                if (thumbWidth < 30) thumbWidth = 30;
                hScrollThumb.setSize(sf::Vector2f(thumbWidth, 12));
                hScrollThumb.setPosition(hScrollTrack.getPosition().x + scrollX * (hScrollTrack.getSize().x - thumbWidth), hScrollTrack.getPosition().y);
                window.draw(hScrollTrack); window.draw(hScrollThumb);
            }
            if (maxScrollY > 0.0f) {
                float thumbHeight = vScrollTrack.getSize().y * (vScrollTrack.getSize().y / totalTreeHeight);
                if (thumbHeight < 30) thumbHeight = 30;
                vScrollThumb.setSize(sf::Vector2f(12, thumbHeight));
                vScrollThumb.setPosition(vScrollTrack.getPosition().x, vScrollTrack.getPosition().y + scrollY * (vScrollTrack.getSize().y - thumbHeight));
                window.draw(vScrollTrack); window.draw(vScrollThumb);
            }
        }

        window.display();
    }

    if (savedRoot) { freeTree(savedRoot); }
    if (decompressRoot) { freeTree(decompressRoot); }

    return 0;
}
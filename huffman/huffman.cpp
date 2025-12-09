// main.cpp
// Huffman Text Compressor with SFML GUI (tree drawing + stats)
// Build: C++17, SFML 2.6.0 (link sfml-graphics, sfml-window, sfml-system)
//CLASS: BSCS 14-A
//Group Members:
//Syed Abbas Raza (517626)
//Shuja Naveed (502379)
//github: https://github.com/abbas72O5/DSA_Project_Huffman
#define _CRT_SECURE_NO_WARNINGS

#include <SFML/Graphics.hpp>
#include <windows.h>        // native file dialog (Windows)
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <unordered_map>
#include <string>

using namespace std;


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
    { return left == nullptr && right == nullptr; }
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
    { return rear == 0; }
    int size()
    { return rear; }
    int getHeight()
    { return h; }

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
      Module 1: Frequency & Tree
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
void storeCodesHashMap(HuffmanNode* root,
    unordered_map<unsigned char, string>& codeMap,
    string path = "") {
    if (!root) return;

    if (root->isLeaf()) {
        codeMap[root->data] = path;  // Direct insertion - O(1)
        return;
    }

    // Recursive calls with path building
    storeCodesHashMap(root->left, codeMap, path + "0");//0 for left 
    storeCodesHashMap(root->right, codeMap, path + "1");// 1 for right
}

/*
 Module 2: Encoding/Decoding & File I/O
 */

void writeCompressedText(const string& text, const string& outPath,
    unordered_map<unsigned char, string>& codeMap,
    unsigned char bytesPresent[256], uint64_t freqs[256]) {

    // compute total bits
    uint64_t totalBits = 0;
    for (unordered_map<unsigned char, string>::const_iterator it = codeMap.begin(); it != codeMap.end(); ++it) {
        unsigned char sym = it->first;
        totalBits += freqs[sym] * it->second.length();
    }

    ofstream out(outPath, ios::binary);
    if (!out) { cerr << "Cannot open output file for writing\n"; return; }

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

	//writing compressed data in the form of bits packed into bytes in output file
    // pack bits into bytes (MSB-first) - NOW WITH O(1) LOOKUP!
    uint8_t outByte = 0;
    int outBits = 0;
    for (size_t p = 0; p < text.size(); ++p) {
        unsigned char ch = (unsigned char)text[p];

		// O(1) lookup instead of O(n) avoiding linear search
        auto it = codeMap.find(ch);
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
    if (!in) { cerr << "cannot open compressed file\n"; return false; }

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

    // collect bytes list
    unsigned char bytesList[256];
    int uniqueCount = 0;
    for (int i = 0; i < 256; i++) if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

    // build tree
    HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
    if (!root) { cerr << "tree build failed\n"; return false; }

    ofstream out(outPath, ios::binary);
    if (!out) { cerr << "cannot open output decode file\n"; return false; }

    uint64_t bitsRead = 0;
    HuffmanNode* node = root;
    char byteBuf;
    while (bitsRead < totalBits && in.get(byteBuf)) {
        uint8_t b = (uint8_t)byteBuf;
        for (int bit = 7; bit >= 0 && bitsRead < totalBits; --bit) {
            int val = (b >> bit) & 1;
            if (val == 0) node = node->left; else node = node->right;
            if (!node) { cerr << "decode traversal error\n"; return false; }
            if (node->isLeaf()) {
                out.put((char)node->data);
                node = root;
            }
            ++bitsRead;
        }
    }
    out.close();

    // free tree
    // (a full free would traverse and delete nodes; done here for completeness)
    // We'll free root with a recursive helper below.
    // freeTree(root); // if you want to free here

    return true;
}

// -----------------------------
// Module 3: Tree layout & SFML visualization
// -----------------------------
struct VizNode {
    HuffmanNode* n;
    int x; // column index from inorder
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

// UPDATED draw function with scrollbar support
void drawTreeSFML(sf::RenderWindow& window, VizNode viz[], int vizCount,
    float nodeRadius, sf::Font& font,
    float zoomLevel, float scrollX, float scrollY,
    float maxScrollX, float maxScrollY) {

    // Calculate viewport size
    float viewportWidth = 760.f;
    float viewportHeight = 640.f;

    // Create a view for the tree area
    sf::View treeView(sf::FloatRect(0, 0, viewportWidth / zoomLevel, viewportHeight / zoomLevel));
    treeView.setViewport(sf::FloatRect(10.f / 1000.f, 10.f / 700.f, 760.f / 1000.f, 640.f / 700.f));

    // Apply scroll offset
    float offsetX = scrollX * maxScrollX;
    float offsetY = scrollY * maxScrollY;
    treeView.setCenter(viewportWidth / (2 * zoomLevel) + offsetX,
        viewportHeight / (2 * zoomLevel) + offsetY);

    window.setView(treeView);

    // Draw edges first with VISIBLE COLORS
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

    // Draw nodes & labels
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

        // Label
        string label;
        if (viz[i].n->isLeaf()) {
            unsigned char c = viz[i].n->data;
            if (c == '\n') label = "\\n";
            else if (c == '\t') label = "\\t";
            else if (c == ' ') label = "SPC";
            else if (isprint(c)) label = string(1, (char)c);
            else {
                char buf[16];
                sprintf_s(buf, sizeof(buf), "0x%02X", c);
                label = buf;
            }
        }
        else {
            label = to_string((uint32_t)viz[i].n->freq);
        }

        // Draw label inside circle
        sf::Text txt(label, font, 11);
        txt.setFillColor(sf::Color::Black);
        sf::FloatRect bounds = txt.getLocalBounds();
        txt.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
        txt.setPosition(viz[i].screenX, viz[i].screenY - 3.f);
        window.draw(txt);

        // Draw frequency below node
        sf::Text freqTxt(to_string((uint32_t)viz[i].n->freq), font, 9);
        freqTxt.setFillColor(sf::Color(40, 40, 40));
        sf::FloatRect freqBounds = freqTxt.getLocalBounds();
        freqTxt.setOrigin(freqBounds.width / 2.f, 0);
        freqTxt.setPosition(viz[i].screenX, viz[i].screenY + nodeRadius + 2.f);
        window.draw(freqTxt);
    }

    // Reset to default view for UI elements
    window.setView(window.getDefaultView());
}

// native Windows file dialog
string openFileDialogWin(const char* filter = "Text Files\0*.txt\0All Files\0*.*\0") {
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

string saveFileDialogWin(const char* filter = "Huffman Compressed\0*.huff\0All Files\0*.*\0",
    const char* defaultName = "compressed.huff") {
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

// free tree helper
void freeTree(HuffmanNode* root) {
    if (!root) return;
    freeTree(root->left);
    freeTree(root->right);
    delete root;
}

// -----------------------------
// Main: SFML application + integration
// -----------------------------
int main() {
    // SFML setup
    sf::RenderWindow window(sf::VideoMode(1000, 700), "Huffman Text Compressor (SFML)");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        cerr << "Warning: could not load arial.ttf from exe folder. Place a ttf file named arial.ttf next to exe.\n";
    }

    enum AppState { MENU, SELECTING, PROCESSING, SHOW_RESULT, DECOMPRESSING, DECOMPRESS_RESULT };
    AppState state = MENU;

    // UI rectangles and texts
    sf::RectangleShape startBtn(sf::Vector2f(260, 60));
    startBtn.setFillColor(sf::Color(120, 180, 255));
    startBtn.setPosition(370, 220);
    sf::Text startTxt("Start Huffman Compressor", font, 20);
    startTxt.setFillColor(sf::Color::Black);
    startTxt.setPosition(385, 235);

    // Decompress button
    sf::RectangleShape decompressBtn(sf::Vector2f(260, 60));
    decompressBtn.setFillColor(sf::Color(255, 180, 120));
    decompressBtn.setPosition(370, 300);
    sf::Text decompressTxt("Decompress .huff File", font, 20);
    decompressTxt.setFillColor(sf::Color::Black);
    decompressTxt.setPosition(395, 315);

    sf::RectangleShape selectBtn(sf::Vector2f(220, 40));
    selectBtn.setFillColor(sf::Color(160, 220, 160));
    selectBtn.setPosition(60, 580);
    sf::Text selectTxt("Select Text File", font, 18);
    selectTxt.setPosition(85, 588);
    selectTxt.setFillColor(sf::Color::Black);

    sf::Text statusTxt("", font, 16);
    statusTxt.setFillColor(sf::Color::Black);
    statusTxt.setPosition(60, 520);

    // results storage
    string inputPath;
    string compressedPath = "output.huff";
    uint64_t origBytes = 0, compBytes = 0;
    double ratio = 0.0;
    bool processed = false;

    // Decompression variables
    string decompressInputPath;
    string decompressOutputPath = "decompressed.txt";
    uint64_t decompressedBytes = 0;
    bool decompressSuccess = false;

    // tree viz arrays
    VizNode viz[1024];
    int vizCount = 0;
    float nodeRadius = 22.f;
    HuffmanNode* savedRoot = nullptr;

    // Camera/View control variables
    float zoomLevel = 1.0f;
    float panX = 0.0f;
    float panY = 0.0f;
    bool isDragging = false;
    sf::Vector2i lastMousePos;

    // Scrollbar variables
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float maxScrollX = 0.0f;
    float maxScrollY = 0.0f;
    float totalTreeWidth = 0.0f;
    float totalTreeHeight = 0.0f;
    bool isDraggingHScroll = false;
    bool isDraggingVScroll = false;

    // Horizontal scrollbar
    sf::RectangleShape hScrollTrack(sf::Vector2f(740, 12));
    hScrollTrack.setPosition(15, 653);
    hScrollTrack.setFillColor(sf::Color(200, 200, 200));
    hScrollTrack.setOutlineThickness(1.f);
    hScrollTrack.setOutlineColor(sf::Color(150, 150, 150));

    sf::RectangleShape hScrollThumb(sf::Vector2f(100, 12));
    hScrollThumb.setFillColor(sf::Color(100, 150, 200));
    hScrollThumb.setOutlineThickness(1.f);
    hScrollThumb.setOutlineColor(sf::Color(70, 100, 150));

    // Vertical scrollbar
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

    // Zoom level display
    sf::Text zoomDisplay("", font, 14);
    zoomDisplay.setFillColor(sf::Color::Black);
    zoomDisplay.setPosition(220, 678);

    // Save button for compressed file
    sf::RectangleShape saveCompressedBtn(sf::Vector2f(180, 35));
    saveCompressedBtn.setFillColor(sf::Color(100, 180, 100));
    saveCompressedBtn.setPosition(790, 320);

    sf::Text saveCompressedTxt("Save Compressed File", font, 14);
    saveCompressedTxt.setFillColor(sf::Color::Black);
    saveCompressedTxt.setPosition(800, 328);

    sf::Text saveStatusTxt("", font, 12);
    saveStatusTxt.setFillColor(sf::Color(0, 120, 0));
    saveStatusTxt.setPosition(790, 365);

    // Decompress result buttons
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

    sf::Text decompressSaveStatus("", font, 12);
    decompressSaveStatus.setFillColor(sf::Color(0, 120, 0));
    decompressSaveStatus.setPosition(300, 555);

    // main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                window.close();
            }

            // Mouse wheel zoom
            if (event.type == sf::Event::MouseWheelScrolled && state == SHOW_RESULT) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    float delta = event.mouseWheelScroll.delta;
                    if (delta > 0) {
                        zoomLevel *= 1.1f;
                    }
                    else {
                        zoomLevel *= 0.9f;
                    }
                    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
                    if (zoomLevel > 5.0f) zoomLevel = 5.0f;
                }
            }

            // Scrollbar dragging
            if (event.type == sf::Event::MouseButtonPressed && state == SHOW_RESULT) {
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

            if (event.type == sf::Event::MouseMoved && state == SHOW_RESULT) {
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
            if (event.type == sf::Event::KeyPressed && state == SHOW_RESULT) {
                float scrollSpeed = 0.05f;
                if (event.key.code == sf::Keyboard::Left) {
                    scrollX -= scrollSpeed;
                    scrollX = max(0.0f, scrollX);
                }
                else if (event.key.code == sf::Keyboard::Right) {
                    scrollX += scrollSpeed;
                    scrollX = min(1.0f, scrollX);
                }
                else if (event.key.code == sf::Keyboard::Up) {
                    scrollY -= scrollSpeed;
                    scrollY = max(0.0f, scrollY);
                }
                else if (event.key.code == sf::Keyboard::Down) {
                    scrollY += scrollSpeed;
                    scrollY = min(1.0f, scrollY);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                sf::Vector2f mfp((float)mp.x, (float)mp.y);

                if (state == MENU) {
                    if (startBtn.getGlobalBounds().contains(mfp)) {
                        state = SELECTING;
                    }
                    else if (decompressBtn.getGlobalBounds().contains(mfp)) {
                        string picked = openFileDialogWin("Huffman Compressed\0*.huff\0All Files\0*.*\0");
                        if (picked.size()) {
                            decompressInputPath = picked;
                            state = DECOMPRESSING;

                            // Generate output filename
                            size_t lastSlash = decompressInputPath.find_last_of("\\/");
                            size_t lastDot = decompressInputPath.find_last_of(".");

                            if (lastSlash != string::npos && lastDot != string::npos && lastDot > lastSlash) {
                                string baseName = decompressInputPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
                                decompressOutputPath = baseName + "_decompressed.txt";
                            }

                            // Perform decompression
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
                else if (state == SELECTING) {
                    if (selectBtn.getGlobalBounds().contains(mfp)) {
                        string picked = openFileDialogWin("Text Files\0*.txt\0All Files\0*.*\0");
                        if (picked.size()) {
                            inputPath = picked;
                            state = PROCESSING;

                            ifstream fin(inputPath, ios::binary);
                            if (!fin) { statusTxt.setString("Error reading file."); state = SELECTING; break; }
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
                            for (int i = 0; i < 256; i++) if (bytesPresent[i]) bytesList[uniqueCount++] = (unsigned char)i;

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
                else if (state == DECOMPRESS_RESULT) {
                    if (saveDecompressedBtn.getGlobalBounds().contains(mfp)) {
                        string suggestedName = decompressOutputPath;
                        string savePath = saveFileDialogWin("Text Files\0*.txt\0All Files\0*.*\0",
                            suggestedName.c_str());

                        if (!savePath.empty()) {
                            ifstream src(decompressOutputPath, ios::binary);
                            ofstream dst(savePath, ios::binary);

                            if (src && dst) {
                                dst << src.rdbuf();
                                src.close();
                                dst.close();

                                decompressSaveStatus.setString("File saved successfully!");
                                decompressSaveStatus.setFillColor(sf::Color(0, 120, 0));
                            }
                            else {
                                decompressSaveStatus.setString("Error saving file.");
                                decompressSaveStatus.setFillColor(sf::Color(200, 0, 0));
                            }
                        }
                    }
                    else if (backToMenuBtn.getGlobalBounds().contains(mfp)) {
                        state = MENU;
                        decompressSuccess = false;
                        decompressedBytes = 0;
                        decompressSaveStatus.setString("");
                    }
                }
            }
        }

        window.clear(sf::Color(245, 245, 245));

        if (state == MENU) {
            sf::Text title("Huffman Text Compressor", font, 30);
            title.setFillColor(sf::Color::Black);
            title.setPosition(300, 120);
            window.draw(title);

            window.draw(startBtn);
            window.draw(startTxt);

            window.draw(decompressBtn);
            window.draw(decompressTxt);

            sf::Text hint("Compress text files or decompress .huff files back to original text.", font, 14);
            hint.setPosition(200, 400);
            hint.setFillColor(sf::Color::Black);
            window.draw(hint);
        }
        else if (state == SELECTING) {
            sf::Text istr("Select a .txt file to compress (native dialog will open)", font, 18);
            istr.setPosition(60, 520);
            istr.setFillColor(sf::Color::Black);
            window.draw(istr);

            window.draw(selectBtn);
            window.draw(selectTxt);

            statusTxt.setString("Waiting for file selection...");
            window.draw(statusTxt);
        }
        else if (state == PROCESSING) {
            sf::Text p("Processing... please wait", font, 22);
            p.setPosition(400, 320);
            p.setFillColor(sf::Color::Black);
            window.draw(p);
        }
        else if (state == DECOMPRESSING) {
            sf::Text p("Decompressing... please wait", font, 22);
            p.setPosition(350, 320);
            p.setFillColor(sf::Color::Black);
            window.draw(p);
        }
        else if (state == SHOW_RESULT) {
            sf::RectangleShape treeBg(sf::Vector2f(760, 640));
            treeBg.setPosition(10, 10);
            treeBg.setFillColor(sf::Color(250, 250, 255));
            treeBg.setOutlineThickness(1.f);
            treeBg.setOutlineColor(sf::Color(200, 200, 200));
            window.draw(treeBg);

            drawTreeSFML(window, viz, vizCount, nodeRadius, font, zoomLevel, scrollX, scrollY, maxScrollX, maxScrollY);

            float hTrackWidth = hScrollTrack.getSize().x;
            float hThumbWidth = max(50.0f, hTrackWidth * (760.f / max(760.f, totalTreeWidth / zoomLevel)));
            hScrollThumb.setSize(sf::Vector2f(hThumbWidth, 12));
            hScrollThumb.setPosition(hScrollTrack.getPosition().x + scrollX * (hTrackWidth - hThumbWidth), 653);

            float vTrackHeight = vScrollTrack.getSize().y;
            float vThumbHeight = max(50.0f, vTrackHeight * (640.f / max(640.f, totalTreeHeight / zoomLevel)));
            vScrollThumb.setSize(sf::Vector2f(12, vThumbHeight));
            vScrollThumb.setPosition(758, vScrollTrack.getPosition().y + scrollY * (vTrackHeight - vThumbHeight));

            window.draw(hScrollTrack);
            window.draw(hScrollThumb);
            window.draw(vScrollTrack);
            window.draw(vScrollThumb);

            window.draw(zoomInBtn);
            window.draw(zoomInTxt);
            window.draw(zoomOutBtn);
            window.draw(zoomOutTxt);
            window.draw(resetViewBtn);
            window.draw(resetViewTxt);

            char zoomBuf[64];
            sprintf_s(zoomBuf, sizeof(zoomBuf), "Zoom: %.1fx", zoomLevel);
            zoomDisplay.setString(zoomBuf);
            window.draw(zoomDisplay);

            sf::Text instructions("Zoom: Wheel | Pan: Arrows | Scroll: Drag bars", font, 11);
            instructions.setPosition(15, 15);
            instructions.setFillColor(sf::Color(50, 50, 50));
            window.draw(instructions);

            sf::RectangleShape infoBg(sf::Vector2f(220, 640));
            infoBg.setPosition(780, 10);
            infoBg.setFillColor(sf::Color(245, 245, 245));
            infoBg.setOutlineThickness(1.f);
            infoBg.setOutlineColor(sf::Color(200, 200, 200));
            window.draw(infoBg);

            sf::Text stats("Compression Statistics", font, 16);
            stats.setPosition(790, 20);
            stats.setFillColor(sf::Color::Black);
            window.draw(stats);

            sf::Text o(("Original size (bytes): " + to_string((unsigned long long)origBytes)), font, 14);
            o.setPosition(790, 60);
            o.setFillColor(sf::Color::Black);
            window.draw(o);

            sf::Text c(("Compressed file: " + compressedPath), font, 12);
            c.setPosition(790, 95);
            c.setFillColor(sf::Color::Black);
            window.draw(c);

            sf::Text cb(("Compressed size (bytes): " + to_string((unsigned long long)compBytes)), font, 14);
            cb.setPosition(790, 130);
            cb.setFillColor(sf::Color::Black);
            window.draw(cb);

            char buf[128];
            sprintf_s(buf, sizeof(buf), "Compression ratio: %.4f", ratio);
            sf::Text r(buf, font, 14);
            r.setPosition(790, 165);
            r.setFillColor(sf::Color::Black);
            window.draw(r);

            double perc = (origBytes > 0) ? (100.0 * (1.0 - ratio)) : 0.0;
            sprintf_s(buf, sizeof(buf), "Space saved: %.2f %%", perc);
            sf::Text ssave(buf, font, 14);
            ssave.setPosition(790, 200);
            ssave.setFillColor(sf::Color::Black);
            window.draw(ssave);

            sf::Text note(("Tree nodes: " + to_string(vizCount)), font, 12);
            note.setPosition(790, 240);
            note.setFillColor(sf::Color::Black);
            window.draw(note);

            window.draw(saveCompressedBtn);
            window.draw(saveCompressedTxt);
            window.draw(saveStatusTxt);

            sf::Text help("Click 'Save Compressed File'\nto export the .huff file.\n\nDecompress using the\nDecompress feature.", font, 11);
            help.setPosition(790, 410);
            help.setFillColor(sf::Color(60, 60, 60));
            window.draw(help);
        }
        else if (state == DECOMPRESS_RESULT) {
            sf::Text title("Decompression Complete!", font, 28);
            title.setFillColor(sf::Color(0, 150, 0));
            title.setPosition(300, 80);
            window.draw(title);

            sf::RectangleShape resultBg(sf::Vector2f(600, 400));
            resultBg.setPosition(200, 150);
            resultBg.setFillColor(sf::Color(240, 250, 240));
            resultBg.setOutlineThickness(2.f);
            resultBg.setOutlineColor(sf::Color(100, 180, 100));
            window.draw(resultBg);

            sf::Text statsTitle("Decompression Statistics", font, 20);
            statsTitle.setPosition(320, 170);
            statsTitle.setFillColor(sf::Color::Black);
            window.draw(statsTitle);
            sf::Text inputLabel("Compressed file:", font, 14);
            inputLabel.setPosition(220, 220);
            inputLabel.setFillColor(sf::Color::Black);
            window.draw(inputLabel);

            string displayPath = decompressInputPath;
            size_t lastSlash = displayPath.find_last_of("\\/");
            if (lastSlash != string::npos) {
                displayPath = displayPath.substr(lastSlash + 1);
            }
            sf::Text inputPath(displayPath, font, 12);
            inputPath.setPosition(240, 245);
            inputPath.setFillColor(sf::Color(60, 60, 60));
            window.draw(inputPath);

            sf::Text outputLabel("Output file:", font, 14);
            outputLabel.setPosition(220, 290);
            outputLabel.setFillColor(sf::Color::Black);
            window.draw(outputLabel);

            sf::Text outputPath(decompressOutputPath, font, 12);
            outputPath.setPosition(240, 315);
            outputPath.setFillColor(sf::Color(60, 60, 60));
            window.draw(outputPath);

            sf::Text sizeText(("Decompressed size: " + to_string(decompressedBytes) + " bytes"), font, 14);
            sizeText.setPosition(220, 360);
            sizeText.setFillColor(sf::Color::Black);
            window.draw(sizeText);

            sf::Text successMsg("✓ File successfully decompressed!", font, 16);
            successMsg.setPosition(290, 410);
            successMsg.setFillColor(sf::Color(0, 150, 0));
            window.draw(successMsg);

            window.draw(saveDecompressedBtn);
            window.draw(saveDecompressedTxt);
            window.draw(backToMenuBtn);
            window.draw(backToMenuTxt);
            window.draw(decompressSaveStatus);
        
        }

        window.display();
    }

    if (savedRoot) freeTree(savedRoot);
    return 0;
}

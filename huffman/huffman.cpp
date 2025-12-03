// main.cpp
// Huffman Text Compressor with SFML GUI (tree drawing + stats)
// Build: C++17, SFML 2.6.x (link sfml-graphics, sfml-window, sfml-system)

#define _CRT_SECURE_NO_WARNINGS

#include <SFML/Graphics.hpp>
#include <windows.h>        // native file dialog (Windows)
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdint>

using namespace std;

// -----------------------------
// Huffman node & BinaryHeap
// -----------------------------
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
        data = 0; // internal node
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
        arr = new HuffmanNode * [capacity + 2]; // 1-based indexing
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
    int getHeight() { return h; }

    HuffmanNode* top() {
        if (rear == 0) return nullptr;
        return arr[1];
    }

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

// -----------------------------
// Module 1: Frequency & Tree
// -----------------------------
HuffmanNode* buildHuffmanTree(unsigned char bytes[], uint64_t freqs[], int uniqueCount) {
    BinaryHeap minHeap(uniqueCount + 5);
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

// DFS to assign codes into arrays (Module 2 helper)
void storeCodes(HuffmanNode* root, unsigned char codeBytes[], char codes[][512], char path[], int depth, int& index) {
    if (!root) return;
    if (root->isLeaf()) {
        codeBytes[index] = root->data;
        path[depth] = '\0';
        // safe copy: destination size 512
        strcpy_s(codes[index], 512, path);
        index++;
        return;
    }
    path[depth] = '0';
    storeCodes(root->left, codeBytes, codes, path, depth + 1, index);
    path[depth] = '1';
    storeCodes(root->right, codeBytes, codes, path, depth + 1, index);
}

// -----------------------------
// Module 2: Encoding/Decoding & File I/O
// -----------------------------
void writeCompressedText(const string& text, const string& outPath,
    unsigned char codeBytes[], char codes[][512], int codeCount,
    unsigned char bytesPresent[256], uint64_t freqs[256]) {
    // compute total bits
    uint64_t totalBits = 0;
    for (int i = 0; i < codeCount; ++i) {
        unsigned char sym = codeBytes[i];
        totalBits += freqs[sym] * strlen(codes[i]);
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

    // Pack bits into bytes (MSB-first)
    uint8_t outByte = 0;
    int outBits = 0; // bits currently in outByte
    for (size_t p = 0; p < text.size(); ++p) {
        unsigned char ch = (unsigned char)text[p];
        // find code for ch
        const char* code = nullptr;
        for (int i = 0; i < codeCount; i++) if (codeBytes[i] == ch) { code = codes[i]; break; }
        if (!code) continue;
        for (size_t k = 0; k < strlen(code); ++k) {
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

// draw function (SFML 2.x style)
void drawTreeSFML(sf::RenderWindow& window, VizNode viz[], int vizCount, float nodeRadius, sf::Font& font) {
    // draw edges first
    for (int i = 0; i < vizCount; i++) {
        HuffmanNode* node = viz[i].n;
        if (!node) continue;
        if (node->left) {
            // find left child viz
            for (int j = 0; j < vizCount; j++) if (viz[j].n == node->left) {
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(viz[i].screenX, viz[i].screenY)),
                    sf::Vertex(sf::Vector2f(viz[j].screenX, viz[j].screenY))
                };
                window.draw(line, 2, sf::Lines);
                break;
            }
        }
        if (node->right) {
            for (int j = 0; j < vizCount; j++) if (viz[j].n == node->right) {
                sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(viz[i].screenX, viz[i].screenY)),
                    sf::Vertex(sf::Vector2f(viz[j].screenX, viz[j].screenY))
                };
                window.draw(line, 2, sf::Lines);
                break;
            }
        }
    }

    // draw nodes & labels
    for (int i = 0; i < vizCount; i++) {
        if (!viz[i].n) continue;
        sf::CircleShape circle(nodeRadius);
        circle.setOrigin(nodeRadius, nodeRadius);
        circle.setPosition(viz[i].screenX, viz[i].screenY);
        circle.setOutlineColor(sf::Color::Black);
        circle.setOutlineThickness(2.f);
        circle.setFillColor(sf::Color(220, 220, 255));
        window.draw(circle);

        // label: printable char or freq
        string label;
        if (viz[i].n->isLeaf()) {
            unsigned char c = viz[i].n->data;
            if (c == '\n') label = "\\n";
            else if (c == '\t') label = "\\t";
            else if (isprint(c)) label = string(1, (char)c);
            else {
                char buf[16];
                sprintf_s(buf, sizeof(buf), "0x%02X", c);
                label = buf;
            }
            label += "\n";
            label += to_string((uint32_t)viz[i].n->freq);
        }
        else {
            label = to_string((uint32_t)viz[i].n->freq);
        }

        sf::Text txt(label, font, 12);
        txt.setFillColor(sf::Color::Black);
        txt.setPosition(viz[i].screenX - nodeRadius, viz[i].screenY - nodeRadius - 18);
        window.draw(txt);
    }
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

    enum AppState { MENU, SELECTING, PROCESSING, SHOW_RESULT };
    AppState state = MENU;

    // UI rectangles and texts
    sf::RectangleShape startBtn(sf::Vector2f(260, 60));
    startBtn.setFillColor(sf::Color(120, 180, 255));
    startBtn.setPosition(370, 220);
    sf::Text startTxt("Start Huffman Compressor", font, 20);
    startTxt.setFillColor(sf::Color::Black);
    startTxt.setPosition(385, 235);

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

    // tree viz arrays
    VizNode viz[1024];
    int vizCount = 0;
    float nodeRadius = 22.f;
    HuffmanNode* savedRoot = nullptr; // keep pointer to free later

    // main loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                // cleanup
                if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                window.close();
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2i mp = sf::Mouse::getPosition(window);
                sf::Vector2f mfp((float)mp.x, (float)mp.y);
                if (state == MENU) {
                    if (startBtn.getGlobalBounds().contains(mfp)) {
                        state = SELECTING;
                    }
                }
                else if (state == SELECTING) {
                    if (selectBtn.getGlobalBounds().contains(mfp)) {
                        // open native file dialog
                        string picked = openFileDialogWin("Text Files\0*.txt\0All Files\0*.*\0");
                        if (picked.size()) {
                            inputPath = picked;
                            state = PROCESSING;

                            // read full text
                            ifstream fin(inputPath, ios::binary);
                            if (!fin) { statusTxt.setString("Error reading file."); state = SELECTING; break; }
                            string text((istreambuf_iterator<char>(fin)), istreambuf_iterator<char>());
                            fin.close();
                            origBytes = text.size();

                            // build freq
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

                            // build tree
                            HuffmanNode* root = buildHuffmanTree(bytesList, freqs, uniqueCount);
                            if (!root) {
                                statusTxt.setString("File empty or unreadable.");
                                state = SELECTING;
                                break;
                            }

                            // store codes
                            unsigned char codeBytes[256];
                            char codes[256][512];
                            char pathArr[512];
                            int codeCount = 0;
                            storeCodes(root, codeBytes, codes, pathArr, 0, codeCount);

                            // write compressed
                            writeCompressedText(text, compressedPath, codeBytes, codes, codeCount, bytesPresent, freqs);

                            // compressed file size
                            compBytes = 0;
                            {
                                ifstream fin2(compressedPath, ios::binary | ios::ate);
                                if (fin2) compBytes = (uint64_t)fin2.tellg();
                            }
                            ratio = (origBytes > 0) ? ((double)compBytes / (double)origBytes) : 0.0;

                            // prepare visualization layout
                            vizCount = 0;
                            int curX = 0;
                            assignPositionsInorder(root, curX, 0, viz, vizCount);
                            float spacingX = 60.f;
                            float spacingY = 90.f;
                            float offsetX = 400.f;
                            float offsetY = 40.f;
                            for (int i = 0; i < vizCount; i++) {
                                viz[i].screenX = offsetX + viz[i].x * spacingX;
                                viz[i].screenY = offsetY + viz[i].depth * spacingY;
                            }

                            processed = true;
                            // keep root pointer to free when program closes or next run
                            if (savedRoot) { freeTree(savedRoot); savedRoot = nullptr; }
                            savedRoot = root;

                            state = SHOW_RESULT;
                        }
                    }
                }
            }
        }

        window.clear(sf::Color(245, 245, 245));

        if (state == MENU) {
            // title
            sf::Text title("Huffman Text Compressor", font, 30);
            title.setFillColor(sf::Color::Black);
            title.setPosition(300, 120);
            window.draw(title);

            window.draw(startBtn);
            window.draw(startTxt);

            sf::Text hint("Click Start to pick a text file and compress it. Uses Huffman coding and draws the tree.", font, 14);
            hint.setPosition(150, 320);
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
            p.setPosition(60, 520);
            p.setFillColor(sf::Color::Black);
            window.draw(p);
        }
        else if (state == SHOW_RESULT) {
            // left: tree area
            sf::RectangleShape treeBg(sf::Vector2f(760, 640));
            treeBg.setPosition(10, 10);
            treeBg.setFillColor(sf::Color(250, 250, 255));
            treeBg.setOutlineThickness(1.f);
            treeBg.setOutlineColor(sf::Color(200, 200, 200));
            window.draw(treeBg);

            // draw tree
            drawTreeSFML(window, viz, vizCount, nodeRadius, font);

            // right: stats
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
            o.setPosition(790, 60); window.draw(o);

            sf::Text c(("Compressed file: " + compressedPath), font, 12);
            c.setPosition(790, 95); window.draw(c);

            sf::Text cb(("Compressed size (bytes): " + to_string((unsigned long long)compBytes)), font, 14);
            cb.setPosition(790, 130); window.draw(cb);

            char buf[128];
            sprintf_s(buf, sizeof(buf), "Compression ratio (comp/orig): %.4f", ratio);
            sf::Text r(buf, font, 14);
            r.setPosition(790, 165); window.draw(r);

            double perc = (origBytes > 0) ? (100.0 * (1.0 - ratio)) : 0.0;
            sprintf_s(buf, sizeof(buf), "Space saved: %.2f %%", perc);
            sf::Text ssave(buf, font, 14);
            ssave.setPosition(790, 200); window.draw(ssave);

            sf::Text note(("Tree nodes: " + to_string(vizCount)), font, 12);
            note.setPosition(790, 240); window.draw(note);

            sf::Text help("You can decompress output.huff using the provided CLI tool or via programmatic load.", font, 12);
            help.setPosition(790, 280); help.setFillColor(sf::Color(60, 60, 60));
            window.draw(help);
        }

        window.display();
    }

    // cleanup saved root on exit
    if (savedRoot) freeTree(savedRoot);
    return 0;
}

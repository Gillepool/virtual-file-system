#include "../include/Compression.h"
#include <algorithm>
#include <bitset>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <climits>

std::string RLECompression::compress(const std::string& input) const {
    if (input.empty()) {
        return "";
    }
    
    std::string result;
    char current = input[0];
    int count = 1;
    
    for (size_t i = 1; i < input.length(); ++i) {
        if (input[i] == current && count < 255) {
            count++;
        } else {
            result.push_back(static_cast<char>(count));
            result.push_back(current);
            current = input[i];
            count = 1;
        }
    }
    
    result.push_back(static_cast<char>(count));
    result.push_back(current);
    
    // Only use compressed version if it's smaller
    return (result.size() < input.size()) ? result : input;
}

std::string RLECompression::decompress(const std::string& input) const {
    if (input.empty()) {
        return "";
    }
    
    // Quick check to see if the data was actually compressed
    // If input length is odd, it wasn't RLE compressed
    if (input.length() % 2 != 0) {
        return input;
    }
    
    std::string result;
    
    for (size_t i = 0; i < input.length(); i += 2) {
        if (i + 1 < input.length()) {
            int count = static_cast<unsigned char>(input[i]);
            char c = input[i + 1];
            result.append(count, c);
        }
    }
    
    return result;
}

std::shared_ptr<HuffmanCompression::HuffmanNode> HuffmanCompression::buildHuffmanTree(const std::string& input) const {
    if (input.empty()) {
        return nullptr;
    }
    
    std::unordered_map<char, int> freqMap;
    for (char c : input) {
        freqMap[c]++;
    }
    
    std::priority_queue<std::shared_ptr<HuffmanNode>, 
                       std::vector<std::shared_ptr<HuffmanNode>>, 
                       std::greater<std::shared_ptr<HuffmanNode>>> pq;
    
    for (const auto& pair : freqMap) {
        pq.push(std::make_shared<HuffmanNode>(pair.first, pair.second));
    }
    
    // Build Huffman Tree: combine nodes until only root remains
    while (pq.size() > 1) {
        auto left = pq.top(); pq.pop();
        auto right = pq.top(); pq.pop();
        
        // Create internal node with these two nodes as children
        auto parent = std::make_shared<HuffmanNode>('\0', left->freq + right->freq, left, right);
        pq.push(parent);
    }
    
    return pq.top();
}

void HuffmanCompression::generateCodes(std::shared_ptr<HuffmanNode> root, std::string code, 
                                     std::unordered_map<char, std::string>& huffmanCodes) const {
    if (!root) {
        return;
    }
    
    // Found a leaf node
    if (!root->left && !root->right) {
        huffmanCodes[root->ch] = code.empty() ? "0" : code;
    }
    
    // Traverse left (add '0' to code)
    generateCodes(root->left, code + '0', huffmanCodes);
    // Traverse right (add '1' to code)
    generateCodes(root->right, code + '1', huffmanCodes);
}

// Serialize Huffman Tree for inclusion in compressed data
std::string HuffmanCompression::serializeHuffmanTree(std::shared_ptr<HuffmanNode> root) const {
    std::string result;
    
    // DFS traversal to serialize the tree
    if (!root) {
        return result;
    }
    
    // Leaf node: add '1' followed by character
    if (!root->left && !root->right) {
        result.push_back('1');
        result.push_back(root->ch);
    } else {
        // Internal node: add '0' and recurse
        result.push_back('0');
        result += serializeHuffmanTree(root->left);
        result += serializeHuffmanTree(root->right);
    }
    
    return result;
}

// Deserialize Huffman Tree from compressed data
std::shared_ptr<HuffmanCompression::HuffmanNode> HuffmanCompression::deserializeHuffmanTree(
    const std::string& serializedTree, size_t& index) const {
    
    if (index >= serializedTree.size()) {
        return nullptr;
    }
    
    // Leaf node
    if (serializedTree[index] == '1') {
        index++; // Skip the '1'
        if (index < serializedTree.size()) {
            char ch = serializedTree[index++];
            return std::make_shared<HuffmanNode>(ch, 0);
        }
        return nullptr;
    }
    
    index++; // Skip the '0'
    auto left = deserializeHuffmanTree(serializedTree, index);
    auto right = deserializeHuffmanTree(serializedTree, index);
    
    return std::make_shared<HuffmanNode>('\0', 0, left, right);
}

std::string HuffmanCompression::compress(const std::string& input) const {
    if (input.empty() || input.size() < 2) {
        return input;
    }
    
    // Build Huffman Tree
    auto root = buildHuffmanTree(input);
    if (!root) {
        return input;
    }
    
    std::unordered_map<char, std::string> huffmanCodes;
    generateCodes(root, "", huffmanCodes);
    
    std::string encodedString;
    for (char ch : input) {
        encodedString += huffmanCodes[ch];
    }
    
    // Serialize the Huffman tree
    std::string serializedTree = serializeHuffmanTree(root);
    
    // Write the header: tree size and encoded string length (in bits)
    std::string header;
    size_t treeSize = serializedTree.size();
    size_t encodedBitsCount = encodedString.size();
    
    header.append(reinterpret_cast<char*>(&treeSize), sizeof(treeSize));
    header.append(reinterpret_cast<char*>(&encodedBitsCount), sizeof(encodedBitsCount));
    
    // Compute padding bits
    unsigned char padding = 8 - (encodedString.length() % 8);
    if (padding == 8) padding = 0;
    header.push_back(static_cast<char>(padding));
    
    // Convert binary string to bytes
    std::string compressedData;
    for (size_t i = 0; i < encodedString.length(); i += 8) {
        std::bitset<8> bits;
        for (size_t j = 0; j < 8 && i + j < encodedString.length(); ++j) {
            if (encodedString[i + j] == '1') {
                bits.set(7 - j);
            }
        }
        compressedData.push_back(static_cast<char>(bits.to_ulong()));
    }
    
    std::string result = header + serializedTree + compressedData;
    
    // Only use compressed version if it's smaller
    return (result.size() < input.size()) ? result : input;
}

std::string HuffmanCompression::decompress(const std::string& input) const {
    if (input.empty() || input.size() <= sizeof(size_t) * 2 + 1) {
        return input;
    }
    
    // Read header
    size_t index = 0;
    size_t treeSize, encodedBitsCount;
    
    if (input.size() < sizeof(treeSize) + sizeof(encodedBitsCount) + 1) {
        return input; // Not enough data for a valid header
    }
    
    treeSize = *reinterpret_cast<const size_t*>(&input[index]);
    index += sizeof(treeSize);
    
    encodedBitsCount = *reinterpret_cast<const size_t*>(&input[index]);
    index += sizeof(encodedBitsCount);
    
    unsigned char padding = static_cast<unsigned char>(input[index++]);
    
    if (input.size() < index + treeSize + ((encodedBitsCount + 7) / 8)) {
        return input; // Input is too short, probably not Huffman compressed
    }
    
    std::string serializedTree = input.substr(index, treeSize);
    index += treeSize;
    
    size_t treeIndex = 0;
    auto root = deserializeHuffmanTree(serializedTree, treeIndex);
    
    if (!root) {
        return input;
    }
    
    // Convert bytes to binary string
    std::string encodedBits;
    for (size_t i = index; i < input.size(); ++i) {
        std::bitset<8> bits(static_cast<unsigned char>(input[i]));
        encodedBits += bits.to_string();
    }
    
    // Trim padding bits
    if (padding > 0 && encodedBits.length() > padding) {
        encodedBits = encodedBits.substr(0, encodedBits.length() - padding);
    }
    
    // Decode bits using Huffman tree
    std::string result;
    auto current = root;
    
    for (size_t i = 0; i < std::min(encodedBits.length(), encodedBitsCount); ++i) {
        if (encodedBits[i] == '0') {
            current = current->left;
        } else {
            current = current->right;
        }
        
        // Reached a leaf node
        if (!current->left && !current->right) {
            result.push_back(current->ch);
            current = root;
        }
    }
    
    return result;
}

// LZW Compression Implementation
std::vector<int> LZWCompression::lzwEncode(const std::string& input) const {
    std::unordered_map<std::string, int> dictionary;
    std::vector<int> result;
    
    // Initialize dictionary with all single characters
    int dictSize = 256;
    for (int i = 0; i < 256; ++i) {
        dictionary[std::string(1, static_cast<char>(i))] = i;
    }
    
    std::string current;
    for (char c : input) {
        std::string next = current + c;
        if (dictionary.find(next) != dictionary.end()) {
            current = next;
        } else {
            result.push_back(dictionary[current]);
            
            dictionary[next] = dictSize++;
            current = std::string(1, c);
        }
    }
    
    if (!current.empty()) {
        result.push_back(dictionary[current]);
    }
    
    return result;
}

std::string LZWCompression::lzwDecode(const std::vector<int>& codes) const {
    std::unordered_map<int, std::string> dictionary;
    std::string result;
    
    int dictSize = 256;
    for (int i = 0; i < 256; ++i) {
        dictionary[i] = std::string(1, static_cast<char>(i));
    }
    
    if (codes.empty()) {
        return result;
    }
    
    // First code is always a single character
    int oldCode = codes[0];
    std::string entry = dictionary[oldCode];
    result += entry;
    
    for (size_t i = 1; i < codes.size(); ++i) {
        int code = codes[i];
        std::string s;
        
        // If code is in dictionary, get the string
        if (dictionary.find(code) != dictionary.end()) {
            s = dictionary[code];
        } else if (code == dictSize) {
            // Special case for code not yet in dictionary
            s = dictionary[oldCode] + dictionary[oldCode][0];
        } else {
            // Invalid code
            return ""; // Error in compressed data
        }
        
        result += s;
        
        dictionary[dictSize++] = dictionary[oldCode] + s[0];
        
        oldCode = code;
    }
    
    return result;
}

std::string LZWCompression::compress(const std::string& input) const {
    if (input.empty() || input.size() < 3) {
        return input;
    }
    
    std::vector<int> codes = lzwEncode(input);
    
    // Convert codes to binary format
    std::string result;
    result.reserve(codes.size() * sizeof(int) + sizeof(size_t));
    
    // Add header with code count
    size_t codeCount = codes.size();
    result.append(reinterpret_cast<char*>(&codeCount), sizeof(codeCount));
    
    for (int code : codes) {
        result.append(reinterpret_cast<char*>(&code), sizeof(code));
    }
    
    // Only use compressed version if it's smaller
    return (result.size() < input.size()) ? result : input;
}

std::string LZWCompression::decompress(const std::string& input) const {
    if (input.empty() || input.size() <= sizeof(size_t)) {
        return input;
    }
    
    // Read header
    size_t codeCount = *reinterpret_cast<const size_t*>(input.data());
    
    if (input.size() < sizeof(size_t) + codeCount * sizeof(int)) {
        return input; // Not enough data, probably not LZW compressed
    }
    
    std::vector<int> codes;
    codes.reserve(codeCount);
    
    const int* codePtr = reinterpret_cast<const int*>(input.data() + sizeof(size_t));
    for (size_t i = 0; i < codeCount; ++i) {
        codes.push_back(codePtr[i]);
    }
    
    return lzwDecode(codes);
}

std::unique_ptr<CompressionAlgorithm> CompressionFactory::createAlgorithm(const std::string& type) {
    if (type == "RLE") {
        return std::make_unique<RLECompression>();
    } else if (type == "Huffman") {
        return std::make_unique<HuffmanCompression>();
    } else if (type == "LZW") {
        return std::make_unique<LZWCompression>();
    } else {
        return std::make_unique<RLECompression>();
    }
}

std::vector<std::string> CompressionFactory::listAvailableAlgorithms() {
    return {"RLE", "Huffman", "LZW"};
}

std::unique_ptr<CompressionAlgorithm> CompressionFactory::getDefaultAlgorithm() {
    return std::make_unique<HuffmanCompression>();
}
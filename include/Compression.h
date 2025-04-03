#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>

class CompressionAlgorithm {
public:
    virtual ~CompressionAlgorithm() = default;
    virtual std::string compress(const std::string& input) const = 0;
    virtual std::string decompress(const std::string& input) const = 0;
    virtual std::string getName() const = 0;
};

// Run-Length Encoding compression algorithm
class RLECompression : public CompressionAlgorithm {
public:
    std::string compress(const std::string& input) const override;
    std::string decompress(const std::string& input) const override;
    std::string getName() const override { return "RLE"; }
};

// Huffman compression algorithm
class HuffmanCompression : public CompressionAlgorithm {
public:
    std::string compress(const std::string& input) const override;
    std::string decompress(const std::string& input) const override;
    std::string getName() const override { return "Huffman"; }
    
private:
    // Huffman Tree Node structure
    struct HuffmanNode {
        char ch;
        int freq;
        std::shared_ptr<HuffmanNode> left, right;
        
        HuffmanNode(char c, int f, std::shared_ptr<HuffmanNode> l = nullptr, std::shared_ptr<HuffmanNode> r = nullptr) 
            : ch(c), freq(f), left(l), right(r) {}
        
        // Compare function for priority queue
        bool operator>(const HuffmanNode& other) const {
            return freq > other.freq;
        }
    };
    
    std::shared_ptr<HuffmanNode> buildHuffmanTree(const std::string& input) const;
    void generateCodes(std::shared_ptr<HuffmanNode> root, std::string code, 
                       std::unordered_map<char, std::string>& huffmanCodes) const;
    std::string serializeHuffmanTree(std::shared_ptr<HuffmanNode> root) const;
    std::shared_ptr<HuffmanNode> deserializeHuffmanTree(const std::string& serializedTree, size_t& index) const;
};

// LZW (Lempel-Ziv-Welch) compression algorithm
class LZWCompression : public CompressionAlgorithm {
public:
    std::string compress(const std::string& input) const override;
    std::string decompress(const std::string& input) const override;
    std::string getName() const override { return "LZW"; }
    
private:
    // Helper methods for LZW coding
    std::vector<int> lzwEncode(const std::string& input) const;
    std::string lzwDecode(const std::vector<int>& codes) const;
};

class CompressionFactory {
public:
    static std::unique_ptr<CompressionAlgorithm> createAlgorithm(const std::string& type);
    static std::vector<std::string> listAvailableAlgorithms();
    static std::unique_ptr<CompressionAlgorithm> getDefaultAlgorithm();
};

#endif // COMPRESSION_H
#include "../include/Encryption.h"
#include <algorithm>
#include <numeric>
#include <cstring>

// XOR Encryption Implementation
std::string XOREncryption::encrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    std::string result = input;
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()];
    }
    
    return result;
}

std::string XOREncryption::decrypt(const std::string& input, const std::string& key) const {
    return encrypt(input, key);
}

// Caesar Cipher Implementation
int CaesarCipher::keyToShift(const std::string& key) const {
    // Convert the key to a shift value by summing the ASCII values and taking modulo 26
    return std::accumulate(key.begin(), key.end(), 0, 
                         [](int sum, char c) { return sum + static_cast<int>(c); }) % 26;
}

std::string CaesarCipher::encrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    int shift = keyToShift(key);
    std::string result = input;
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            result[i] = static_cast<char>((result[i] - base + shift) % 26 + base);
        }
    }
    
    return result;
}

std::string CaesarCipher::decrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    // For decryption, use 26 - shift
    int shift = keyToShift(key);
    shift = (26 - shift) % 26;
    
    std::string result = input;
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            result[i] = static_cast<char>((result[i] - base + shift) % 26 + base);
        }
    }
    
    return result;
}

// Vigenere Cipher Implementation
std::string VigenereCipher::encrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    std::string result = input;
    size_t keyIndex = 0;
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            char keyChar = tolower(key[keyIndex % key.size()]) - 'a';
            result[i] = static_cast<char>((result[i] - base + keyChar) % 26 + base);
            keyIndex++;
        }
    }
    
    return result;
}

std::string VigenereCipher::decrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    std::string result = input;
    size_t keyIndex = 0;
    
    for (size_t i = 0; i < result.size(); ++i) {
        if (isalpha(result[i])) {
            char base = islower(result[i]) ? 'a' : 'A';
            char keyChar = tolower(key[keyIndex % key.size()]) - 'a';
            result[i] = static_cast<char>((result[i] - base - keyChar + 26) % 26 + base);
            keyIndex++;
        }
    }
    
    return result;
}

// Simplified AES Implementation
// Note: This is a very simplified version of AES

// AES S-box (simplified af)
static const unsigned char sBox[16] = {
    0x9, 0x4, 0xA, 0xB, 0xD, 0x1, 0x8, 0x5,
    0x6, 0x2, 0x0, 0x3, 0xC, 0xE, 0xF, 0x7
};

// AES Inverse S-box
static const unsigned char invSBox[16] = {
    0xA, 0x5, 0x9, 0xB, 0x1, 0x7, 0x8, 0xF,
    0x6, 0x0, 0x2, 0x3, 0xC, 0x4, 0xD, 0xE
};

std::vector<unsigned char> AESEncryption::padKey(const std::string& key) const {
    std::vector<unsigned char> result(16, 0); // 16 bytes for AES-128
    
    // Copy key bytes (up to 16)
    for (size_t i = 0; i < std::min(key.size(), size_t(16)); ++i) {
        result[i] = static_cast<unsigned char>(key[i]);
    }
    
    return result;
}

std::vector<unsigned char> AESEncryption::padInput(const std::vector<unsigned char>& input) const {
    // PKCS#7 padding
    size_t padSize = 16 - (input.size() % 16);
    std::vector<unsigned char> padded = input;
    
    for (size_t i = 0; i < padSize; ++i) {
        padded.push_back(static_cast<unsigned char>(padSize));
    }
    
    return padded;
}

std::vector<unsigned char> AESEncryption::processBlock(const std::vector<unsigned char>& block,
                                                     const std::vector<unsigned char>& key,
                                                     bool encrypt) const {
    // Extremely simplified AES block processing    
    std::vector<unsigned char> result = block;
    
    for (size_t i = 0; i < result.size(); ++i) {
        // Apply S-box or inverse S-box based on encrypt/decrypt
        unsigned char nibbleHigh = (result[i] >> 4) & 0xF;
        unsigned char nibbleLow = result[i] & 0xF;
        
        if (encrypt) {
            nibbleHigh = sBox[nibbleHigh];
            nibbleLow = sBox[nibbleLow];
        } else {
            nibbleHigh = invSBox[nibbleHigh];
            nibbleLow = invSBox[nibbleLow];
        }
        
        // Recombine nibbles
        result[i] = (nibbleHigh << 4) | nibbleLow;
        
        // XOR with key (simplified key addition)
        result[i] ^= key[i % key.size()];
    }
    
    return result;
}

std::string AESEncryption::encrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    std::vector<unsigned char> bytes(input.begin(), input.end());    
    std::vector<unsigned char> padded = padInput(bytes);
    std::vector<unsigned char> processedKey = padKey(key);
    
    std::vector<unsigned char> result;
    for (size_t i = 0; i < padded.size(); i += 16) {
        std::vector<unsigned char> block(padded.begin() + i, padded.begin() + i + 16);
        std::vector<unsigned char> processedBlock = processBlock(block, processedKey, true);
        result.insert(result.end(), processedBlock.begin(), processedBlock.end());
    }
    
    return std::string(result.begin(), result.end());
}

std::string AESEncryption::decrypt(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty() || input.size() % 16 != 0) {
        return input; // Input must be a multiple of block size
    }
    
    std::vector<unsigned char> bytes(input.begin(), input.end());    
    std::vector<unsigned char> processedKey = padKey(key);
    
    std::vector<unsigned char> result;
    for (size_t i = 0; i < bytes.size(); i += 16) {
        std::vector<unsigned char> block(bytes.begin() + i, bytes.begin() + i + 16);
        std::vector<unsigned char> processedBlock = processBlock(block, processedKey, false);
        result.insert(result.end(), processedBlock.begin(), processedBlock.end());
    }
    
    // Remove padding
    if (!result.empty()) {
        unsigned char padValue = result.back();
        if (padValue <= 16) {
            bool validPadding = true;
            for (size_t i = 0; i < padValue && validPadding; ++i) {
                if (result[result.size() - 1 - i] != padValue) {
                    validPadding = false;
                }
            }
            
            if (validPadding) {
                result.resize(result.size() - padValue);
            }
        }
    }
    
    return std::string(result.begin(), result.end());
}

std::unique_ptr<EncryptionAlgorithm> EncryptionFactory::createAlgorithm(const std::string& type) {
    if (type == "XOR") {
        return std::make_unique<XOREncryption>();
    } else if (type == "Caesar") {
        return std::make_unique<CaesarCipher>();
    } else if (type == "Vigenere") {
        return std::make_unique<VigenereCipher>();
    } else if (type == "AES") {
        return std::make_unique<AESEncryption>();
    } else {
        // Default to AES
        return std::make_unique<AESEncryption>();
    }
}

std::vector<std::string> EncryptionFactory::listAvailableAlgorithms() {
    return {"XOR", "Caesar", "Vigenere", "AES"};
}

std::unique_ptr<EncryptionAlgorithm> EncryptionFactory::getDefaultAlgorithm() {
    return std::make_unique<AESEncryption>(); // AES as default
}
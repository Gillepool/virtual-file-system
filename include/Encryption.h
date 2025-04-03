#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <string>
#include <vector>
#include <memory>

class EncryptionAlgorithm {
public:
    virtual ~EncryptionAlgorithm() = default;
    virtual std::string encrypt(const std::string& input, const std::string& key) const = 0;
    virtual std::string decrypt(const std::string& input, const std::string& key) const = 0;
    virtual std::string getName() const = 0;
};

// Simple XOR encryption algorithm
class XOREncryption : public EncryptionAlgorithm {
public:
    std::string encrypt(const std::string& input, const std::string& key) const override;
    std::string decrypt(const std::string& input, const std::string& key) const override;
    std::string getName() const override { return "XOR"; }
};

// Caesar Cipher algorithm
class CaesarCipher : public EncryptionAlgorithm {
public:
    std::string encrypt(const std::string& input, const std::string& key) const override;
    std::string decrypt(const std::string& input, const std::string& key) const override;
    std::string getName() const override { return "Caesar"; }
private:
    int keyToShift(const std::string& key) const;
};

// Vigenere Cipher algorithm
class VigenereCipher : public EncryptionAlgorithm {
public:
    std::string encrypt(const std::string& input, const std::string& key) const override;
    std::string decrypt(const std::string& input, const std::string& key) const override;
    std::string getName() const override { return "Vigenere"; }
};

// AES encryption algorithm - simple implementation
class AESEncryption : public EncryptionAlgorithm {
public:
    std::string encrypt(const std::string& input, const std::string& key) const override;
    std::string decrypt(const std::string& input, const std::string& key) const override;
    std::string getName() const override { return "AES"; }
    
private:
    std::vector<unsigned char> padKey(const std::string& key) const;
    std::vector<unsigned char> padInput(const std::vector<unsigned char>& input) const;
    std::vector<unsigned char> processBlock(const std::vector<unsigned char>& block, 
                                           const std::vector<unsigned char>& key, 
                                           bool encrypt) const;
};

class EncryptionFactory {
public:
    static std::unique_ptr<EncryptionAlgorithm> createAlgorithm(const std::string& type);
    static std::vector<std::string> listAvailableAlgorithms();
    static std::unique_ptr<EncryptionAlgorithm> getDefaultAlgorithm();
};

#endif // ENCRYPTION_H
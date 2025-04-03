#ifndef FILENODE_H
#define FILENODE_H

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <deque>
#include <ctime>
#include <algorithm>
#include <stdexcept>
#include "Compression.h"
#include "Encryption.h"

class FileNodeVersion;

class FileNode {
public:
    FileNode(const std::string& name, bool isDirectory, FileNode* parent = nullptr);
    ~FileNode();
    FileNode(const FileNode& other);

    std::string getName() const;
    bool isDirectory() const;
    FileNode* getParent() const;
    std::vector<std::unique_ptr<FileNode>>& getChildren();
    std::string getContent() const;
    size_t getSize() const;
    std::string getPath() const;

    void setContent(const std::string& content);
    void addChild(std::unique_ptr<FileNode> child);
    
    FileNode* findChild(const std::string& name) const;
    void removeChild(const std::string& name);

    void setCompressed(bool compressed, const std::string& algorithmName = "");
    bool isCompressed() const;
    std::string getCompressedContent() const;
    std::string getCompressionAlgorithm() const;
    
    // Encryption
    void setEncrypted(bool encrypted, const std::string& key = "", const std::string& algorithmName = "");
    bool isEncrypted() const;
    void setEncryptionKey(const std::string& key);
    std::string getEncryptionKey() const;
    std::string getEncryptionAlgorithm() const;
    
    // Versioning
    void saveVersion();
    bool restoreVersion(size_t versionIndex);
    size_t getVersionCount() const;
    std::vector<std::time_t> getVersionTimestamps() const;

private:
    std::string name;
    bool isDir;
    FileNode* parent;
    std::vector<std::unique_ptr<FileNode>> children;
    std::string content;
    size_t size;

    bool compressed;
    std::string compressedContent;
    std::string compressionAlgorithm;
    
    bool encrypted;
    std::string encryptionKey;
    std::string encryptionAlgorithm;
    
    std::deque<std::unique_ptr<FileNodeVersion>> versions;
    size_t maxVersions = 10; // Keep at most 10 versions by default
    
    std::string compressContent(const std::string& content) const;
    std::string decompressContent(const std::string& compressedContent) const;
    std::string encryptContent(const std::string& content, const std::string& key) const;
    std::string decryptContent(const std::string& encryptedContent, const std::string& key) const;
};

// Class to store versions of file content
class FileNodeVersion {
public:
    FileNodeVersion(const std::string& content);
    
    FileNodeVersion(const FileNodeVersion& other);
    
    std::string getContent() const;
    std::time_t getTimestamp() const;
    
private:
    std::string content;
    std::time_t timestamp;
};


inline std::time_t getNodeModificationTime(FileNode* node) {
    if (!node) {
        return std::time(nullptr);
    }
    
    auto timestamps = node->getVersionTimestamps();
    if (!timestamps.empty()) {
        return timestamps.back();
    }
    
    return std::time(nullptr);
}

#endif // FILENODE_H
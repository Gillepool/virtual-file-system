#include "../include/FileNode.h"
#include "../include/Compression.h"
#include "../include/Encryption.h"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <iomanip>

FileNode::FileNode(const FileNode& other)
    : name(other.name),
      isDir(other.isDir),
      parent(nullptr), // Will be set by the parent when adding to children
      content(other.content),
      size(other.size),
      compressed(other.compressed),
      compressedContent(other.compressedContent),
      compressionAlgorithm(other.compressionAlgorithm),
      encrypted(other.encrypted),
      encryptionKey(other.encryptionKey),
      encryptionAlgorithm(other.encryptionAlgorithm),
      maxVersions(other.maxVersions)
{
    // Deep copy children
    for (const auto& child : other.children) {
        auto childCopy = std::make_unique<FileNode>(*child);
        childCopy->parent = this;
        children.push_back(std::move(childCopy));
    }
    
    // Deep copy versions
    for (const auto& version : other.versions) {
        auto versionCopy = std::make_unique<FileNodeVersion>(*version);
        versions.push_back(std::move(versionCopy));
    }
}

FileNode::FileNode(const std::string& name, bool isDirectory, FileNode* parent)
    : name(name), isDir(isDirectory), parent(parent), content(""), size(0),
      compressed(false), encrypted(false), compressionAlgorithm(""), encryptionAlgorithm("") {
}

FileNode::~FileNode() {
    // Children will be automatically deleted by unique_ptr
}

std::string FileNode::getName() const {
    return name;
}

bool FileNode::isDirectory() const {
    return isDir;
}

FileNode* FileNode::getParent() const {
    return parent;
}

std::vector<std::unique_ptr<FileNode>>& FileNode::getChildren() {
    return children;
}

std::string FileNode::getContent() const {
    if (isDir) {
        return "";
    }
    
    std::string result = content;
    
    // If content is encrypted, decrypt it first
    if (encrypted && !encryptionKey.empty()) {
        result = decryptContent(result, encryptionKey);
    }
    
    // If content is compressed, decompress it
    if (compressed) {
        result = decompressContent(compressedContent);
    }
    
    return result;
}

size_t FileNode::getSize() const {
    return size;
}

std::string FileNode::getPath() const {
    if (parent == nullptr) {
        // This is the root
        return "/";
    }
    
    if (parent->getParent() == nullptr) {
        // Parent is the root
        return "/" + name;
    }
    
    // Recursively build path
    return parent->getPath() + "/" + name;
}

void FileNode::setContent(const std::string& newContent) {
    if (!isDir) {
        // Save a version before changing content
        if (!content.empty()) {
            saveVersion();
        }
        
        content = newContent;
        size = content.size();
        
        // If compression is enabled, compress the content
        if (compressed) {
            compressedContent = compressContent(content);
        }
        
        // If encryption is enabled, encrypt the content
        if (encrypted && !encryptionKey.empty()) {
            content = encryptContent(content, encryptionKey);
        }
    }
}

void FileNode::addChild(std::unique_ptr<FileNode> child) {
    if (isDir) {
        children.push_back(std::move(child));
    }
}

FileNode* FileNode::findChild(const std::string& childName) const {
    for (const auto& child : children) {
        if (child->getName() == childName) {
            return child.get();
        }
    }
    return nullptr;
}

void FileNode::removeChild(const std::string& childName) {
    auto it = std::remove_if(children.begin(), children.end(), 
                            [&childName](const std::unique_ptr<FileNode>& child) {
                                return child->getName() == childName;
                            });
    
    if (it != children.end()) {
        children.erase(it, children.end());
    }
}

void FileNode::setCompressed(bool compress, const std::string& algorithmName) {
    if (isDir || compressed == compress) {
        return;
    }
    
    compressed = compress;
    
    if (compressed) {
        if (!algorithmName.empty()) {
            compressionAlgorithm = algorithmName;
        } else {
            compressionAlgorithm = CompressionFactory::getDefaultAlgorithm()->getName();
        }
        
        compressedContent = compressContent(content);
    } else {
        compressionAlgorithm = "";
        compressedContent = "";
    }
}

bool FileNode::isCompressed() const {
    return compressed;
}

std::string FileNode::getCompressedContent() const {
    return compressedContent;
}

std::string FileNode::getCompressionAlgorithm() const {
    return compressionAlgorithm;
}

std::string FileNode::compressContent(const std::string& input) const {
    if (input.empty()) {
        return "";
    }
    
    auto algorithm = CompressionFactory::createAlgorithm(compressionAlgorithm);
    return algorithm->compress(input);
}

std::string FileNode::decompressContent(const std::string& input) const {
    if (input.empty()) {
        return "";
    }
    
    auto algorithm = CompressionFactory::createAlgorithm(compressionAlgorithm);
    return algorithm->decompress(input);
}

// Encryption methods
void FileNode::setEncrypted(bool encrypt, const std::string& key, const std::string& algorithmName) {
    if (isDir || encrypted == encrypt) {
        return;
    }
    
    if (encrypt && !key.empty()) {
        // Set encryption algorithm if specified, otherwise use default
        if (!algorithmName.empty()) {
            encryptionAlgorithm = algorithmName;
        } else {
            encryptionAlgorithm = EncryptionFactory::getDefaultAlgorithm()->getName();
        }
        
        encryptionKey = key;
        
        // We need to decrypt content first if it was already encrypted
        std::string decrypted = encrypted ? decryptContent(content, encryptionKey) : content;
        encrypted = true;
        content = encryptContent(decrypted, encryptionKey);
    } 
    else if (!encrypt && encrypted) {
        // Decrypt the content
        content = decryptContent(content, encryptionKey);
        encrypted = false;
        encryptionKey = "";
        encryptionAlgorithm = "";
    }
}

bool FileNode::isEncrypted() const {
    return encrypted;
}

void FileNode::setEncryptionKey(const std::string& key) {
    if (encrypted && !key.empty() && key != encryptionKey) {
        // Decrypt with old key, then encrypt with new key
        std::string decrypted = decryptContent(content, encryptionKey);
        encryptionKey = key;
        content = encryptContent(decrypted, encryptionKey);
    } else if (!encrypted) {
        encryptionKey = key;
    }
}

std::string FileNode::getEncryptionKey() const {
    return encryptionKey;
}

std::string FileNode::getEncryptionAlgorithm() const {
    return encryptionAlgorithm;
}

std::string FileNode::encryptContent(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    auto algorithm = EncryptionFactory::createAlgorithm(encryptionAlgorithm);
    return algorithm->encrypt(input, key);
}

std::string FileNode::decryptContent(const std::string& input, const std::string& key) const {
    if (input.empty() || key.empty()) {
        return input;
    }
    
    auto algorithm = EncryptionFactory::createAlgorithm(encryptionAlgorithm);
    return algorithm->decrypt(input, key);
}

void FileNode::saveVersion() {
    if (isDir) {
        return;
    }
    
    auto version = std::make_unique<FileNodeVersion>(content);
    versions.push_front(std::move(version));
    
    while (versions.size() > maxVersions) {
        versions.pop_back();
    }
}

bool FileNode::restoreVersion(size_t versionIndex) {
    if (isDir || versionIndex >= versions.size()) {
        return false;
    }
    
    saveVersion();
    
    std::string versionContent = versions[versionIndex]->getContent();
    
    // We bypass the regular setContent to avoid creating another version
    content = versionContent;
    size = content.size();
    
    // Apply compression and encryption if needed
    if (compressed) {
        compressedContent = compressContent(content);
    }
    
    if (encrypted && !encryptionKey.empty()) {
        content = encryptContent(content, encryptionKey);
    }
    
    return true;
}

size_t FileNode::getVersionCount() const {
    return versions.size();
}

std::vector<std::time_t> FileNode::getVersionTimestamps() const {
    std::vector<std::time_t> timestamps;
    for (const auto& version : versions) {
        timestamps.push_back(version->getTimestamp());
    }
    return timestamps;
}

FileNodeVersion::FileNodeVersion(const std::string& content)
    : content(content) {
    timestamp = std::time(nullptr);
}

std::string FileNodeVersion::getContent() const {
    return content;
}

std::time_t FileNodeVersion::getTimestamp() const {
    return timestamp;
}

FileNodeVersion::FileNodeVersion(const FileNodeVersion& other)
    : content(other.content), timestamp(other.timestamp)
{
}
#include "../include/VirtualFileSystem.h"
#include "../include/Compression.h"
#include "../include/Encryption.h"
#include <sstream>
#include <algorithm>
#include <iterator>
#include <stack>
#include <filesystem>

VirtualFileSystem::VirtualFileSystem(size_t diskSize)
    : diskSize(diskSize), usedSpace(0) {
    // Create the root directory
    root = std::make_unique<FileNode>("/", true);
    currentDirectory = root.get();
}

VirtualFileSystem::~VirtualFileSystem() {
    // Unmount all volumes before destruction
    auto volumesCopy = listMountedVolumes();
    for (const auto& mountPoint : volumesCopy) {
        unmountVolume(mountPoint);
    }
    // FileNode cleanup is handled by smart pointers
}

VirtualFileSystem& VirtualFileSystem::operator=(const VirtualFileSystem& other) {
    if (this != &other) {
        if (other.root) {
            root = std::make_unique<FileNode>(*other.root);
            
            std::string currentPath = other.getCurrentPath();
            currentDirectory = resolvePath(currentPath);
            if (!currentDirectory) {
                currentDirectory = root.get();
            }
        } else {
            root = nullptr;
            currentDirectory = nullptr;
        }
        
        diskSize = other.diskSize;
        usedSpace = other.usedSpace;
        
        mountedVolumes.clear();
        for (const auto& [path, info] : other.mountedVolumes) {
            MountInfo newInfo;
            newInfo.diskImage = info.diskImage;
            newInfo.fs = std::make_unique<VirtualFileSystem>();
            newInfo.fs->loadFromDisk(info.diskImage);
            
            std::string mountPath = path;
            newInfo.mountPoint = resolvePath(mountPath);
            
            mountedVolumes[path] = std::move(newInfo);
        }
        
        fileTags = other.fileTags;
    }
    return *this;
}

VirtualFileSystem* VirtualFileSystem::getResponsibleFS(const std::string& path, std::string& localPath) {
    std::string volumePath = getVolumeForPath(path);
    
    if (volumePath.empty()) {
        localPath = path;
        return this;
    } else {
        localPath = path.substr(volumePath.length());
        if (localPath.empty()) {
            localPath = "/";
        }
        return mountedVolumes[volumePath].fs.get();
    }
}

bool VirtualFileSystem::isMountPoint(const std::string& path) const {
    return mountedVolumes.find(path) != mountedVolumes.end();
}

std::string VirtualFileSystem::getVolumeForPath(const std::string& path) const {
    std::string normalized = path;
    
    // Ensure path starts with /
    if (path.empty() || path[0] != '/') {
        normalized = currentDirectory->getPath() + 
                    (path.empty() || path[0] != '/' ? "/" : "") + 
                    path;
    }
    
    // Find the longest matching mount point
    std::string matchedVolume;
    for (const auto& [mountPoint, _] : mountedVolumes) {
        if (normalized.find(mountPoint) == 0) {
            if (mountPoint.length() > matchedVolume.length()) {
                matchedVolume = mountPoint;
            }
        }
    }
    
    return matchedVolume;
}


bool VirtualFileSystem::mkdir(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->mkdir(localPath);
    }
    
    FileNode* targetParent = currentDirectory;
    std::string dirName = path;
    
    if (path.find('/') != std::string::npos) {
        size_t lastSlash = path.find_last_of('/');
        std::string parentPath = path.substr(0, lastSlash);
        dirName = path.substr(lastSlash + 1);
        
        if (parentPath.empty() && path[0] == '/') {
            // Path starts with '/', use root as parent
            targetParent = root.get();
        } else {
            targetParent = resolvePath(parentPath);
            if (!targetParent || !targetParent->isDirectory()) {
                return false;
            }
        }
    }
    
    if (targetParent->findChild(dirName)) {
        return false;
    }
    
    auto newDir = std::make_unique<FileNode>(dirName, true, targetParent);
    targetParent->addChild(std::move(newDir));
    updateUsedSpace();
    
    return true;
}

bool VirtualFileSystem::touch(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->touch(localPath);
    }
    
    FileNode* targetParent = currentDirectory;
    std::string fileName = path;
    
    if (path.find('/') != std::string::npos) {
        size_t lastSlash = path.find_last_of('/');
        std::string parentPath = path.substr(0, lastSlash);
        fileName = path.substr(lastSlash + 1);
        
        if (parentPath.empty() && path[0] == '/') {
            targetParent = root.get();
        } else {
            targetParent = resolvePath(parentPath);
            if (!targetParent || !targetParent->isDirectory()) {
                return false;
            }
        }
    }
    
    if (targetParent->findChild(fileName)) {
        return false;
    }
    
    auto newFile = std::make_unique<FileNode>(fileName, false, targetParent);
    targetParent->addChild(std::move(newFile));
    updateUsedSpace();
    
    return true;
}

bool VirtualFileSystem::cd(const std::string& path) {
    if (path == "/") {
        currentDirectory = root.get();
        return true;
    } else if (path == "..") {
        if (currentDirectory->getParent()) {
            currentDirectory = currentDirectory->getParent();
            return true;
        }
        return false;
    }
    
    std::string fullPath = path[0] == '/' ? path : (currentDirectory->getPath() + "/" + path);
    if (isMountPoint(fullPath)) {
        // We can't cd into a mount point, as it's a different filesystem
        return false;
    }
    
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        // Can't cd across filesystems
        return false;
    }
    
    FileNode* target = resolvePath(path);
    if (target && target->isDirectory()) {
        currentDirectory = target;
        return true;
    }
    
    return false;
}

std::vector<std::string> VirtualFileSystem::ls(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->ls(localPath);
    }
    
    FileNode* target = currentDirectory;
    
    if (!path.empty()) {
        target = resolvePath(path);
        if (!target || !target->isDirectory()) {
            return {};
        }
    }
    
    std::vector<std::string> result;
    for (const auto& child : target->getChildren()) {
        std::string entry = child->getName();
        if (child->isDirectory()) {
            entry += "/";
        }
        result.push_back(entry);
    }
    
    if (target == root.get()) {
        for (const auto& [mountPoint, _] : mountedVolumes) {
            std::string mountName = mountPoint;
            if (mountPoint.length() > 1) {  // Skip the root
                size_t lastSlash = mountPoint.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    mountName = mountPoint.substr(lastSlash + 1);
                }
                // Only add if it's directly under root
                if (mountPoint.find('/', 1) == std::string::npos) {
                    result.push_back(mountName + "@");  // Use @ to mark mount points
                }
            }
        }
    }
    
    return result;
}

std::string VirtualFileSystem::cat(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->cat(localPath);
    }
    
    FileNode* target = resolvePath(path);
    
    if (target && !target->isDirectory()) {
        return target->getContent();
    }
    
    return "";
}

bool VirtualFileSystem::write(const std::string& path, const std::string& content) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->write(localPath, content);
    }
    
    FileNode* target = resolvePath(path);
    
    if (!target) {
        size_t lastSlash = path.find_last_of('/');
        if (lastSlash != std::string::npos) {
            std::string dirPath = path.substr(0, lastSlash);
            std::string fileName = path.substr(lastSlash + 1);
            
            FileNode* parent = resolvePath(dirPath);
            if (parent && parent->isDirectory()) {
                auto newFile = std::make_unique<FileNode>(fileName, false, parent);
                newFile->setContent(content);
                parent->addChild(std::move(newFile));
                updateUsedSpace();
                return true;
            }
            return false;
        } else {
            auto newFile = std::make_unique<FileNode>(path, false, currentDirectory);
            newFile->setContent(content);
            currentDirectory->addChild(std::move(newFile));
            updateUsedSpace();
            return true;
        }
    }
    
    if (!target->isDirectory()) {
        size_t oldSize = target->getSize();
        target->setContent(content);
        usedSpace = usedSpace - oldSize + content.size();
        return true;
    }
    
    return false;
}

bool VirtualFileSystem::remove(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->remove(localPath);
    }
    
    FileNode* target = resolvePath(path);
    if (!target) {
        return false;
    }
    
    if (isMountPoint(target->getPath())) {
        return false;
    }
    
    FileNode* parent = target->getParent();
    if (!parent) {
        return false;
    }
    
    parent->removeChild(target->getName());
    updateUsedSpace();
    return true;
}


bool VirtualFileSystem::createVolume(const std::string& volumeName, size_t volumeSize) {
    auto newFS = std::make_unique<VirtualFileSystem>(volumeSize);
    
    if (!newFS->saveToDisk(volumeName)) {
        return false;
    }
    
    return true;
}

bool VirtualFileSystem::mountVolume(const std::string& diskImage, const std::string& mountPoint) {
    if (!std::filesystem::exists(diskImage)) {
        return false;
    }
    
    if (isMountPoint(mountPoint)) {
        return false;
    }
    
    std::string normalizedMountPoint = mountPoint;
    if (normalizedMountPoint.empty() || normalizedMountPoint.back() != '/') {
        normalizedMountPoint += '/';
    }
    
    FileNode* mountDir = resolvePath(mountPoint);
    if (!mountDir) {
        if (!mkdir(mountPoint)) {
            return false;
        }
        mountDir = resolvePath(mountPoint);
    }
    
    if (!mountDir || !mountDir->isDirectory()) {
        return false;
    }
    
    auto newFS = std::make_unique<VirtualFileSystem>();
    if (!newFS->loadFromDisk(diskImage)) {
        return false;
    }
    
    MountInfo mountInfo;
    mountInfo.diskImage = diskImage;
    mountInfo.fs = std::move(newFS);
    mountInfo.mountPoint = mountDir;
    
    mountedVolumes[mountDir->getPath()] = std::move(mountInfo);
    
    return true;
}

bool VirtualFileSystem::unmountVolume(const std::string& mountPoint) {
    std::string normalizedMountPoint = mountPoint;
    if (!normalizedMountPoint.empty() && normalizedMountPoint.back() != '/') {
        normalizedMountPoint += '/';
    }
    
    auto it = mountedVolumes.find(normalizedMountPoint);
    if (it == mountedVolumes.end()) {
        return false;
    }
    
    it->second.fs->saveToDisk(it->second.diskImage);
    
    mountedVolumes.erase(it);
    
    return true;
}

std::vector<std::string> VirtualFileSystem::listMountedVolumes() const {
    std::vector<std::string> result;
    for (const auto& [mountPoint, info] : mountedVolumes) {
        result.push_back(mountPoint);
    }
    return result;
}

bool VirtualFileSystem::compressFile(const std::string& path, bool compress, const std::string& algorithm) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->compressFile(localPath, compress, algorithm);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    target->setCompressed(compress, algorithm);
    return true;
}

bool VirtualFileSystem::isFileCompressed(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->isFileCompressed(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    return target->isCompressed();
}

std::string VirtualFileSystem::getFileCompressionAlgorithm(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->getFileCompressionAlgorithm(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory() || !target->isCompressed()) {
        return "";
    }
    
    return target->getCompressionAlgorithm();
}

std::vector<std::string> VirtualFileSystem::listCompressionAlgorithms() const {
    return CompressionFactory::listAvailableAlgorithms();
}

bool VirtualFileSystem::encryptFile(const std::string& path, const std::string& key, const std::string& algorithm) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->encryptFile(localPath, key, algorithm);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    target->setEncrypted(true, key, algorithm);
    return true;
}

bool VirtualFileSystem::decryptFile(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->decryptFile(localPath);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    target->setEncrypted(false);
    return true;
}

bool VirtualFileSystem::isFileEncrypted(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->isFileEncrypted(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    return target->isEncrypted();
}

std::string VirtualFileSystem::getFileEncryptionAlgorithm(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->getFileEncryptionAlgorithm(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory() || !target->isEncrypted()) {
        return "";
    }
    
    return target->getEncryptionAlgorithm();
}

bool VirtualFileSystem::changeEncryptionKey(const std::string& path, const std::string& newKey) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->changeEncryptionKey(localPath, newKey);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    if (!target->isEncrypted()) {
        return false;
    }
    
    target->setEncryptionKey(newKey);
    return true;
}

std::vector<std::string> VirtualFileSystem::listEncryptionAlgorithms() const {
    return EncryptionFactory::listAvailableAlgorithms();
}

bool VirtualFileSystem::saveFileVersion(const std::string& path) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->saveFileVersion(localPath);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    target->saveVersion();
    return true;
}

bool VirtualFileSystem::restoreFileVersion(const std::string& path, size_t versionIndex) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->restoreFileVersion(localPath, versionIndex);
    }
    
    FileNode* target = resolvePath(path);
    if (!target || target->isDirectory()) {
        return false;
    }
    
    return target->restoreVersion(versionIndex);
}

size_t VirtualFileSystem::getFileVersionCount(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->getFileVersionCount(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory()) {
        return 0;
    }
    
    return target->getVersionCount();
}

std::vector<std::time_t> VirtualFileSystem::getFileVersionTimestamps(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->getFileVersionTimestamps(localPath);
    }
    
    FileNode* target = nonConstThis->resolvePath(path);
    if (!target || target->isDirectory()) {
        return {};
    }
    
    return target->getVersionTimestamps();
}

FileNode* VirtualFileSystem::resolvePath(const std::string& path) {
    if (path.empty()) {
        return currentDirectory;
    }
    
    FileNode* startNode = currentDirectory;
    if (path[0] == '/') {
        // Absolute path
        startNode = root.get();
    }
    
    std::vector<std::string> pathParts = splitPath(path);
    FileNode* current = startNode;
    
    for (const auto& part : pathParts) {
        if (part.empty() || part == ".") {
            continue;
        } else if (part == "..") {
            if (current->getParent()) {
                current = current->getParent();
            }
        } else {
            FileNode* next = current->findChild(part);
            if (!next) {
                return nullptr;
            }
            current = next;
        }
    }
    
    return current;
}

std::string VirtualFileSystem::getCurrentPath() const {
    return currentDirectory->getPath();
}

std::vector<std::string> VirtualFileSystem::splitPath(const std::string& path) {
    std::vector<std::string> parts;
    std::string normalizedPath = path;
    
    // Skip leading slash for absolute paths
    if (!normalizedPath.empty() && normalizedPath[0] == '/') {
        normalizedPath = normalizedPath.substr(1);
    }
    
    std::istringstream iss(normalizedPath);
    std::string part;
    
    while (std::getline(iss, part, '/')) {
        parts.push_back(part);
    }
    
    return parts;
}

void VirtualFileSystem::updateUsedSpace() {
    usedSpace = 0;
    
    std::function<void(FileNode*)> calculateSize;
    calculateSize = [this, &calculateSize](FileNode* node) {
        usedSpace += sizeof(FileNode);
        usedSpace += node->getName().size();
        
        if (!node->isDirectory()) {
            usedSpace += node->getContent().size();
        } else {
            for (const auto& child : node->getChildren()) {
                calculateSize(child.get());
            }
        }
    };
    
    calculateSize(root.get());
}


bool VirtualFileSystem::saveToDisk(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.write(reinterpret_cast<char*>(&diskSize), sizeof(diskSize));
    file.write(reinterpret_cast<char*>(&usedSpace), sizeof(usedSpace));
    
    std::function<void(FileNode*, std::ofstream&)> serializeNode;
    serializeNode = [&serializeNode](FileNode* node, std::ofstream& out) {
        size_t nameLen = node->getName().size();
        out.write(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        out.write(node->getName().c_str(), nameLen);
        
        bool isDir = node->isDirectory();
        out.write(reinterpret_cast<char*>(&isDir), sizeof(isDir));
        
        if (!isDir) {
            std::string content = node->getContent();
            size_t contentLen = content.size();
            out.write(reinterpret_cast<char*>(&contentLen), sizeof(contentLen));
            out.write(content.c_str(), contentLen);
            
            bool compressed = node->isCompressed();
            out.write(reinterpret_cast<char*>(&compressed), sizeof(compressed));
            
            std::string compressionAlg = node->getCompressionAlgorithm();
            size_t compAlgLen = compressionAlg.size();
            out.write(reinterpret_cast<char*>(&compAlgLen), sizeof(compAlgLen));
            if (compAlgLen > 0) {
                out.write(compressionAlg.c_str(), compAlgLen);
            }
            
            bool encrypted = node->isEncrypted();
            out.write(reinterpret_cast<char*>(&encrypted), sizeof(encrypted));
            
            if (encrypted) {
                std::string encryptionAlg = node->getEncryptionAlgorithm();
                size_t encAlgLen = encryptionAlg.size();
                out.write(reinterpret_cast<char*>(&encAlgLen), sizeof(encAlgLen));
                if (encAlgLen > 0) {
                    out.write(encryptionAlg.c_str(), encAlgLen);
                }
                
                std::string key = node->getEncryptionKey();
                size_t keyLen = key.size();
                out.write(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
                out.write(key.c_str(), keyLen);
            }
            
            size_t versionCount = node->getVersionCount();
            out.write(reinterpret_cast<char*>(&versionCount), sizeof(versionCount));
            
            if (versionCount > 0) {
                auto timestamps = node->getVersionTimestamps();
                for (size_t i = 0; i < versionCount; ++i) {
                    std::time_t timestamp = timestamps[i];
                    out.write(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
                }
            }
        } else {
            size_t childCount = node->getChildren().size();
            out.write(reinterpret_cast<char*>(&childCount), sizeof(childCount));
            
            for (const auto& child : node->getChildren()) {
                serializeNode(child.get(), out);
            }
        }
    };
    
    serializeNode(root.get(), file);
    
    std::string currentPath = currentDirectory->getPath();
    size_t pathLen = currentPath.size();
    file.write(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
    file.write(currentPath.c_str(), pathLen);
    
    size_t mountCount = mountedVolumes.size();
    file.write(reinterpret_cast<char*>(&mountCount), sizeof(mountCount));
    
    for (const auto& [mountPoint, info] : mountedVolumes) {
        size_t mountPointLen = mountPoint.size();
        file.write(reinterpret_cast<char*>(&mountPointLen), sizeof(mountPointLen));
        file.write(mountPoint.c_str(), mountPointLen);
        
        size_t diskImageLen = info.diskImage.size();
        file.write(reinterpret_cast<char*>(&diskImageLen), sizeof(diskImageLen));
        file.write(info.diskImage.c_str(), diskImageLen);
        
        info.fs->saveToDisk(info.diskImage);
    }
    
    return true;
}

bool VirtualFileSystem::loadFromDisk(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    file.read(reinterpret_cast<char*>(&diskSize), sizeof(diskSize));
    file.read(reinterpret_cast<char*>(&usedSpace), sizeof(usedSpace));
    
    std::function<std::unique_ptr<FileNode>(FileNode*, std::ifstream&)> deserializeNode;
    deserializeNode = [&deserializeNode](FileNode* parent, std::ifstream& in) {
        size_t nameLen;
        in.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        std::string name(nameLen, '\0');
        in.read(&name[0], nameLen);
        
        bool isDir;
        in.read(reinterpret_cast<char*>(&isDir), sizeof(isDir));
        
        auto node = std::make_unique<FileNode>(name, isDir, parent);
        
        if (!isDir) {
            size_t contentLen;
            in.read(reinterpret_cast<char*>(&contentLen), sizeof(contentLen));
            std::string content(contentLen, '\0');
            in.read(&content[0], contentLen);
            
            bool compressed;
            in.read(reinterpret_cast<char*>(&compressed), sizeof(compressed));
            
            std::string compressionAlg;
            size_t compAlgLen;
            in.read(reinterpret_cast<char*>(&compAlgLen), sizeof(compAlgLen));
            if (compAlgLen > 0) {
                compressionAlg.resize(compAlgLen, '\0');
                in.read(&compressionAlg[0], compAlgLen);
            }
            
            bool encrypted;
            in.read(reinterpret_cast<char*>(&encrypted), sizeof(encrypted));
            
            std::string encryptionAlg;
            std::string key;
            if (encrypted) {
                size_t encAlgLen;
                in.read(reinterpret_cast<char*>(&encAlgLen), sizeof(encAlgLen));
                if (encAlgLen > 0) {
                    encryptionAlg.resize(encAlgLen, '\0');
                    in.read(&encryptionAlg[0], encAlgLen);
                }
                
                size_t keyLen;
                in.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
                key.resize(keyLen, '\0');
                in.read(&key[0], keyLen);
            }
            
            size_t versionCount;
            in.read(reinterpret_cast<char*>(&versionCount), sizeof(versionCount));
            
            if (versionCount > 0) {
                for (size_t i = 0; i < versionCount; ++i) {
                    std::time_t timestamp;
                    in.read(reinterpret_cast<char*>(&timestamp), sizeof(timestamp));
                    // We don't need to do anything with the timestamp here
                    // as we're not restoring versions at load time
                }
            }
            
            node->setContent(content);
            
            if (compressed) {
                node->setCompressed(true, compressionAlg);
            }
            
            if (encrypted && !key.empty()) {
                node->setEncrypted(true, key, encryptionAlg);
            }
        } else {
            size_t childCount;
            in.read(reinterpret_cast<char*>(&childCount), sizeof(childCount));
            
            for (size_t i = 0; i < childCount; ++i) {
                auto child = deserializeNode(node.get(), in);
                node->addChild(std::move(child));
            }
        }
        
        return node;
    };
    
    root = deserializeNode(nullptr, file);
    
    size_t pathLen;
    file.read(reinterpret_cast<char*>(&pathLen), sizeof(pathLen));
    std::string currentPath(pathLen, '\0');
    file.read(&currentPath[0], pathLen);
    
    currentDirectory = resolvePath(currentPath);
    if (!currentDirectory) {
        currentDirectory = root.get();
    }
    
    size_t mountCount;
    if (file.read(reinterpret_cast<char*>(&mountCount), sizeof(mountCount))) {
        for (size_t i = 0; i < mountCount; ++i) {
            size_t mountPointLen;
            file.read(reinterpret_cast<char*>(&mountPointLen), sizeof(mountPointLen));
            std::string mountPoint(mountPointLen, '\0');
            file.read(&mountPoint[0], mountPointLen);
            
            size_t diskImageLen;
            file.read(reinterpret_cast<char*>(&diskImageLen), sizeof(diskImageLen));
            std::string diskImage(diskImageLen, '\0');
            file.read(&diskImage[0], diskImageLen);
            
            mountVolume(diskImage, mountPoint);
        }
    }
    
    return true;
}

size_t VirtualFileSystem::getFreeSpace() const {
    return diskSize - usedSpace;
}

size_t VirtualFileSystem::getTotalSpace() const {
    return diskSize;
}

size_t VirtualFileSystem::getUsedSpace() const {
    return usedSpace;
}

std::vector<std::string> VirtualFileSystem::search(const SearchFilter& filter, const std::string& startPath) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(startPath, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->search(filter, localPath);
    }
    
    FileNode* startNode = currentDirectory;
    if (!startPath.empty()) {
        startNode = resolvePath(startPath);
        if (!startNode) {
            return {};
        }
    }
    
    std::vector<std::string> results;
    
    std::string basePath = startPath.empty() ? getCurrentPath() : startPath;
    searchRecursive(startNode, basePath, filter, results);
    
    return results;
}

void VirtualFileSystem::searchRecursive(FileNode* node, const std::string& currentPath, 
                                        const SearchFilter& filter, 
                                        std::vector<std::string>& results) {
    if (!node) {
        return;
    }
    
    bool isMatch = matchesFilter(node, currentPath, filter);
    
    if (isMatch) {
        std::string relativePath = (currentPath == "/" || currentPath.empty()) 
                                  ? "/" + node->getName() 
                                  : currentPath + "/" + node->getName();
        
        if (node == root.get()) {
            relativePath = "/";
        }
        
        while (relativePath.find("//") != std::string::npos) {
            relativePath.replace(relativePath.find("//"), 2, "/");
        }
        
        results.push_back(relativePath);
    }
    
    if (node->isDirectory() && !filter.filesOnly) {
        std::string newPath = (currentPath == "/" || currentPath.empty()) 
                             ? "/" + node->getName() 
                             : currentPath + "/" + node->getName();
        
        if (node == root.get()) {
            newPath = "/";
        }
        
        while (newPath.find("//") != std::string::npos) {
            newPath.replace(newPath.find("//"), 2, "/");
        }
        
        for (const auto& child : node->getChildren()) {
            searchRecursive(child.get(), newPath, filter, results);
        }
    }
}

bool VirtualFileSystem::matchesFilter(FileNode* node, const std::string& filePath, const SearchFilter& filter) {
    if (!node) {
        return false;
    }
    
    if (filter.filesOnly && node->isDirectory()) {
        return false;
    }
    if (filter.directoriesOnly && !node->isDirectory()) {
        return false;
    }
    
    if (filter.nameContains.has_value()) {
        if (node->getName().find(filter.nameContains.value()) == std::string::npos) {
            return false;
        }
    }
    
    if (filter.namePattern.has_value()) {
        std::string name = node->getName();
        if (!std::regex_search(name, filter.namePattern.value())) {
            return false;
        }
    }
    
    if (!node->isDirectory()) {
        size_t fileSize = node->getSize();
        
        if (filter.minSize.has_value() && fileSize < filter.minSize.value()) {
            return false;
        }
        
        if (filter.maxSize.has_value() && fileSize > filter.maxSize.value()) {
            return false;
        }
    }
    
    std::time_t modTime = getNodeModificationTime(node);
    
    if (filter.modifiedAfter.has_value() && modTime < filter.modifiedAfter.value()) {
        return false;
    }
    
    if (filter.modifiedBefore.has_value() && modTime > filter.modifiedBefore.value()) {
        return false;
    }
    
    if (!node->isDirectory()) {
        if (filter.contentContains.has_value()) {
            if (!contentMatches(node, filter.contentContains.value(), false)) {
                return false;
            }
        }
        
        if (filter.contentPattern.has_value()) {
            if (!contentMatches(node, "", true)) {
                return false;
            }
        }
    }
    
    if (!filter.tags.empty()) {
        std::string normalizedPath = filePath;
        if (normalizedPath.empty() || normalizedPath[0] != '/') {
            normalizedPath = "/" + normalizedPath;
        }
        normalizedPath += "/" + node->getName();
        
        auto it = fileTags.find(normalizedPath);
        if (it == fileTags.end()) {
            return false; // No tags for this file
        }
        
        const std::vector<std::string>& nodeTags = it->second;
        
        for (const auto& tag : filter.tags) {
            if (std::find(nodeTags.begin(), nodeTags.end(), tag) == nodeTags.end()) {
                return false; // Tag not found
            }
        }
    }
    
    if (filter.customFilter) {
        if (!filter.customFilter(node)) {
            return false;
        }
    }
    
    // If we got here, the node matches all filters
    return true;
}

bool VirtualFileSystem::contentMatches(FileNode* node, const std::string& pattern, bool isRegex) {
    if (!node || node->isDirectory()) {
        return false;
    }
    
    std::string content = node->getContent();
    
    if (isRegex) {
        try {
            std::regex regex(pattern);
            return std::regex_search(content, regex);
        } catch (const std::regex_error&) {
            return false;
        }
    } else {
        return content.find(pattern) != std::string::npos;
    }
}


std::vector<std::string> VirtualFileSystem::searchByName(const std::string& namePattern, bool useRegex, const std::string& startPath) {
    SearchFilter filter;
    
    if (useRegex) {
        try {
            filter.namePattern = std::regex(namePattern);
        } catch (const std::regex_error&) {
            return {}; 
        }
    } else {
        filter.nameContains = namePattern;
    }
    
    return search(filter, startPath);
}

std::vector<std::string> VirtualFileSystem::searchByContent(const std::string& contentPattern, bool useRegex, const std::string& startPath) {
    SearchFilter filter;
    filter.filesOnly = true; 
    
    if (useRegex) {
        try {
            filter.contentPattern = std::regex(contentPattern);
        } catch (const std::regex_error&) {
            return {}; 
        }
    } else {
        filter.contentContains = contentPattern;
    }
    
    return search(filter, startPath);
}

std::vector<std::string> VirtualFileSystem::searchByTag(const std::string& tag, const std::string& startPath) {
    SearchFilter filter;
    filter.tags.push_back(tag);
    
    return search(filter, startPath);
}

std::vector<std::string> VirtualFileSystem::searchBySize(size_t minSize, size_t maxSize, const std::string& startPath) {
    SearchFilter filter;
    filter.filesOnly = true;
    filter.minSize = minSize;
    filter.maxSize = maxSize;
    
    return search(filter, startPath);
}

std::vector<std::string> VirtualFileSystem::searchByDate(time_t modifiedAfter, time_t modifiedBefore, const std::string& startPath) {
    SearchFilter filter;
    filter.modifiedAfter = modifiedAfter;
    filter.modifiedBefore = modifiedBefore;
    
    return search(filter, startPath);
}


bool VirtualFileSystem::addTag(const std::string& path, const std::string& tag) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->addTag(localPath, tag);
    }
    
    FileNode* node = resolvePath(path);
    if (!node) {
        return false;
    }
    
    std::string normalizedPath = path;
    if (normalizedPath.empty() || normalizedPath[0] != '/') {
        normalizedPath = "/" + normalizedPath;
    }
    
    auto& tags = fileTags[normalizedPath];
    if (std::find(tags.begin(), tags.end(), tag) == tags.end()) {
        tags.push_back(tag);
    }
    
    return true;
}

bool VirtualFileSystem::removeTag(const std::string& path, const std::string& tag) {
    std::string localPath;
    VirtualFileSystem* responsibleFS = getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->removeTag(localPath, tag);
    }
    
    FileNode* node = resolvePath(path);
    if (!node) {
        return false;
    }
    
    std::string normalizedPath = path;
    if (normalizedPath.empty() || normalizedPath[0] != '/') {
        normalizedPath = "/" + normalizedPath;
    }
    
    auto it = fileTags.find(normalizedPath);
    if (it != fileTags.end()) {
        auto& tags = it->second;
        tags.erase(std::remove(tags.begin(), tags.end(), tag), tags.end());
        return true;
    }
    
    return false;
}

std::vector<std::string> VirtualFileSystem::getFileTags(const std::string& path) const {
    std::string localPath;
    VirtualFileSystem* nonConstThis = const_cast<VirtualFileSystem*>(this);
    VirtualFileSystem* responsibleFS = nonConstThis->getResponsibleFS(path, localPath);
    
    if (responsibleFS != this) {
        return responsibleFS->getFileTags(localPath);
    }
    
    FileNode* node = nonConstThis->resolvePath(path);
    if (!node) {
        return {};
    }
    
    std::string normalizedPath = path;
    if (normalizedPath.empty() || normalizedPath[0] != '/') {
        normalizedPath = "/" + normalizedPath;
    }
    
    auto it = fileTags.find(normalizedPath);
    if (it != fileTags.end()) {
        return it->second;
    }
    
    return {};
}

std::vector<std::string> VirtualFileSystem::getAllTags() const {
    std::vector<std::string> allTags;
    std::set<std::string> uniqueTags;
    
    for (const auto& [path, tags] : fileTags) {
        for (const auto& tag : tags) {
            uniqueTags.insert(tag);
        }
    }
    
    allTags.assign(uniqueTags.begin(), uniqueTags.end());
    return allTags;
}
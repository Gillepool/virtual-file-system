#ifndef VIRTUALFILESYSTEM_H
#define VIRTUALFILESYSTEM_H

#include "FileNode.h"
#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <map>
#include <set>
#include <regex>
#include <ctime>
#include <optional>
#include <functional>

struct SearchFilter {
    std::optional<std::string> nameContains;
    std::optional<std::regex> namePattern;

    std::optional<std::string> contentContains;
    std::optional<std::regex> contentPattern;

    bool filesOnly = false;
    bool directoriesOnly = false;

    // Size filters (in bytes)
    std::optional<size_t> minSize;
    std::optional<size_t> maxSize;

    // Date filters (as time_t)
    std::optional<time_t> modifiedAfter;
    std::optional<time_t> modifiedBefore;

    // Tag filters
    std::vector<std::string> tags;

    // Custom filter function
    std::function<bool(const FileNode*)> customFilter;
};

class VirtualFileSystem {
public:
    VirtualFileSystem(size_t diskSize = 10 * 1024 * 1024); // Default 10MB
    ~VirtualFileSystem();

    VirtualFileSystem& operator=(const VirtualFileSystem& other);

    // File system operations
    bool mkdir(const std::string& path);
    bool touch(const std::string& path);
    bool cd(const std::string& path);
    std::vector<std::string> ls(const std::string& path = "");
    std::string cat(const std::string& path);
    bool write(const std::string& path, const std::string& content);
    bool remove(const std::string& path);

    // Disk operations
    bool saveToDisk(const std::string& filename = "virtual_disk.bin");
    bool loadFromDisk(const std::string& filename = "virtual_disk.bin");
    size_t getFreeSpace() const;
    size_t getTotalSpace() const;
    size_t getUsedSpace() const;

    FileNode* resolvePath(const std::string& path);
    std::string getCurrentPath() const;

    bool createVolume(const std::string& volumeName, size_t volumeSize);
    bool mountVolume(const std::string& diskImage, const std::string& mountPoint);
    bool unmountVolume(const std::string& mountPoint);
    std::vector<std::string> listMountedVolumes() const;
    bool isMountPoint(const std::string& path) const;

    bool compressFile(const std::string& path, bool compress = true, const std::string& algorithm = "");
    bool isFileCompressed(const std::string& path) const;
    std::string getFileCompressionAlgorithm(const std::string& path) const;
    std::vector<std::string> listCompressionAlgorithms() const;

    bool encryptFile(const std::string& path, const std::string& key, const std::string& algorithm = "");
    bool decryptFile(const std::string& path);
    bool isFileEncrypted(const std::string& path) const;
    std::string getFileEncryptionAlgorithm(const std::string& path) const;
    bool changeEncryptionKey(const std::string& path, const std::string& newKey);
    std::vector<std::string> listEncryptionAlgorithms() const;

    bool saveFileVersion(const std::string& path);
    bool restoreFileVersion(const std::string& path, size_t versionIndex);
    size_t getFileVersionCount(const std::string& path) const;
    std::vector<std::time_t> getFileVersionTimestamps(const std::string& path) const;

    std::vector<std::string> search(const SearchFilter& filter, const std::string& startPath = "");
    std::vector<std::string> searchByName(const std::string& namePattern, bool useRegex = false, const std::string& startPath = "");
    std::vector<std::string> searchByContent(const std::string& contentPattern, bool useRegex = false, const std::string& startPath = "");
    std::vector<std::string> searchByTag(const std::string& tag, const std::string& startPath = "");
    std::vector<std::string> searchBySize(size_t minSize, size_t maxSize, const std::string& startPath = "");
    std::vector<std::string> searchByDate(time_t modifiedAfter, time_t modifiedBefore, const std::string& startPath = "");

    bool addTag(const std::string& path, const std::string& tag);
    bool removeTag(const std::string& path, const std::string& tag);
    std::vector<std::string> getFileTags(const std::string& path) const;
    std::vector<std::string> getAllTags() const;

private:
    std::unique_ptr<FileNode> root;
    FileNode* currentDirectory;
    size_t diskSize;
    size_t usedSpace;

    struct MountInfo {
        std::string diskImage;
        std::unique_ptr<VirtualFileSystem> fs;
        FileNode* mountPoint;
    };

    std::map<std::string, MountInfo> mountedVolumes; // key: mount path

    std::vector<std::string> splitPath(const std::string& path);
    void updateUsedSpace();
    VirtualFileSystem* getResponsibleFS(const std::string& path, std::string& localPath);
    std::string getVolumeForPath(const std::string& path) const;

    void searchRecursive(FileNode* node, const std::string& currentPath, const SearchFilter& filter, std::vector<std::string>& results);
    bool matchesFilter(FileNode* node, const std::string& filePath, const SearchFilter& filter);
    bool contentMatches(FileNode* node, const std::string& pattern, bool isRegex);

    std::map<std::string, std::vector<std::string>> fileTags; // Maps file paths to their tags
};

#endif // VIRTUALFILESYSTEM_H
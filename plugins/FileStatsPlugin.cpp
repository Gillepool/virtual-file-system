#include "FileStatsPlugin.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <numeric>

FileStatsPlugin::FileStatsPlugin() : shell(nullptr) {
}

FileStatsPlugin::~FileStatsPlugin() {
}

bool FileStatsPlugin::initialize(Shell* shell) {
    this->shell = shell;
    
    // Initialize commands
    commands = {
        {"filestats", [this](Shell* shell, const std::vector<std::string>& args) { this->cmdFileStats(shell, args); }},
        {"diskusage", [this](Shell* shell, const std::vector<std::string>& args) { this->cmdDiskUsage(shell, args); }},
        {"findduplicates", [this](Shell* shell, const std::vector<std::string>& args) { this->cmdFindDuplicates(shell, args); }}
    };
    
    std::cout << "FileStats plugin initialized" << std::endl;
    return true;
}

bool FileStatsPlugin::shutdown() {
    std::cout << "FileStats plugin shutdown" << std::endl;
    return true;
}

std::vector<std::pair<std::string, Shell::CommandFunction>> FileStatsPlugin::getCommands() const {
    return commands;
}

void FileStatsPlugin::cmdFileStats(Shell* shell, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: filestats <file_path>" << std::endl;
        return;
    }
    
    std::string path = args[0];
    VirtualFileSystem& vfs = shell->getVFS();
    
    FileNode* node = vfs.resolvePath(path);
    if (!node) {
        std::cout << "File not found: " << path << std::endl;
        return;
    }
    
    if (node->isDirectory()) {
        std::cout << "Path is a directory: " << path << std::endl;
        return;
    }
    
    size_t fileSize = node->getSize();
    std::string content = vfs.cat(path);
    size_t lineCount = 0;
    size_t wordCount = 0;
    size_t charCount = content.length();
    
    // Count lines and words
    bool inWord = false;
    for (char c : content) {
        if (c == '\n') {
            lineCount++;
        }
        
        if (std::isspace(c)) {
            if (inWord) {
                inWord = false;
                wordCount++;
            }
        } else {
            inWord = true;
        }
    }
    
    if (inWord) {
        wordCount++;
    }
    
    // If content doesn't end with newline, add one to line count
    if (!content.empty() && content.back() != '\n') {
        lineCount++;
    }
    
    // Count character frequencies
    std::unordered_map<char, int> charFreq;
    for (char c : content) {
        charFreq[c]++;
    }
    
    std::cout << "File statistics for: " << path << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Size:           " << formatFileSize(fileSize) << " (" << fileSize << " bytes)" << std::endl;
    std::cout << "File type:      " << getFileType(path, vfs) << std::endl;
    std::cout << "Line count:     " << lineCount << std::endl;
    std::cout << "Word count:     " << wordCount << std::endl;
    std::cout << "Character count: " << charCount << std::endl;
    std::cout << "Is compressed:  " << (node->isCompressed() ? "Yes" : "No") << std::endl;
    std::cout << "Is encrypted:   " << (node->isEncrypted() ? "Yes" : "No") << std::endl;
    std::cout << "Versions:       " << node->getVersionCount() << std::endl;
    
    std::cout << "\nTop 5 most frequent characters:" << std::endl;
    std::vector<std::pair<char, int>> sortedFreq(charFreq.begin(), charFreq.end());
    std::sort(sortedFreq.begin(), sortedFreq.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    int count = 0;
    for (const auto& pair : sortedFreq) {
        if (count >= 5) break;
        
        char c = pair.first;
        std::string charDisplay;
        
        if (c == '\n') charDisplay = "'\\n'";
        else if (c == '\t') charDisplay = "'\\t'";
        else if (c == '\r') charDisplay = "'\\r'";
        else if (c == ' ') charDisplay = "'space'";
        else if (std::isprint(c)) charDisplay = std::string("'") + c + "'";
        else charDisplay = std::to_string(static_cast<int>(c));
        
        std::cout << "  " << charDisplay << ": " << pair.second 
                  << " (" << std::fixed << std::setprecision(2)
                  << (static_cast<double>(pair.second) / charCount * 100) << "%)" << std::endl;
        count++;
    }
}

void FileStatsPlugin::cmdDiskUsage(Shell* shell, const std::vector<std::string>& args) {
    std::string path = args.empty() ? "." : args[0];
    bool sortBySize = false;
    
    // Check for sort flag
    for (const auto& arg : args) {
        if (arg == "--sort" || arg == "-s") {
            sortBySize = true;
        }
    }
    
    VirtualFileSystem& vfs = shell->getVFS();
    FileNode* rootNode = vfs.resolvePath(path);
    
    if (!rootNode) {
        std::cout << "Directory not found: " << path << std::endl;
        return;
    }
    
    if (!rootNode->isDirectory()) {
        std::cout << "Path is not a directory: " << path << std::endl;
        return;
    }
    
    // Function to recursively calculate directory size
    std::function<size_t(FileNode*, std::map<std::string, size_t>&, const std::string&)> calcDirSize = 
    [&](FileNode* node, std::map<std::string, size_t>& dirSizes, const std::string& currentPath) -> size_t {
        size_t totalSize = 0;
        
        if (node->isDirectory()) {
            std::vector<std::unique_ptr<FileNode>>& children = node->getChildren();
            
            for (size_t i = 0; i < children.size(); ++i) {
                std::string childName = children[i]->getName();
                std::string childPath = currentPath.empty() ? childName : currentPath + "/" + childName;
                totalSize += calcDirSize(children[i].get(), dirSizes, childPath);
            }
            
            dirSizes[currentPath] = totalSize;
        } else {
            totalSize = node->getSize();
        }
        
        return totalSize;
    };
    
    std::map<std::string, size_t> dirSizes;
    size_t totalSize = calcDirSize(rootNode, dirSizes, path == "." ? "" : path);
    
    // Convert to vector for sorting
    std::vector<std::pair<std::string, size_t>> sortedDirs(dirSizes.begin(), dirSizes.end());
    
    if (sortBySize) {
        std::sort(sortedDirs.begin(), sortedDirs.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
    }
    
    std::cout << "Disk usage for: " << path << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(12) << "Size" << "Directory" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    
    for (const auto& pair : sortedDirs) {
        std::cout << std::left << std::setw(12) << formatFileSize(pair.second) 
                  << pair.first << std::endl;
    }
    
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "Total: " << formatFileSize(totalSize) << std::endl;
}

void FileStatsPlugin::cmdFindDuplicates(Shell* shell, const std::vector<std::string>& args) {
    std::string path = args.empty() ? "." : args[0];
    VirtualFileSystem& vfs = shell->getVFS();
    
    // Group files by size first (quick filter for potential duplicates)
    std::map<size_t, std::vector<std::string>> filesBySize;
    
    // Function to recursively scan directories
    std::function<void(const std::string&)> scanDir = [&](const std::string& dirPath) {
        std::vector<std::string> entries = vfs.ls(dirPath);
        
        for (const auto& entry : entries) {
            // Remove trailing / or @ if present
            std::string entryName = entry;
            bool isDir = false;
            
            if (entryName.back() == '/') {
                isDir = true;
                entryName.pop_back();
            } else if (entryName.back() == '@') {
                // Skip mount points
                continue;
            }
            
            // Build full path
            std::string fullPath = dirPath == "." ? entryName : dirPath + "/" + entryName;
            FileNode* node = vfs.resolvePath(fullPath);
            
            if (!node) {
                continue; // Skip if not found
            }
            
            if (isDir || node->isDirectory()) {
                // Recurse into directories
                scanDir(fullPath);
            } else {
                // Add file to the size map
                size_t fileSize = node->getSize();
                if (fileSize > 0) { // Skip empty files
                    filesBySize[fileSize].push_back(fullPath);
                }
            }
        }
    };
    
    scanDir(path);
    
    // Check for duplicates
    std::map<std::string, std::vector<std::string>> duplicateGroups;
    size_t duplicateCount = 0;
    size_t wastedSpace = 0;
    
    for (const auto& sizeGroup : filesBySize) {
        if (sizeGroup.second.size() < 2) {
            continue; // Skip unique file sizes
        }
        
        // For files of the same size, compare content
        std::map<std::string, std::vector<std::string>> contentGroups;
        
        for (const auto& filePath : sizeGroup.second) {
            std::string content = vfs.cat(filePath);
            contentGroups[content].push_back(filePath);
        }
        
        // Check which files have identical content
        for (const auto& contentGroup : contentGroups) {
            if (contentGroup.second.size() > 1) {
                std::string hash = std::to_string(std::hash<std::string>{}(contentGroup.first));
                duplicateGroups[hash] = contentGroup.second;
                duplicateCount += contentGroup.second.size() - 1;
                wastedSpace += (contentGroup.second.size() - 1) * sizeGroup.first;
            }
        }
    }
    
    if (duplicateGroups.empty()) {
        std::cout << "No duplicate files found in " << path << std::endl;
        return;
    }
    
    std::cout << "Found " << duplicateCount << " duplicate files in " << path 
              << " wasting " << formatFileSize(wastedSpace) << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    
    int groupNum = 1;
    for (const auto& group : duplicateGroups) {
        const auto& files = group.second;
        FileNode* node = vfs.resolvePath(files[0]);
        size_t fileSize = node ? node->getSize() : 0;
        
        std::cout << "Duplicate group #" << groupNum++ << " (" << formatFileSize(fileSize) << "):" << std::endl;
        for (const auto& file : files) {
            std::cout << "  " << file << std::endl;
        }
        std::cout << std::endl;
    }
}

std::string FileStatsPlugin::formatFileSize(size_t sizeBytes) const {
    if (sizeBytes < 1024) {
        return std::to_string(sizeBytes) + " B";
    } else if (sizeBytes < 1024 * 1024) {
        double kb = static_cast<double>(sizeBytes) / 1024;
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << kb << " KB";
        return oss.str();
    } else {
        double mb = static_cast<double>(sizeBytes) / (1024 * 1024);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << mb << " MB";
        return oss.str();
    }
}

std::string FileStatsPlugin::getFileType(const std::string& path, VirtualFileSystem& vfs) const {
    std::string extension;
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = path.substr(dotPos + 1);
    }
    
    std::string content = vfs.cat(path);
    if (content.empty()) {
        return "Empty file";
    }
    
    // Check for binary content (sample the first 1000 bytes)
    size_t sampleSize = std::min(content.size(), static_cast<size_t>(1000));
    bool isBinary = false;
    for (size_t i = 0; i < sampleSize; ++i) {
        char c = content[i];
        if (c == 0 || (c < 32 && c != '\n' && c != '\r' && c != '\t' && c != '\b')) {
            isBinary = true;
            break;
        }
    }
    
    // Determine file type based on extension and content analysis
    if (isBinary) {
        return "Binary file";
    } else {
        if (extension == "txt") return "Text file";
        else if (extension == "json") return "JSON file";
        else if (extension == "xml") return "XML file";
        else if (extension == "html") return "HTML file";
        else if (extension == "css") return "CSS file";
        else if (extension == "js") return "JavaScript file";
        else if (extension == "cpp" || extension == "h") return "C++ source file";
        else if (extension == "py") return "Python source file";
        else if (extension == "md") return "Markdown file";
        else if (extension == "csv") return "CSV file";
        else return "Text file";
    }
}

IMPLEMENT_PLUGIN(FileStatsPlugin)
#include "../include/ShellAssistant.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

ShellAssistant::ShellAssistant(VirtualFileSystem* vfs) : vfs(vfs) {
    initializePatterns();
}

std::string ShellAssistant::getName() const {
    return "VFS Assistant";
}

std::string ShellAssistant::getHelpInfo() const {
    std::stringstream ss;
    ss << "I am " << getName() << ", your Virtual File System assistant.\n";
    ss << "You can ask me questions like:\n\n";
    
    for (const auto& pattern : queryPatterns) {
        ss << "- " << pattern.description << "\n";
    }
    
    ss << "\nOr just chat with me about the VFS!";
    return ss.str();
}

bool ShellAssistant::canHandleQuery(const std::string& query) {
    // Check if the query matches any of our patterns
    for (const auto& pattern : queryPatterns) {
        std::smatch match;
        if (std::regex_search(query, match, pattern.pattern)) {
            return true;
        }
    }
    
    // For generic queries that don't match specific patterns
    // Check if they contain keywords related to files, directories, or VFS operations
    std::vector<std::string> keywords = {
        "file", "directory", "folder", "vfs", "create", "delete", "copy", 
        "move", "encrypt", "compress", "mount", "help", "how to"
    };
    
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
                  
    for (const auto& keyword : keywords) {
        if (lowerQuery.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// Very basic query processing
std::string ShellAssistant::processQuery(const std::string& query) {
    // Try to match the query against our patterns
    for (const auto& pattern : queryPatterns) {
        std::smatch match;
        if (std::regex_search(query, match, pattern.pattern)) {
            return pattern.handler(this, match);
        }
    }
    
    // Fall back to generic responses
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    if (lowerQuery.find("hello") != std::string::npos || 
        lowerQuery.find("hi") != std::string::npos) {
        return "Hello! I'm your VFS assistant. How can I help you with your virtual file system today?";
    }
    
    if (lowerQuery.find("thank") != std::string::npos) {
        return "You're welcome! Let me know if you need anything else.";
    }
    
    if (lowerQuery.find("help") != std::string::npos) {
        return getHelpInfo();
    }
    
    // Default response for queries we can't specifically handle
    return "I'm not sure how to help with that specific query. You can ask me how to perform specific "
           "file system operations, or ask for help with a specific command.";
}

void ShellAssistant::initializePatterns() {
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) create (?:a |an )?(directory|folder|dir)(?:\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string dirName = match[2].matched ? match[2].str() : "example_dir";
            return "To create a directory, use the 'mkdir' command:\n\nmkdir " + dirName + 
                   "\n\nThis will create a new directory called '" + dirName + "' in the current location.";
        },
        "How do I create a directory?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) create (?:a |an )?(file)(?:\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string fileName = match[2].matched ? match[2].str() : "example.txt";
            return "To create a new empty file, use the 'touch' command:\n\ntouch " + fileName + 
                   "\n\nTo create a file with content, use the 'write' command:\n\nwrite " + fileName + " \"Your file content here\"";
        },
        "How do I create a file?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) delete|remove (?:a |an )?(file|directory|folder|dir)(?:\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string name = match[2].matched ? match[2].str() : "example";
            return "To remove a file or directory, use the 'rm' command:\n\nrm " + name + 
                   "\n\nThis will permanently remove '" + name + "' from the file system.";
        },
        "How do I delete a file or directory?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) (?:see|view|read|display) (?:a |an )?(file(?:'s)? contents?|contents? of (?:a |an )?file)(?:\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string fileName = match[2].matched ? match[2].str() : "example.txt";
            return "To view the contents of a file, use the 'cat' command:\n\ncat " + fileName + 
                   "\n\nThis will display the entire contents of '" + fileName + "' in the console.";
        },
        "How do I view a file's contents?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) (?:see|view|list) (?:directory|folder|dir) contents?(?:\\s+(?:of|in)\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string dirName = match[1].matched ? match[1].str() : "";
            std::string cmd = "ls" + (dirName.empty() ? "" : " " + dirName);
            return "To list directory contents, use the 'ls' command:\n\n" + cmd + 
                   "\n\nThis will show all files and directories" + (dirName.empty() ? " in the current location." : " in '" + dirName + "'.");
        },
        "How do I list directory contents?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) change (?:directory|folder|dir|location)(?:\\s+(?:to)\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string dirName = match[1].matched ? match[1].str() : "example_dir";
            return "To change your current directory, use the 'cd' command:\n\ncd " + dirName + 
                   "\n\nYou can use 'cd ..' to go up one level or 'cd /' to go to the root directory.";
        },
        "How do I change directories?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:how do (?:I|i)|how to|how can (?:I|i)) (compress|uncompress|encrypt|decrypt) (?:a |an )?(file)(?:\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string operation = match[1].str();
            std::transform(operation.begin(), operation.end(), operation.begin(), 
                          [](unsigned char c){ return std::tolower(c); });
            
            std::string fileName = match[3].matched ? match[3].str() : "example.txt";
            
            if (operation == "compress") {
                return "To compress a file, use the 'compress' command:\n\ncompress " + fileName;
            } else if (operation == "uncompress") {
                return "To uncompress a file, use the 'uncompress' command:\n\nuncompress " + fileName;
            } else if (operation == "encrypt") {
                return "To encrypt a file, use the 'encrypt' command with a key:\n\nencrypt " + fileName + " your_secret_key";
            } else if (operation == "decrypt") {
                return "To decrypt a file, use the 'decrypt' command:\n\ndecrypt " + fileName;
            }
            
            return "I'm not sure about that specific operation.";
        },
        "How do I compress/encrypt a file?"
    });
    
    queryPatterns.push_back({
        std::regex("(?:show|find|what is) (?:me )?(?:the )?(largest|biggest) file(?:\\s+in\\s+(.+))?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            std::string dirName = match[2].matched ? match[2].str() : ".";
            return assistant->findLargestFile(dirName);
        },
        "Show me the biggest file in [directory]"
    });
    
    queryPatterns.push_back({
        std::regex("(?:explain|what does|what is) (?:the )?(?:command )?([a-zA-Z]+)(?: command| do)?", std::regex::icase),
        [](ShellAssistant* assistant, const std::smatch& match) -> std::string {
            return assistant->explainCommand(match[1].str());
        },
        "Explain [command]"
    });
}

std::string ShellAssistant::formatSize(size_t sizeInBytes) const {
    if (sizeInBytes < 1024) {
        return std::to_string(sizeInBytes) + " B";
    } else if (sizeInBytes < 1024 * 1024) {
        double sizeInKB = static_cast<double>(sizeInBytes) / 1024;
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << sizeInKB << " KB";
        return ss.str();
    } else {
        double sizeInMB = static_cast<double>(sizeInBytes) / (1024 * 1024);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << sizeInMB << " MB";
        return ss.str();
    }
}

std::string ShellAssistant::findLargestFile(const std::string& path) const {
    if (!vfs) {
        return "Sorry, I can't access the file system information right now.";
    }
    
    std::vector<std::string> entries;
    try {
        entries = vfs->ls(path);
    } catch (...) {
        return "I couldn't access directory '" + path + "'. Please make sure it exists and you have permission to access it.";
    }
    
    if (entries.empty()) {
        return "The directory '" + path + "' is empty.";
    }
    
    std::string largestFileName;
    size_t largestSize = 0;
    
    for (const auto& entry : entries) {
        if (entry.back() == '/') {
            continue;
        }
        
        std::string filePath = (path == "." || path == "./") ? entry : 
                              (path == "/") ? "/" + entry : path + "/" + entry;
        
        FileNode* node = vfs->resolvePath(filePath);
        if (node && !node->isDirectory()) {
            size_t size = node->getSize();
            if (size > largestSize) {
                largestSize = size;
                largestFileName = entry;
            }
        }
    }
    
    if (largestFileName.empty()) {
        return "I couldn't find any files in '" + path + "', only directories.";
    }
    
    return "The largest file in '" + path + "' is '" + largestFileName + "' with a size of " + formatSize(largestSize) + ".";
}

std::string ShellAssistant::listDirectoryContents(const std::string& path) const {
    if (!vfs) {
        return "Sorry, I can't access the file system information right now.";
    }
    
    std::vector<std::string> entries;
    try {
        entries = vfs->ls(path);
    } catch (...) {
        return "I couldn't access directory '" + path + "'. Please make sure it exists and you have permission to access it.";
    }
    
    if (entries.empty()) {
        return "The directory '" + path + "' is empty.";
    }
    
    std::stringstream ss;
    ss << "Contents of '" << path << "':\n";
    
    for (const auto& entry : entries) {
        ss << "- " << entry << "\n";
    }
    
    return ss.str();
}

std::string ShellAssistant::explainCommand(const std::string& command) const {
    std::string cmd = command;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), 
                  [](unsigned char c){ return std::tolower(c); });
    
    // Map of commands and their explanations
    std::map<std::string, std::string> commandExplanations = {
        {"mkdir", "Creates a new directory (folder) in the file system.\nUsage: mkdir <directory_name>"},
        {"touch", "Creates a new empty file.\nUsage: touch <file_name>"},
        {"cd", "Changes the current directory (location) in the file system.\nUsage: cd <directory_path>"},
        {"ls", "Lists the contents of a directory.\nUsage: ls [directory_path]"},
        {"cat", "Displays the contents of a file.\nUsage: cat <file_path>"},
        {"write", "Writes text content to a file.\nUsage: write <file_path> <content>"},
        {"rm", "Removes (deletes) a file or directory from the file system.\nUsage: rm <path>"},
        {"help", "Displays help information about available commands.\nUsage: help"},
        {"exit", "Exits the shell.\nUsage: exit"},
        {"save", "Saves the current state of the file system to disk.\nUsage: save [filename]"},
        {"load", "Loads a file system from disk.\nUsage: load [filename]"},
        {"diskinfo", "Displays information about disk usage.\nUsage: diskinfo"},
        {"createvolume", "Creates a new virtual disk volume.\nUsage: createvolume <volume_name> <size_in_mb>"},
        {"mount", "Mounts a virtual disk image at a specified mount point.\nUsage: mount <disk_image> <mount_point>"},
        {"unmount", "Unmounts a previously mounted volume.\nUsage: unmount <mount_point>"},
        {"mounts", "Lists all mounted volumes.\nUsage: mounts"},
        {"compress", "Compresses a file to save space.\nUsage: compress <file_path>"},
        {"uncompress", "Uncompresses a previously compressed file.\nUsage: uncompress <file_path>"},
        {"iscompressed", "Checks if a file is compressed.\nUsage: iscompressed <file_path>"},
        {"encrypt", "Encrypts a file for security.\nUsage: encrypt <file_path> <key>"},
        {"decrypt", "Decrypts a previously encrypted file.\nUsage: decrypt <file_path>"},
        {"isencrypted", "Checks if a file is encrypted.\nUsage: isencrypted <file_path>"},
        {"changekey", "Changes the encryption key for an encrypted file.\nUsage: changekey <file_path> <new_key>"},
        {"saveversion", "Saves the current version of a file.\nUsage: saveversion <file_path>"},
        {"restoreversion", "Restores a file to a previously saved version.\nUsage: restoreversion <file_path> <version_index>"},
        {"listversions", "Lists all available versions of a file.\nUsage: listversions <file_path>"},
        {"ask", "Asks the assistant a question about the file system.\nUsage: ask <your question>"},
        {"assistant", "Activates the assistant to answer a question.\nUsage: assistant <your question>"}
    };
    
    auto it = commandExplanations.find(cmd);
    if (it != commandExplanations.end()) {
        return "Command: " + cmd + "\n" + it->second;
    }
    
    return "I don't have information about the '" + cmd + "' command. Try 'help' to see a list of available commands.";
}
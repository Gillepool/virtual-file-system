#include "../include/Shell.h"
#include "../include/ShellAssistant.h"
#include "../include/PluginManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <regex>
#include <algorithm>
#include <filesystem>

Shell::Shell() : running(true) {
    vfs = VirtualFileSystem(10 * 1024 * 1024);
    
    assistant = std::make_unique<ShellAssistant>(&vfs);    
    pluginManager = std::make_unique<PluginManager>(this);
    
    commands["mkdir"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMkdir(args); };
    commands["touch"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdTouch(args); };
    commands["cd"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCd(args); };
    commands["ls"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLs(args); };
    commands["cat"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCat(args); };
    commands["write"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdWrite(args); };
    commands["rm"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRm(args); };
    commands["help"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdHelp(args); };
    commands["exit"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdExit(args); };
    commands["save"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdSave(args); };
    commands["load"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLoad(args); };
    commands["diskinfo"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdDiskInfo(args); };
    commands["pwd"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdPwd(args); };
    commands["cp"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCp(args); };
    commands["mv"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMv(args); };
    commands["createvolume"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCreateVolume(args); };
    commands["mount"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMount(args); };
    commands["unmount"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUnmount(args); };
    commands["mounts"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMounts(args); };
    commands["compress"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCompress(args); };
    commands["uncompress"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUncompress(args); };
    commands["iscompressed"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdIsCompressed(args); };
    commands["encrypt"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdEncrypt(args); };
    commands["decrypt"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdDecrypt(args); };
    commands["isencrypted"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdIsEncrypted(args); };
    commands["changekey"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdChangeKey(args); };
    commands["saveversion"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdSaveVersion(args); };
    commands["restoreversion"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRestoreVersion(args); };
    commands["ask"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAsk(args); };
    commands["assistant"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAssistant(args); };
    commands["find"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFind(args); };
    commands["findname"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByName(args); };
    commands["grep"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByContent(args); };
    commands["findsize"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindBySize(args); };
    commands["finddate"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByDate(args); };
    commands["findtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByTag(args); };
    commands["addtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAddTag(args); };
    commands["rmtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRemoveTag(args); };
    commands["tags"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdListTags(args); };
    commands["loadplugin"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLoadPlugin(args); };
    commands["unloadplugin"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUnloadPlugin(args); };
    commands["plugins"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdListPlugins(args); };
    
    initializeBuiltinCommands();
    
    // Try to load plugins from a default location
    std::string pluginsDir = "./plugins";
    if (std::filesystem::exists(pluginsDir)) {
        pluginManager->discoverAndLoadPlugins(pluginsDir);
    }
}

Shell::Shell(std::shared_ptr<VirtualFileSystem> vfsPtr) : running(true), sharedVfs(vfsPtr) {
    if (sharedVfs) {
        vfs = *sharedVfs;
    } else {
        vfs = VirtualFileSystem(10 * 1024 * 1024);
    }
    
    assistant = std::make_unique<ShellAssistant>(&vfs);
    
    pluginManager = std::make_unique<PluginManager>(this);

    commands["mkdir"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMkdir(args); };
    commands["touch"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdTouch(args); };
    commands["cd"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCd(args); };
    commands["ls"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLs(args); };
    commands["cat"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCat(args); };
    commands["write"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdWrite(args); };
    commands["rm"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRm(args); };
    commands["help"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdHelp(args); };
    commands["exit"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdExit(args); };
    commands["save"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdSave(args); };
    commands["load"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLoad(args); };
    commands["diskinfo"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdDiskInfo(args); };
    commands["pwd"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdPwd(args); };
    commands["cp"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCp(args); };
    commands["mv"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMv(args); };
    commands["createvolume"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCreateVolume(args); };
    commands["mount"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMount(args); };
    commands["unmount"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUnmount(args); };
    commands["mounts"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdMounts(args); };
    commands["compress"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdCompress(args); };
    commands["uncompress"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUncompress(args); };
    commands["iscompressed"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdIsCompressed(args); };
    commands["encrypt"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdEncrypt(args); };
    commands["decrypt"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdDecrypt(args); };
    commands["isencrypted"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdIsEncrypted(args); };
    commands["changekey"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdChangeKey(args); };
    commands["saveversion"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdSaveVersion(args); };
    commands["restoreversion"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRestoreVersion(args); };
    commands["listversions"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdListVersions(args); };
    commands["ask"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAsk(args); };
    commands["assistant"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAssistant(args); };
    commands["find"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFind(args); };
    commands["findname"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByName(args); };
    commands["grep"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByContent(args); };
    commands["findsize"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindBySize(args); };
    commands["finddate"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByDate(args); };
    commands["findtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdFindByTag(args); };
    commands["addtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdAddTag(args); };
    commands["rmtag"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdRemoveTag(args); };
    commands["tags"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdListTags(args); };
    commands["loadplugin"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdLoadPlugin(args); };
    commands["unloadplugin"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdUnloadPlugin(args); };
    commands["plugins"] = [this](Shell* shell, const std::vector<std::string>& args) { cmdListPlugins(args); };
    
    initializeBuiltinCommands();
    
    std::string pluginsDir = "./plugins";
    if (std::filesystem::exists(pluginsDir)) {
        pluginManager->discoverAndLoadPlugins(pluginsDir);
    }
}

Shell::~Shell() {
}

void Shell::run() {
    std::cout << "Virtual File System Shell" << std::endl;
    std::cout << "Type 'help' for a list of commands" << std::endl;
    
    std::string cmdLine;
    
    while (running) {
        std::cout << vfs.getCurrentPath() << "> ";
        std::getline(std::cin, cmdLine);
        
        if (cmdLine.empty()) {
            continue;
        }
        
        std::vector<std::string> args = parseCommand(cmdLine);
        
        if (!args.empty()) {
            std::string cmd = args[0];
            args.erase(args.begin()); // Remove command name, leaving only args
            
            if (commands.find(cmd) != commands.end()) {
                commands[cmd](this, args);
            } else {
                std::cout << "Unknown command: " << cmd << std::endl;
                std::cout << "Type 'help' for a list of commands" << std::endl;
            }
        }
    }
}

std::vector<std::string> Shell::parseCommand(const std::string& cmdLine) {
    std::vector<std::string> args;
    std::istringstream iss(cmdLine);
    std::string arg;
    
    while (iss >> std::quoted(arg)) {
        args.push_back(arg);
    }
    
    return args;
}


void Shell::cmdMkdir(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: mkdir <directory_name>" << std::endl;
        return;
    }
    
    if (vfs.mkdir(args[0])) {
        std::cout << "Directory created: " << args[0] << std::endl;
        if (sharedVfs) {
            sharedVfs->mkdir(args[0]);
        }
    } else {
        std::cout << "Failed to create directory: " << args[0] << std::endl;
    }
}

void Shell::cmdTouch(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: touch <file_name>" << std::endl;
        return;
    }
    
    if (vfs.touch(args[0])) {
        std::cout << "File created: " << args[0] << std::endl;
        if (sharedVfs) {
            sharedVfs->touch(args[0]);
        }
    } else {
        std::cout << "Failed to create file: " << args[0] << std::endl;
    }
}

void Shell::cmdCd(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: cd <directory_path>" << std::endl;
        return;
    }
    
    if (vfs.cd(args[0])) {
        if (sharedVfs) {
            sharedVfs->cd(args[0]);
        }
    } else {
        std::cout << "Failed to change directory: " << args[0] << std::endl;
    }
}

void Shell::cmdLs(const std::vector<std::string>& args) {
    std::string path;
    bool showMetadata = false;
    
    for (const auto& arg : args) {
        if (arg == "-l") {
            showMetadata = true;
        } else {
            path = arg; 
        }
    }
    
    std::vector<std::string> entries = vfs.ls(path);
    
    if (entries.empty()) {
        std::cout << "Directory is empty or doesn't exist" << std::endl;
        return;
    }
    
    if (!showMetadata) {
        std::cout << "Contents of directory:" << std::endl;
        for (const auto& entry : entries) {
            std::cout << "  " << entry << std::endl;
        }
    } else {
        std::cout << "Detailed contents of directory:" << std::endl;
        std::cout << std::left << std::setw(10) << "Size" 
                  << std::setw(20) << "Modified"
                  << std::setw(15) << "Attributes" 
                  << "Name" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        for (const auto& entry : entries) {
            // Extract the base name without trailing / or @
            std::string baseName = entry;
            bool isDir = false;
            bool isMount = false;
            
            if (baseName.back() == '/') {
                isDir = true;
                baseName.pop_back();
            } else if (baseName.back() == '@') {
                isMount = true;
                baseName.pop_back();
            }
            
            std::string fullPath = path.empty() ? baseName : path + "/" + baseName;
            FileNode* node = vfs.resolvePath(fullPath);
            
            if (!node) {
                // This might be a mount point
                std::cout << std::left << std::setw(10) << "<mount>"
                          << std::setw(20) << "-" 
                          << std::setw(15) << "mount-point"
                          << entry << std::endl;
                continue;
            }
            
            size_t size = node->getSize();
            std::time_t modTime = getNodeModificationTime(node);
            
            std::string attrs;
            attrs += isDir ? "d" : "-";
            attrs += "rw-";  // Assume read/write permissions
            
            if (node && !node->isDirectory()) {
                attrs += node->isCompressed() ? "c" : "-";
                attrs += node->isEncrypted() ? "e" : "-";
                attrs += (node->getVersionCount() > 0) ? "v" : "-";
            } else {
                attrs += "---";
            }
            
            std::cout << std::left << std::setw(10) << (isDir ? "<DIR>" : formatSize(size))
                      << std::setw(20) << formatTimestamp(modTime)
                      << std::setw(15) << attrs 
                      << entry << std::endl;
        }
    }
}

void Shell::cmdCat(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: cat <file_path>" << std::endl;
        return;
    }
    
    std::string content = vfs.cat(args[0]);
    if (!content.empty()) {
        std::cout << content << std::endl;
    } else {
        std::cout << "File is empty or doesn't exist" << std::endl;
    }
}

void Shell::cmdWrite(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: write <file_path> <content>" << std::endl;
        return;
    }
    
    std::string path = args[0];
    std::string content;
    
    for (size_t i = 1; i < args.size(); ++i) {
        if (i > 1) {
            content += " ";
        }
        content += args[i];
    }
    
    if (vfs.write(path, content)) {
        std::cout << "Successfully wrote to " << path << std::endl;
        if (sharedVfs) {
            sharedVfs->write(path, content);
        }
    } else {
        std::cout << "Failed to write to " << path << std::endl;
    }
}

void Shell::cmdRm(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: rm <path>" << std::endl;
        return;
    }
    
    if (vfs.remove(args[0])) {
        std::cout << "Successfully removed " << args[0] << std::endl;
        if (sharedVfs) {
            sharedVfs->remove(args[0]);
        }
    } else {
        std::cout << "Failed to remove " << args[0] << std::endl;
    }
}

void Shell::cmdHelp(const std::vector<std::string>& args) {
    (void)args;
    
    std::cout << "Available commands:" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    std::cout << "File Operations:" << std::endl;
    std::cout << "  pwd                 - Print current working directory" << std::endl;
    std::cout << "  mkdir <dir>         - Create a new directory" << std::endl;
    std::cout << "  touch <file>        - Create a new empty file" << std::endl;
    std::cout << "  cd <path>           - Change current directory" << std::endl;
    std::cout << "  ls [path]           - List contents of a directory" << std::endl;
    std::cout << "  ls -l [path]        - List contents with details" << std::endl;
    std::cout << "  cat <file>          - Display the contents of a file" << std::endl;
    std::cout << "  write <file> <text> - Write text to a file" << std::endl;
    std::cout << "  cp <src> <dest>     - Copy a file" << std::endl;
    std::cout << "  mv <src> <dest>     - Move or rename a file" << std::endl;
    std::cout << "  rm <path>           - Remove a file or directory" << std::endl;
    std::cout << std::endl;
    
    std::cout << "VFS Management:" << std::endl;
    std::cout << "  save [filename]     - Save the file system to disk" << std::endl;
    std::cout << "  load [filename]     - Load the file system from disk" << std::endl;
    std::cout << "  diskinfo            - Display disk usage information" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Search Commands:" << std::endl;
    std::cout << "  find [options]      - Advanced search with multiple filters" << std::endl;
    std::cout << "  findname <pattern>  - Search by filename pattern" << std::endl;
    std::cout << "  grep <pattern>      - Search by file content" << std::endl;
    std::cout << "  findsize <min> <max> - Search by file size" << std::endl;
    std::cout << "  finddate <after> <before> - Search by modification date" << std::endl;
    std::cout << "  findtag <tag>       - Search by tag" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Volume Management:" << std::endl;
    std::cout << "  createvolume <name> <size_mb> - Create a new volume" << std::endl;
    std::cout << "  mount <diskimg> <mountpoint> - Mount a volume" << std::endl;
    std::cout << "  unmount <mountpoint> - Unmount a volume" << std::endl;
    std::cout << "  mounts              - List mounted volumes" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Compression Commands:" << std::endl;
    std::cout << "  compress <file>     - Compress a file" << std::endl;
    std::cout << "  uncompress <file>   - Uncompress a file" << std::endl;
    std::cout << "  iscompressed <file> - Check if a file is compressed" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Encryption Commands:" << std::endl;
    std::cout << "  encrypt <file> <key> - Encrypt a file" << std::endl;
    std::cout << "  decrypt <file>     - Decrypt a file" << std::endl;
    std::cout << "  isencrypted <file> - Check if a file is encrypted" << std::endl;
    std::cout << "  changekey <file> <newkey> - Change encryption key" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Versioning Commands:" << std::endl;
    std::cout << "  saveversion <file>  - Save current file version" << std::endl;
    std::cout << "  restoreversion <file> <idx> - Restore file to version index" << std::endl;
    std::cout << "  listversions <file> - List available versions" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Tag Management:" << std::endl;
    std::cout << "  addtag <file> <tag> - Add a tag to a file" << std::endl;
    std::cout << "  rmtag <file> <tag>  - Remove a tag from a file" << std::endl;
    std::cout << "  tags [file]         - List tags for file or all tags" << std::endl;
    std::cout << std::endl;

    std::cout << "Plugin Management:" << std::endl;
    std::cout << "  loadplugin <path>   - Load a plugin from a specified path" << std::endl;
    std::cout << "  unloadplugin <name> - Unload a plugin by name" << std::endl;
    std::cout << "  plugins             - List all loaded plugins" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Assistant:" << std::endl;
    std::cout << "  ask <query>         - Ask the assistant a question" << std::endl;
    std::cout << "  assistant <query>   - Same as 'ask'" << std::endl;
    std::cout << std::endl;
    
    std::cout << "System Commands:" << std::endl;
    std::cout << "  help                - Display this help message" << std::endl;
    std::cout << "  exit                - Exit the shell" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    
    auto pluginList = pluginManager->getLoadedPlugins();
    if (!pluginList.empty()) {
        std::cout << "Plugin Commands:" << std::endl;
        
        for (const auto& pluginName : pluginList) {
            Plugin* plugin = pluginManager->getPlugin(pluginName);
            if (plugin) {
                auto pluginCommands = plugin->getCommands();
                if (!pluginCommands.empty()) {
                    std::cout << "  " << pluginName << " Plugin:" << std::endl;
                    for (const auto& cmd : pluginCommands) {
                        std::cout << "    " << cmd.first << std::endl;
                    }
                }
            }
        }
        std::cout << "------------------------------------------------------" << std::endl;
    }
}

void Shell::cmdExit(const std::vector<std::string>& args) {
    std::cout << "Exiting VFS Shell..." << std::endl;
    running = false;
}

void Shell::cmdSave(const std::vector<std::string>& args) {
    std::string filename = args.empty() ? "virtual_disk.bin" : args[0];
    
    if (vfs.saveToDisk(filename)) {
        std::cout << "File system saved to " << filename << std::endl;
    } else {
        std::cout << "Failed to save file system to " << filename << std::endl;
    }
}

void Shell::cmdLoad(const std::vector<std::string>& args) {
    std::string filename = args.empty() ? "virtual_disk.bin" : args[0];
    
    if (vfs.loadFromDisk(filename)) {
        std::cout << "File system loaded from " << filename << std::endl;
    } else {
        std::cout << "Failed to load file system from " << filename << std::endl;
    }
}

void Shell::cmdDiskInfo(const std::vector<std::string>& args) {
    size_t totalSpace = vfs.getTotalSpace();
    size_t usedSpace = vfs.getUsedSpace();
    size_t freeSpace = vfs.getFreeSpace();
    
    double percentUsed = (static_cast<double>(usedSpace) / totalSpace) * 100;
    
    std::cout << "Disk Information:" << std::endl;
    std::cout << "  Total Space: " << formatSize(totalSpace) << std::endl;
    std::cout << "  Used Space: " << formatSize(usedSpace) << " (" << std::fixed << std::setprecision(2) << percentUsed << "%)" << std::endl;
    std::cout << "  Free Space: " << formatSize(freeSpace) << std::endl;
}

std::string Shell::formatSize(size_t sizeInBytes) const {
    if (sizeInBytes < 1024) {
        return std::to_string(sizeInBytes) + " B";
    } else if (sizeInBytes < 1024 * 1024) {
        return std::to_string(sizeInBytes / 1024) + " KB";
    } else {
        double sizeInMB = static_cast<double>(sizeInBytes) / (1024 * 1024);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << sizeInMB << " MB";
        return oss.str();
    }
}

std::string Shell::formatTimestamp(std::time_t timestamp) const {
    char buffer[80];
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

void Shell::cmdCreateVolume(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: createvolume <volume_name> <size_in_mb>" << std::endl;
        return;
    }
    
    std::string volumeName = args[0];
    size_t sizeMB;
    try {
        sizeMB = std::stoul(args[1]);
    } catch (const std::exception&) {
        std::cout << "Invalid size: must be a positive integer" << std::endl;
        return;
    }
    
    size_t sizeBytes = sizeMB * 1024 * 1024;
    
    if (vfs.createVolume(volumeName, sizeBytes)) {
        std::cout << "Created volume " << volumeName << " with size " << formatSize(sizeBytes) << std::endl;
    } else {
        std::cout << "Failed to create volume" << std::endl;
    }
}

void Shell::cmdMount(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: mount <disk_image> <mount_point>" << std::endl;
        return;
    }
    
    std::string diskImage = args[0];
    std::string mountPoint = args[1];
    
    if (vfs.mountVolume(diskImage, mountPoint)) {
        std::cout << "Mounted " << diskImage << " at " << mountPoint << std::endl;
    } else {
        std::cout << "Failed to mount volume. Check if disk image exists and mount point is valid." << std::endl;
    }
}

void Shell::cmdUnmount(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: unmount <mount_point>" << std::endl;
        return;
    }
    
    std::string mountPoint = args[0];
    
    if (vfs.unmountVolume(mountPoint)) {
        std::cout << "Unmounted volume at " << mountPoint << std::endl;
    } else {
        std::cout << "Failed to unmount. Check if the mount point exists." << std::endl;
    }
}

void Shell::cmdMounts(const std::vector<std::string>& args) {
    auto volumes = vfs.listMountedVolumes();
    
    if (volumes.empty()) {
        std::cout << "No mounted volumes" << std::endl;
        return;
    }
    
    std::cout << "Mounted volumes:" << std::endl;
    for (const auto& volume : volumes) {
        std::cout << "  " << volume << std::endl;
    }
}

// Compression
void Shell::cmdCompress(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: compress <file_path>" << std::endl;
        return;
    }
    
    if (vfs.compressFile(args[0], true)) {
        std::cout << "File compressed: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to compress file. Check if it exists and is not a directory." << std::endl;
    }
}

void Shell::cmdUncompress(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: uncompress <file_path>" << std::endl;
        return;
    }
    
    if (vfs.compressFile(args[0], false)) {
        std::cout << "File uncompressed: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to uncompress file. Check if it exists and is not a directory." << std::endl;
    }
}

void Shell::cmdIsCompressed(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: iscompressed <file_path>" << std::endl;
        return;
    }
    
    if (vfs.isFileCompressed(args[0])) {
        std::cout << "File is compressed: " << args[0] << std::endl;
    } else {
        std::cout << "File is not compressed: " << args[0] << std::endl;
    }
}

// Encryption
void Shell::cmdEncrypt(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: encrypt <file_path> <encryption_key>" << std::endl;
        return;
    }
    
    if (vfs.encryptFile(args[0], args[1])) {
        std::cout << "File encrypted: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to encrypt file. Check if it exists and is not a directory." << std::endl;
    }
}

void Shell::cmdDecrypt(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: decrypt <file_path>" << std::endl;
        return;
    }
    
    if (vfs.decryptFile(args[0])) {
        std::cout << "File decrypted: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to decrypt file. Check if it exists and is not a directory." << std::endl;
    }
}

void Shell::cmdIsEncrypted(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: isencrypted <file_path>" << std::endl;
        return;
    }
    
    if (vfs.isFileEncrypted(args[0])) {
        std::cout << "File is encrypted: " << args[0] << std::endl;
    } else {
        std::cout << "File is not encrypted: " << args[0] << std::endl;
    }
}

void Shell::cmdChangeKey(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: changekey <file_path> <new_key>" << std::endl;
        return;
    }
    
    if (vfs.changeEncryptionKey(args[0], args[1])) {
        std::cout << "Encryption key changed for file: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to change encryption key. Check if the file exists and is encrypted." << std::endl;
    }
}

// Versioning
void Shell::cmdSaveVersion(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: saveversion <file_path>" << std::endl;
        return;
    }
    
    if (vfs.saveFileVersion(args[0])) {
        std::cout << "Version saved for file: " << args[0] << std::endl;
    } else {
        std::cout << "Failed to save version. Check if the file exists and is not a directory." << std::endl;
    }
}

void Shell::cmdRestoreVersion(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: restoreversion <file_path> <version_index>" << std::endl;
        return;
    }
    
    size_t versionIndex;
    try {
        versionIndex = std::stoul(args[1]);
    } catch (const std::exception&) {
        std::cout << "Invalid version index: must be a non-negative integer" << std::endl;
        return;
    }
    
    if (vfs.restoreFileVersion(args[0], versionIndex)) {
        std::cout << "File restored to version " << versionIndex << ": " << args[0] << std::endl;
    } else {
        std::cout << "Failed to restore version. Check if the file and version exist." << std::endl;
    }
}

void Shell::cmdListVersions(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: listversions <file_path>" << std::endl;
        return;
    }
    
    size_t versionCount = vfs.getFileVersionCount(args[0]);
    
    if (versionCount == 0) {
        std::cout << "No versions available for file: " << args[0] << std::endl;
        return;
    }
    
    std::vector<std::time_t> timestamps = vfs.getFileVersionTimestamps(args[0]);
    
    std::cout << "Versions for file " << args[0] << ":" << std::endl;
    for (size_t i = 0; i < timestamps.size(); ++i) {
        std::cout << "  [" << i << "] " << formatTimestamp(timestamps[i]) << std::endl;
    }
}

void Shell::cmdPwd(const std::vector<std::string>& args) {
    std::string currentPath = vfs.getCurrentPath();
    std::cout << currentPath << std::endl;
}

void Shell::cmdCp(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: cp <source> <destination>" << std::endl;
        return;
    }
    
    std::string sourcePath = args[0];
    std::string destPath = args[1];
    
    FileNode* sourceNode = vfs.resolvePath(sourcePath);
    if (!sourceNode) {
        std::cout << "Source file/directory not found: " << sourcePath << std::endl;
        return;
    }
    
    std::string sourceBaseName = sourcePath;
    size_t lastSlash = sourcePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        sourceBaseName = sourcePath.substr(lastSlash + 1);
    }
    
    FileNode* destNode = vfs.resolvePath(destPath);
    std::string finalDestPath = destPath;
    
    if (destNode && destNode->isDirectory()) {
        if (destPath.back() != '/') {
            finalDestPath += '/';
        }
        finalDestPath += sourceBaseName;
    }
    
    if (!sourceNode->isDirectory()) {
        std::string content = vfs.cat(sourcePath);
        
        if (vfs.write(finalDestPath, content)) {
            std::cout << "File copied from " << sourcePath << " to " << finalDestPath << std::endl;
            
            if (sharedVfs) {
                sharedVfs->write(finalDestPath, content);
            }
        } else {
            std::cout << "Failed to copy file to " << finalDestPath << std::endl;
        }
    } else {
        std::cout << "Copying directories not supported yet" << std::endl;
    }
}

void Shell::cmdMv(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: mv <source> <destination>" << std::endl;
        return;
    }
    
    std::string sourcePath = args[0];
    std::string destPath = args[1];
    
    FileNode* sourceNode = vfs.resolvePath(sourcePath);
    if (!sourceNode) {
        std::cout << "Source file/directory not found: " << sourcePath << std::endl;
        return;
    }
    
    std::string sourceBaseName = sourcePath;
    size_t lastSlash = sourcePath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        sourceBaseName = sourcePath.substr(lastSlash + 1);
    }
    
    FileNode* destNode = vfs.resolvePath(destPath);
    std::string finalDestPath = destPath;
    
    if (destNode && destNode->isDirectory()) {
        if (destPath.back() != '/') {
            finalDestPath += '/';
        }
        finalDestPath += sourceBaseName;
    }
    
    if (!sourceNode->isDirectory()) {
        std::string content = vfs.cat(sourcePath);
        
        if (vfs.write(finalDestPath, content)) {
            if (vfs.remove(sourcePath)) {
                std::cout << "File moved from " << sourcePath << " to " << finalDestPath << std::endl;
                
                if (sharedVfs) {
                    sharedVfs->write(finalDestPath, content);
                    sharedVfs->remove(sourcePath);
                }
            } else {
                std::cout << "Copied to destination, but failed to remove source: " << sourcePath << std::endl;
            }
        } else {
            std::cout << "Failed to move file: could not write to " << finalDestPath << std::endl;
        }
    } else {
        std::cout << "Moving directories not supported yet" << std::endl;
    }
}

void Shell::cmdAsk(const std::vector<std::string>& args) {
    if (!assistant) {
        std::cout << "Assistant is not available." << std::endl;
        return;
    }
    
    if (args.empty()) {
        std::cout << assistant->getHelpInfo() << std::endl;
        return;
    }
    
    std::string query;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) {
            query += " ";
        }
        query += args[i];
    }
    
    std::string response = assistant->processQuery(query);
    std::cout << "\n" << response << "\n" << std::endl;
}

void Shell::cmdAssistant(const std::vector<std::string>& args) {
    // This is just an alias for cmdAsk
    cmdAsk(args);
}

void Shell::cmdFind(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: find [--name pattern] [--regex] [--content pattern] [--size min:max] [--date after:before] [--type f|d] [--tag tag] [path]" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  find --name notes              - Find by filename containing 'notes'" << std::endl;
        std::cout << "  find --name .txt --regex       - Find by filename regex pattern" << std::endl;
        std::cout << "  find --content hello           - Find files containing 'hello'" << std::endl;
        std::cout << "  find --size 1024:5120          - Find files between 1KB and 5KB" << std::endl;
        std::cout << "  find --date 2023-01-01:        - Find files modified after Jan 1, 2023" << std::endl;
        std::cout << "  find --type f                  - Find only files" << std::endl;
        std::cout << "  find --tag important           - Find by tag" << std::endl;
        return;
    }
    
    SearchFilter filter;
    std::string startPath = ".";
    
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--name" && i + 1 < args.size()) {
            filter.nameContains = args[++i];
        } else if (args[i] == "--regex") {
            if (filter.nameContains.has_value()) {
                try {
                    filter.namePattern = std::regex(filter.nameContains.value());
                    filter.nameContains.reset();
                } catch (const std::regex_error&) {
                    std::cout << "Invalid regex pattern: " << filter.nameContains.value() << std::endl;
                    return;
                }
            } else {
                std::cout << "Error: --regex must follow a --name argument" << std::endl;
                return;
            }
        } else if (args[i] == "--content" && i + 1 < args.size()) {
            filter.contentContains = args[++i];
        } else if (args[i] == "--size" && i + 1 < args.size()) {
            std::string sizeStr = args[++i];
            size_t colonPos = sizeStr.find(':');
            
            if (colonPos != std::string::npos) {
                std::string minStr = sizeStr.substr(0, colonPos);
                std::string maxStr = sizeStr.substr(colonPos + 1);
                
                if (!minStr.empty()) {
                    try {
                        filter.minSize = std::stoul(minStr);
                    } catch (const std::exception&) {
                        std::cout << "Invalid minimum size: " << minStr << std::endl;
                        return;
                    }
                }
                
                if (!maxStr.empty()) {
                    try {
                        filter.maxSize = std::stoul(maxStr);
                    } catch (const std::exception&) {
                        std::cout << "Invalid maximum size: " << maxStr << std::endl;
                        return;
                    }
                }
            } else {
                std::cout << "Invalid size format. Use min:max format (e.g., 1024:5120)." << std::endl;
                return;
            }
        } else if (args[i] == "--date" && i + 1 < args.size()) {
            std::string dateStr = args[++i];
            size_t colonPos = dateStr.find(':');
            
            if (colonPos != std::string::npos) {
                std::string afterStr = dateStr.substr(0, colonPos);
                std::string beforeStr = dateStr.substr(colonPos + 1);
                
                if (!afterStr.empty()) {
                    struct tm tm = {};
                    if (strptime(afterStr.c_str(), "%Y-%m-%d", &tm)) {
                        filter.modifiedAfter = mktime(&tm);
                    } else {
                        std::cout << "Invalid 'after' date format. Use YYYY-MM-DD." << std::endl;
                        return;
                    }
                }
                
                if (!beforeStr.empty()) {
                    struct tm tm = {};
                    if (strptime(beforeStr.c_str(), "%Y-%m-%d", &tm)) {
                        filter.modifiedBefore = mktime(&tm);
                    } else {
                        std::cout << "Invalid 'before' date format. Use YYYY-MM-DD." << std::endl;
                        return;
                    }
                }
            } else {
                std::cout << "Invalid date format. Use after:before format (e.g., 2023-01-01:2023-12-31)." << std::endl;
                return;
            }
        } else if (args[i] == "--type" && i + 1 < args.size()) {
            if (args[i + 1] == "f") {
                filter.filesOnly = true;
                filter.directoriesOnly = false;
            } else if (args[i + 1] == "d") {
                filter.filesOnly = false;
                filter.directoriesOnly = true;
            } else {
                std::cout << "Invalid type. Use 'f' for files or 'd' for directories." << std::endl;
                return;
            }
            i++;
        } else if (args[i] == "--tag" && i + 1 < args.size()) {
            filter.tags.push_back(args[++i]);
        } else if (args[i][0] != '-') {
            // Assume it's a path if it doesn't start with -
            startPath = args[i];
        } else {
            std::cout << "Unknown option: " << args[i] << std::endl;
            return;
        }
    }
    
    auto results = vfs.search(filter, startPath);
    
    if (results.empty()) {
        std::cout << "No matching files found." << std::endl;
    } else {
        std::cout << "Found " << results.size() << " matching item(s):" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result << std::endl;
        }
    }
}

void Shell::cmdFindByName(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: findname <pattern> [--regex] [path]" << std::endl;
        return;
    }
    
    std::string pattern = args[0];
    bool useRegex = false;
    std::string path = ".";
    
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--regex") {
            useRegex = true;
        } else if (args[i][0] != '-') {
            path = args[i];
        }
    }
    
    auto results = vfs.searchByName(pattern, useRegex, path);
    
    if (results.empty()) {
        std::cout << "No files found matching name pattern: " << pattern << std::endl;
    } else {
        std::cout << "Files matching name pattern \"" << pattern << "\":" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result << std::endl;
        }
    }
}

void Shell::cmdFindByContent(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: grep <pattern> [--regex] [path]" << std::endl;
        return;
    }
    
    std::string pattern = args[0];
    bool useRegex = false;
    std::string path = ".";
    
    for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--regex") {
            useRegex = true;
        } else if (args[i][0] != '-') {
            path = args[i];
        }
    }
    
    auto results = vfs.searchByContent(pattern, useRegex, path);
    
    if (results.empty()) {
        std::cout << "No files found containing: " << pattern << std::endl;
    } else {
        std::cout << "Files containing \"" << pattern << "\":" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result << std::endl;
        }
    }
}

void Shell::cmdFindBySize(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: findsize <min_size> <max_size> [path]" << std::endl;
        std::cout << "Sizes can be specified in bytes, or with KB/MB suffix (e.g., 5KB, 2MB)" << std::endl;
        return;
    }
    
    auto parseSize = [](const std::string& sizeStr) -> size_t {
        if (sizeStr.empty()) {
            return 0;
        }
        
        size_t size = 0;
        size_t multiplier = 1;
        
        if (sizeStr.find("KB") != std::string::npos) {
            multiplier = 1024;
        } else if (sizeStr.find("MB") != std::string::npos) {
            multiplier = 1024 * 1024;
        }
        
        try {
            size = std::stoul(sizeStr);
            return size * multiplier;
        } catch (const std::exception&) {
            try {
                // Try parsing just the numeric part
                size = std::stoul(sizeStr.substr(0, sizeStr.find_first_not_of("0123456789")));
                return size * multiplier;
            } catch (const std::exception&) {
                return 0; // Default if parsing fails
            }
        }
    };
    
    size_t minSize = parseSize(args[0]);
    size_t maxSize = parseSize(args[1]);
    std::string path = args.size() > 2 ? args[2] : ".";
    
    if (minSize > maxSize && maxSize != 0) {
        std::cout << "Error: Minimum size cannot be greater than maximum size" << std::endl;
        return;
    }
    
    auto results = vfs.searchBySize(minSize, maxSize, path);
    
    if (results.empty()) {
        std::cout << "No files found in the specified size range" << std::endl;
    } else {
        std::cout << "Files with size between " << formatSize(minSize) << " and " 
                  << (maxSize > 0 ? formatSize(maxSize) : "unlimited") << ":" << std::endl;
        for (const auto& result : results) {
            FileNode* node = vfs.resolvePath(result);
            if (node && !node->isDirectory()) {
                std::cout << "  " << result << " (" << formatSize(node->getSize()) << ")" << std::endl;
            } else {
                std::cout << "  " << result << std::endl;
            }
        }
    }
}

void Shell::cmdFindByDate(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: finddate <after_date> <before_date> [path]" << std::endl;
        std::cout << "Dates must be in YYYY-MM-DD format. Use 'all' to skip a date." << std::endl;
        return;
    }
    
    std::string afterStr = args[0];
    std::string beforeStr = args[1];
    std::string path = args.size() > 2 ? args[2] : ".";
    
    time_t afterDate = 0;
    time_t beforeDate = std::numeric_limits<time_t>::max();
    
    if (afterStr != "all") {
        struct tm tm = {};
        if (!strptime(afterStr.c_str(), "%Y-%m-%d", &tm)) {
            std::cout << "Invalid 'after' date format. Use YYYY-MM-DD." << std::endl;
            return;
        }
        afterDate = mktime(&tm);
    }
    
    if (beforeStr != "all") {
        struct tm tm = {};
        if (!strptime(beforeStr.c_str(), "%Y-%m-%d", &tm)) {
            std::cout << "Invalid 'before' date format. Use YYYY-MM-DD." << std::endl;
            return;
        }
        beforeDate = mktime(&tm);
    }
    
    // Perform search
    auto results = vfs.searchByDate(afterDate, beforeDate, path);
    
    // Display results
    if (results.empty()) {
        std::cout << "No files found in the specified date range" << std::endl;
    } else {
        std::cout << "Files modified between " 
                  << (afterStr == "all" ? "any time" : afterStr) << " and " 
                  << (beforeStr == "all" ? "now" : beforeStr) << ":" << std::endl;
        for (const auto& result : results) {
            FileNode* node = vfs.resolvePath(result);
            if (node) {
                std::time_t modTime = getNodeModificationTime(node);
                std::cout << "  " << result << " (" << formatTimestamp(modTime) << ")" << std::endl;
            } else {
                std::cout << "  " << result << std::endl;
            }
        }
    }
}

void Shell::cmdAddTag(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: addtag <file_path> <tag>" << std::endl;
        return;
    }
    
    std::string path = args[0];
    std::string tag = args[1];
    
    if (vfs.addTag(path, tag)) {
        std::cout << "Added tag '" << tag << "' to " << path << std::endl;
    } else {
        std::cout << "Failed to add tag. Check if the file exists." << std::endl;
    }
}

void Shell::cmdRemoveTag(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: rmtag <file_path> <tag>" << std::endl;
        return;
    }
    
    std::string path = args[0];
    std::string tag = args[1];
    
    if (vfs.removeTag(path, tag)) {
        std::cout << "Removed tag '" << tag << "' from " << path << std::endl;
    } else {
        std::cout << "Failed to remove tag. Check if the file exists and has that tag." << std::endl;
    }
}

void Shell::cmdListTags(const std::vector<std::string>& args) {
    if (args.empty()) {
        auto tags = vfs.getAllTags();
        
        if (tags.empty()) {
            std::cout << "No tags found in the system" << std::endl;
        } else {
            std::cout << "All tags in the system:" << std::endl;
            for (const auto& tag : tags) {
                std::cout << "  " << tag << std::endl;
            }
        }
    } else {
        std::string path = args[0];
        auto tags = vfs.getFileTags(path);
        
        if (tags.empty()) {
            std::cout << "No tags found for " << path << std::endl;
        } else {
            std::cout << "Tags for " << path << ":" << std::endl;
            for (const auto& tag : tags) {
                std::cout << "  " << tag << std::endl;
            }
        }
    }
}

void Shell::cmdFindByTag(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: findtag <tag> [path]" << std::endl;
        return;
    }
    
    std::string tag = args[0];
    std::string path = args.size() > 1 ? args[1] : ".";
    
    auto results = vfs.searchByTag(tag, path);
    
    if (results.empty()) {
        std::cout << "No files found with tag: " << tag << std::endl;
    } else {
        std::cout << "Files with tag \"" << tag << "\":" << std::endl;
        for (const auto& result : results) {
            std::cout << "  " << result << std::endl;
        }
    }
}

void Shell::initializeBuiltinCommands() {
    for (const auto& pair : commands) {
        builtinCommands[pair.first] = true;
    }
}

bool Shell::registerCommand(const std::string& name, CommandFunction func) {
    if (isBuiltinCommand(name)) {
        std::cerr << "Cannot register plugin command '" << name << "': name conflicts with built-in command" << std::endl;
        return false;
    }
    
    if (commands.find(name) != commands.end()) {
        std::cerr << "Command '" << name << "' is already registered" << std::endl;
        return false;
    }
    
    commands[name] = func;
    return true;
}

bool Shell::unregisterCommand(const std::string& name) {
    if (isBuiltinCommand(name)) {
        std::cerr << "Cannot unregister built-in command '" << name << "'" << std::endl;
        return false;
    }
    
    auto it = commands.find(name);
    if (it == commands.end()) {
        std::cerr << "Command '" << name << "' is not registered" << std::endl;
        return false;
    }
    
    commands.erase(it);
    return true;
}

bool Shell::isBuiltinCommand(const std::string& name) const {
    auto it = builtinCommands.find(name);
    return (it != builtinCommands.end() && it->second);
}

void Shell::loadPlugin(const std::string& path) {
    if (pluginManager->loadPlugin(path)) {
        std::cout << "Successfully loaded plugin from: " << path << std::endl;
    } else {
        std::cerr << "Failed to load plugin from: " << path << std::endl;
    }
}

void Shell::unloadPlugin(const std::string& name) {
    if (pluginManager->unloadPlugin(name)) {
        std::cout << "Successfully unloaded plugin: " << name << std::endl;
    } else {
        std::cerr << "Failed to unload plugin: " << name << std::endl;
    }
}

void Shell::listPlugins() {
    auto plugins = pluginManager->getLoadedPlugins();
    
    if (plugins.empty()) {
        std::cout << "No plugins are currently loaded" << std::endl;
        return;
    }
    
    std::cout << "Loaded plugins:" << std::endl;
    std::cout << "------------------------------------------------------" << std::endl;
    
    for (const auto& pluginName : plugins) {
        Plugin* plugin = pluginManager->getPlugin(pluginName);
        if (plugin) {
            std::cout << plugin->getName() << " (v" << plugin->getVersion() << ")" << std::endl;
            std::cout << "  Author: " << plugin->getAuthor() << std::endl;
            std::cout << "  Description: " << plugin->getDescription() << std::endl;
            
            auto commands = plugin->getCommands();
            if (!commands.empty()) {
                std::cout << "  Commands:" << std::endl;
                for (const auto& cmd : commands) {
                    std::cout << "    - " << cmd.first << std::endl;
                }
            }
            std::cout << "------------------------------------------------------" << std::endl;
        }
    }
}

void Shell::cmdLoadPlugin(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        std::cout << "Usage: loadplugin <path_to_plugin>" << std::endl;
        return;
    }
    
    loadPlugin(args[0]);
}

void Shell::cmdUnloadPlugin(const std::vector<std::string>& args) {
    if (args.size() < 1) {
        std::cout << "Usage: unloadplugin <plugin_name>" << std::endl;
        return;
    }
    
    unloadPlugin(args[0]);
}

void Shell::cmdListPlugins(const std::vector<std::string>& args) {
    listPlugins();
}
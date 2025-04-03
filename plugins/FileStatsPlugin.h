#ifndef FILE_STATS_PLUGIN_H
#define FILE_STATS_PLUGIN_H

#include "../include/Plugin.h"
#include "../include/Shell.h"
#include "../include/VirtualFileSystem.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>

/**
 * @brief FileStats Plugin
 * 
 * This plugin provides commands for analyzing files in the VFS:
 * - filestats: Show statistics about a file
 * - diskusage: Show disk usage statistics by directory
 * - findduplicates: Find duplicate files
 */
class FileStatsPlugin : public Plugin {
public:
    FileStatsPlugin();
    ~FileStatsPlugin() override;
    
    std::string getName() const override { return "FileStats"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { 
        return "Provides commands for file statistics and analysis"; 
    }
    std::string getAuthor() const override { return "Me"; }
    
    bool initialize(Shell* shell) override;
    bool shutdown() override;
    std::vector<std::pair<std::string, Shell::CommandFunction>> getCommands() const override;
    
private:
    Shell* shell;
    
    void cmdFileStats(Shell* shell, const std::vector<std::string>& args);
    void cmdDiskUsage(Shell* shell, const std::vector<std::string>& args);
    void cmdFindDuplicates(Shell* shell, const std::vector<std::string>& args);
    
    std::string formatFileSize(size_t sizeBytes) const;
    std::string getFileType(const std::string& path, VirtualFileSystem& vfs) const;
    
    std::vector<std::pair<std::string, Shell::CommandFunction>> commands;
};

#endif // FILE_STATS_PLUGIN_H
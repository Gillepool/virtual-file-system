#ifndef SHELL_H
#define SHELL_H

#include "VirtualFileSystem.h"
#include "AssistantInterface.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <functional>

class PluginManager;

class Shell {
public:
    Shell();
    Shell(std::shared_ptr<VirtualFileSystem> vfsPtr);
    ~Shell();
    
    void run();
    
    VirtualFileSystem& getVFS() { return vfs; }
    AssistantInterface* getAssistant() { return assistant.get(); }
    std::vector<std::string> parseCommand(const std::string& cmdLine);
    
    using CommandFunction = std::function<void(Shell*, const std::vector<std::string>&)>;
    std::map<std::string, CommandFunction> commands;
    
    bool registerCommand(const std::string& name, CommandFunction func);
    bool unregisterCommand(const std::string& name);
    bool isBuiltinCommand(const std::string& name) const;
    
    void loadPlugin(const std::string& path);
    void unloadPlugin(const std::string& name);
    void listPlugins();
    
    PluginManager* getPluginManager() { return pluginManager.get(); }
    
private:
    VirtualFileSystem vfs;
    std::shared_ptr<VirtualFileSystem> sharedVfs;
    std::unique_ptr<AssistantInterface> assistant;
    std::unique_ptr<PluginManager> pluginManager;
    std::map<std::string, bool> builtinCommands;
    bool running;

    void cmdMkdir(const std::vector<std::string>& args);
    void cmdTouch(const std::vector<std::string>& args);
    void cmdCd(const std::vector<std::string>& args);
    void cmdLs(const std::vector<std::string>& args);
    void cmdCat(const std::vector<std::string>& args);
    void cmdWrite(const std::vector<std::string>& args);
    void cmdRm(const std::vector<std::string>& args);
    void cmdHelp(const std::vector<std::string>& args);
    void cmdExit(const std::vector<std::string>& args);
    void cmdSave(const std::vector<std::string>& args);
    void cmdLoad(const std::vector<std::string>& args);
    void cmdDiskInfo(const std::vector<std::string>& args);
    void cmdPwd(const std::vector<std::string>& args);
    void cmdCp(const std::vector<std::string>& args);
    void cmdMv(const std::vector<std::string>& args);
    void cmdCreateVolume(const std::vector<std::string>& args);
    void cmdMount(const std::vector<std::string>& args);
    void cmdUnmount(const std::vector<std::string>& args);
    void cmdMounts(const std::vector<std::string>& args);
    void cmdCompress(const std::vector<std::string>& args);
    void cmdUncompress(const std::vector<std::string>& args);
    void cmdIsCompressed(const std::vector<std::string>& args);
    void cmdEncrypt(const std::vector<std::string>& args);
    void cmdDecrypt(const std::vector<std::string>& args);
    void cmdIsEncrypted(const std::vector<std::string>& args);
    void cmdChangeKey(const std::vector<std::string>& args);
    
    void cmdSaveVersion(const std::vector<std::string>& args);
    void cmdRestoreVersion(const std::vector<std::string>& args);
    void cmdListVersions(const std::vector<std::string>& args);
    
    void cmdAsk(const std::vector<std::string>& args);
    void cmdAssistant(const std::vector<std::string>& args);
    
    void cmdFind(const std::vector<std::string>& args);
    void cmdFindByName(const std::vector<std::string>& args);
    void cmdFindByContent(const std::vector<std::string>& args);
    void cmdFindBySize(const std::vector<std::string>& args);
    void cmdFindByDate(const std::vector<std::string>& args);
    
    void cmdAddTag(const std::vector<std::string>& args);
    void cmdRemoveTag(const std::vector<std::string>& args);
    void cmdListTags(const std::vector<std::string>& args);
    void cmdFindByTag(const std::vector<std::string>& args);

    void cmdLoadPlugin(const std::vector<std::string>& args);
    void cmdUnloadPlugin(const std::vector<std::string>& args);
    void cmdListPlugins(const std::vector<std::string>& args);
    
    std::string formatSize(size_t sizeInBytes) const;
    std::string formatTimestamp(std::time_t timestamp) const;
    void initializeBuiltinCommands();
};

#endif // SHELL_H
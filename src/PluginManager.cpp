#include "../include/PluginManager.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

#ifdef _WIN32
// Windows specific includes
#else
#include <dlfcn.h>
#include <dirent.h>
#endif

PluginManager::PluginManager(Shell* shell) : shell(shell) {
}

PluginManager::~PluginManager() {
    std::vector<std::string> pluginNames;
    for (const auto& pair : plugins) {
        pluginNames.push_back(pair.first);
    }
    
    for (const std::string& name : pluginNames) {
        unloadPlugin(name);
    }
}

bool PluginManager::loadPlugin(const std::string& path) {
    LibraryHandle handle = loadLibrary(path);
    if (!handle) {
        std::cerr << "Failed to load plugin library: " << path << std::endl;
        return false;
    }
    
    CreatePluginFunc createFunc = reinterpret_cast<CreatePluginFunc>(
        getFunction(handle, "createPlugin")
    );
    
    if (!createFunc) {
        std::cerr << "Failed to find createPlugin function in: " << path << std::endl;
        unloadLibrary(handle);
        return false;
    }
    
    Plugin* pluginInstance = createFunc();
    if (!pluginInstance) {
        std::cerr << "Failed to create plugin instance from: " << path << std::endl;
        unloadLibrary(handle);
        return false;
    }
    
    if (!pluginInstance->initialize(shell)) {
        std::cerr << "Failed to initialize plugin: " << pluginInstance->getName() << std::endl;
        delete pluginInstance;
        unloadLibrary(handle);
        return false;
    }
    
    PluginInfo info;
    info.path = path;
    info.handle = handle;
    info.instance = pluginInstance;
    info.createFunc = createFunc;
    
    plugins[pluginInstance->getName()] = info;
    
    auto commands = pluginInstance->getCommands();
    for (const auto& cmdPair : commands) {
        shell->registerCommand(cmdPair.first, cmdPair.second);
    }
    
    std::cout << "Loaded plugin: " << pluginInstance->getName() 
              << " v" << pluginInstance->getVersion() 
              << " by " << pluginInstance->getAuthor() << std::endl;
    
    return true;
}

bool PluginManager::unloadPlugin(const std::string& name) {
    auto it = plugins.find(name);
    if (it == plugins.end()) {
        std::cerr << "Plugin not found: " << name << std::endl;
        return false;
    }
    
    PluginInfo& info = it->second;
    
    if (!info.instance->shutdown()) {
        std::cerr << "Failed to properly shut down plugin: " << name << std::endl;
        // Continue anyway to clean up resources
    }
    
    auto commands = info.instance->getCommands();
    for (const auto& cmdPair : commands) {
        shell->unregisterCommand(cmdPair.first);
    }
    
    delete info.instance;
    
    if (!unloadLibrary(info.handle)) {
        std::cerr << "Warning: Failed to unload plugin library: " << info.path << std::endl;
    }
    
    plugins.erase(it);
    
    std::cout << "Unloaded plugin: " << name << std::endl;
    return true;
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::vector<std::string> result;
    for (const auto& pair : plugins) {
        result.push_back(pair.first);
    }
    return result;
}

Plugin* PluginManager::getPlugin(const std::string& name) const {
    auto it = plugins.find(name);
    if (it == plugins.end()) {
        return nullptr;
    }
    return it->second.instance;
}

int PluginManager::discoverAndLoadPlugins(const std::string& directory) {
    int count = 0;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            std::string path = entry.path().string();
            std::string extension = entry.path().extension().string();
            
            // Check if this is a dynamic library file
            bool isPlugin = false;
            
            #ifdef _WIN32
            isPlugin = (extension == ".dll");
            #elif defined(__APPLE__)
            isPlugin = (extension == ".dylib");
            #else
            isPlugin = (extension == ".so");
            #endif
            
            if (isPlugin && loadPlugin(path)) {
                count++;
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing plugin directory: " << e.what() << std::endl;
    }
    
    return count;
}

int PluginManager::registerPluginCommands() {
    int count = 0;
    
    for (const auto& pair : plugins) {
        Plugin* plugin = pair.second.instance;
        auto commands = plugin->getCommands();
        
        for (const auto& cmdPair : commands) {
            shell->registerCommand(cmdPair.first, cmdPair.second);
            count++;
        }
    }
    
    return count;
}

// Platform-specific implementation for loading dynamic libraries
#ifdef _WIN32
// Windows implementation
LibraryHandle PluginManager::loadLibrary(const std::string& path) {
    return LoadLibraryA(path.c_str());
}

bool PluginManager::unloadLibrary(LibraryHandle handle) {
    return FreeLibrary(handle) != 0;
}

void* PluginManager::getFunction(LibraryHandle handle, const std::string& funcName) {
    return reinterpret_cast<void*>(GetProcAddress(handle, funcName.c_str()));
}
#else
// UNIX implementation (Linux, macOS)
LibraryHandle PluginManager::loadLibrary(const std::string& path) {
    return dlopen(path.c_str(), RTLD_LAZY);
}

bool PluginManager::unloadLibrary(LibraryHandle handle) {
    return dlclose(handle) == 0;
}

void* PluginManager::getFunction(LibraryHandle handle, const std::string& funcName) {
    return dlsym(handle, funcName.c_str());
}
#endif
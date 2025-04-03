#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Plugin.h"
#include "Shell.h"

#ifdef _WIN32
#include <windows.h>
typedef HMODULE LibraryHandle;
#else
typedef void* LibraryHandle;
#endif

/**
 * @brief Manages the loading, unloading, and interaction with plugins
 * 
 * The PluginManager is responsible for discovering, loading, initializing,
 * and managing the lifecycle of all plugins.
 */
class PluginManager {
public:
    /**
     * @brief Constructor
     * @param shell Pointer to the Shell instance for plugin initialization
     */
    explicit PluginManager(Shell* shell);
    
    /**
     * @brief Destructor
     * Ensures all plugins are properly unloaded
     */
    ~PluginManager();
    
    /**
     * @brief Load a plugin from a dynamic library file
     * @param path Path to the plugin library (.so, .dll, or .dylib)
     * @return true if successfully loaded, false otherwise
     */
    bool loadPlugin(const std::string& path);
    
    /**
     * @brief Unload a plugin by name
     * @param name Name of the plugin to unload
     * @return true if successfully unloaded, false otherwise
     */
    bool unloadPlugin(const std::string& name);
    
    /**
     * @brief Get a list of all loaded plugins
     * @return Vector of plugin names
     */
    std::vector<std::string> getLoadedPlugins() const;
    
    /**
     * @brief Get a pointer to a loaded plugin by name
     * @param name Name of the plugin to retrieve
     * @return Pointer to the plugin or nullptr if not found
     */
    Plugin* getPlugin(const std::string& name) const;
    
    /**
     * @brief Discover and load all plugins in a directory
     * @param directory Path to the plugins directory
     * @return Number of plugins successfully loaded
     */
    int discoverAndLoadPlugins(const std::string& directory);
    
    /**
     * @brief Register all plugin commands with the shell
     * This is called after loading a plugin
     * @return Number of commands registered
     */
    int registerPluginCommands();

private:
    /**
     * @brief Internal structure to track loaded plugins
     */
    struct PluginInfo {
        std::string path;          // Path to the plugin library file
        LibraryHandle handle;      // Handle to the loaded library
        Plugin* instance;          // Instance of the plugin
        CreatePluginFunc createFunc; // Plugin creation function
        
        PluginInfo() : handle(nullptr), instance(nullptr), createFunc(nullptr) {}
    };
    
    /**
     * @brief Helper function to load a dynamic library
     * @param path Path to the library file
     * @return Handle to the loaded library or nullptr on failure
     */
    LibraryHandle loadLibrary(const std::string& path);
    
    /**
     * @brief Helper function to unload a dynamic library
     * @param handle Handle to the loaded library
     * @return true if successfully unloaded, false otherwise
     */
    bool unloadLibrary(LibraryHandle handle);
    
    /**
     * @brief Helper function to get a function from a loaded library
     * @param handle Handle to the loaded library
     * @param funcName Name of the function to retrieve
     * @return Pointer to the function or nullptr on failure
     */
    void* getFunction(LibraryHandle handle, const std::string& funcName);
    
    Shell* shell;                                  // Pointer to the shell instance
    std::map<std::string, PluginInfo> plugins;     // Map of plugin name to plugin info
};

#endif // PLUGIN_MANAGER_H
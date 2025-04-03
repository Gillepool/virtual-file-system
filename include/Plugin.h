#ifndef PLUGIN_H
#define PLUGIN_H

#include <string>
#include <vector>
#include <functional>
#include "Shell.h"

/**
 * @brief Base interface for all VFS plugins
 *
 * This class defines the interface that all plugins must implement to be
 * loaded and used by the VFS system.
 */
class Plugin {
public:
    /**
     * Virtual destructor for proper cleanup
     */
    virtual ~Plugin() = default;

    /**
     * @brief Get the name of the plugin
     * @return The plugin's name
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get the version of the plugin
     * @return The plugin's version string
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Get a description of the plugin's functionality
     * @return Description string
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Get the author of the plugin
     * @return Author string
     */
    virtual std::string getAuthor() const = 0;
    
    /**
     * @brief Initialize the plugin
     * 
     * This method is called when the plugin is loaded.
     * 
     * @param shell Pointer to the Shell instance for command registration
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize(Shell* shell) = 0;
    
    /**
     * @brief Shutdown the plugin
     * 
     * This method is called before the plugin is unloaded.
     * 
     * @return true if shutdown was successful, false otherwise
     */
    virtual bool shutdown() = 0;
    
    /**
     * @brief Get the commands provided by this plugin
     * 
     * Each command should be a pair of the command name and its handler function.
     * 
     * @return Vector of command name and handler function pairs
     */
    virtual std::vector<std::pair<std::string, Shell::CommandFunction>> getCommands() const = 0;
};

// Define the plugin creation function type
// This will be implemented by each plugin and exported as a C-style function
typedef Plugin* (*CreatePluginFunc)();

// Macro to export the plugin creation function
#ifdef _WIN32
#define EXPORT_PLUGIN extern "C" __declspec(dllexport)
#else
#define EXPORT_PLUGIN extern "C"
#endif

// Macro to help plugin developers implement the export function
#define IMPLEMENT_PLUGIN(PluginClass) \
    EXPORT_PLUGIN Plugin* createPlugin() { \
        return new PluginClass(); \
    }

#endif // PLUGIN_H
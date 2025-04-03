# Virtual File System (VFS)

This is a work-in-progress virtual file system project! It includes a mix of CLI and GUI interfaces and aims to provide a functional virtual file management environment. The plan is to support features like directories, files, volumes, mounting, compression, encryption, versioning and stuffs. Some things might not work perfectly (or at all yet), but I’m actively experimenting and improving it!

## Features

- **Basic File Operations**: Create, read, write, copy, move, and delete files and directories
- **Multiple Volumes & Mounting**: Create and mount virtual disk images
- **Compression**: Compress and decompress files to save space
- **Encryption**: Secure your files with encryption
- **File Versioning**: Track and restore previous versions of files
- **Advanced Search**: Find files by name, content, size, date, or tags
- **Tagging System**: Organize files with custom tags
- **Plugin Support**: Extend functionality through loadable plugins
- **Interactive Shell**: Full-featured command-line interface
- **Graphical Interface**: User-friendly GUI alternative (Qt-based)
- **Built-in Assistant**: Get help and guidance while using the system

## Building from Source

### Prerequisites

- C++17 compatible compiler (GCC or Clang)
- Make build system
- Qt 5 (for GUI version)

### Build Instructions

1. Clone the repository:

2. Build the CLI version:
```
make cli
```

3. Build the GUI version (requires Qt 5):
```
make gui
```

4. Build plugins:
```
make plugins
```

5. Build everything:
```
make
```

### Running the Application

Run the CLI version:
```
./bin/vfs
```

Run the GUI version:
```
./bin/vfs-gui
```

## Command Line Interface Usage

The VFS shell provides a Unix-like command system. Here are the available commands:

### Basic Commands

- `pwd` - Print current working directory
- `mkdir <dir>` - Create a new directory
- `touch <file>` - Create a new empty file
- `cd <path>` - Change current directory
- `ls [path]` - List contents of a directory
- `ls -l [path]` - List contents with detailed information
- `cat <file>` - Display the contents of a file
- `write <file> <text>` - Write text to a file
- `cp <src> <dest>` - Copy a file
- `mv <src> <dest>` - Move or rename a file
- `rm <path>` - Remove a file or directory
- `save [filename]` - Save the file system to disk
- `load [filename]` - Load the file system from disk
- `diskinfo` - Display disk usage information
- `exit` - Exit the shell
- `help` - Display help message

### Search Commands

- `find [options]` - Advanced search with multiple filters
- `findname <pattern>` - Search by filename pattern
- `grep <pattern>` - Search by file content
- `findsize <min> <max>` - Search by file size
- `finddate <after> <before>` - Search by modification date
- `findtag <tag>` - Search by tag

### Multiple Volumes & Mounting Commands

- `createvolume <name> <size_mb>` - Create a new volume
- `mount <diskimg> <mountpoint>` - Mount a volume
- `unmount <mountpoint>` - Unmount a volume
- `mounts` - List mounted volumes

### Compression Commands

- `compress <file>` - Compress a file
- `uncompress <file>` - Uncompress a file
- `iscompressed <file>` - Check if a file is compressed

### Encryption Commands

- `encrypt <file> <key>` - Encrypt a file
- `decrypt <file>` - Decrypt a file
- `isencrypted <file>` - Check if a file is encrypted
- `changekey <file> <newkey>` - Change encryption key

### Versioning Commands

- `saveversion <file>` - Save current file version
- `restoreversion <file> <idx>` - Restore file to version index
- `listversions <file>` - List available versions

### Tag Management Commands

- `addtag <file> <tag>` - Add a tag to a file
- `rmtag <file> <tag>` - Remove a tag from a file
- `tags [file]` - List tags for file or all tags

### Plugin Management Commands

- `loadplugin <path>` - Load a plugin from a specified path
- `unloadplugin <name>` - Unload a plugin by name
- `plugins` - List all loaded plugins

### Assistant Commands

- `ask <query>` - Ask the assistant a question
- `assistant <query>` - Same as 'ask'

## Tab Completion

The VFS shell includes tab completion for commands and file paths:

- Press `Tab` once to autocomplete if there is only one matching option
- Press `Tab` twice to display all available options when there are multiple matches
- Works for both commands and file/directory paths

## Examples

Create and navigate directories:
```
mkdir documents
cd documents
pwd
mkdir notes
ls
```

Create and edit files:
```
touch example.txt
write example.txt This is a sample text file
cat example.txt
```

Copy and move files:
```
cp example.txt backup.txt
ls
mv backup.txt renamed.txt
ls
```

Create a compressed file:
```
write large.txt This is a large text file with lots of content...
compress large.txt
iscompressed large.txt
```

Encrypt a sensitive file:
```
touch secret.txt
write secret.txt This is confidential information
encrypt secret.txt mypassword
isencrypted secret.txt
```

Version control:
```
write versioned.txt Version 1
saveversion versioned.txt
write versioned.txt Version 2
listversions versioned.txt
restoreversion versioned.txt 0
cat versioned.txt
```

Advanced search:
```
find --name .txt       # Find all txt files
find --content hello   # Find files containing "hello"
grep hello             # Another way to search by content
find --size 0:1024     # Files smaller than 1KB
```

## Plugin System

The VFS includes a plugin system that allows extending its functionality:

### Built-in Plugin: FileStats

The FileStats plugin provides commands for analyzing files in the VFS:
- `filestats <file>` - Show detailed statistics about a file (size, type, line count, etc.)
- `diskusage [path] [--sort]` - Show disk usage statistics by directory
- `findduplicates [path]` - Find duplicate files in the filesystem

### Creating Plugins

You can create your own plugins by:
1. Creating a C++ class that inherits from the `Plugin` base class
2. Implementing required virtual methods like `getName()`, `initialize()`, etc.
3. Using the `IMPLEMENT_PLUGIN` macro to export your plugin
4. Building your plugin as a shared library (.dylib/.so/.dll)

Example plugin header:
```cpp
#include "../include/Plugin.h"
class MyPlugin : public Plugin {
public:
    // Required plugin interface methods
    std::string getName() const override { return "MyPlugin"; }
    std::string getVersion() const override { return "1.0.0"; }
    std::string getDescription() const override { return "My custom plugin"; }
    std::string getAuthor() const override { return "Your Name"; }
    
    bool initialize(Shell* shell) override;
    bool shutdown() override;
    std::vector<std::pair<std::string, Shell::CommandFunction>> getCommands() const override;
    
    // Your plugin implementation...
};
IMPLEMENT_PLUGIN(MyPlugin)
```

### Loading Plugins

You can load plugins by:
1. Building them with `make plugins`
2. Placing the shared libraries in the `plugins/` directory
3. Using the `loadplugin` command in the shell

Default plugins in the `plugins/` directory are loaded automatically when the shell starts.
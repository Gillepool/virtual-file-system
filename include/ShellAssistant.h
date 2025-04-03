#ifndef SHELL_ASSISTANT_H
#define SHELL_ASSISTANT_H

#include "AssistantInterface.h"
#include "VirtualFileSystem.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <regex>

class ShellAssistant : public AssistantInterface {
public:
    ShellAssistant(VirtualFileSystem* vfs);
    virtual ~ShellAssistant() = default;
    
    std::string processQuery(const std::string& query) override;
    bool canHandleQuery(const std::string& query) override;
    std::string getName() const override;
    std::string getHelpInfo() const override;
    
private:
    VirtualFileSystem* vfs;
    
    using QueryHandler = std::function<std::string(ShellAssistant*, const std::smatch&)>;
    
    struct QueryPattern {
        std::regex pattern;
        QueryHandler handler;
        std::string description;
    };
    
    std::vector<QueryPattern> queryPatterns;
    
    void initializePatterns();
    
    std::string formatSize(size_t sizeInBytes) const;
    std::string findLargestFile(const std::string& path) const;
    std::string listDirectoryContents(const std::string& path) const;
    std::string explainCommand(const std::string& command) const;
};

#endif // SHELL_ASSISTANT_H
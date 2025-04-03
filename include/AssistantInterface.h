#ifndef ASSISTANT_INTERFACE_H
#define ASSISTANT_INTERFACE_H

#include <string>

class AssistantInterface {
public:
    virtual ~AssistantInterface() = default;
    
    virtual std::string processQuery(const std::string& query) = 0;
    
    virtual bool canHandleQuery(const std::string& query) = 0;
    
    virtual std::string getName() const = 0;
    
    virtual std::string getHelpInfo() const = 0;
};

#endif // ASSISTANT_INTERFACE_H
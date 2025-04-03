#ifdef CLI_MODE_ONLY
// CLI-only mode - include only necessary headers
#include <iostream>
#include "../include/Shell.h"
#else
// GUI mode - include Qt headers
#include <QtWidgets/QApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include <iostream>
#include "../include/MainWindow.h"
#include "../include/Shell.h"
#endif

int main(int argc, char *argv[])
{
#ifndef CLI_MODE_ONLY
    // Check if we should start in CLI mode or GUI mode
    bool cliMode = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--cli" || arg == "-c") {
            cliMode = true;
            break;
        }
    }
    
    if (cliMode) {
        Shell shell;
        shell.run();
        return 0;
    } else {
        QApplication app(argc, argv);
        
        QCommandLineParser parser;
        parser.setApplicationDescription("Virtual File System");
        parser.addHelpOption();
        
        QCommandLineOption loadFileOption(QStringList() << "l" << "load", 
                                        "Load file system from <file>",
                                        "file");
        parser.addOption(loadFileOption);
        
        parser.process(app);
        
        MainWindow mainWindow;
        mainWindow.show();
        
        if (parser.isSet(loadFileOption)) {
            QString filePath = parser.value(loadFileOption);
            std::cout << "Loading file: " << filePath.toStdString() << std::endl;
        }
        
        return app.exec();
    }
#else
    // CLI-only mode - start the shell directly
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning
    Shell shell;
    shell.run();
    return 0;
#endif
}
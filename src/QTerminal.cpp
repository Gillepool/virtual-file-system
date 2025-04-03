#include "../include/QTerminal.h"
#include <QtGui/QKeyEvent>
#include <QtGui/QFontMetrics>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <QtCore/QDebug>

QTerminal::QTerminal(QWidget *parent) : QWidget(parent), shell(nullptr), historyPosition(0), oldStdout(-1)
{
    display = new QTextEdit(this);
    display->setReadOnly(true);
    display->setFontFamily("Courier New");
    display->setLineWrapMode(QTextEdit::NoWrap);
    
    commandLine = new QLineEdit(this);
    commandLine->setFont(display->font());
    commandLine->installEventFilter(this);
    
    connect(commandLine, &QLineEdit::returnPressed, this, &QTerminal::onCommandEntered);
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(display);
    layout->addWidget(commandLine);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    
    setLayout(layout);
    
    writeOutput("Virtual File System Terminal\nType 'help' for a list of commands.\n");
    updatePrompt();
    
    // Ensure the command line gets focus
    commandLine->setFocus();
    
    historyPosition = 0;
}

void QTerminal::setShell(Shell* shellInstance) 
{
    shell = shellInstance;
    
    if (shell) {
        currentDirectory = QString::fromStdString(shell->getVFS().getCurrentPath());
        updatePrompt();
    }
}

void QTerminal::clear() 
{
    display->clear();
}

void QTerminal::writeOutput(const QString& text) 
{
    display->append(text);
    
    QScrollBar *sb = display->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void QTerminal::onCommandEntered() 
{
    QString command = commandLine->text();
    if (command.isEmpty()) {
        updatePrompt();
        return;
    }
    
    display->append(currentPrompt + command);
    
    commandHistory.prepend(command);
    historyPosition = 0;
    
    commandLine->clear();
    
    executeCommand(command);
    
    // Update prompt (incase the directory changed)
    if (shell) {
        currentDirectory = QString::fromStdString(shell->getVFS().getCurrentPath());
    }
    updatePrompt();
    
    // Emit signal that a command was executed
    emit commandExecuted(command);
}

bool QTerminal::eventFilter(QObject* obj, QEvent* event) 
{
    if (obj == commandLine) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) {
                handleHistoryNavigation(keyEvent->key());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void QTerminal::handleHistoryNavigation(int key) 
{
    if (commandHistory.isEmpty()) {
        return;
    }
    
    if (key == Qt::Key_Up) {
        if (historyPosition < commandHistory.size()) {
            historyPosition++;
            commandLine->setText(commandHistory[historyPosition - 1]);
        }
    } else if (key == Qt::Key_Down) {
        if (historyPosition > 1) {
            historyPosition--;
            commandLine->setText(commandHistory[historyPosition - 1]);
        } else {
            historyPosition = 0;
            commandLine->clear();
        }
    }
}

void QTerminal::executeCommand(const QString& command) 
{
    if (!shell) {
        writeOutput("Error: Shell not initialized");
        return;
    }

    writeOutput("\n");
    
    std::string oldPath;
    if (shell) {
        oldPath = shell->getVFS().getCurrentPath();
    }
    
    std::string cmd = command.toStdString();
    std::vector<std::string> args = shell->parseCommand(cmd);
    
    if (args.empty()) {
        return; 
    }
    
    std::string cmdName = args[0];
    args.erase(args.begin());
    
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        writeOutput("Error: Failed to create pipe for command execution");
        return;
    }
    
    int oldStdoutFd = dup(STDOUT_FILENO);
    if (oldStdoutFd < 0) {
        writeOutput("Error: Failed to duplicate stdout");
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        return;
    }
    
    if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
        writeOutput("Error: Failed to redirect stdout");
        ::close(pipefd[0]);
        ::close(pipefd[1]);
        ::close(oldStdoutFd);
        return;
    }
    
    ::close(pipefd[1]);
    
    bool commandFound = false;
    if (shell->commands.find(cmdName) != shell->commands.end()) {
        commandFound = true;
        shell->commands[cmdName](shell, args);
    }
    
    fflush(stdout);
    
    dup2(oldStdoutFd, STDOUT_FILENO);
    ::close(oldStdoutFd);
    
    // If command not found, write directly since stdout redirection is over
    if (!commandFound) {
        std::cout << "Unknown command: " << cmdName << std::endl;
        std::cout << "Type 'help' for a list of commands" << std::endl;
        writeOutput("Unknown command: " + QString::fromStdString(cmdName) + "\nType 'help' for a list of commands");
    }
    
    // Read all data from the pipe
    char buffer[4096];
    ssize_t bytesRead;
    std::string output;
    
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }
    
    // Close read end of pipe
    ::close(pipefd[0]);
    
    // Display output
    if (!output.empty()) {
        writeOutput(QString::fromStdString(output));
    }
    
    // Check if the directory changed
    if (shell) {
        std::string newPath = shell->getVFS().getCurrentPath();
        if (newPath != oldPath) {
            emit currentDirectoryChanged(QString::fromStdString(newPath));
            // Update our internal state
            currentDirectory = QString::fromStdString(newPath);
            updatePrompt();
        }
    }
    
    emit commandExecuted(command);
}

void QTerminal::updatePrompt() 
{
    if (shell) {
        currentPrompt = QString::fromStdString(shell->getVFS().getCurrentPath()) + "> ";
    } else {
        currentPrompt = "> ";
    }
    
    commandLine->setPlaceholderText(currentPrompt);
}

void QTerminal::redirectStdout() 
{
    // Create pipe
    if (pipe(pipeFd) == -1) {
        qDebug() << "Failed to create pipe for stdout redirection";
        return;
    }
    
    // Save original stdout
    oldStdout = dup(fileno(stdout));
    
    // Redirect stdout to pipe write end
    dup2(pipeFd[1], fileno(stdout));
    
    // Close the write end as it's not needed in this context
    ::close(pipeFd[1]);
    
    // Flush stdout to ensure all buffered content is processed
    fflush(stdout);
}

void QTerminal::restoreStdout() 
{
    if (oldStdout != -1) {
        dup2(oldStdout, fileno(stdout));
        ::close(oldStdout);
        oldStdout = -1;
        
        ::close(pipeFd[0]);
        ::close(pipeFd[1]);
    }
}
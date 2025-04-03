#ifndef QTERMINAL_H
#define QTERMINAL_H

// Use QtWidgets namespace for Qt classes
#include <QtWidgets/QWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QEvent>
#include <QtWidgets/QScrollBar>
#include <QtCore/QStringList>
#include "Shell.h"

/**
 * @brief The QTerminal class provides a terminal-like interface for the VFS Shell
 * 
 * This widget integrates with the Shell class to provide a terminal interface
 * directly in the GUI application.
 */
class QTerminal : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Construct a new QTerminal widget
     * @param parent The parent widget
     */
    explicit QTerminal(QWidget *parent = nullptr);
    
    /**
     * @brief Set the Shell instance that this terminal will interact with
     * @param shell Pointer to the Shell instance
     */
    void setShell(Shell* shell);
    
    /**
     * @brief Clear the terminal display
     */
    void clear();
    
    /**
     * @brief Write output to the terminal display
     * @param text The text to write
     */
    void writeOutput(const QString& text);
    
    /**
     * @brief Filter events to handle key presses
     * @param obj The object the event was sent to
     * @param event The event
     * @return true if the event was handled
     */
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    /**
     * @brief Signal emitted when a command is executed in the terminal
     * @param command The command that was executed
     */
    void commandExecuted(const QString& command);
    
    /**
     * @brief Signal emitted when the current directory changes
     * @param path The new current directory path
     */
    void currentDirectoryChanged(const QString& path);

private slots:
    /**
     * @brief Handle command input from the command line
     */
    void onCommandEntered();
    
    /**
     * @brief Handle key press events in the input line
     * @param key The pressed key
     */
    void handleHistoryNavigation(int key);

private:
    /**
     * @brief Handle command execution via the shell
     * @param command The command to execute
     */
    void executeCommand(const QString& command);
    
    /**
     * @brief Update the command prompt to show the current directory
     */
    void updatePrompt();
    
    /**
     * @brief Redirect stdout to capture output from the shell
     */
    void redirectStdout();
    
    /**
     * @brief Restore stdout to its original state
     */
    void restoreStdout();

    QTextEdit* display;
    QLineEdit* commandLine;
    Shell* shell;
    
    QStringList commandHistory;
    int historyPosition;
    
    QString currentPrompt;
    QString currentDirectory;
    
    int oldStdout;
    int pipeFd[2];
};

#endif // QTERMINAL_H
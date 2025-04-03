#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QStandardItemModel>
#include <QtCore/QModelIndex>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <memory>

#include "VirtualFileSystem.h"
#include "Shell.h"
#include "QTerminal.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief The MainWindow class represents the main GUI window for the VFS application
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Constructs the main window
     * @param parent The parent widget
     */
    explicit MainWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor for MainWindow
     */
    ~MainWindow();

private slots:
    void on_actionNew_File_triggered();
    void on_actionNew_Directory_triggered();
    void on_actionDelete_triggered();
    void on_actionRename_triggered();
    void on_actionSave_triggered();
    void on_actionLoad_triggered();
    void on_actionExit_triggered();
    
    void on_actionCopy_triggered();
    void on_actionPaste_triggered();
    void on_actionCut_triggered();
    
    void on_actionCompress_triggered();
    void on_actionUncompress_triggered();
    
    // Encryption
    void on_actionEncrypt_triggered();
    void on_actionDecrypt_triggered();
    void on_actionChange_Key_triggered();
    
    // Versioning
    void on_actionSave_Version_triggered();
    void on_actionRestore_Version_triggered();
    void on_actionList_Versions_triggered();
    
    void on_actionCreate_Volume_triggered();
    void on_actionMount_triggered();
    void on_actionUnmount_triggered();
    
    void on_actionAbout_triggered();
    void on_actionAssistant_triggered();
    
    void on_fileSystemView_clicked(const QModelIndex &index);
    void on_fileSystemView_doubleClicked(const QModelIndex &index);
    
    void on_searchButton_clicked();
    void on_searchResults_doubleClicked(const QModelIndex &index);
    
    // Terminal
    void handleCommandExecuted(const QString &command);
    void handleCurrentDirectoryChanged(const QString &path);

private:
    /**
     * @brief Connect all signal-slot relationships for the UI
     */
    void connectSignals();
    
    /**
     * @brief Initialize the file system tree view
     */
    void initFileSystemView();
    
    /**
     * @brief Refresh the file system tree view
     * @param path Path to set as current, or empty to keep current
     */
    void refreshFileSystemView(const QString &path = QString());
    
    /**
     * @brief Get the currently selected file or directory path
     * @return The selected path or empty string if nothing is selected
     */
    QString getSelectedPath() const;
    
    /**
     * @brief Update file properties display
     * @param path Path to the file or directory to display properties for
     */
    void updateFileProperties(const QString &path);
    
    /**
     * @brief Display file content in the editor
     * @param path Path to the file to display
     */
    void displayFileContent(const QString &path);
    
    /**
     * @brief Populate the tree model with file system contents
     * @param parentItem The parent item in the tree model
     * @param path The file system path to populate from
     */
    void populateTreeView(QStandardItem *parentItem, const std::string &path);

    Ui::MainWindow *ui;               
    std::shared_ptr<VirtualFileSystem> vfs;
    std::unique_ptr<Shell> shell;      
    QTerminal *terminal;               
    QStandardItemModel *fileModel;    
    
    QString clipboardPath;             
    bool isCut;                        
};

#endif // MAINWINDOW_H
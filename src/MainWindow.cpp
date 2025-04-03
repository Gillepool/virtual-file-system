#include "../include/MainWindow.h"
#include "../src/generated/ui_mainwindow.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>
#include <QtCore/QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), isCut(false)
{
    ui->setupUi(this);
    
    vfs = std::make_shared<VirtualFileSystem>(100 * 1024 * 1024); // 100MB default size
    
    shell = std::make_unique<Shell>(vfs);
    
    terminal = new QTerminal(this);
    terminal->setShell(shell.get());
    ui->terminalWidget->setLayout(new QVBoxLayout());
    ui->terminalWidget->layout()->addWidget(terminal);
    ui->terminalWidget->layout()->setContentsMargins(0, 0, 0, 0);
    
    initFileSystemView();
    
    connectSignals();
    
    refreshFileSystemView();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectSignals()
{
    connect(terminal, &QTerminal::commandExecuted, this, &MainWindow::handleCommandExecuted);
    connect(terminal, &QTerminal::currentDirectoryChanged, this, &MainWindow::handleCurrentDirectoryChanged);
}

void MainWindow::initFileSystemView()
{
    fileModel = new QStandardItemModel(this);
    fileModel->setHorizontalHeaderLabels(QStringList() << "Name");
    
    ui->fileSystemView->setModel(fileModel);
    ui->fileSystemView->setAnimated(true);
    ui->fileSystemView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->fileSystemView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->fileSystemView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::refreshFileSystemView(const QString &path)
{
    fileModel->clear();
    fileModel->setHorizontalHeaderLabels(QStringList() << "Name");
    
    QStandardItem *rootItem = new QStandardItem("/");
    fileModel->appendRow(rootItem);
    
    // Recursively populate the tree
    populateTreeView(rootItem, "/");
    
    ui->fileSystemView->expand(rootItem->index());
    
    if (!path.isEmpty()) {
        // Find and select the item
        QStringList pathParts = path.split('/', Qt::SkipEmptyParts);
        QModelIndex currentIndex = rootItem->index();
        
        for (const QString &part : pathParts) {
            bool found = false;
            
            for (int i = 0; i < fileModel->rowCount(currentIndex); i++) {
                QModelIndex childIndex = fileModel->index(i, 0, currentIndex);
                if (fileModel->data(childIndex).toString() == part) {
                    ui->fileSystemView->expand(currentIndex);
                    currentIndex = childIndex;
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                break;
            }
        }
        
        ui->fileSystemView->setCurrentIndex(currentIndex);
        ui->fileSystemView->scrollTo(currentIndex);
        
        updateFileProperties(path);
        displayFileContent(path);
    }
}

void MainWindow::populateTreeView(QStandardItem *parentItem, const std::string &path)
{
    std::vector<std::string> entries = vfs->ls(path);
    
    for (const auto &entry : entries) {
        QString itemName = QString::fromStdString(entry);
        bool isDirectory = itemName.endsWith('/');
        
        if (isDirectory) {
            itemName = itemName.left(itemName.length() - 1); // Remove trailing slash
        }
        
        QStandardItem *item = new QStandardItem(itemName);
        
        if (isDirectory) {
            item->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
        } else {
            item->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
        }
        
        parentItem->appendRow(item);
        
        // If it's a directory, recursively populate it
        if (isDirectory) {
            std::string childPath = path;
            if (childPath.back() != '/') {
                childPath += '/';
            }
            childPath += itemName.toStdString();
            
            populateTreeView(item, childPath);
        }
    }
}

QString MainWindow::getSelectedPath() const
{
    QModelIndex current = ui->fileSystemView->currentIndex();
    if (!current.isValid()) {
        return QString();
    }
    
    // Build path by traversing up the tree
    QString path;
    QModelIndex index = current;
    
    while (index.isValid()) {
        QString name = fileModel->data(index).toString();
        
        if (path.isEmpty()) {
            path = name;
        } else {
            path = name + "/" + path;
        }
        
        index = index.parent();
    }
    
    return path.startsWith('/') ? path : "/" + path;
}

void MainWindow::updateFileProperties(const QString &path)
{
    if (path.isEmpty()) {
        ui->nameEdit->clear();
        ui->typeEdit->clear();
        ui->sizeEdit->clear();
        ui->compressedEdit->clear();
        ui->encryptedEdit->clear();
        ui->tagsEdit->clear();
        return;
    }
    
    std::string stdPath = path.toStdString();
    
    FileNode* node = vfs->resolvePath(stdPath);
    if (!node) {
        updateFileProperties(QString());
        return;
    }
    
    ui->nameEdit->setText(QString::fromStdString(node->getName()));
    ui->typeEdit->setText(node->isDirectory() ? "Directory" : "File");
    
    if (!node->isDirectory()) {
        ui->sizeEdit->setText(QString::number(node->getSize()) + " bytes");
        ui->compressedEdit->setText(node->isCompressed() ? 
                                   "Yes (" + QString::fromStdString(node->getCompressionAlgorithm()) + ")" : 
                                   "No");
        ui->encryptedEdit->setText(node->isEncrypted() ? 
                                  "Yes (" + QString::fromStdString(node->getEncryptionAlgorithm()) + ")" : 
                                  "No");
        
        std::vector<std::string> tags = vfs->getFileTags(stdPath);
        QString tagStr;
        for (const auto &tag : tags) {
            if (!tagStr.isEmpty()) {
                tagStr += ", ";
            }
            tagStr += QString::fromStdString(tag);
        }
        
        ui->tagsEdit->setText(tagStr);
    } else {
        ui->sizeEdit->setText("--");
        ui->compressedEdit->setText("--");
        ui->encryptedEdit->setText("--");
        ui->tagsEdit->setText("--");
    }
}

void MainWindow::displayFileContent(const QString &path)
{
    ui->fileContentEdit->clear();
    
    if (path.isEmpty()) {
        return;
    }
    
    std::string stdPath = path.toStdString();
    FileNode* node = vfs->resolvePath(stdPath);
    
    if (node && !node->isDirectory()) {
        ui->fileContentEdit->setPlainText(QString::fromStdString(node->getContent()));
    }
}

void MainWindow::on_fileSystemView_clicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    QString path = getSelectedPath();
    updateFileProperties(path);
    displayFileContent(path);
}

void MainWindow::on_fileSystemView_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    QString path = getSelectedPath();
    
    std::string stdPath = path.toStdString();
    FileNode* node = vfs->resolvePath(stdPath);
    
    if (node && node->isDirectory()) {
        vfs->cd(stdPath);
        shell->getVFS() = *vfs;
        
        terminal->writeOutput("cd " + path);
        emit terminal->currentDirectoryChanged(path);
    } else {
        displayFileContent(path);
        ui->tabWidget->setCurrentIndex(0);
    }
}

void MainWindow::on_actionNew_File_triggered()
{
    QString currentDir = QString::fromStdString(vfs->getCurrentPath());
    bool ok;
    QString fileName = QInputDialog::getText(this, tr("New File"),
                                            tr("Enter file name:"), QLineEdit::Normal,
                                            QString(), &ok);
    
    if (ok && !fileName.isEmpty()) {
        std::string path = currentDir.toStdString();
        if (path.back() != '/') {
            path += '/';
        }
        path += fileName.toStdString();
        
        if (vfs->touch(path)) {
            refreshFileSystemView(currentDir + "/" + fileName);
            terminal->writeOutput("Created file: " + fileName);
            return;
        }
        
        QMessageBox::warning(this, tr("Error"), tr("Failed to create file."));
    }
}

void MainWindow::on_actionNew_Directory_triggered()
{
    QString currentDir = QString::fromStdString(vfs->getCurrentPath());
    bool ok;
    QString dirName = QInputDialog::getText(this, tr("New Directory"),
                                           tr("Enter directory name:"), QLineEdit::Normal,
                                           QString(), &ok);
    
    if (ok && !dirName.isEmpty()) {
        std::string path = currentDir.toStdString();
        if (path.back() != '/') {
            path += '/';
        }
        path += dirName.toStdString();
        
        if (vfs->mkdir(path)) {
            refreshFileSystemView(currentDir + "/" + dirName);
            terminal->writeOutput("Created directory: " + dirName);
            return;
        }
        
        QMessageBox::warning(this, tr("Error"), tr("Failed to create directory."));
    }
}

void MainWindow::on_actionDelete_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    // Check if it's root
    if (path == "/") {
        QMessageBox::warning(this, tr("Error"), tr("Cannot delete the root directory."));
        return;
    }
    
    int result = QMessageBox::question(this, tr("Confirm Delete"),
                                      tr("Are you sure you want to delete '%1'?").arg(path),
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        if (vfs->remove(path.toStdString())) {
            terminal->writeOutput("Deleted: " + path);
            refreshFileSystemView();
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to delete item."));
        }
    }
}

void MainWindow::on_actionRename_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    // Check if it's root
    if (path == "/") {
        QMessageBox::warning(this, tr("Error"), tr("Cannot rename the root directory."));
        return;
    }
    
    QString name = path.split('/').last();
    QString parentPath = path.left(path.length() - name.length() - 1);
    if (parentPath.isEmpty()) {
        parentPath = "/";
    }
    
    bool ok;
    QString newName = QInputDialog::getText(this, tr("Rename"),
                                           tr("Enter new name:"), QLineEdit::Normal,
                                           name, &ok);
    
    if (ok && !newName.isEmpty() && newName != name) {
        FileNode* node = vfs->resolvePath(path.toStdString());
        if (!node) {
            QMessageBox::warning(this, tr("Error"), tr("Item not found."));
            return;
        }
        
        std::string newPath = parentPath.toStdString();
        if (newPath.back() != '/') {
            newPath += '/';
        }
        newPath += newName.toStdString();
        
        bool success = false;
        
        if (node->isDirectory()) {
            // Create new directory and recursively copy contents
            if (vfs->mkdir(newPath)) {
                // TODO: Copy directory contents
                success = true;
            }
        } else {
            std::string content = node->getContent();
            if (vfs->touch(newPath)) {
                success = vfs->write(newPath, content);
                
                if (node->isCompressed()) {
                    vfs->compressFile(newPath, true, node->getCompressionAlgorithm());
                }
                
                if (node->isEncrypted()) {
                    vfs->encryptFile(newPath, node->getEncryptionKey(), node->getEncryptionAlgorithm());
                }
            }
        }
        
        if (success) {
            if (vfs->remove(path.toStdString())) {
                terminal->writeOutput("Renamed: " + path + " to " + QString::fromStdString(newPath));
                refreshFileSystemView(QString::fromStdString(newPath));
                return;
            }
        }
        
        QMessageBox::warning(this, tr("Error"), tr("Failed to rename item."));
    }
}

void MainWindow::on_actionSave_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File System"),
                                                  QDir::homePath(),
                                                  tr("VFS Files (*.vfs)"));
    
    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".vfs")) {
            fileName += ".vfs";
        }
        
        if (vfs->saveToDisk(fileName.toStdString())) {
            statusBar()->showMessage(tr("File system saved successfully"), 3000);
            terminal->writeOutput("File system saved to: " + fileName);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to save file system."));
        }
    }
}

void MainWindow::on_actionLoad_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load File System"),
                                                  QDir::homePath(),
                                                  tr("VFS Files (*.vfs)"));
    
    if (!fileName.isEmpty()) {
        if (vfs->loadFromDisk(fileName.toStdString())) {
            shell->getVFS() = *vfs;
            refreshFileSystemView();
            statusBar()->showMessage(tr("File system loaded successfully"), 3000);
            terminal->writeOutput("File system loaded from: " + fileName);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to load file system."));
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionCopy_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    clipboardPath = path;
    isCut = false;
    statusBar()->showMessage(tr("Copied: %1").arg(path), 3000);
}

void MainWindow::on_actionCut_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    // Can't cut root
    if (path == "/") {
        QMessageBox::warning(this, tr("Error"), tr("Cannot cut the root directory."));
        return;
    }
    
    clipboardPath = path;
    isCut = true;
    statusBar()->showMessage(tr("Cut: %1").arg(path), 3000);
}

void MainWindow::on_actionPaste_triggered()
{
    if (clipboardPath.isEmpty()) {
        return;
    }
    
    QString destPath = QString::fromStdString(vfs->getCurrentPath());
    
    QString name = clipboardPath.split('/').last();
    
    QString fullDestPath = destPath;
    if (fullDestPath.back() != '/') {
        fullDestPath += '/';
    }
    fullDestPath += name;
    
    FileNode* destNode = vfs->resolvePath(fullDestPath.toStdString());
    if (destNode) {
        QMessageBox::warning(this, tr("Error"), tr("Destination already exists."));
        return;
    }
    
    FileNode* sourceNode = vfs->resolvePath(clipboardPath.toStdString());
    if (!sourceNode) {
        QMessageBox::warning(this, tr("Error"), tr("Source item not found."));
        return;
    }
    
    bool success = false;
    
    if (sourceNode->isDirectory()) {
        if (vfs->mkdir(fullDestPath.toStdString())) {
            // TODO: Copy directory contents
            success = true;
        }
    } else {
        // Copy file
        if (vfs->touch(fullDestPath.toStdString())) {
            success = vfs->write(fullDestPath.toStdString(), sourceNode->getContent());
            
            if (success) {
                if (sourceNode->isCompressed()) {
                    vfs->compressFile(fullDestPath.toStdString(), true, sourceNode->getCompressionAlgorithm());
                }
                
                if (sourceNode->isEncrypted()) {
                    vfs->encryptFile(fullDestPath.toStdString(), sourceNode->getEncryptionKey(),
                                    sourceNode->getEncryptionAlgorithm());
                }
            }
        }
    }
    
    if (success && isCut) {
        if (vfs->remove(clipboardPath.toStdString())) {
            clipboardPath.clear();
            terminal->writeOutput("Moved: " + name + " to " + destPath);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to remove source after move."));
        }
    } else if (success) {
        terminal->writeOutput("Copied: " + name + " to " + destPath);
    }
    
    if (success) {
        refreshFileSystemView(fullDestPath);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Operation failed."));
    }
}

void MainWindow::on_actionCompress_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory()) {
        QMessageBox::warning(this, tr("Error"), tr("Can only compress files."));
        return;
    }
    
    std::vector<std::string> algorithms = vfs->listCompressionAlgorithms();
    QStringList algoList;
    for (const auto& algo : algorithms) {
        algoList << QString::fromStdString(algo);
    }
    
    bool ok;
    QString algorithm = QInputDialog::getItem(this, tr("Compress File"),
                                            tr("Select compression algorithm:"),
                                            algoList, 0, false, &ok);
    
    if (ok && !algorithm.isEmpty()) {
        if (vfs->compressFile(path.toStdString(), true, algorithm.toStdString())) {
            terminal->writeOutput("Compressed: " + path + " using " + algorithm);
            updateFileProperties(path);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to compress file."));
        }
    }
}

void MainWindow::on_actionUncompress_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory() || !node->isCompressed()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected file is not compressed."));
        return;
    }
    
    if (vfs->compressFile(path.toStdString(), false)) {
        terminal->writeOutput("Uncompressed: " + path);
        updateFileProperties(path);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to uncompress file."));
    }
}

void MainWindow::on_actionEncrypt_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory()) {
        QMessageBox::warning(this, tr("Error"), tr("Can only encrypt files."));
        return;
    }
    
    bool ok;
    QString key = QInputDialog::getText(this, tr("Encrypt File"),
                                       tr("Enter encryption key:"),
                                       QLineEdit::Password, QString(), &ok);
    
    if (ok && !key.isEmpty()) {
        std::vector<std::string> algorithms = vfs->listEncryptionAlgorithms();
        QStringList algoList;
        for (const auto& algo : algorithms) {
            algoList << QString::fromStdString(algo);
        }
        
        QString algorithm = QInputDialog::getItem(this, tr("Encrypt File"),
                                                tr("Select encryption algorithm:"),
                                                algoList, 0, false, &ok);
        
        if (ok && !algorithm.isEmpty()) {
            if (vfs->encryptFile(path.toStdString(), key.toStdString(), algorithm.toStdString())) {
                terminal->writeOutput("Encrypted: " + path + " using " + algorithm);
                updateFileProperties(path);
            } else {
                QMessageBox::warning(this, tr("Error"), tr("Failed to encrypt file."));
            }
        }
    }
}

void MainWindow::on_actionDecrypt_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory() || !node->isEncrypted()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected file is not encrypted."));
        return;
    }
    
    if (vfs->decryptFile(path.toStdString())) {
        terminal->writeOutput("Decrypted: " + path);
        updateFileProperties(path);
        displayFileContent(path);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to decrypt file."));
    }
}

void MainWindow::on_actionChange_Key_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory() || !node->isEncrypted()) {
        QMessageBox::warning(this, tr("Error"), tr("Selected file is not encrypted."));
        return;
    }
    
    bool ok;
    QString key = QInputDialog::getText(this, tr("Change Encryption Key"),
                                       tr("Enter new encryption key:"),
                                       QLineEdit::Password, QString(), &ok);
    
    if (ok && !key.isEmpty()) {
        if (vfs->changeEncryptionKey(path.toStdString(), key.toStdString())) {
            terminal->writeOutput("Changed encryption key for: " + path);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to change encryption key."));
        }
    }
}

void MainWindow::on_actionSave_Version_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory()) {
        QMessageBox::warning(this, tr("Error"), tr("Can only version files."));
        return;
    }
    
    if (vfs->saveFileVersion(path.toStdString())) {
        terminal->writeOutput("Saved version for: " + path);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save version."));
    }
}

void MainWindow::on_actionRestore_Version_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory()) {
        QMessageBox::warning(this, tr("Error"), tr("Can only restore versioned files."));
        return;
    }
    
    size_t versionCount = vfs->getFileVersionCount(path.toStdString());
    if (versionCount == 0) {
        QMessageBox::warning(this, tr("Error"), tr("No versions available for this file."));
        return;
    }
    
    std::vector<std::time_t> timestamps = vfs->getFileVersionTimestamps(path.toStdString());
    QStringList versionList;
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        QDateTime datetime = QDateTime::fromSecsSinceEpoch(timestamps[i]);
        versionList << tr("Version %1: %2").arg(i).arg(datetime.toString(Qt::TextDate));
    }
    
    bool ok;
    QString selectedVersion = QInputDialog::getItem(this, tr("Restore Version"),
                                                  tr("Select version to restore:"),
                                                  versionList, 0, false, &ok);
    
    if (ok && !selectedVersion.isEmpty()) {
        int versionIndex = selectedVersion.mid(8, selectedVersion.indexOf(':') - 8).toInt();
        
        if (vfs->restoreFileVersion(path.toStdString(), versionIndex)) {
            terminal->writeOutput("Restored version " + QString::number(versionIndex) + " for: " + path);
            displayFileContent(path); // Refresh content view
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to restore version."));
        }
    }
}

void MainWindow::on_actionList_Versions_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    FileNode* node = vfs->resolvePath(path.toStdString());
    if (!node || node->isDirectory()) {
        QMessageBox::warning(this, tr("Error"), tr("Only files can have versions."));
        return;
    }
    
    size_t versionCount = vfs->getFileVersionCount(path.toStdString());
    if (versionCount == 0) {
        QMessageBox::information(this, tr("Versions"), tr("No versions available for this file."));
        return;
    }
    
    std::vector<std::time_t> timestamps = vfs->getFileVersionTimestamps(path.toStdString());
    QString versionInfo = tr("Versions for %1:\n\n").arg(path);
    
    for (size_t i = 0; i < timestamps.size(); ++i) {
        QDateTime datetime = QDateTime::fromSecsSinceEpoch(timestamps[i]);
        versionInfo += tr("Version %1: %2\n").arg(i).arg(datetime.toString(Qt::TextDate));
    }
    
    QMessageBox::information(this, tr("Versions"), versionInfo);
}

void MainWindow::on_actionCreate_Volume_triggered()
{
    bool ok;
    QString volumeName = QInputDialog::getText(this, tr("Create Volume"),
                                             tr("Enter volume name:"), QLineEdit::Normal,
                                             "new_volume.vfs", &ok);
    
    if (!ok || volumeName.isEmpty()) {
        return;
    }
    
    QString sizeStr = QInputDialog::getText(this, tr("Create Volume"),
                                          tr("Enter volume size in MB:"), QLineEdit::Normal,
                                          "10", &ok);
    
    if (!ok || sizeStr.isEmpty()) {
        return;
    }
    
    bool convOk;
    size_t sizeMB = sizeStr.toULong(&convOk);
    
    if (!convOk || sizeMB == 0) {
        QMessageBox::warning(this, tr("Error"), tr("Invalid volume size."));
        return;
    }
    
    if (vfs->createVolume(volumeName.toStdString(), sizeMB)) {
        terminal->writeOutput("Created volume: " + volumeName + " (" + sizeStr + " MB)");
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to create volume."));
    }
}

void MainWindow::on_actionMount_triggered()
{
    QString diskImage = QFileDialog::getOpenFileName(this, tr("Select Volume File"),
                                                   QDir::homePath(),
                                                   tr("VFS Files (*.vfs)"));
    
    if (diskImage.isEmpty()) {
        return;
    }
    
    bool ok;
    QString mountPoint = QInputDialog::getText(this, tr("Mount Volume"),
                                             tr("Enter mount point (relative to current directory):"), QLineEdit::Normal,
                                             "mnt", &ok);
    
    if (!ok || mountPoint.isEmpty()) {
        return;
    }
    
    std::string currentPath = vfs->getCurrentPath();
    std::string fullMountPath = currentPath;
    if (fullMountPath.back() != '/') {
        fullMountPath += '/';
    }
    fullMountPath += mountPoint.toStdString();
    
    if (vfs->mountVolume(diskImage.toStdString(), fullMountPath)) {
        terminal->writeOutput("Mounted " + diskImage + " at " + QString::fromStdString(fullMountPath));
        refreshFileSystemView();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to mount volume."));
    }
}

void MainWindow::on_actionUnmount_triggered()
{
    QString path = getSelectedPath();
    if (path.isEmpty()) {
        return;
    }
    
    if (!vfs->isMountPoint(path.toStdString())) {
        QMessageBox::warning(this, tr("Error"), tr("Selected path is not a mount point."));
        return;
    }
    
    if (vfs->unmountVolume(path.toStdString())) {
        terminal->writeOutput("Unmounted: " + path);
        refreshFileSystemView();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to unmount volume."));
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About VFS"),
                      tr("Virtual File System (VFS)\n\n"
                         "A feature-rich virtual file system with GUI interface.\n"
                         "Supports compression, encryption, versioning, and more."));
}

void MainWindow::on_actionAssistant_triggered()
{
    bool ok;
    QString query = QInputDialog::getText(this, tr("Assistant"),
                                        tr("What would you like to know?"), QLineEdit::Normal,
                                        QString(), &ok);
    
    if (ok && !query.isEmpty()) {
        std::string response = shell->getAssistant()->processQuery(query.toStdString());
        
        QMessageBox::information(this, tr("Assistant Response"), QString::fromStdString(response));
    }
}

void MainWindow::on_searchButton_clicked()
{
    QString searchQuery = ui->searchEdit->text();
    if (searchQuery.isEmpty()) {
        return;
    }
    
    int searchType = ui->searchTypeCombo->currentIndex();
    bool useRegex = ui->regexCheck->isChecked();
    
    std::string startPath = vfs->getCurrentPath();
    std::vector<std::string> results;
    
    if (searchType == 0) { // By Name
        results = vfs->searchByName(searchQuery.toStdString(), useRegex, startPath);
    }
    else if (searchType == 1) { // By Content
        results = vfs->searchByContent(searchQuery.toStdString(), useRegex, startPath);
    }
    else if (searchType == 2) { // By Size
        // Parse size range
        QStringList parts = searchQuery.split(':');
        if (parts.size() != 2) {
            QMessageBox::warning(this, tr("Error"), tr("Size search requires format 'min:max' (in bytes)."));
            return;
        }
        
        size_t minSize = parts[0].toULong();
        size_t maxSize = parts[1].toULong();
        
        results = vfs->searchBySize(minSize, maxSize, startPath);
    }
    else if (searchType == 3) { // By Tag
        results = vfs->searchByTag(searchQuery.toStdString(), startPath);
    }
    else if (searchType == 4) { // By Date
        // Parse date range
        QStringList parts = searchQuery.split(':');
        if (parts.size() != 2) {
            QMessageBox::warning(this, tr("Error"), tr("Date search requires format 'YYYY-MM-DD:YYYY-MM-DD' or 'all:YYYY-MM-DD' or 'YYYY-MM-DD:all'."));
            return;
        }
        
        time_t afterDate = 0;
        time_t beforeDate = std::numeric_limits<time_t>::max();
        
        if (parts[0] != "all") {
            QDateTime after = QDateTime::fromString(parts[0], "yyyy-MM-dd");
            if (!after.isValid()) {
                QMessageBox::warning(this, tr("Error"), tr("Invalid 'after' date format. Use YYYY-MM-DD."));
                return;
            }
            afterDate = after.toSecsSinceEpoch();
        }
        
        if (parts[1] != "all") {
            QDateTime before = QDateTime::fromString(parts[1], "yyyy-MM-dd");
            if (!before.isValid()) {
                QMessageBox::warning(this, tr("Error"), tr("Invalid 'before' date format. Use YYYY-MM-DD."));
                return;
            }
            beforeDate = before.toSecsSinceEpoch();
        }
        
        results = vfs->searchByDate(afterDate, beforeDate, startPath);
    }
    
    ui->searchResults->clear();
    
    if (results.empty()) {
        ui->searchResults->addItem("No results found.");
    } else {
        for (const auto &result : results) {
            ui->searchResults->addItem(QString::fromStdString(result));
        }
    }
}

void MainWindow::on_searchResults_doubleClicked(const QModelIndex &index)
{
    QString item = ui->searchResults->item(index.row())->text();
    
    if (item.startsWith("No results")) {
        return;
    }
    
    refreshFileSystemView(item);
}

void MainWindow::handleCommandExecuted(const QString &command)
{
    Q_UNUSED(command);
    refreshFileSystemView();
}

void MainWindow::handleCurrentDirectoryChanged(const QString &path)
{
    Q_UNUSED(path);
    refreshFileSystemView();
}
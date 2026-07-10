#include "filecontentdialog.h"
#include "../cvfs_wrapper.h"
#include "../persistence.h"
#include "../directory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFont>
#include <QApplication>

FileContentDialog::FileContentDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent), m_fileName(fileName)
{
    setWindowTitle("File: " + fileName);
    setMinimumSize(600, 450);
    resize(700, 500);
    setupUI();
    applyStyles();
    loadContent();
}

void FileContentDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(16, 16, 16, 16);

    m_infoLabel = new QLabel(QString("Editing: <b>%1</b>").arg(m_fileName));
    m_infoLabel->setStyleSheet("color: #a0a0c0; font-size: 14px; padding: 4px 0;");
    layout->addWidget(m_infoLabel);

    m_editor = new QPlainTextEdit;
    m_editor->setTabStopDistance(20);
    m_editor->setPlaceholderText("File content appears here...");
    m_editor->setStyleSheet(R"(
        QPlainTextEdit {
            background: rgba(22, 22, 40, 220);
            color: #d0d0e0;
            border: 1px solid #3a3a5a;
            border-radius: 6px;
            padding: 10px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 14px;
            selection-background-color: #4a6fa5;
        }
        QPlainTextEdit:focus {
            border-color: #4a9eff;
        }
    )");
    layout->addWidget(m_editor, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    m_saveBtn = new QPushButton("Save");
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setMinimumHeight(38);
    m_saveBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a);
            color: white; border: none; border-radius: 6px;
            padding: 8px 24px; font-size: 14px; font-weight: bold;
        }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 #5a7fb5, stop:1 #4a6a9a); }
        QPushButton:pressed { background: #3a5a8a; }
    )");
    connect(m_saveBtn, &QPushButton::clicked, this, &FileContentDialog::onSave);

    m_truncateBtn = new QPushButton("Truncate");
    m_truncateBtn->setCursor(Qt::PointingHandCursor);
    m_truncateBtn->setMinimumHeight(38);
    m_truncateBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(200, 80, 80, 60);
            color: #f0a0a0;
            border: 1px solid rgba(200, 80, 80, 100);
            border-radius: 6px; padding: 8px 16px; font-size: 13px;
        }
        QPushButton:hover { background: rgba(200, 80, 80, 100); color: white; }
    )");
    connect(m_truncateBtn, &QPushButton::clicked, this, &FileContentDialog::onTruncate);

    m_refreshBtn = new QPushButton("Reload");
    m_refreshBtn->setCursor(Qt::PointingHandCursor);
    m_refreshBtn->setMinimumHeight(38);
    m_refreshBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(74, 158, 255, 40);
            color: #a0c0e0;
            border: 1px solid rgba(74, 158, 255, 60);
            border-radius: 6px; padding: 8px 16px; font-size: 13px;
        }
        QPushButton:hover { background: rgba(74, 158, 255, 80); color: white; }
    )");
    connect(m_refreshBtn, &QPushButton::clicked, this, &FileContentDialog::onRefresh);

    QPushButton *cancelBtn = new QPushButton("Close");
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setMinimumHeight(38);
    cancelBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255,255,255,10);
            color: #a0a0c0;
            border: 1px solid #3a3a5a;
            border-radius: 6px; padding: 8px 16px; font-size: 13px;
        }
        QPushButton:hover { background: rgba(255,255,255,20); color: white; }
    )");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_truncateBtn);
    btnLayout->addWidget(m_refreshBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);
}

void FileContentDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a2e, stop:0.5 #16213e, stop:1 #0f3460);
        }
    )");
}

void FileContentDialog::loadContent()
{
    QByteArray nameBytes = m_fileName.toUtf8();
    PINODE node = FindFile(nameBytes.data());
    if (!node || node->FileType != REGULAR) {
        m_editor->setPlainText("Error: File not found or not a regular file.");
        m_editor->setReadOnly(true);
        m_saveBtn->setEnabled(false);
        return;
    }

    // Use raw functions for reading (no visualization needed for loading)
    int fd = OpenFile(nameBytes.data(), READ);
    if (fd < 0) {
        m_editor->setPlainText("Error: Could not open file for reading.");
        m_editor->setReadOnly(true);
        return;
    }

    char buffer[2049] = {0};
    int bytesRead = ReadFile(fd, buffer, node->FileActualSize);
    CloseFileByFD(fd);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        m_editor->setPlainText(QString::fromUtf8(buffer, bytesRead));
    } else {
        m_editor->clear();
    }

    m_infoLabel->setText(QString("Editing: <b>%1</b>  |  Inode: %2  |  Size: %3 bytes")
        .arg(m_fileName)
        .arg(node->InodeNumber)
        .arg(node->FileActualSize));
}

void FileContentDialog::onSave()
{
    QByteArray nameBytes = m_fileName.toUtf8();
    QString text = m_editor->toPlainText();
    QByteArray data = text.toUtf8();
    int dataLen = qMin(data.size(), 2048);

    PINODE node = FindFile(nameBytes.data());
    if (!node) return;

    // Use raw OpenFile to get fd (no visualization)
    int fd = OpenFile(nameBytes.data(), WRITE);
    if (fd < 0) {
        QMessageBox::warning(this, "Error", "Could not open file for writing.");
        return;
    }

    if (node->Buffer) {
        memset(node->Buffer, 0, MAXFILESIZE);
    }
    node->FileActualSize = 0;
    UFDTArr[fd].ptrfiletable->writeoffset = 0;

    if (dataLen > 0) {
        // Use W_WriteFile to trigger write visualization via step hooks
        int written = W_WriteFile(fd, data.data(), dataLen);
        if (written > 0) {
            sync_inode(node->InodeNumber);
            sync_superblock();
        }
    } else {
        sync_inode(node->InodeNumber);
        sync_superblock();
    }

    CloseFileByFD(fd);

    if (dataLen > 0) {
        emit fileSaved(m_fileName, dataLen);
    }

    m_infoLabel->setText(QString("Editing: <b>%1</b>  |  Size: %2 bytes (saved)")
        .arg(m_fileName).arg(dataLen));

    QMessageBox::information(this, "Saved", QString("File saved (%1 bytes).").arg(dataLen));
}

void FileContentDialog::onTruncate()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Truncate",
        QString("Clear all content of '%1'?").arg(m_fileName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    QByteArray nameBytes = m_fileName.toUtf8();

    PINODE node = FindFile(nameBytes.data());
    if (!node) return;

    int fd = OpenFile(nameBytes.data(), WRITE);
    if (fd < 0) {
        QMessageBox::warning(this, "Error", "Could not open file.");
        return;
    }

    if (node->Buffer) {
        memset(node->Buffer, 0, MAXFILESIZE);
    }
    node->FileActualSize = 0;
    UFDTArr[fd].ptrfiletable->readoffset = 0;
    UFDTArr[fd].ptrfiletable->writeoffset = 0;

    sync_inode(node->InodeNumber);
    sync_superblock();
    CloseFileByFD(fd);

    m_editor->clear();
    m_infoLabel->setText(QString("Editing: <b>%1</b>  |  Size: 0 bytes (truncated)")
        .arg(m_fileName));
}

void FileContentDialog::onRefresh()
{
    loadContent();
}

QString FileContentDialog::content() const
{
    return m_editor->toPlainText();
}

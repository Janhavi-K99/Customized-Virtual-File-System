#include "filepropertiesdialog.h"
#include "../CVFS.h"
#include "../directory.h"
#include "../persistence.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QApplication>

FilePropertiesDialog::FilePropertiesDialog(const QString &fileName, QWidget *parent)
    : QDialog(parent), m_fileName(fileName)
{
    setWindowTitle("Properties: " + fileName);
    setMinimumSize(420, 380);
    resize(450, 420);
    applyStyles();
    setupUI();
}

void FilePropertiesDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a2e, stop:0.5 #16213e, stop:1 #0f3460);
        }
    )");
}

QWidget* FilePropertiesDialog::makePropRow(const QString &label, const QString &value, const QString &valueColor)
{
    QWidget *row = new QWidget;
    row->setStyleSheet("background: transparent;");
    QHBoxLayout *hl = new QHBoxLayout(row);
    hl->setContentsMargins(8, 4, 8, 4);

    QLabel *lbl = new QLabel(label);
    lbl->setFixedWidth(140);
    lbl->setStyleSheet("color: #8888aa; font-size: 12px; font-weight: bold; background: transparent;");

    QLabel *val = new QLabel(value);
    val->setStyleSheet(QString("color: %1; font-size: 12px; background: transparent;").arg(valueColor));

    hl->addWidget(lbl);
    hl->addWidget(val, 1);
    return row;
}

void FilePropertiesDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(20, 20, 20, 20);

    QLabel *title = new QLabel(QString("📄 <b>%1</b>").arg(m_fileName));
    title->setStyleSheet("color: #e0e0f0; font-size: 16px; padding: 8px 0; background: transparent;");
    layout->addWidget(title);

    QFrame *sep = new QFrame;
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background: rgba(74, 158, 255, 40); max-height: 1px;");
    layout->addWidget(sep);
    layout->addSpacing(12);

    QByteArray nameBytes = m_fileName.toUtf8();
    PINODE node = FindFile(nameBytes.data());

    if (!node) {
        layout->addWidget(makePropRow("Status:", "File not found", "#f87171"));
    } else {
        QString typeStr = (node->FileType == DIRECTORY) ? "Directory" :
                          (node->FileType == REGULAR) ? "Regular File" : "Special";
        QString typeColor = (node->FileType == DIRECTORY) ? "#60d060" : "#fbbf24";

        layout->addWidget(makePropRow("Inode:", QString::number(node->InodeNumber), "#4a9eff"));
        layout->addWidget(makePropRow("Type:", typeStr, typeColor));
        layout->addWidget(makePropRow("Size:", QString::number(node->FileActualSize) + " bytes (max " + QString::number(node->FileSize) + ")"));
        layout->addWidget(makePropRow("Link Count:", QString::number(node->LinkCount)));
        layout->addWidget(makePropRow("Reference Count:", QString::number(node->ReferenceCount)));

        QString permStr;
        permStr += (node->permission & READ) ? "Read " : "";
        permStr += (node->permission & WRITE) ? "Write" : "";
        if (permStr.isEmpty()) permStr = "None";
        layout->addWidget(makePropRow("Permissions:", permStr));

        QString permBits;
        permBits += (node->permission & READ) ? "r" : "-";
        permBits += (node->permission & WRITE) ? "w" : "-";
        layout->addWidget(makePropRow("Permission Bits:", permBits));

        int blkUsed = 0;
        if (node->Buffer && node->FileActualSize > 0) {
            blkUsed = (node->FileActualSize + 1023) / 1024;
            if (blkUsed > 4) blkUsed = 4;
        }
        layout->addWidget(makePropRow("Data Blocks Used:", QString::number(blkUsed) + " / 4"));

        QString fsState;
        if (node->FileType != 0) {
            int freeInodes = SUPERBLOCKobj.FreeInode;
            int usedInodes = SUPERBLOCKobj.TotalInodes - freeInodes;
            int usedBlocks = 502 - get_free_blocks_count();
            fsState = QString("Inodes: %1/50 used | Blocks: %2/502 used")
                .arg(usedInodes).arg(usedBlocks);
        } else {
            fsState = "Inode free";
        }
        layout->addWidget(makePropRow("FS State:", fsState, "#a0a0c0"));
    }

    layout->addStretch();

    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setMinimumHeight(36);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a);
            color: white;
            border: none;
            border-radius: 6px;
            padding: 8px 24px;
            font-size: 13px;
            font-weight: bold;
        }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 #5a7fb5, stop:1 #4a6a9a); }
    )");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);
}

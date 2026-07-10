#include "filemanagerpanel.h"
#include "filesystemmodel.h"
#include "../directory.h"
#include <QHBoxLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QApplication>
#include <QHeaderView>

FileManagerPanel::FileManagerPanel(FileSystemModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setObjectName("fileManagerPanel");
    setStyleSheet(R"(
        QWidget#fileManagerPanel {
            background: rgba(26, 26, 46, 200);
            border: 1px solid rgba(74, 158, 255, 30);
            border-radius: 6px;
        }
    )");
    setupUI();

    m_contextMenu = new QMenu(this);
    m_contextMenu->setStyleSheet(R"(
        QMenu {
            background: #252540; color: #c0c0d0;
            border: 1px solid #3a3a5a; border-radius: 6px; padding: 4px;
        }
        QMenu::item {
            padding: 6px 24px; border-radius: 4px; margin: 2px;
        }
        QMenu::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a); color: white;
        }
        QMenu::separator { height: 1px; background: #3a3a5a; margin: 4px 8px; }
    )");
    m_contextMenu->addAction("New File...", this, &FileManagerPanel::onNewFile);
    m_contextMenu->addAction("New Folder...", this, &FileManagerPanel::onNewFolder);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction("Open", this, &FileManagerPanel::onOpen);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction("Delete", this, &FileManagerPanel::onDelete);
    m_contextMenu->addAction("Rename", this, &FileManagerPanel::onRename);
    m_contextMenu->addAction("Truncate", this, &FileManagerPanel::onTruncate);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction("Properties", this, &FileManagerPanel::onProperties);
    m_contextMenu->addSeparator();
    m_contextMenu->addAction("Refresh", this, &FileManagerPanel::onRefresh);

    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QWidget::customContextMenuRequested,
            this, &FileManagerPanel::onCustomContextMenu);
}

QPushButton* FileManagerPanel::createToolButton(const QString &text, const QString &tip, const char *slot)
{
    QPushButton *btn = new QPushButton(text);
    btn->setToolTip(tip);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setFixedHeight(28);
    btn->setStyleSheet(R"(
        QPushButton {
            color: #c0c0d0; background: rgba(255,255,255,6);
            border: 1px solid rgba(74,158,255,25);
            border-radius: 4px; padding: 2px 10px; font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover { background: rgba(74,158,255,30); color: white; border-color: rgba(74,158,255,60); }
        QPushButton:pressed { background: rgba(74,158,255,60); }
    )");
    connect(btn, SIGNAL(clicked()), this, slot);
    return btn;
}

QLabel* FileManagerPanel::createToolIcon(const QString &symbol, const QString &color)
{
    QLabel *lbl = new QLabel(symbol);
    lbl->setFixedWidth(24);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setStyleSheet(QString("color: %1; font-size: 14px; background: transparent;").arg(color));
    return lbl;
}

void FileManagerPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // ── Header ──
    QWidget *headerBar = new QWidget;
    headerBar->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d2d44, stop:1 #1e1e32); border-bottom: 1px solid #3a3a5a;");
    QHBoxLayout *headerL = new QHBoxLayout(headerBar);
    headerL->setContentsMargins(10, 5, 10, 5);
    m_headerLabel = new QLabel("CVFS File Manager");
    m_headerLabel->setStyleSheet("color: #4a9eff; font-size: 12px; font-weight: bold; background: transparent;");
    headerL->addWidget(m_headerLabel);
    headerL->addStretch();
    mainLayout->addWidget(headerBar);

    // ── Toolbar ──
    m_toolbar = new QWidget;
    m_toolbar->setStyleSheet("background: rgba(26,26,46,180); border-bottom: 1px solid #2a2a4a;");
    QHBoxLayout *tbLayout = new QHBoxLayout(m_toolbar);
    tbLayout->setContentsMargins(6, 3, 6, 3);
    tbLayout->setSpacing(2);

    tbLayout->addWidget(createToolIcon("◀", "#6a8fbf"));
    tbLayout->addWidget(createToolButton("", "Go back", SLOT(goUp())));
    tbLayout->addWidget(createToolIcon("▶", "#6a8fbf"));
    tbLayout->addWidget(createToolButton("", "Go forward", SLOT(goHome())));
    tbLayout->addWidget(createToolIcon("⌂", "#6a8fbf"));
    tbLayout->addWidget(createToolButton("", "Home /", SLOT(goHome())));

    QFrame *sep1 = new QFrame;
    sep1->setFrameShape(QFrame::VLine);
    sep1->setStyleSheet("color: #3a3a5a; max-width: 1px;");
    tbLayout->addWidget(sep1);

    tbLayout->addWidget(createToolButton("+ File", "Create new file (Ctrl+N)", SLOT(onNewFile())));
    tbLayout->addWidget(createToolButton("+ Folder", "Create new folder (Ctrl+Shift+N)", SLOT(onNewFolder())));

    QFrame *sep2 = new QFrame;
    sep2->setFrameShape(QFrame::VLine);
    sep2->setStyleSheet("color: #3a3a5a; max-width: 1px;");
    tbLayout->addWidget(sep2);

    tbLayout->addWidget(createToolButton("Open", "Open selected file (Enter)", SLOT(onOpen())));
    tbLayout->addWidget(createToolButton("Delete", "Move to trash (Delete)", SLOT(onDelete())));
    tbLayout->addWidget(createToolButton("Rename", "Rename (F2)", SLOT(onRename())));

    QFrame *sep3 = new QFrame;
    sep3->setFrameShape(QFrame::VLine);
    sep3->setStyleSheet("color: #3a3a5a; max-width: 1px;");
    tbLayout->addWidget(sep3);

    tbLayout->addWidget(createToolButton("↻", "Refresh (F5)", SLOT(onRefresh())));
    tbLayout->addWidget(createToolButton("ℹ", "Properties (Alt+Enter)", SLOT(onProperties())));

    tbLayout->addStretch();
    mainLayout->addWidget(m_toolbar);

    // ── Path / Breadcrumb bar ──
    QWidget *pathBar = new QWidget;
    pathBar->setStyleSheet("background: rgba(20,20,40,160); border-bottom: 1px solid #2a2a4a;");
    QHBoxLayout *pathL = new QHBoxLayout(pathBar);
    pathL->setContentsMargins(8, 2, 8, 2);

    QLabel *locIcon = new QLabel("📍");
    locIcon->setStyleSheet("font-size: 11px; background: transparent;");
    pathL->addWidget(locIcon);

    m_pathLabel = new QLabel("/");
    m_pathLabel->setStyleSheet("color: #88aacc; font-size: 12px; font-weight: bold; background: transparent; padding: 2px 4px;");
    pathL->addWidget(m_pathLabel, 1);
    mainLayout->addWidget(pathBar);

    // ── Tree view ──
    m_treeView = new QTreeView;
    m_treeView->setModel(m_model);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setItemsExpandable(false);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setEditTriggers(QAbstractItemView::DoubleClicked);
    m_treeView->setAnimated(false);
    m_treeView->setIndentation(0);
    m_treeView->setAllColumnsShowFocus(true);
    m_treeView->setMouseTracking(true);
    m_treeView->setSortingEnabled(false);
    m_treeView->setAlternatingRowColors(true);

    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    m_treeView->setStyleSheet(R"(
        QTreeView {
            background: rgba(22,22,40,200);
            color: #c0c0d0;
            border: none;
            font-size: 12px;
            outline: none;
            alternate-background-color: rgba(255,255,255,4);
        }
        QTreeView::item {
            border-radius: 2px;
            padding: 4px 6px;
            margin: 0px 2px;
            border-bottom: 1px solid rgba(58,58,90,30);
        }
        QTreeView::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3d6fa5, stop:1 #2d5a8a);
            color: white;
        }
        QTreeView::item:hover {
            background: rgba(74,158,255,25);
            border: 1px solid rgba(74,158,255,40);
        }
        QTreeView::item:selected:hover {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4d7fb5, stop:1 #3d6a9a);
        }
        QHeaderView::section {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2d2d44, stop:1 #1e1e32);
            color: #8899bb;
            font-size: 11px;
            font-weight: bold;
            padding: 4px 8px;
            border: none;
            border-right: 1px solid #3a3a5a;
            border-bottom: 1px solid #3a3a5a;
        }
        QHeaderView::section:hover {
            color: #aabbdd;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #3d3d54, stop:1 #2e2e42);
        }
    )");

    connect(m_treeView, &QTreeView::doubleClicked, this, &FileManagerPanel::onItemDoubleClicked);
    mainLayout->addWidget(m_treeView, 1);

    // ── Status bar ──
    QWidget *statusBar = new QWidget;
    statusBar->setStyleSheet("background: rgba(20,20,40,160); border-top: 1px solid #2a2a4a;");
    QHBoxLayout *statusL = new QHBoxLayout(statusBar);
    statusL->setContentsMargins(10, 2, 10, 2);

    m_statusLabel = new QLabel("0 items");
    m_statusLabel->setStyleSheet("color: #8888aa; font-size: 11px; background: transparent;");
    statusL->addWidget(m_statusLabel);
    statusL->addStretch();
    mainLayout->addWidget(statusBar);
}

void FileManagerPanel::refresh()
{
    m_pathLabel->setText(dir_get_cwd_path());
    int count = m_model->rowCount();
    m_statusLabel->setText(QString("%1 item%2")
        .arg(count)
        .arg(count == 1 ? "" : "s"));
}

void FileManagerPanel::updatePathLabel()
{
    m_pathLabel->setText(dir_get_cwd_path());
    int count = m_model->rowCount();
    m_statusLabel->setText(QString("%1 item%2")
        .arg(count)
        .arg(count == 1 ? "" : "s"));
}

void FileManagerPanel::goUp()
{
    m_model->cdUp();
    refresh();
}

void FileManagerPanel::goHome()
{
    m_model->cd(ROOT_INODE);
    refresh();
}

void FileManagerPanel::onItemDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int inode = m_model->inodeFromIndex(index);
    PINODE node = head;
    while (node) {
        if (node->InodeNumber == inode) break;
        node = node->next;
    }
    if (node && node->FileType == DIRECTORY) {
        m_model->cd(inode);
        refresh();
    } else if (node && node->FileType == REGULAR) {
        m_model->openFile(index);
    }
}

void FileManagerPanel::onCustomContextMenu(const QPoint &pos)
{
    QModelIndex index = m_treeView->indexAt(pos);
    if (index.isValid()) {
        m_treeView->setCurrentIndex(index);
        m_contextMenu->exec(m_treeView->viewport()->mapToGlobal(pos));
    } else {
        QMenu menu(this);
        menu.setStyleSheet(m_contextMenu->styleSheet());
        menu.addAction("+ New File", this, &FileManagerPanel::onNewFile);
        menu.addAction("+ New Folder", this, &FileManagerPanel::onNewFolder);
        menu.addSeparator();
        menu.addAction("Refresh", this, &FileManagerPanel::onRefresh);
        menu.exec(m_treeView->viewport()->mapToGlobal(pos));
    }
}

void FileManagerPanel::onNewFile()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New File", "File name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_model->createFile(name);
        refresh();
    }
}

void FileManagerPanel::onNewFolder()
{
    bool ok;
    QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok);
    if (ok && !name.isEmpty()) {
        m_model->createDirectory(name);
        refresh();
    }
}

void FileManagerPanel::onDelete()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;

    int inode = m_model->inodeFromIndex(index);
    PINODE node = head;
    while (node) {
        if (node->InodeNumber == inode) break;
        node = node->next;
    }

    QString name = m_model->data(index, Qt::DisplayRole).toString();
    QString typeStr = (node && node->FileType == DIRECTORY) ? "folder" : "file";
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirm Delete",
        QString("Delete %1 '%2'?").arg(typeStr, name),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if (node && node->FileType == DIRECTORY)
            m_model->deleteDirectory(index);
        else
            m_model->deleteFile(index);
        refresh();
    }
}

void FileManagerPanel::onRename()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    QString oldName = m_model->data(index, Qt::DisplayRole).toString();
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename", "New name:", QLineEdit::Normal, oldName, &ok);
    if (ok && !newName.isEmpty() && newName != oldName) {
        m_model->renameFile(index, newName);
        refresh();
    }
}

void FileManagerPanel::onRefresh()
{
    m_model->refresh();
    refresh();
}

void FileManagerPanel::onOpen()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    int inode = m_model->inodeFromIndex(index);
    PINODE node = head;
    while (node) {
        if (node->InodeNumber == inode) break;
        node = node->next;
    }
    if (node && node->FileType == REGULAR) {
        m_model->openFile(index);
    }
}

void FileManagerPanel::onTruncate()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    QString name = m_model->data(index, Qt::DisplayRole).toString();
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Truncate", QString("Clear all content of '%1'?").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_model->truncateFile(index);
        refresh();
    }
}

void FileManagerPanel::onProperties()
{
    QModelIndex index = m_treeView->currentIndex();
    if (!index.isValid()) return;
    m_model->showProperties(index);
}

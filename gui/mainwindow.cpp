#include "mainwindow.h"
#include "filesystemmodel.h"
#include "filemanagerpanel.h"
#include "visualizationpanel.h"
#include "visualizationengine.h"
#include "../cvfs_wrapper.h"
#include "../directory.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QObject>

static VisualizationEngine *g_globalEngine = nullptr;

static void cvfsStepCallback(const char *component, const char *action,
                              int targetIndex, int extraIndex,
                              const char *operation, const char *fileName,
                              int progressPercent, const char *explanation)
{
    if (!g_globalEngine) return;
    StepEvent ev;
    if (component) ev.component = QString::fromUtf8(component);
    if (action) ev.action = QString::fromUtf8(action);
    ev.targetIndex = targetIndex;
    ev.extraIndex = extraIndex;
    if (operation) ev.operation = QString::fromUtf8(operation);
    if (fileName) ev.fileName = QString::fromUtf8(fileName);
    ev.progressPercent = progressPercent;
    if (explanation) ev.explanation = QString::fromUtf8(explanation);
    g_globalEngine->onLiveStep(ev);
    QApplication::processEvents();
}

static void showUserManual(QWidget *parent)
{
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("CVFS User Manual");
    dlg->setMinimumSize(600, 500);
    dlg->setStyleSheet(R"(
        QDialog {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a2e, stop:0.5 #16213e, stop:1 #0f3460);
        }
    )");

    QVBoxLayout *l = new QVBoxLayout(dlg);
    l->setContentsMargins(16, 16, 16, 16);

    QTextEdit *text = new QTextEdit;
    text->setReadOnly(true);
    text->setStyleSheet(R"(
        QTextEdit {
            background: rgba(20,20,40,200); color: #c0c0d0;
            border: 1px solid #3a3a5a; border-radius: 6px;
            padding: 12px; font-size: 12px;
        }
    )");

    QString manual = R"(
<h2 style='color:#4a9eff;'>CVFS - Customized Virtual File System</h2>
<p style='color:#a0a0c0;'>An educational Linux-inspired file system explorer with live visualization.</p>

<hr style='border-color:#3a3a5a;'>

<h3 style='color:#fbbf24;'> Left Panel: File Browser</h3>
<p style='color:#c0c0d0;'>
<b>Operations available:</b><br>
• <b>New File</b> - Create a regular file<br>
• <b>New Folder</b> - Create a subdirectory<br>
• <b>Open</b> - View and edit file contents<br>
• <b>Delete</b> - Remove a file or empty directory<br>
• <b>Rename</b> - Change name<br>
• <b>Truncate</b> - Clear contents<br>
• <b>Properties</b> - View inode metadata<br>
</p>

<h3 style='color:#fbbf24;'> Right Panel: Live Animation Engine</h3>
<p style='color:#c0c0d0;'>
Every filesystem operation is animated step-by-step in real-time as the backend executes.<br>
<b>Flowchart</b> on the left shows the operation pipeline.<br>
<b>Structure view</b> on the right shows SuperBlock, UFDT, Inode Bitmap, Block Bitmap, Inode Table, DILB.<br>
<b>Hover</b> over any section for an educational tooltip explaining what it is and how it works.<br>
<b>Replay</b> button lets you re-watch any animation without re-executing.<br>
<b>Speed</b> control (0.5x / 1x / 2x) lets you slow down or speed up the visualization.<br>
</p>

<h3 style='color:#fbbf24;'> How the Filesystem Works</h3>
<p style='color:#c0c0d0;'>
<b>SuperBlock:</b> Global metadata — total/free inodes and blocks.<br>
<b>DILB:</b> Disk Inode List Block — persistent storage for all inodes.<br>
<b>Inode Bitmap:</b> Tracks which inodes are free (1) or allocated (0).<br>
<b>Block Bitmap:</b> Tracks which data blocks are free (1) or allocated (0).<br>
<b>Inode Table:</b> In-memory linked list of active inodes with metadata.<br>
<b>UFDT:</b> User File Descriptor Table — maps fd numbers to open files.<br>
<b>FileTable:</b> Per-open-file metadata (offsets, mode, inode pointer).<br>
<b>Directory:</b> Maps filenames to inode numbers via directory entries.<br>
</p>

<h3 style='color:#fbbf24;'> Tips</h3>
<p style='color:#c0c0d0;'>
• Double-click a directory to navigate<br>
• Double-click a file to open the content editor<br>
• Right-click for context menu<br>
• Watch the right panel animate every internal FS step<br>
• Use Replay + Speed controls for demos and learning<br>
</p>
    )";

    text->setHtml(manual);
    l->addWidget(text);

    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setMinimumHeight(32);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a);
            color: white; border: none; border-radius: 4px;
            padding: 6px 24px; font-size: 12px;
        }
        QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
            stop:0 #5a7fb5, stop:1 #4a6a9a); }
    )");
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QHBoxLayout *btnL = new QHBoxLayout;
    btnL->addStretch();
    btnL->addWidget(closeBtn);
    l->addLayout(btnL);

    dlg->exec();
    dlg->deleteLater();
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("CVFS File System Explorer");
    setMinimumSize(1200, 750);
    resize(1350, 800);

    setupStyleSheet();

    m_model = new FileSystemModel(this);
    m_visPanel = new VisualizationPanel;
    m_visEngine = new VisualizationEngine(this);
    m_visEngine->setPanel(m_visPanel);

    m_filePanel = new FileManagerPanel(m_model);

    g_preOpHook = nullptr;
    g_postOpHook = nullptr;
    g_globalEngine = m_visEngine;
    g_stepHook = cvfsStepCallback;

    setupMenuBar();
    setupCentralWidget();
    setupStatusBar();
    connectSignals();

    m_statusIcon->setText("●");
    m_statusLabel->setText("CVFS mounted. Ready.");
}

MainWindow::~MainWindow()
{
    g_preOpHook = nullptr;
    g_postOpHook = nullptr;
    g_stepHook = nullptr;
    g_globalEngine = nullptr;
}

void MainWindow::setupStyleSheet()
{
    setStyleSheet(R"(
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1c1c30, stop:0.5 #18182a, stop:1 #121224);
        }
        QToolTip {
            background: #1e1e2e; color: #e0e0e0;
            border: 1px solid #4a9eff; padding: 6px; border-radius: 4px;
            font-size: 12px;
        }
        QScrollBar:vertical {
            background: rgba(20,20,40,120); width: 10px;
            border: none; border-radius: 5px;
        }
        QScrollBar::handle:vertical {
            background: rgba(74,158,255,60); min-height: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(74,158,255,100); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            background: rgba(20,20,40,120); height: 10px;
            border: none; border-radius: 5px;
        }
        QScrollBar::handle:horizontal {
            background: rgba(74,158,255,60); min-width: 30px;
            border-radius: 5px;
        }
        QScrollBar::handle:horizontal:hover { background: rgba(74,158,255,100); }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        QSplitter::handle {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:0.5 #3a5a8a, stop:1 #4a6fa5);
            margin: 2px 0; border-radius: 2px;
        }
    )");
}

void MainWindow::setupMenuBar()
{
    QMenuBar *mb = menuBar();
    mb->setStyleSheet(R"(
        QMenuBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #2d2d44, stop:1 #1e1e32);
            color: #c0c0d0; border-bottom: 1px solid #3a3a5a;
            padding: 2px 0; font-size: 12px;
        }
        QMenuBar::item { padding: 4px 12px; border-radius: 3px; margin: 2px 1px; }
        QMenuBar::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a); color: white;
        }
        QMenu {
            background: #252540; color: #c0c0d0;
            border: 1px solid #3a3a5a; border-radius: 6px; padding: 4px;
        }
        QMenu::item { padding: 5px 28px 5px 16px; border-radius: 4px; margin: 2px; }
        QMenu::item:selected {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #4a6fa5, stop:1 #3a5a8a); color: white;
        }
        QMenu::separator { height: 1px; background: #3a3a5a; margin: 4px 8px; }
    )");

    QMenu *fileMenu = mb->addMenu("&File");
    fileMenu->addAction("New File", m_filePanel, &FileManagerPanel::onNewFile, QKeySequence::New);
    fileMenu->addAction("New Folder", m_filePanel, &FileManagerPanel::onNewFolder);
    fileMenu->addSeparator();
    fileMenu->addAction("Open", m_filePanel, &FileManagerPanel::onOpen);
    fileMenu->addSeparator();
    fileMenu->addAction("Exit", qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu *editMenu = mb->addMenu("&Edit");
    editMenu->addAction("Delete", m_filePanel, &FileManagerPanel::onDelete, QKeySequence::Delete);
    editMenu->addAction("Rename", m_filePanel, &FileManagerPanel::onRename);
    editMenu->addAction("Truncate", m_filePanel, &FileManagerPanel::onTruncate);

    QMenu *viewMenu = mb->addMenu("&View");
    viewMenu->addAction("Refresh", m_filePanel, &FileManagerPanel::onRefresh, QKeySequence::Refresh);
    viewMenu->addAction("Properties", m_filePanel, &FileManagerPanel::onProperties);

    QMenu *helpMenu = mb->addMenu("&Help");
    helpMenu->addAction("User Manual", [this]() { showUserManual(this); });
    helpMenu->addAction("About CVFS", [this]() {
        QMessageBox::about(this, "About CVFS",
            "CVFS v2.0\n\n"
            "Customized Virtual File System\n"
            "An educational Linux-inspired filesystem\n"
            "with live visualization of internal operations.\n\n"
            "Built with C++ and Qt.");
    });
}

void MainWindow::setupCentralWidget()
{
    m_splitter = new QSplitter(Qt::Horizontal);
    m_splitter->setHandleWidth(4);
    m_splitter->setChildrenCollapsible(false);

    m_splitter->addWidget(m_filePanel);
    m_splitter->addWidget(m_visPanel);
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 3);
    m_splitter->setSizes({500, 800});

    setCentralWidget(m_splitter);
}

void MainWindow::setupStatusBar()
{
    QStatusBar *sb = statusBar();
    sb->setStyleSheet(R"(
        QStatusBar {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 #1a1a30, stop:1 #121222);
            color: #8888aa; border-top: 1px solid #3a3a5a;
            font-size: 11px; padding: 2px 10px;
        }
    )");

    QWidget *sw = new QWidget;
    QHBoxLayout *sl = new QHBoxLayout(sw);
    sl->setContentsMargins(4, 0, 4, 0);
    sl->setSpacing(6);

    m_statusIcon = new QLabel("●");
    m_statusIcon->setStyleSheet("color: #4ade80; font-size: 12px;");
    sl->addWidget(m_statusIcon);

    m_statusLabel = new QLabel("Ready");
    m_statusLabel->setStyleSheet("color: #a0a0c0; font-size: 11px;");
    sl->addWidget(m_statusLabel);

    sl->addStretch();
    sb->addWidget(sw, 1);
}

void MainWindow::connectSignals()
{
    connect(m_model, &FileSystemModel::operationStarted,
            this, &MainWindow::onOperationStarted);
    connect(m_model, &FileSystemModel::operationFinished,
            this, &MainWindow::onOperationFinished);

    connect(m_visPanel, &VisualizationPanel::replayRequested, this, [this]() {
        m_visEngine->replay();
    });
    connect(m_visPanel, &VisualizationPanel::speedChanged, this, [this](double speed) {
        m_visEngine->setSpeed(speed);
    });
}

void MainWindow::onOperationStarted(const QString &opName)
{
    m_statusIcon->setStyleSheet("color: #fbbf24; font-size: 12px;");
    m_statusLabel->setText("⏳ " + opName + "...");
    m_visEngine->startOperation(opName);
}

void MainWindow::onOperationFinished(const QString &opName, bool success)
{
    if (success) {
        m_statusIcon->setStyleSheet("color: #4ade80; font-size: 12px;");
        m_statusLabel->setText("✓ " + opName + " completed.");
    } else {
        m_statusIcon->setStyleSheet("color: #f87171; font-size: 12px;");
        m_statusLabel->setText("✗ " + opName + " failed.");
    }
    m_visEngine->endOperation(success);
    m_visPanel->refreshStats();
}

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStatusBar>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

class FileSystemModel;
class FileManagerPanel;
class VisualizationPanel;
class VisualizationEngine;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOperationStarted(const QString &opName);
    void onOperationFinished(const QString &opName, bool success);

private:
    void setupStyleSheet();
    void setupMenuBar();
    void setupCentralWidget();
    void setupStatusBar();
    void connectSignals();
    void applyGlobalShadow(QWidget *w);

    FileSystemModel *m_model;
    FileManagerPanel *m_filePanel;
    VisualizationPanel *m_visPanel;
    VisualizationEngine *m_visEngine;

    QSplitter *m_splitter;
    QLabel *m_statusLabel;
    QLabel *m_statusIcon;
};

#endif

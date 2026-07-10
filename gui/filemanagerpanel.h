#ifndef FILEMANAGERPANEL_H
#define FILEMANAGERPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QHeaderView>
#include <QLineEdit>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

class FileSystemModel;

class FileManagerPanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerPanel(FileSystemModel *model, QWidget *parent = nullptr);
    void refresh();

signals:
    void directoryChanged(const QString &path);

public slots:
    void goUp();
    void goHome();
    void onNewFile();
    void onNewFolder();
    void onDelete();
    void onRename();
    void onRefresh();
    void onOpen();
    void onTruncate();
    void onProperties();

private slots:
    void onItemDoubleClicked(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &pos);

private:
    void setupUI();
    void updatePathLabel();
    void setupStyleSheet();
    QPushButton* createToolButton(const QString &text, const QString &tip, const char *slot);
    QLabel* createToolIcon(const QString &symbol, const QString &color);

    FileSystemModel *m_model;
    QLabel *m_headerLabel;
    QLabel *m_pathLabel;
    QLabel *m_statusLabel;
    QTreeView *m_treeView;
    QWidget *m_toolbar;
    QMenu *m_contextMenu;
};

#endif

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QStringList>
#include <QVector>
#include <QIcon>
#include "../CVFS.h"
#include "../directory.h"

class FileSystemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column { Name = 0, Size, Type, Permission, Inode, ColumnCount };

    explicit FileSystemModel(QObject *parent = nullptr);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int inodeFromIndex(const QModelIndex &index) const;
    QModelIndex indexFromInode(int inode) const;

    void refresh();
    void cd(int dirInode);
    void cdUp();
    int currentDirInode() const { return m_currentDir; }

    void createFile(const QString &name, int perm = 3);
    void deleteFile(const QModelIndex &index);
    void renameFile(const QModelIndex &index, const QString &newName);
    void createDirectory(const QString &name);
    void deleteDirectory(const QModelIndex &index);
    void openFile(const QModelIndex &index);
    void truncateFile(const QModelIndex &index);
    void showProperties(const QModelIndex &index);

    static QIcon iconForType(int type, const QString &name = QString());

signals:
    void operationStarted(const QString &opName);
    void operationFinished(const QString &opName, bool success);

private:
    static QPixmap createFolderPixmap(const QColor &base, const QColor &accent);
    static QPixmap createFilePixmap(const QColor &base, const QColor &accent);

    int m_currentDir;
    DirListing m_listing;

    static QIcon s_folderIcon;
    static QIcon s_fileIcon;
    static bool s_iconsInitialized;
};

#endif

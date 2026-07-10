#include "filesystemmodel.h"
#include "filecontentdialog.h"
#include "filepropertiesdialog.h"
#include "../cvfs_wrapper.h"
#include "../persistence.h"
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QFont>

QIcon FileSystemModel::s_folderIcon;
QIcon FileSystemModel::s_fileIcon;
bool FileSystemModel::s_iconsInitialized = false;

QPixmap FileSystemModel::createFolderPixmap(const QColor &base, const QColor &accent)
{
    QPixmap pix(48, 48);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath back;
    back.moveTo(6, 14);
    back.lineTo(6, 40);
    back.lineTo(42, 40);
    back.lineTo(42, 14);
    back.lineTo(22, 14);
    back.lineTo(18, 8);
    back.lineTo(6, 8);
    back.closeSubpath();
    p.fillPath(back, base);

    QPainterPath front;
    front.addRoundedRect(4, 12, 40, 28, 3, 3);
    p.fillPath(front, base);

    QPainterPath tab;
    tab.moveTo(6, 12);
    tab.lineTo(18, 12);
    tab.lineTo(22, 18);
    tab.lineTo(42, 18);
    tab.lineTo(42, 14);
    tab.lineTo(22, 14);
    tab.lineTo(18, 8);
    tab.lineTo(6, 8);
    tab.closeSubpath();
    p.fillPath(tab, accent);

    p.end();
    return pix;
}

QPixmap FileSystemModel::createFilePixmap(const QColor &base, const QColor &accent)
{
    QPixmap pix(48, 48);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.moveTo(12, 4);
    path.lineTo(32, 4);
    path.lineTo(42, 14);
    path.lineTo(42, 42);
    path.lineTo(12, 42);
    path.lineTo(6, 38);
    path.lineTo(6, 8);
    path.closeSubpath();
    p.fillPath(path, base);

    QPainterPath fold;
    fold.moveTo(30, 4);
    fold.lineTo(30, 16);
    fold.lineTo(42, 16);
    fold.closeSubpath();
    p.fillPath(fold, accent);

    p.setPen(QPen(QColor(0,0,0,30), 1));
    p.drawLine(14, 24, 38, 24);
    p.drawLine(14, 30, 34, 30);
    p.drawLine(14, 36, 30, 36);

    p.end();
    return pix;
}

FileSystemModel::FileSystemModel(QObject *parent)
    : QAbstractItemModel(parent), m_currentDir(ROOT_INODE)
{
    if (!s_iconsInitialized) {
        s_folderIcon = QIcon(createFolderPixmap(QColor("#5b7db5"), QColor("#7da0d5")));
        s_fileIcon = QIcon(createFilePixmap(QColor("#4a5568"), QColor("#6b7b8d")));
        s_iconsInitialized = true;
    }
    refresh();
}

QModelIndex FileSystemModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return createIndex(row, column, quintptr(ROOT_INODE));
    }
    if (row < 0 || row >= m_listing.count || column < 0 || column >= ColumnCount)
        return QModelIndex();
    return createIndex(row, column, quintptr(m_listing.entries[row].inode));
}

QModelIndex FileSystemModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

int FileSystemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) return m_listing.count;
    return 0;
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    return ColumnCount;
}

QVariant FileSystemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_listing.count)
        return QVariant();

    const DirListingEntry &entry = m_listing.entries[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Name: return QString(entry.name);
        case Size:
            if (entry.type == DIRECTORY) return QString("--");
            return QString::number(entry.size) + " B";
        case Type:
            if (entry.type == DIRECTORY) return QString("Directory");
            return QString("File");
        case Permission: {
            QString perm;
            perm += (entry.permission & READ) ? "r" : "-";
            perm += (entry.permission & WRITE) ? "w" : "-";
            return perm;
        }
        case Inode: return QString::number(entry.inode);
        }
    }

    if (role == Qt::DecorationRole && index.column() == Name) {
        if (entry.type == DIRECTORY) return s_folderIcon;
        return s_fileIcon;
    }

    if (role == Qt::ForegroundRole) {
        if (entry.type == DIRECTORY) return QColor(100, 160, 255);
        return QColor(200, 200, 210);
    }

    if (role == Qt::FontRole && index.column() == Name) {
        QFont f;
        f.setPointSize(11);
        return f;
    }

    if (role == Qt::ToolTipRole) {
        QString tip = QString("<b>%1</b><br>").arg(entry.name);
        tip += QString("Inode: %1<br>").arg(entry.inode);
        tip += QString("Type: %1<br>").arg(entry.type == DIRECTORY ? "Directory" : "File");
        tip += QString("Size: %1 bytes<br>").arg(entry.size);
        QString perm;
        perm += (entry.permission & READ) ? "r" : "-";
        perm += (entry.permission & WRITE) ? "w" : "-";
        tip += QString("Perm: %1").arg(perm);
        return tip;
    }

    return QVariant();
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case Name: return "Name";
        case Size: return "Size";
        case Type: return "Type";
        case Permission: return "Perm";
        case Inode: return "Inode";
        }
    }
    return QVariant();
}

bool FileSystemModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole || index.column() != Name) return false;
    QString newName = value.toString();
    if (newName.isEmpty()) return false;

    renameFile(index, newName);
    return true;
}

Qt::ItemFlags FileSystemModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.column() == Name) f |= Qt::ItemIsEditable;
    return f;
}

int FileSystemModel::inodeFromIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_listing.count)
        return -1;
    return m_listing.entries[index.row()].inode;
}

QModelIndex FileSystemModel::indexFromInode(int inode) const
{
    for (int i = 0; i < m_listing.count; i++) {
        if (m_listing.entries[i].inode == inode)
            return createIndex(i, 0, quintptr(inode));
    }
    return QModelIndex();
}

void FileSystemModel::refresh()
{
    beginResetModel();
    dir_list_at(m_currentDir, &m_listing);
    endResetModel();
}

void FileSystemModel::cd(int dirInode)
{
    PINODE node = head;
    while (node) {
        if (node->InodeNumber == dirInode && node->FileType == DIRECTORY) {
            m_currentDir = dirInode;
            dir_set_cwd_inode(dirInode);
            refresh();
            return;
        }
        node = node->next;
    }
}

void FileSystemModel::cdUp()
{
    PINODE curNode = head;
    while (curNode) {
        if (curNode->InodeNumber == m_currentDir) break;
        curNode = curNode->next;
    }
    if (!curNode || !curNode->Buffer) return;

    DirEntry *entries = (DirEntry *)curNode->Buffer;
    int numEntries = curNode->FileActualSize / sizeof(DirEntry);
    for (int i = 0; i < numEntries; i++) {
        if (entries[i].d_inode > 0 && strcmp(entries[i].d_name, "..") == 0) {
            cd(entries[i].d_inode);
            return;
        }
    }
}

void FileSystemModel::createFile(const QString &name, int perm)
{
    QByteArray nameBytes = name.toUtf8();
    emit operationStarted("create");
    int ret = W_CreateFile(nameBytes.data(), perm);
    if (ret >= 0)
        CloseFileByFD(ret);
    emit operationFinished("create", ret >= 0);
    refresh();
}

void FileSystemModel::deleteFile(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString name = data(index, Qt::DisplayRole).toString();
    QByteArray nameBytes = name.toUtf8();

    emit operationStarted("delete");
    int ret = W_rm_File(nameBytes.data());
    emit operationFinished("delete", ret >= 0);
    refresh();
}

void FileSystemModel::renameFile(const QModelIndex &index, const QString &newName)
{
    if (!index.isValid()) return;
    QString oldName = data(index, Qt::DisplayRole).toString();
    QByteArray oldBytes = oldName.toUtf8();
    QByteArray newBytes = newName.toUtf8();

    emit operationStarted("rename");

    PINODE inode = FindFile(oldBytes.data());
    if (inode) {
        strncpy(inode->FileName, newBytes.data(), 49);
        inode->FileName[49] = '\0';
        sync_inode(inode->InodeNumber);

        PINODE parentNode = head;
        while (parentNode) {
            if (parentNode->FileType == DIRECTORY && parentNode->Buffer) {
                DirEntry *entries = (DirEntry *)parentNode->Buffer;
                int numEntries = parentNode->FileActualSize / sizeof(DirEntry);
                bool found = false;
                for (int i = 0; i < numEntries && i < MAX_DIR_ENTRIES; i++) {
                    if (entries[i].d_inode == inode->InodeNumber && entries[i].d_inode > 0) {
                        strncpy(entries[i].d_name, newBytes.data(), 58);
                        entries[i].d_name[58] = '\0';
                        sync_inode(parentNode->InodeNumber);
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            parentNode = parentNode->next;
        }
    }

    emit operationFinished("rename", inode != NULL);
    refresh();
}

void FileSystemModel::createDirectory(const QString &name)
{
    QByteArray nameBytes = name.toUtf8();
    emit operationStarted("mkdir");
    int ret = W_dir_create(nameBytes.data());
    emit operationFinished("mkdir", ret >= 0);
    refresh();
}

void FileSystemModel::deleteDirectory(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString name = data(index, Qt::DisplayRole).toString();
    QByteArray nameBytes = name.toUtf8();

    emit operationStarted("rmdir");
    int ret = W_dir_remove(nameBytes.data());
    emit operationFinished("rmdir", ret >= 0);
    refresh();
}

void FileSystemModel::openFile(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString name = data(index, Qt::DisplayRole).toString();

    QByteArray nameBytes = name.toUtf8();
    PINODE node = FindFile(nameBytes.data());
    if (!node) return;

    FileContentDialog dlg(name, qobject_cast<QWidget *>(QObject::parent()));
    dlg.exec();
}

void FileSystemModel::truncateFile(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString name = data(index, Qt::DisplayRole).toString();
    QByteArray nameBytes = name.toUtf8();

    emit operationStarted("truncate");
    int ret = W_truncate_File(nameBytes.data());
    emit operationFinished("truncate", ret >= 0);
    refresh();
}

void FileSystemModel::showProperties(const QModelIndex &index)
{
    if (!index.isValid()) return;
    QString name = data(index, Qt::DisplayRole).toString();

    FilePropertiesDialog dlg(name, qobject_cast<QWidget *>(QObject::parent()));
    dlg.exec();
}

QIcon FileSystemModel::iconForType(int type, const QString &name)
{
    if (type == DIRECTORY) return s_folderIcon;
    return s_fileIcon;
}

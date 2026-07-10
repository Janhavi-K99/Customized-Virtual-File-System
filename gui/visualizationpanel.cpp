#include "visualizationpanel.h"
#include "visualizationengine.h"
#include "flowchartwidget.h"
#include "systeminfowidget.h"
#include "../persistence.h"
#include "../CVFS.h"
#include "../directory.h"
#include <QPainterPath>
#include <cmath>
#include <QScrollBar>
#include <QToolTip>
#include <QMouseEvent>
#include <QHelpEvent>

static QColor withAlpha(const QColor &c, int a) { return QColor(c.red(), c.green(), c.blue(), a); }

FsSceneWidget::FsSceneWidget(QWidget *parent)
    : QWidget(parent), m_highlightedCellBitmap(0), m_highlightedCellIndex(-1),
      m_highlightColor(74, 158, 255), m_pulse(0),
      m_relDirInode(-1), m_relFileInode(-1), m_relBlockIndex(-1), m_relOpacity(0)
{
    setMinimumHeight(360);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    connect(&m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulse += 0.08f;
        if (m_pulse > 6.28f) m_pulse -= 6.28f;
        if (m_relOpacity > 0) m_relOpacity -= 0.005f;
        update();
    });
    m_pulseTimer.start(40);
}

void FsSceneWidget::setHighlightedSection(const QString &s) { m_highlightedSection = s; update(); }

void FsSceneWidget::setCellHighlight(int bt, int idx, const QColor &c)
{
    m_highlightedCellBitmap = bt;
    m_highlightedCellIndex = idx;
    m_highlightColor = c;
    update();
}

void FsSceneWidget::clearAllHighlights()
{
    m_highlightedSection.clear();
    m_highlightedCellBitmap = 0;
    m_highlightedCellIndex = -1;
    clearRelationship();
    update();
}

void FsSceneWidget::refreshData() { update(); }

void FsSceneWidget::setRelationship(int dirInode, int fileInode, int blockIndex)
{
    m_relDirInode = dirInode;
    m_relFileInode = fileInode;
    m_relBlockIndex = blockIndex;
    m_relOpacity = 1.0f;
    update();
}

void FsSceneWidget::clearRelationship()
{
    m_relDirInode = -1;
    m_relFileInode = -1;
    m_relBlockIndex = -1;
    m_relOpacity = 0;
}

QColor FsSceneWidget::inodeCellColor(int idx) const
{
    PINODE t = head;
    while (t) { if (t->InodeNumber == idx + 1 && t->FileType != 0) return QColor(50, 200, 50); t = t->next; }
    return QColor(40, 40, 65);
}

QColor FsSceneWidget::blockCellColor(int idx) const
{
    return is_block_free(idx) ? QColor(40, 40, 65) : QColor(50, 200, 50);
}

void FsSceneWidget::registerSection(const QString &name, const QRect &rect, const QString &tooltip)
{
    m_sectionRects.append({name, rect, tooltip});
}

void FsSceneWidget::paintOverlay(QPainter &p, int x, int y, int w, int h, const QString &label)
{
    float glow = 0.5f + 0.5f * sin(m_pulse);
    QColor c(255, 220, 50);

    QPainterPath path;
    path.addRoundedRect(x - 4, y - 4, w + 8, h + 8, 10, 10);
    p.setPen(QPen(withAlpha(c, int(180 + 75 * glow)), 4));
    p.setBrush(withAlpha(c, int(25 + 25 * glow)));
    p.drawPath(path);

    p.setPen(QColor(255, 255, 200));
    p.setFont(QFont("Segoe UI", 14, QFont::Bold));
    QRectF tr(x, y + h / 2 - 18, w, 36);
    p.fillRect(tr, QColor(0, 0, 0, 180));
    p.drawText(tr, Qt::AlignCenter, label);
}

void FsSceneWidget::paintRelationshipLines(QPainter &p, int W, int H)
{
    if (m_relOpacity <= 0.01f) return;
    Q_UNUSED(W); Q_UNUSED(H);

    int alpha = (int)(m_relOpacity * 200);
    p.setPen(QPen(withAlpha(QColor(255, 200, 80), alpha), 3, Qt::DashLine));
    p.setBrush(Qt::NoBrush);

    QString steps;
    if (m_relDirInode > 0) steps += QString("Dir(#%1)").arg(m_relDirInode);
    if (m_relFileInode > 0) steps += QString("  Inode(#%1)").arg(m_relFileInode);
    if (m_relBlockIndex >= 0) steps += QString("  Block[%1]").arg(m_relBlockIndex);
    steps += "  OK";

    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.setPen(withAlpha(QColor(255, 220, 100), alpha));
    p.drawText(rect(), Qt::AlignBottom | Qt::AlignRight, steps);
}

void FsSceneWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    m_sectionRects.clear();

    int W = width(), H = height();
    int mrg = 4, gap = 4;

    int topRowH = 90;
    int midRowH = 120;
    int botRowH = H - topRowH - midRowH - 2 * gap - 2 * mrg;

    int c0W = W * 50 / 100 - gap / 2;
    int c1W = W - c0W - gap - 2 * mrg;

    int c0x = mrg;
    int c1x = mrg + c0W + gap;

    paintSuperBlock(p, c0x, mrg, c0W, topRowH);
    paintUfdt(p, c1x, mrg, c1W, topRowH);

    paintInodeBitmap(p, c0x, mrg + topRowH + gap, c0W, midRowH);
    paintBlockBitmap(p, c1x, mrg + topRowH + gap, c1W, midRowH);

    paintInodeTable(p, c0x, mrg + topRowH + midRowH + 2 * gap, c0W, botRowH);
    paintDilb(p, c1x, mrg + topRowH + midRowH + 2 * gap, c1W, botRowH);

    paintRelationshipLines(p, W, H);

    QString sec = m_highlightedSection;
    if (sec == "SuperBlock")
        paintOverlay(p, c0x, mrg, c0W, topRowH, "SUPER BLOCK");
    else if (sec == "UFDT" || sec == "FileTable")
        paintOverlay(p, c1x, mrg, c1W, topRowH, "UFDT");
    else if (sec == "InodeBitmap")
        paintOverlay(p, c0x, mrg + topRowH + gap, c0W, midRowH, "INODE BITMAP");
    else if (sec == "BlockBitmap")
        paintOverlay(p, c1x, mrg + topRowH + gap, c1W, midRowH, "BLOCK BITMAP");
    else if (sec == "InodeTable")
        paintOverlay(p, c0x, mrg + topRowH + midRowH + 2 * gap, c0W, botRowH, "INODE TABLE");
    else if (sec == "DILB" || sec == "Directory")
        paintOverlay(p, c1x, mrg + topRowH + midRowH + 2 * gap, c1W, botRowH, "DILB");
    else if (sec == "DataBlocks")
        paintOverlay(p, c1x, mrg + topRowH + midRowH + 2 * gap, c1W, botRowH, "DATA BLOCKS");
    else if (sec == "DiskSync")
        paintOverlay(p, c0x + (c0W + c1W + gap) / 4, mrg + topRowH + midRowH + 2 * gap,
                     (c0W + c1W + gap) / 2, botRowH, "DISK SYNC");
}

bool FsSceneWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *he = static_cast<QHelpEvent *>(event);
        for (const auto &sr : m_sectionRects) {
            if (sr.rect.contains(he->pos())) {
                QToolTip::showText(he->globalPos(), sr.tooltip);
                return true;
            }
        }
        QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}

void FsSceneWidget::mouseMoveEvent(QMouseEvent *event)
{
    for (const auto &sr : m_sectionRects) {
        if (sr.rect.contains(event->pos())) {
            setToolTip(sr.tooltip);
            return;
        }
    }
    setToolTip("");
}

void FsSceneWidget::paintSuperBlock(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "SUPERBLOCK");

    registerSection("SuperBlock", QRect(x, y, w, h),
        "<b>Super Block</b><br>"
        "The Super Block is the filesystem's global metadata. It stores total/free "
        "inode counts and total/free data block counts. Every mount reads it first.");

    int usedInodes = SUPERBLOCKobj.TotalInodes - SUPERBLOCKobj.FreeInode;
    int totalInodes = SUPERBLOCKobj.TotalInodes;
    int usedBlocks = NUM_DATA_BLOCKS - get_free_blocks_count();
    int totalBlocks = NUM_DATA_BLOCKS;

    auto drawBar = [&](int cx, int row, const QString &label, int used, int total) {
        int ry = y + 26 + row * 26;
        p.setPen(QColor(180, 180, 200));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(cx + 6, ry + 12, label + QString(" %1/%2").arg(used).arg(total));
        int barX = cx + 70, barY = ry, barW = w - 80, barH = 16;
        p.fillRect(barX, barY, barW, barH, QColor(30, 30, 55));
        float frac = total > 0 ? (float)used / total : 0;
        int fillW = qMin((int)(barW * frac), barW);
        QColor fill = frac > 0.8f ? QColor(220, 80, 80) : frac > 0.5f ? QColor(220, 180, 50) : QColor(70, 210, 70);
        p.fillRect(barX, barY, fillW, barH, fill);
    };

    drawBar(x, 0, "Inodes:", usedInodes, totalInodes);
    drawBar(x, 1, "Blocks:", usedBlocks, totalBlocks);
}

void FsSceneWidget::paintUfdt(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "UFDT");

    registerSection("UFDT", QRect(x, y, w, h),
        "<b>UFDT - User File Descriptor Table</b><br>"
        "An array of 50 entries, each pointing to a FileTable (or NULL if free).<br>"
        "Maps user-space fd numbers to kernel-space open file descriptions.");

    p.setFont(QFont("Segoe UI", 10));
    int rowH = 18;
    int maxRows = (h - 28) / rowH;
    int rows = 0;

    for (int i = 0; i < 50 && rows < maxRows; i++) {
        if (UFDTArr[i].ptrfiletable != NULL) {
            int ry = y + 26 + rows * rowH;
            if (rows % 2) p.fillRect(x + 4, ry, w - 8, rowH - 1, QColor(255, 255, 255, 6));
            PFILETABLE ft = UFDTArr[i].ptrfiletable;
            QString modeStr = ft->mode == 1 ? "R" : ft->mode == 2 ? "W" : "RW";
            QString name = ft->ptrinode ? QString(ft->ptrinode->FileName) : "?";
            QString line = QString("fd\u2009%1  [%2]  %3").arg(i).arg(modeStr).arg(name);
            p.setPen(QColor(200, 200, 220));
            p.drawText(x + 8, ry + 13, line);
            rows++;
        }
    }

    if (rows == 0) {
        p.setPen(QColor(120, 120, 150));
        p.drawText(x + 8, y + h / 2 + 5, "(no open files)");
    }
}

void FsSceneWidget::paintInodeBitmap(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "INODE BITMAP");

    registerSection("InodeBitmap", QRect(x, y, w, h),
        "<b>Inode Bitmap</b><br>"
        "One bit per inode. 0 (green) = allocated, 1 (dark) = free.<br>"
        "Enables O(1) lookup of free inodes during file creation.");

    int cs = 18, g = 2, cols = 10;
    int gW = cols * (cs + g), gH = 5 * (cs + g);
    int sx = x + (w - gW) / 2, sy = y + (h - gH) / 2 + 4;

    float pulse = 0.5f + 0.5f * sin(m_pulse * 2);
    for (int i = 0; i < 50; i++) {
        int cx = sx + (i % cols) * (cs + g);
        int cy = sy + (i / cols) * (cs + g);
        bool hl = (m_highlightedSection == "InodeBitmap" && m_highlightedCellIndex == i);

        QRectF cr(cx, cy, cs, cs);
        QPainterPath cp;
        cp.addRoundedRect(cr, 3, 3);
        p.fillPath(cp, hl ? QColor(74, 220, 255) : inodeCellColor(i));
        p.setPen(QPen(hl ? QColor(255, 255, 200) : QColor(80, 80, 120), hl ? 2 : 1));
        p.drawPath(cp);

        if (hl) {
            float r = cs * 0.5f + 8 * pulse;
            QRadialGradient rg(cx + cs / 2, cy + cs / 2, r);
            rg.setColorAt(0, withAlpha(QColor(255, 255, 200), 100));
            rg.setColorAt(1, Qt::transparent);
            p.setBrush(rg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx + cs / 2, cy + cs / 2), r, r);
        }

        p.setPen(hl ? QColor(0, 0, 0) : QColor(130, 130, 160));
        p.setFont(QFont("Segoe UI", 8, QFont::Bold));
        p.drawText(cr, Qt::AlignCenter, QString::number(i + 1));
    }
}

void FsSceneWidget::paintBlockBitmap(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "BLOCK BITMAP");

    registerSection("BlockBitmap", QRect(x, y, w, h),
        "<b>Block Bitmap</b><br>"
        "One bit per data block (502 total). 0 (green) = allocated, 1 (dark) = free.<br>"
        "Allocates blocks during writes, frees them during deletion/truncation.");

    int totalCells = NUM_DATA_BLOCKS;
    int cs = 7, g = 1;
    int cols = (w - 16) / (cs + g);
    if (cols < 1) cols = 1;
    int visRows = (totalCells + cols - 1) / cols;
    int maxRows = (h - 30) / (cs + g);
    if (visRows > maxRows) visRows = maxRows;
    if (visRows < 1) visRows = 1;

    int gW = cols * (cs + g);
    int sx = x + (w - gW) / 2, sy = y + 26;

    float pulse = 0.5f + 0.5f * sin(m_pulse * 2);
    int visible = cols * visRows;
    for (int i = 0; i < visible; i++) {
        int cx = sx + (i % cols) * (cs + g);
        int cy = sy + (i / cols) * (cs + g);
        bool hl = (m_highlightedSection == "BlockBitmap" && m_highlightedCellIndex == i);

        QRectF cr(cx, cy, cs, cs);
        QPainterPath cp;
        cp.addRoundedRect(cr, 1, 1);
        p.fillPath(cp, hl ? QColor(74, 220, 255) : blockCellColor(i));
        p.setPen(QPen(hl ? QColor(255, 255, 200) : QColor(60, 60, 90), hl ? 1 : 0));
        p.drawPath(cp);

        if (hl) {
            float r = cs * 0.5f + 4 * pulse;
            QRadialGradient rg(cx + cs / 2, cy + cs / 2, r);
            rg.setColorAt(0, withAlpha(QColor(255, 255, 200), 80));
            rg.setColorAt(1, Qt::transparent);
            p.setBrush(rg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(cx + cs / 2, cy + cs / 2), r, r);
        }
    }

    QString info = QString("%1 data blocks").arg(totalCells);
    p.setPen(QColor(120, 120, 150));
    p.setFont(QFont("Segoe UI", 8));
    p.drawText(x + 8, y + h - 6, info);
}

void FsSceneWidget::paintInodeTable(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "INODE TABLE  —  " + QString(dir_get_cwd_path()));

    registerSection("InodeTable", QRect(x, y, w, h),
        "<b>Inode Table</b><br>"
        "Each inode stores: name, number, type (REG/DIR), size, permission, "
        "link count, reference count, and a data buffer pointer.");

    int colX[] = {x + 6, x + 48, x + 150, x + 210, x + 260, x + 310};
    QString headers[] = {"Inode", "Name", "Type", "Size", "Links", "Perm"};
    QFont hdrFont("Segoe UI", 10, QFont::Bold);
    p.setPen(QColor(74, 158, 255));
    p.setFont(hdrFont);
    for (int i = 0; i < 6; i++) p.drawText(colX[i], y + 34, headers[i]);

    int rowH = 20;
    int maxRows = (h - 42) / rowH;
    QVector<PINODE> inodes;
    PINODE t = head;
    while (t && inodes.size() < maxRows) { if (t->FileType != 0) inodes.append(t); t = t->next; }

    p.setFont(QFont("Segoe UI", 10));
    for (int i = 0; i < inodes.size(); i++) {
        PINODE n = inodes[i];
        int ry = y + 40 + i * rowH;
        if (i % 2) p.fillRect(x + 4, ry - 1, w - 8, rowH - 1, QColor(255, 255, 255, 6));

        bool hl = (m_highlightedSection == "InodeTable" && m_highlightedCellIndex == n->InodeNumber - 1);

        if (hl) {
            float glow = 0.3f + 0.3f * sin(m_pulse);
            p.fillRect(x + 4, ry - 1, w - 8, rowH - 1, withAlpha(QColor(74, 220, 255), int(40 + 40 * glow)));
        }

        p.setPen(hl ? QColor(255, 255, 220) : QColor(74, 158, 255));
        p.drawText(colX[0], ry + 14, QString::number(n->InodeNumber));
        p.setPen(hl ? QColor(255, 255, 220) : (n->FileType == DIRECTORY ? QColor(130, 190, 255) : QColor(230, 230, 240)));
        QString nm = n->FileName;
        if (strlen(n->FileName) == 0 && n->FileType == DIRECTORY) { if (n->InodeNumber == ROOT_INODE) nm = "/"; }
        p.drawText(colX[1], ry + 14, nm.length() > 14 ? nm.left(14) + ".." : nm);
        p.setPen(hl ? QColor(255, 255, 220) : (n->FileType == DIRECTORY ? QColor(100, 220, 100) : QColor(250, 200, 50)));
        QString tStr = n->FileType == DIRECTORY ? "DIR" : n->FileType == REGULAR ? "REG" : "SPE";
        p.drawText(colX[2], ry + 14, tStr);
        p.setPen(hl ? QColor(255, 255, 220) : QColor(200, 200, 220));
        p.drawText(colX[3], ry + 14, QString::number(n->FileActualSize));
        p.drawText(colX[4], ry + 14, QString::number(n->LinkCount));
        QString perm;
        perm += (n->permission & READ) ? "r" : "-";
        perm += (n->permission & WRITE) ? "w" : "-";
        p.drawText(colX[5], ry + 14, perm);
    }
}

void FsSceneWidget::paintDilb(QPainter &p, int x, int y, int w, int h)
{
    QPainterPath bg;
    bg.addRoundedRect(x, y, w, h, 6, 6);
    p.fillPath(bg, QColor(18, 18, 42, 210));
    p.setPen(QPen(QColor(74, 158, 255, 70), 1));
    p.drawPath(bg);

    p.setPen(QColor(74, 158, 255));
    p.setFont(QFont("Segoe UI", 12, QFont::Bold));
    p.drawText(x + 8, y + 18, "DILB  (linked list)");

    registerSection("DILB", QRect(x, y, w, h),
        "<b>DILB - Disk Inode List Block</b><br>"
        "The inodes form a linked list chain (head  next  next  ...).<br>"
        "Each node is an in-memory INODE structure with metadata and data buffer.");

    // Draw as linked list: head → inode1 → inode2 → ...
    int boxW = w - 24;
    int boxH = 30;
    int arrowH = 20;
    int maxNodes = (h - 36) / (boxH + arrowH);
    if (maxNodes < 1) maxNodes = 1;

    QVector<PINODE> inodes;
    PINODE t = head;
    while (t && inodes.size() < maxNodes) { if (t->FileType != 0) inodes.append(t); t = t->next; }

    int startY = y + 32;
    int bx = x + (w - boxW) / 2;

    // Draw "head" label
    p.setPen(QColor(200, 200, 220));
    p.setFont(QFont("Segoe UI", 9, QFont::Bold));
    p.drawText(bx, startY - 4, "head");

    // Draw arrow from head to first node
    int arrowX = bx + boxW / 2;
    p.setPen(QPen(QColor(100, 180, 255), 2));
    p.drawLine(arrowX, startY, arrowX, startY + 6);

    for (int i = 0; i < inodes.size(); i++) {
        PINODE n = inodes[i];
        int ny = startY + 6 + i * (boxH + arrowH);

        bool hl = (m_highlightedSection == "DILB" && m_highlightedCellIndex == n->InodeNumber - 1);

        // Box
        QColor boxColor = hl ? QColor(74, 158, 255, 180) :
                          (n->FileType == DIRECTORY ? QColor(30, 70, 140, 180) : QColor(40, 50, 80, 180));
        QPainterPath bp;
        bp.addRoundedRect(bx, ny, boxW, boxH, 4, 4);
        p.fillPath(bp, boxColor);
        p.setPen(QPen(hl ? QColor(255, 255, 200) : QColor(100, 180, 255), hl ? 2 : 1));
        p.drawPath(bp);

        // Content
        p.setFont(QFont("Segoe UI", 9, QFont::Bold));
        QString label = QString("#%1 %2")
            .arg(n->InodeNumber)
            .arg(n->FileName[0] ? n->FileName : (n->InodeNumber == ROOT_INODE ? "/" : "?"));
        p.setPen(hl ? QColor(255, 255, 220) : QColor(200, 220, 255));
        p.drawText(bx + 6, ny + 12, label);

        p.setFont(QFont("Segoe UI", 8));
        QString typeStr = n->FileType == DIRECTORY ? "DIR" : "REG";
        QString info = QString("%1  |  size=%2  |  refs=%3")
            .arg(typeStr).arg(n->FileActualSize).arg(n->ReferenceCount);
        p.setPen(QColor(160, 180, 210));
        p.drawText(bx + 6, ny + 24, info);

        // Arrow to next node
        if (i < inodes.size() - 1) {
            int ay = ny + boxH;
            int ah = arrowH - 4;
            p.setPen(QPen(QColor(100, 180, 255), 2));
            p.drawLine(arrowX, ay, arrowX, ay + ah);
            // Arrowhead
            QPainterPath arrowHead;
            arrowHead.moveTo(arrowX - 5, ay + ah - 2);
            arrowHead.lineTo(arrowX, ay + ah + 4);
            arrowHead.lineTo(arrowX + 5, ay + ah - 2);
            p.setBrush(QColor(100, 180, 255));
            p.setPen(Qt::NoPen);
            p.drawPath(arrowHead);
        }

        if (i == inodes.size() - 1 && inodes.size() < maxNodes) {
            // Show "next → NULL" for last visible node
            int ay = ny + boxH;
            p.setPen(QPen(QColor(80, 80, 120), 1, Qt::DashLine));
            p.drawLine(arrowX, ay, arrowX, ay + 6);
            p.setPen(QColor(120, 120, 150));
            p.setFont(QFont("Segoe UI", 8));
            p.drawText(arrowX + 8, ay + 6, "NULL");
        }
    }

    if (inodes.isEmpty()) {
        p.setPen(QColor(120, 120, 150));
        p.setFont(QFont("Segoe UI", 10));
        p.drawText(x + 8, y + h / 2 + 5, "(no inodes allocated)");
    }
}

VisualizationPanel::VisualizationPanel(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(520);
    setStyleSheet("background: transparent;");
    m_showInfo = false;
    setupUI();
}

void VisualizationPanel::setupUI()
{
    QVBoxLayout *mainL = new QVBoxLayout(this);
    mainL->setSpacing(3);
    mainL->setContentsMargins(4, 4, 4, 4);

    // ── Header ──
    QWidget *hdrBar = new QWidget;
    hdrBar->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2d2d44, stop:1 #1e1e32); border: 1px solid rgba(255,220,68,80); border-radius: 5px;");
    QHBoxLayout *hdrL = new QHBoxLayout(hdrBar);
    hdrL->setContentsMargins(8, 4, 8, 4);

    QLabel *animIcon = new QLabel("⚙");
    animIcon->setStyleSheet("font-size: 16px; background: transparent; color: #ffdd44;");
    hdrL->addWidget(animIcon);

    m_headerLabel = new QLabel("CVFS Animation Engine");
    m_headerLabel->setStyleSheet("color: #ffdd44; font-size: 13px; font-weight: bold; background: transparent;");
    hdrL->addWidget(m_headerLabel);
    hdrL->addStretch();

    m_infoToggle = new QPushButton("System Info");
    m_infoToggle->setFixedHeight(24);
    m_infoToggle->setCursor(Qt::PointingHandCursor);
    m_infoToggle->setStyleSheet(R"(
        QPushButton {
            color: #a0c0e0; background: rgba(74,158,255,20);
            border: 1px solid rgba(74,158,255,40); border-radius: 4px;
            padding: 2px 10px; font-size: 10px; font-weight: bold;
        }
        QPushButton:hover { background: rgba(74,158,255,50); color: white; }
        QPushButton:checked { background: rgba(74,158,255,80); color: white; border-color: #4a9eff; }
    )");
    m_infoToggle->setCheckable(true);
    connect(m_infoToggle, &QPushButton::toggled, this, &VisualizationPanel::showSystemInfo);
    hdrL->addWidget(m_infoToggle);

    mainL->addWidget(hdrBar);

    // ── Operation label ──
    m_opLabel = new QLabel("Ready");
    m_opLabel->setStyleSheet(R"(
        QLabel {
            color: #fbbf24; font-size: 13px; font-weight: bold;
            padding: 3px 10px; background: rgba(30,30,50,160);
            border: 1px solid rgba(255,200,50,60); border-radius: 4px;
        }
    )");
    mainL->addWidget(m_opLabel);

    // ── Step counter + progress ──
    QHBoxLayout *stepL = new QHBoxLayout;
    stepL->setSpacing(4);

    m_stepCounter = new QLabel("0 / 0");
    m_stepCounter->setFixedWidth(62);
    m_stepCounter->setAlignment(Qt::AlignCenter);
    m_stepCounter->setStyleSheet(R"(
        QLabel {
            color: #ffffff; font-size: 12px; font-weight: bold;
            background: rgba(74,158,255,80); padding: 3px 6px;
            border-radius: 4px;
        }
    )");

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setFixedHeight(18);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            background: #2a2a4a; border: 1px solid #4a4a6a;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #4a9eff, stop:1 #ffdd44);
            border-radius: 2px;
        }
    )");

    stepL->addWidget(m_stepCounter);
    stepL->addWidget(m_progressBar, 1);
    mainL->addLayout(stepL);

    // ── Explanation ──
    m_explanationLabel = new QLabel("Create a file or folder to begin.");
    m_explanationLabel->setWordWrap(true);
    m_explanationLabel->setMinimumHeight(38);
    m_explanationLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_explanationLabel->setStyleSheet(R"(
        QLabel {
            color: #e0e0f0; font-size: 12px;
            padding: 5px 10px;
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
                stop:0 rgba(74,158,255,50), stop:1 rgba(30,30,60,190));
            border: 1px solid rgba(74,158,255,100);
            border-radius: 5px;
        }
    )");
    mainL->addWidget(m_explanationLabel);

    // ── Control row ──
    m_controlLayout = new QHBoxLayout;
    m_controlLayout->setSpacing(4);

    m_replayBtn = new QPushButton("▶ Replay");
    m_replayBtn->setFixedHeight(26);
    m_replayBtn->setCursor(Qt::PointingHandCursor);
    m_replayBtn->setStyleSheet(R"(
        QPushButton {
            color: #a0c0e0; background: rgba(74,158,255,25);
            border: 1px solid rgba(74,158,255,50); border-radius: 4px;
            padding: 2px 12px; font-size: 11px; font-weight: bold;
        }
        QPushButton:hover { background: rgba(74,158,255,55); color: white; }
        QPushButton:disabled { color: #555; background: rgba(50,50,70,30); border-color: #3a3a5a; }
    )");
    m_replayBtn->setEnabled(false);
    connect(m_replayBtn, &QPushButton::clicked, this, &VisualizationPanel::replayRequested);

    QLabel *speedLabel = new QLabel("Speed:");
    speedLabel->setStyleSheet("color: #a0a0c0; font-size: 11px;");

    m_speedCombo = new QComboBox;
    m_speedCombo->addItems({"0.1x", "0.25x", "0.5x", "1x", "2x", "4x"});
    m_speedCombo->setCurrentIndex(3);
    m_speedCombo->setFixedWidth(60);
    m_speedCombo->setFixedHeight(26);
    m_speedCombo->setStyleSheet(R"(
        QComboBox {
            color: #c0c0d0; background: rgba(30,30,50,160);
            border: 1px solid #3a3a5a; border-radius: 4px;
            padding: 2px 4px; font-size: 11px;
        }
        QComboBox::drop-down { border: none; width: 14px; }
        QComboBox QAbstractItemView {
            background: #252540; color: #c0c0d0; border: 1px solid #3a3a5a;
            selection-background-color: #4a6fa5; font-size: 11px;
        }
    )");
    connect(m_speedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        double speeds[] = {0.1, 0.25, 0.5, 1.0, 2.0, 4.0};
        if (idx >= 0 && idx < 6) emit speedChanged(speeds[idx]);
    });

    m_controlLayout->addWidget(m_replayBtn);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(speedLabel);
    m_controlLayout->addWidget(m_speedCombo);
    mainL->addLayout(m_controlLayout);

    // ── Mid section: flow chart + scene ──
    m_midLayout = new QHBoxLayout;
    m_midLayout->setSpacing(3);

    m_flowChart = new FlowChartWidget;
    m_flowChart->setMinimumHeight(280);
    m_flowChart->setFixedWidth(130);
    m_flowChart->setStyleSheet("background: rgba(16,16,35,180); border: 1px solid #3a3a5a; border-radius: 4px;");
    m_midLayout->addWidget(m_flowChart);

    m_sceneWidget = new FsSceneWidget;
    m_sceneWidget->setStyleSheet("background: transparent;");
    m_midLayout->addWidget(m_sceneWidget, 1);

    m_sysInfo = new SystemInfoWidget;
    m_sysInfo->setVisible(false);
    m_midLayout->addWidget(m_sysInfo, 1);

    mainL->addLayout(m_midLayout, 1);

    // ── Step log ──
    m_stepList = new QListWidget;
    m_stepList->setMaximumHeight(110);
    m_stepList->setStyleSheet(R"(
        QListWidget {
            background: rgba(18,18,35,160); color: #d0d0e8;
            font-size: 13px; border: 1px solid #3a3a5a;
            border-radius: 4px;
        }
        QListWidget::item { padding: 3px 8px; border-bottom: 1px solid rgba(58,58,90,30); }
        QListWidget::item:selected { background: rgba(74,158,255,50); color: #ffffff; }
    )");
    mainL->addWidget(m_stepList);

    // ── Stats bar ──
    m_sbInfo = new QLabel("Inodes: 0/50  |  Blocks: 0/502");
    m_sbInfo->setStyleSheet(R"(
        QLabel {
            color: #a0a0c0; font-size: 11px;
            padding: 2px 8px; background: rgba(18,18,40,140);
            border: 1px solid #3a3a5a; border-radius: 3px;
        }
    )");
    mainL->addWidget(m_sbInfo);
}

void VisualizationPanel::resetAll()
{
    m_sceneWidget->clearAllHighlights();
    m_opLabel->setText("Ready");
    m_stepCounter->setText("0 / 0");
    m_progressBar->setValue(0);
    m_explanationLabel->setText("Create a file or folder to begin.");
    m_stepList->clear();
    m_flowChart->reset();
    m_replayBtn->setEnabled(false);
    refreshStats();
}

void VisualizationPanel::highlightSection(const QString &name)
{
    m_sceneWidget->setHighlightedSection(name);
    m_flowChart->setCurrentNode(name);
}

void VisualizationPanel::setFlowNodes(const QStringList &nodes)
{
    m_flowChart->setFlow(nodes);
}

void VisualizationPanel::setCurrentFlowNode(const QString &node)
{
    m_flowChart->setCurrentNode(node);
}

void VisualizationPanel::setCellHighlight(int bt, int idx, const QColor &c)
{
    m_sceneWidget->setCellHighlight(bt, idx, c);
}

void VisualizationPanel::setCellState(const StepEvent &ev)
{
    if (ev.component == "InodeBitmap" && ev.targetIndex >= 0) {
        QColor c = ev.action == "allocate" ? QColor(60, 220, 60) :
                   ev.action == "free" ? QColor(220, 60, 60) : QColor(74, 220, 255);
        m_sceneWidget->setCellHighlight(0, ev.targetIndex, c);
    }
    if (ev.component == "BlockBitmap" && ev.targetIndex >= 0) {
        QColor c = ev.action == "allocate" ? QColor(60, 220, 60) :
                   ev.action == "free" ? QColor(220, 60, 60) : QColor(74, 220, 255);
        m_sceneWidget->setCellHighlight(1, ev.targetIndex, c);
    }
}

void VisualizationPanel::showOperationName(const QString &opName)
{
    QString icon = "";
    if (opName == "create") icon = "CREATE FILE";
    else if (opName == "delete") icon = "DELETE FILE";
    else if (opName == "rename") icon = "RENAME";
    else if (opName == "mkdir") icon = "CREATE DIRECTORY";
    else if (opName == "rmdir") icon = "DELETE DIRECTORY";
    else if (opName == "write") icon = "WRITE FILE";
    else if (opName == "read") icon = "READ FILE";
    else if (opName == "open") icon = "OPEN FILE";
    else if (opName == "close") icon = "CLOSE FILE";
    else if (opName == "truncate") icon = "TRUNCATE";
    else if (opName == "chmod") icon = "CHANGE PERMISSIONS";
    else icon = opName.toUpper();
    m_opLabel->setText("  " + icon);
}

void VisualizationPanel::showStepExplanation(const QString &text, int step, int total)
{
    if (total > 0)
        m_stepCounter->setText(QString("%1 / %2").arg(step).arg(total));
    else
        m_stepCounter->setText(QString("Step %1").arg(step));
    m_progressBar->setRange(0, total > 0 ? total : 1);
    m_progressBar->setValue(step);
    m_explanationLabel->setText(text);

    if (m_stepList->count() < step) {
        new QListWidgetItem(QString("Step %1: %2").arg(step).arg(text.left(60)), m_stepList);
    }
    if (step > 0 && step <= m_stepList->count()) {
        m_stepList->setCurrentRow(step - 1);
    }
    m_stepList->scrollToBottom();
}

void VisualizationPanel::setStepProgress(int percent)
{
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(percent);
}

void VisualizationPanel::completeAll()
{
    m_sceneWidget->clearAllHighlights();
    m_opLabel->setText("  Operation Complete");
    m_stepCounter->setText("Done");
    m_progressBar->setValue(m_progressBar->maximum());
    m_flowChart->reset();
    m_replayBtn->setEnabled(true);
    refreshStats();
}

void VisualizationPanel::appendLog(const QString &text, const QString &color)
{
    QListWidgetItem *item = new QListWidgetItem(" " + text);
    item->setForeground(QColor(color));
    m_stepList->addItem(item);
    m_stepList->scrollToBottom();
}

void VisualizationPanel::refreshStats()
{
    int usedInodes = SUPERBLOCKobj.TotalInodes - SUPERBLOCKobj.FreeInode;
    int usedBlocks = NUM_DATA_BLOCKS - get_free_blocks_count();
    m_sbInfo->setText(QString("Inodes: %1/50  |  Blocks: %2/%3")
        .arg(usedInodes).arg(usedBlocks).arg(NUM_DATA_BLOCKS));
    m_sceneWidget->refreshData();
}

void VisualizationPanel::showSystemInfo(bool show)
{
    m_showInfo = show;
    m_sceneWidget->setVisible(!show);
    m_flowChart->setVisible(!show);
    m_sysInfo->setVisible(show);
    m_infoToggle->setText(show ? "Animation" : "System Info");
    m_infoToggle->setChecked(show);
    m_headerLabel->setText(show ? "CVFS System Architecture" : "CVFS Animation Engine");
    m_opLabel->setVisible(!show);
    m_stepCounter->setVisible(!show);
    m_progressBar->setVisible(!show);
    m_explanationLabel->setVisible(!show);
    m_replayBtn->setVisible(!show);
    m_stepList->setVisible(!show);
    m_sbInfo->setVisible(!show);
}

double VisualizationPanel::speed() const
{
    int idx = m_speedCombo->currentIndex();
    double speeds[] = {0.1, 0.25, 0.5, 1.0, 2.0, 4.0};
    return (idx >= 0 && idx < 6) ? speeds[idx] : 1.0;
}

void VisualizationPanel::setSpeed(double speed)
{
    if (speed <= 0.175) m_speedCombo->setCurrentIndex(0);
    else if (speed <= 0.375) m_speedCombo->setCurrentIndex(1);
    else if (speed <= 0.75) m_speedCombo->setCurrentIndex(2);
    else if (speed <= 1.5) m_speedCombo->setCurrentIndex(3);
    else if (speed <= 3.0) m_speedCombo->setCurrentIndex(4);
    else m_speedCombo->setCurrentIndex(5);
}

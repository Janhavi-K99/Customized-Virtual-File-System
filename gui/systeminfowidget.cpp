#include "systeminfowidget.h"
#include "../persistence.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QToolTip>
#include <cmath>

static QColor withAlpha(const QColor &c, int a) {
    return QColor(c.red(), c.green(), c.blue(), a);
}

SystemInfoWidget::SystemInfoWidget(QWidget *parent)
    : QWidget(parent), m_page(0), m_hotIdx(-1), m_pulse(0)
{
    setMouseTracking(true);
    connect(&m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulse += 0.05f;
        if (m_pulse > 6.2832f) m_pulse -= 6.2832f;
        update();
    });
    m_pulseTimer.start(50);
}

void SystemInfoWidget::setPage(int page)
{
    if (page >= 0 && page < 3) {
        m_page = page;
        m_hotRects.clear();
        update();
    }
}

void SystemInfoWidget::drawNavBar(QPainter &p, int W)
{
    static const char *titles[] = {"Disk Layout", "Data Structures", "How It Works"};
    int bw = 140, bh = 26, gap = 8;
    int totalW = bw * 3 + gap * 2;
    int sx = (W - totalW) / 2, sy = 6;

    m_hotRects.clear();
    p.setFont(QFont("Segoe UI", 11, QFont::Bold));
    for (int i = 0; i < 3; i++) {
        QRect r(sx + i * (bw + gap), sy, bw, bh);
        bool sel = (i == m_page);
        QColor bg = sel ? QColor(74, 158, 255) : QColor(40, 40, 70);
        QColor border = sel ? QColor(100, 180, 255) : QColor(58, 58, 90);
        p.setPen(QPen(border, 1));
        p.setBrush(bg);
        p.drawRoundedRect(r, 4, 4);
        p.setPen(sel ? Qt::white : QColor(160, 160, 190));
        p.drawText(r, Qt::AlignCenter, titles[i]);
        m_hotRects.append({r, titles[i]});
    }
}

void SystemInfoWidget::drawSectionTitle(QPainter &p, int W, const QString &title)
{
    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.setPen(QColor(255, 220, 68));
    QRect tr(20, 40, W - 40, 30);
    p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, title);
    p.setPen(QPen(QColor(74, 158, 255, 80), 1));
    p.drawLine(20, 70, W - 20, 70);
}

void SystemInfoWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(14, 14, 30));

    int W = width(), H = height();
    drawNavBar(p, W);

    switch (m_page) {
    case 0: paintDiskLayout(p, W, H); break;
    case 1: paintDataStructures(p, W, H); break;
    case 2: paintHowItWorks(p, W, H); break;
    }
}

// ─── Page 1: Disk Layout ──────────────────────────────────────
void SystemInfoWidget::paintDiskLayout(QPainter &p, int W, int H)
{
    int W2 = W - 40;
    drawSectionTitle(p, W, "Virtual Disk Layout (512 blocks x 1024 bytes)");

    // Rows of colored block groups
    struct BlockGroup {
        const char *name;
        int start, count;
        QColor color;
        const char *desc;
    };
    BlockGroup groups[] = {
        {"SuperBlock",       0, 1, QColor(220, 80, 80),   "Inodes: 50, Free: varies"},
        {"Block Bitmap",     1, 1, QColor(220, 160, 60),  "Tracks free/used data blocks"},
        {"Inode Bitmap",     2, 1, QColor(60, 180, 100),  "Tracks free/used inodes"},
        {"Inode Table",      3, 4, QColor(60, 140, 220),  "50 inodes, 84 bytes each"},
        {"Data Blocks",      7, 505, QColor(80, 70, 140), "File contents and directories"},
    };
    int nGroups = sizeof(groups) / sizeof(groups[0]);

    int barY = 85, barH = 32, gap = 4;
    int barTotal = W2;
    // Scale: first 7 blocks shown with individual width, data blocks squashed
    int firstBlocksW = 320; // width for first 7 blocks
    int dataW = barTotal - firstBlocksW;
    if (dataW < 200) dataW = 200;

    // Per-block metrics for blocks 0-6
    int bW = firstBlocksW / 7;
    int blkW[7];
    for (int i = 0; i < 7; i++) blkW[i] = bW;
    int blkX[7];
    blkX[0] = 20;
    for (int i = 1; i < 7; i++) blkX[i] = blkX[i-1] + blkW[i-1] + 1;

    m_hotRects.clear();

    // Draw individual blocks 0-6
    for (int i = 0; i < 7; i++) {
        QColor c;
        const char *n = "";
        if (i == 0) { c = groups[0].color; n = "SB"; }
        else if (i == 1) { c = groups[1].color; n = "BB"; }
        else if (i == 2) { c = groups[2].color; n = "IB"; }
        else { c = groups[3].color; n = "IT"; }
        QRect r(blkX[i], barY, blkW[i] - 1, barH);
        p.setPen(QPen(QColor(255,255,255,30), 1));
        p.setBrush(c);
        p.drawRect(r);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 7));
        p.drawText(r, Qt::AlignCenter, n);
        QString tip = QString("Block %1: %2").arg(i).arg(groups[i < 4 ? i : 3].desc);
        m_hotRects.append({r, tip});
    }

    // Data blocks block (7-511) as one wide block
    {
        QRect dr(blkX[6] + blkW[6] + 1, barY, dataW, barH);
        p.setPen(QPen(QColor(255,255,255,30), 1));
        p.setBrush(groups[4].color);
        p.drawRect(dr);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(dr, Qt::AlignCenter, "Data Blocks (505 blocks)");
        m_hotRects.append({dr, "Blocks 7-511: File contents and directory data"});
    }

    // Legend below the bar
    int legY = barY + barH + 12;
    p.setFont(QFont("Segoe UI", 9));
    for (int i = 0; i < nGroups; i++) {
        int lx = 20 + (i % 3) * (W2 / 3);
        int ly = legY + (i / 3) * 22;
        p.fillRect(lx, ly, 12, 12, groups[i].color);
        p.setPen(QColor(180, 180, 200));
        p.drawText(lx + 18, ly + 11, QString("%1: %2").arg(groups[i].name).arg(groups[i].desc));
    }

    // Detailed explanations
    int infoY = legY + 70;
    p.setFont(QFont("Segoe UI", 10));
    p.setPen(QColor(200, 200, 220));

    static const char *details[] = {
        "SuperBlock  (Block 0)   — Stores total inodes (50), free inode count, block count, magic number.",
        "Block Bitmap (Block 1)  — 1 bit per data block. 0 = allocated, 1 = free. 505 entries.",
        "Inode Bitmap (Block 2)  — 1 bit per inode. 0 = allocated, 1 = free. 50 entries.",
        "Inode Table  (Blocks 3-6) — Array of 50 DiskInode records (84 bytes each). ~30 per block.",
        "Data Blocks  (Blocks 7-511) — Stores file contents, directory entries, and inode buffers."
    };
    for (int i = 0; i < 5; i++) {
        p.drawText(24, infoY + i * 20, details[i]);
    }
}

// ─── Page 2: Data Structures ──────────────────────────────────
void SystemInfoWidget::paintDataStructures(QPainter &p, int W, int H)
{
    drawSectionTitle(p, W, "Core Data Structures");

    struct Card {
        const char *name;
        const char *tag;           // "memory" or "disk"
        const char *fields[10];
        const char *desc;
    };

    Card cards[] = {
        {"INODE (incore)", "MEMORY",
            {"FileName[50]  — filename string",
             "InodeNumber   — unique ID (1-50)",
             "FileType      — 0=free, REGULAR=1, DIRECTORY=3",
             "FileSize / FileActualSize",
             "Buffer*       — 2048B data buffer",
             "LinkCount / ReferenceCount",
             "permission    — Read=1, Write=2, RW=3",
             "next*         — next in DILB linked list"},
         "Every file and directory has an incore inode allocated in the DILB. The Buffer holds the "
         "entire file content in memory. FileType 0 marks a free slot."},

        {"UFDT + FILETABLE", "MEMORY",
            {"UFDTArr[0..49]  — file descriptor table",
             "  each slot: ptrfiletable* -> FILETABLE",
             "FILETABLE {",
             "  count     — open-handle reference count",
             "  mode      — READ=1, WRITE=2, RW=3",
             "  readoffset / writeoffset",
             "  ptrinode* -> INODE",
             "}"},
         "The UFDT maps integer file descriptors (fd) to a FileTable that tracks per-open state: "
         "position, mode, and which inode it points to. max 50 concurrent opens."},

        {"DiskInode", "DISK",
            {"i_filename[48]  — padded filename",
             "i_mode          — file type | (perm << 2)",
             "i_size          — exact byte count",
             "i_links_count   — hard link counter",
             "i_blocks_count  — number of data blocks used",
             "i_block[4]      — block pointers",
             "i_reserved[48]  — padding to 128 bytes"},
         "On-disk inode format stored in blocks 3-6 (Inode Table). Each DiskInode is 84 bytes (packed). "
         "Read from disk during mount, written back via sync_inode()."},

        {"DirEntry", "DISK",
            {"d_inode   — 4 bytes, inode number",
             "d_name[59] — filename string",
             "d_type     — REGULAR=1 / DIRECTORY=3"},
         "Directory contents are stored as an array of DirEntry records inside the parent "
         "directory's INODE Buffer. Root (inode 1) starts with '.' and '..'."},
    };

    // column layout
    int leftX = 20, rightX = 20 + (W - 40) / 2 + 8;
    int colW = (W - 40) / 2 - 8;
    int cardX[4] = {leftX, rightX, leftX, rightX};
    int cardY[4];

    // compute card heights first
    int py = 85;
    for (int i = 0; i < 4; i++) {
        cardY[i] = py;
        int titleH = 18;
        int nf = 0;
        while (cards[i].fields[nf] && cards[i].fields[nf][0]) nf++;
        int fieldsH = nf * 14;
        int descH = 34;
        int totalH = 10 + titleH + 4 + fieldsH + 8 + descH + 10;
        py += totalH + 12;
        if (i == 1) py = 85; // reset for right column
    }

    // draw cards
    for (int i = 0; i < 4; i++) {
        int cx = cardX[i], cy = cardY[i], cw = colW;

        // compute height
        int nf = 0;
        while (cards[i].fields[nf] && cards[i].fields[nf][0]) nf++;
        int titleH = 18;
        int fieldsH = nf * 14;
        int descH = 34;
        int ch = 10 + titleH + 4 + fieldsH + 8 + descH + 10;

        // card bg
        {
            QPainterPath path;
            path.addRoundedRect(cx, cy, cw, ch, 6, 6);
            p.setPen(QPen(QColor(60, 60, 100, 50), 1));
            p.setBrush(QColor(18, 18, 44));
            p.drawPath(path);
        }

        // accent bar at top
        {
            QPainterPath bar;
            bar.addRoundedRect(cx + 1, cy + 1, cw - 2, 3, 2, 2);
            QColor accent = (i < 2) ? QColor(74, 158, 255, 120) : QColor(160, 140, 210, 120);
            p.fillPath(bar, accent);
        }

        // tag label
        {
            QColor tagC = (i < 2) ? QColor(74, 158, 255, 160) : QColor(160, 140, 210, 160);
            QColor tagBg = (i < 2) ? QColor(74, 158, 255, 30) : QColor(160, 140, 210, 30);
            QRect tr(cx + cw - 58, cy + 8, 48, 14);
            p.setPen(Qt::NoPen);
            p.setBrush(tagBg);
            p.drawRoundedRect(tr, 3, 3);
            p.setFont(QFont("Segoe UI", 7, QFont::Bold));
            p.setPen(tagC);
            p.drawText(tr, Qt::AlignCenter, cards[i].tag);
        }

        // title
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        p.setPen(QColor(190, 200, 220));
        p.drawText(cx + 12, cy + 24, QString(cards[i].name));

        // separator line
        p.setPen(QPen(QColor(60, 60, 100, 60), 1));
        p.drawLine(cx + 12, cy + 28, cx + cw - 12, cy + 28);

        // fields
        p.setFont(QFont("Segoe UI", 9));
        int fy = cy + 34;
        for (int fi = 0; fi < nf; fi++) {
            bool isIndent = (cards[i].fields[fi][0] == ' ');
            int lx = cx + 12 + (isIndent ? 12 : 0);
            // bullet for top-level fields
            if (!isIndent) {
                p.setPen(QColor(100, 140, 200, 80));
                p.drawEllipse(lx - 2, fy - 4, 4, 4);
            }
            QColor fc = isIndent ? QColor(150, 160, 185) : QColor(195, 205, 220);
            p.setPen(fc);
            p.drawText(lx + 4, fy + 4, cards[i].fields[fi]);
            fy += 14;
        }

        // description
        QRect dr(cx + 10, fy + 2, cw - 20, descH);
        p.setFont(QFont("Segoe UI", 8));
        p.setPen(QColor(120, 130, 165));
        p.drawText(dr, Qt::AlignLeft | Qt::TextWordWrap, QString(cards[i].desc));

        // draw connecting arrow between memory cards and disk cards
        if (i == 1) {
            // arrow from UFDT to DiskInode on right column
            int ax = rightX - 6, ay = cy + ch / 2;
            p.setPen(QPen(QColor(80, 80, 140, 60), 1));
            p.drawLine(ax, ay, ax + 6, ay);
            // small arrow head
            QPainterPath ah;
            ah.moveTo(ax + 6, ay);
            ah.lineTo(ax + 2, ay - 3);
            ah.lineTo(ax + 2, ay + 3);
            ah.closeSubpath();
            p.fillPath(ah, QColor(80, 80, 140, 60));
        }
        if (i == 0) {
            // arrow from INODE to UFDT below
            int ax1 = leftX + colW, ay1 = cy + ch;
            p.setPen(QPen(QColor(80, 80, 140, 60), 1));
            p.drawLine(ax1, ay1, ax1, cardY[1]);
        }
    }
}

// ─── Page 3: How It Works ─────────────────────────────────────
void SystemInfoWidget::paintHowItWorks(QPainter &p, int W, int H)
{
    drawSectionTitle(p, W, "How File Operations Work");

    int y0 = 85;
    p.setFont(QFont("Segoe UI", 9));
    p.setPen(QColor(200, 200, 220));

    struct FlowStep {
        const char *icon;
        const char *text;
        QColor color;
    };

    // Create File flow
    {
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        p.setPen(QColor(100, 220, 140));
        p.drawText(20, y0, "Create File Flow");
        y0 += 22;

        FlowStep steps[] = {
            {"1", "Check SuperBlock — is there a free inode?", QColor(220, 80, 80)},
            {"2", "Scan Inode Bitmap — find first 0 bit → allocate inode #N", QColor(60, 140, 220)},
            {"3", "Initialize incore INODE: set name, type=REGULAR, alloc Buffer (2048B)", QColor(60, 180, 100)},
            {"4", "Find free UFDT slot → allocate FILETABLE → set mode, offsets, ptrinode", QColor(220, 160, 60)},
            {"5", "Add DirEntry to parent directory's Buffer: name→inode #N", QColor(80, 70, 140)},
            {"6", "Synchronize inode, directory, and SuperBlock to disk", QColor(140, 100, 200)},
            {"7", "Return file descriptor (fd) — file is ready!", QColor(100, 220, 140)},
        };

        for (int i = 0; i < 7; i++) {
            int by = y0 + i * 22;
            p.setPen(QPen(steps[i].color, 1));
            p.setBrush(steps[i].color);
            p.drawRoundedRect(20, by, 18, 18, 9, 9);
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            p.drawText(QRect(20, by, 18, 18), Qt::AlignCenter, steps[i].icon);
            p.setFont(QFont("Segoe UI", 9));
            p.setPen(QColor(200, 200, 220));
            p.drawText(44, by + 13, steps[i].text);
        }
        y0 += 7 * 22 + 10;
    }

    // Write File flow
    {
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        p.setPen(QColor(100, 220, 140));
        p.drawText(W / 2 + 10, 85, "Write File Flow");
        int wy0 = 107;

        FlowStep wsteps[] = {
            {"1", "Lookup fd in UFDT → find FILETABLE", QColor(220, 80, 80)},
            {"2", "Check FILETABLE mode permits write", QColor(60, 140, 220)},
            {"3", "Read INODE: current size, buffer ready", QColor(60, 180, 100)},
            {"4", "Write data into INODE.Buffer at writeoffset", QColor(220, 160, 60)},
            {"5", "Advance writeoffset, update FileActualSize", QColor(80, 70, 140)},
            {"6", "Synchronize inode to disk via write_inode()", QColor(140, 100, 200)},
        };

        for (int i = 0; i < 6; i++) {
            int by = wy0 + i * 22;
            p.setPen(QPen(wsteps[i].color, 1));
            p.setBrush(wsteps[i].color);
            p.drawRoundedRect(W / 2 + 10, by, 18, 18, 9, 9);
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            p.drawText(QRect(W / 2 + 10, by, 18, 18), Qt::AlignCenter, wsteps[i].icon);
            p.setFont(QFont("Segoe UI", 9));
            p.setPen(QColor(200, 200, 220));
            p.drawText(W / 2 + 34, by + 13, wsteps[i].text);
        }
    }

    // Bottom note
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(QColor(100, 100, 140));
    p.drawText(20, H - 20, "Each operation triggers step events → animation engine → highlights in scene + flowchart + step log.");
}

void SystemInfoWidget::mouseMoveEvent(QMouseEvent *event)
{
    QPoint pos = event->pos();
    m_hotIdx = -1;
    for (int i = 0; i < m_hotRects.size(); i++) {
        if (m_hotRects[i].r.contains(pos)) {
            m_hotIdx = i;
            setToolTip(m_hotRects[i].tip);
            return;
        }
    }
    setToolTip("");
}

void SystemInfoWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;
    QPoint pos = event->pos();

    // Check nav bar clicks
    int bw = 140, bh = 26, gap = 8;
    int totalW = bw * 3 + gap * 2;
    int sx = (width() - totalW) / 2, sy = 6;
    for (int i = 0; i < 3; i++) {
        QRect r(sx + i * (bw + gap), sy, bw, bh);
        if (r.contains(pos)) {
            setPage(i);
            return;
        }
    }
}

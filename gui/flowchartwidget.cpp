#include "flowchartwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

FlowChartWidget::FlowChartWidget(QWidget *parent)
    : QWidget(parent), m_pulse(0), m_completed(-1)
{
    setMinimumWidth(140);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    buildDefaultFlow();
    connect(&m_pulseTimer, &QTimer::timeout, this, [this]() {
        m_pulse += 0.08f;
        if (m_pulse > 6.28f) m_pulse -= 6.28f;
        update();
    });
    m_pulseTimer.start(40);
}

void FlowChartWidget::buildDefaultFlow()
{
    m_defaultFlow = QStringList()
        << "SuperBlock" << "InodeBitmap" << "InodeTable"
        << "BlockBitmap" << "UFDT" << "FileTable"
        << "Directory" << "DILB" << "DataBlocks" << "DiskSync";
}

void FlowChartWidget::setFlow(const QStringList &nodes)
{
    m_nodes = nodes;
    m_currentNode.clear();
    m_completed = -1;
    update();
}

void FlowChartWidget::setCurrentNode(const QString &node)
{
    m_currentNode = node;
    m_completed = -1;
    for (int i = 0; i < m_nodes.size(); i++) {
        if (m_nodes[i] == node) {
            m_completed = i;
            break;
        }
    }
    update();
}

void FlowChartWidget::reset()
{
    m_nodes = m_defaultFlow;
    m_currentNode.clear();
    m_completed = -1;
    update();
}

static QColor withAlpha(const QColor &c, int a) { return QColor(c.red(), c.green(), c.blue(), a); }

void FlowChartWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int W = width();
    int H = height();

    p.fillRect(rect(), QColor(16, 16, 35, 180));

    if (m_nodes.isEmpty()) {
        p.setPen(QColor("#666688"));
        p.setFont(QFont("Segoe UI", 9));
        p.drawText(rect(), Qt::AlignCenter, "No operation");
        return;
    }

    int n = m_nodes.size();
    if (n == 0) return;

    int boxW = W - 20;
    int boxH = 22;
    int arrowH = 10;
    int totalH = n * boxH + (n - 1) * arrowH;
    int startY = (H - totalH) / 2;
    if (startY < 10) startY = 10;

    float glow = 0.5f + 0.5f * sin(m_pulse * 2);

    QFont nodeFont("Segoe UI", 7, QFont::Bold);
    QFont counterFont("Segoe UI", 6);

    for (int i = 0; i < n; i++) {
        int bx = (W - boxW) / 2;
        int by = startY + i * (boxH + arrowH);

        bool isCurrent = (m_nodes[i] == m_currentNode);
        bool isCompleted = (m_completed >= i);
        bool isFuture = !isCurrent && !isCompleted;

        QColor boxColor;
        QColor textColor;
        QColor borderColor;

        if (isCurrent) {
            boxColor = QColor(74, 158, 255, 180 + int(75 * glow));
            textColor = QColor(255, 255, 255);
            borderColor = QColor(255, 220, 50, 180 + int(75 * glow));
        } else if (isCompleted) {
            boxColor = QColor(50, 180, 50, 120);
            textColor = QColor(180, 255, 180);
            borderColor = QColor(50, 180, 50, 150);
        } else {
            boxColor = QColor(30, 30, 55, 150);
            textColor = QColor(80, 80, 110);
            borderColor = QColor(50, 50, 70, 80);
        }

        QPainterPath bp;
        bp.addRoundedRect(bx, by, boxW, boxH, 4, 4);
        p.fillPath(bp, boxColor);
        p.setPen(QPen(borderColor, isCurrent ? 2 : 1));
        p.drawPath(bp);

        QString label = m_nodes[i];
        QString icon;
        if (label == "SuperBlock") icon = "SB";
        else if (label == "InodeBitmap") icon = "IBmp";
        else if (label == "InodeTable") icon = "ITbl";
        else if (label == "BlockBitmap") icon = "BBmp";
        else if (label == "UFDT") icon = "UFDT";
        else if (label == "FileTable") icon = "FTbl";
        else if (label == "Directory") icon = "Dir";
        else if (label == "DILB") icon = "DILB";
        else if (label == "DataBlocks") icon = "Data";
        else if (label == "DiskSync") icon = "Disk";
        else icon = label.left(5);

        p.setFont(nodeFont);
        p.setPen(textColor);
        p.drawText(QRect(bx + 4, by, boxW - 8, boxH), Qt::AlignVCenter | Qt::AlignLeft, icon);

        p.setFont(counterFont);
        p.setPen(withAlpha(textColor, 120));
        p.drawText(QRect(bx + 4, by, boxW - 8, boxH), Qt::AlignVCenter | Qt::AlignRight, QString::number(i + 1));

        if (isCurrent) {
            float r = boxW * 0.3f + 8 * glow;
            QRadialGradient rg(bx + boxW / 2, by + boxH / 2, r);
            rg.setColorAt(0, withAlpha(QColor(255, 220, 50), 60));
            rg.setColorAt(1, Qt::transparent);
            p.setBrush(rg);
            p.setPen(Qt::NoPen);
            p.drawEllipse(QPointF(bx + boxW / 2, by + boxH / 2), r, r);
        }

        if (i < n - 1) {
            int ax = W / 2;
            int ay = by + boxH;
            int ah = arrowH - 2;

            QColor arrowColor = isCurrent ? QColor(74, 158, 255, 200) : QColor(50, 50, 80, 100);

            p.setPen(QPen(arrowColor, 2));
            p.drawLine(ax, ay, ax, ay + ah);

            QPainterPath arrowHead;
            arrowHead.moveTo(ax - 4, ay + ah - 1);
            arrowHead.lineTo(ax, ay + ah + 3);
            arrowHead.lineTo(ax + 4, ay + ah - 1);
            p.setBrush(arrowColor);
            p.setPen(Qt::NoPen);
            p.drawPath(arrowHead);
        }
    }
}

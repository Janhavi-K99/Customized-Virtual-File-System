#ifndef SYSTEMINFOWIDGET_H
#define SYSTEMINFOWIDGET_H

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QTimer>

class SystemInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SystemInfoWidget(QWidget *parent = nullptr);

    void setPage(int page); // 0=DiskLayout, 1=DataStructures, 2=HowItWorks
    int page() const { return m_page; }
    int pageCount() const { return 3; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void paintDiskLayout(QPainter &p, int W, int H);
    void paintDataStructures(QPainter &p, int W, int H);
    void paintHowItWorks(QPainter &p, int W, int H);
    void drawNavBar(QPainter &p, int W);
    void drawSectionTitle(QPainter &p, int W, const QString &title);

    struct HotRect { QRect r; QString tip; };
    QVector<HotRect> m_hotRects;
    int m_hotIdx;

    int m_page;
    float m_pulse;
    QTimer m_pulseTimer;
};

#endif

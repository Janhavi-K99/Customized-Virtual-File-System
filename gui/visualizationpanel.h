#ifndef VISUALIZATIONPANEL_H
#define VISUALIZATIONPANEL_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidget>
#include <QProgressBar>
#include <QPainter>
#include <QTimer>
#include <QPushButton>
#include <QComboBox>

class FlowChartWidget;
class SystemInfoWidget;

struct StepEvent;

class FsSceneWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FsSceneWidget(QWidget *parent = nullptr);

    void setHighlightedSection(const QString &section);
    void setCellHighlight(int bitmapType, int index, const QColor &color);
    void clearAllHighlights();
    void refreshData();
    QString highlightedSection() const { return m_highlightedSection; }

    void setRelationship(int dirInode, int fileInode, int blockIndex);
    void clearRelationship();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

private:
    struct SectionRect {
        QString name;
        QRect rect;
        QString tooltip;
    };
    QVector<SectionRect> m_sectionRects;

    void paintSuperBlock(QPainter &p, int x, int y, int w, int h);
    void paintUfdt(QPainter &p, int x, int y, int w, int h);
    void paintInodeBitmap(QPainter &p, int x, int y, int w, int h);
    void paintBlockBitmap(QPainter &p, int x, int y, int w, int h);
    void paintInodeTable(QPainter &p, int x, int y, int w, int h);
    void paintDilb(QPainter &p, int x, int y, int w, int h);
    void paintRelationshipLines(QPainter &p, int W, int H);
    void paintOverlay(QPainter &p, int x, int y, int w, int h, const QString &label);
    QColor inodeCellColor(int idx) const;
    QColor blockCellColor(int idx) const;
    void registerSection(const QString &name, const QRect &rect, const QString &tooltip);

    QString m_highlightedSection;
    int m_highlightedCellBitmap;
    int m_highlightedCellIndex;
    QColor m_highlightColor;
    float m_pulse;
    QTimer m_pulseTimer;

    int m_relDirInode;
    int m_relFileInode;
    int m_relBlockIndex;
    float m_relOpacity;
};

class VisualizationPanel : public QWidget
{
    Q_OBJECT
public:
    explicit VisualizationPanel(QWidget *parent = nullptr);

    void resetAll();
    void highlightSection(const QString &name);
    void setCellHighlight(int bt, int idx, const QColor &c);
    void setCellState(const StepEvent &ev);
    void showOperationName(const QString &opName);
    void showStepExplanation(const QString &text, int step, int total);
    void setFlowNodes(const QStringList &nodes);
    void setCurrentFlowNode(const QString &node);
    void setStepProgress(int percent);
    void completeAll();
    void appendLog(const QString &text, const QString &color = "#aaa");
    void refreshStats();
    void showSystemInfo(bool show);

    double speed() const;
    void setSpeed(double speed);

signals:
    void speedChanged(double speed);
    void replayRequested();

private:
    void setupUI();

    QLabel *m_headerLabel;
    QLabel *m_opLabel;
    QLabel *m_stepCounter;
    QProgressBar *m_progressBar;
    QLabel *m_explanationLabel;
    FlowChartWidget *m_flowChart;
    FsSceneWidget *m_sceneWidget;
    SystemInfoWidget *m_sysInfo;
    QListWidget *m_stepList;
    QLabel *m_sbInfo;
    QPushButton *m_replayBtn;
    QPushButton *m_infoToggle;
    QComboBox *m_speedCombo;
    QHBoxLayout *m_controlLayout;
    QHBoxLayout *m_midLayout;
    bool m_showInfo;
};

#endif

#ifndef FLOWCHARTWIDGET_H
#define FLOWCHARTWIDGET_H

#include <QWidget>
#include <QStringList>
#include <QTimer>

class FlowChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FlowChartWidget(QWidget *parent = nullptr);
    void setFlow(const QStringList &nodes);
    void setCurrentNode(const QString &node);
    void reset();
    int nodeCount() const { return m_nodes.size(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QStringList m_nodes;
    QString m_currentNode;
    QStringList m_defaultFlow;
    float m_pulse;
    QTimer m_pulseTimer;
    int m_completed;
    void buildDefaultFlow();
};

#endif

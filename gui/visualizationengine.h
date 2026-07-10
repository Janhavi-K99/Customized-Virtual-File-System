#ifndef VISUALIZATIONENGINE_H
#define VISUALIZATIONENGINE_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVector>
#include <QElapsedTimer>
#include <QVariant>
#include <QQueue>

struct StepEvent
{
    QString component;
    QString action;
    int targetIndex;
    int extraIndex;
    QString operation;
    QString fileName;
    int progressPercent;
    QString explanation;
};

class VisualizationPanel;

class VisualizationEngine : public QObject
{
    Q_OBJECT

public:
    explicit VisualizationEngine(QObject *parent = nullptr);

    void setPanel(VisualizationPanel *panel) { m_panel = panel; }
    VisualizationPanel *panel() const { return m_panel; }

    void onLiveStep(const StepEvent &ev);
    void startOperation(const QString &opName);
    void endOperation(bool success);

    void replay();
    void stop();
    bool isPlaying() const { return m_timer.isActive() || m_replayActive; }
    bool hasRecording() const { return !m_recordedSteps.isEmpty(); }

    double speed() const { return m_speed; }
    void setSpeed(double speed) { m_speed = qBound(0.1, speed, 4.0); }
    QStringList speedOptions() const { return {"0.1x", "0.25x", "0.5x", "1x", "2x", "4x"}; }

signals:
    void playbackFinished();

private slots:
    void onReplayTick();

private:
    void applyStep(const StepEvent &ev, bool isLive);

    VisualizationPanel *m_panel;
    QTimer m_timer;
    QVector<StepEvent> m_recordedSteps;
    int m_replayIndex;
    bool m_replayActive;
    double m_speed;
    int m_baseIntervalMs;
    QString m_currentOperation;
    int m_lastProgress;
    int m_totalSteps;
};

#endif

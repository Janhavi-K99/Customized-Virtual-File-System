#include "visualizationengine.h"
#include "visualizationpanel.h"
#include <QApplication>

VisualizationEngine::VisualizationEngine(QObject *parent)
    : QObject(parent), m_panel(nullptr), m_replayIndex(0),
      m_replayActive(false), m_speed(1.0), m_baseIntervalMs(180), m_lastProgress(0)
{
    connect(&m_timer, &QTimer::timeout, this, &VisualizationEngine::onReplayTick);
}

void VisualizationEngine::onLiveStep(const StepEvent &ev)
{
    if (!m_panel) return;

    if (ev.action == "complete" || ev.progressPercent >= 100) {
        m_recordedSteps.append(ev);
        m_totalSteps = m_recordedSteps.size();
        if (!m_replayActive) {
            applyStep(ev, true);
            m_panel->completeAll();
            m_panel->refreshStats();
        }
        emit playbackFinished();
        return;
    }

    if (m_currentOperation != ev.operation && !ev.operation.isEmpty()) {
        m_currentOperation = ev.operation;
    }

    if (m_recordedSteps.isEmpty() && !ev.operation.isEmpty()) {
        m_panel->showOperationName(ev.operation);
    }

    m_recordedSteps.append(ev);
    m_lastProgress = ev.progressPercent;

    if (!m_replayActive) {
        applyStep(ev, true);
    }
}

void VisualizationEngine::startOperation(const QString &opName)
{
    m_currentOperation = opName;
    m_recordedSteps.clear();
    m_replayIndex = 0;
    m_lastProgress = 0;
    m_totalSteps = 0;
    m_timer.stop();
    m_replayActive = false;

    if (m_panel) {
        m_panel->resetAll();
        m_panel->showOperationName(opName);
    }
}

void VisualizationEngine::endOperation(bool success)
{
    if (m_panel) {
        if (success) {
            m_panel->completeAll();
        }
        m_panel->refreshStats();
    }
    emit playbackFinished();
}

void VisualizationEngine::replay()
{
    if (m_recordedSteps.isEmpty()) return;
    stop();

    m_replayActive = true;
    m_replayIndex = 0;

    if (m_panel) {
        m_panel->resetAll();
        m_panel->showOperationName(m_currentOperation);
    }

    int interval = (int)(m_baseIntervalMs / m_speed);
    if (interval < 20) interval = 20;

    if (m_recordedSteps.size() > 0) {
        applyStep(m_recordedSteps[0], false);
        m_replayIndex = 1;
        if (m_recordedSteps.size() > 1) {
            m_timer.start(interval);
        } else {
            m_replayActive = false;
            if (m_panel) m_panel->completeAll();
            emit playbackFinished();
        }
    }
}

void VisualizationEngine::stop()
{
    m_timer.stop();
    m_replayActive = false;
}

void VisualizationEngine::applyStep(const StepEvent &ev, bool isLive)
{
    if (!m_panel) return;

    int stepNum = isLive ? m_recordedSteps.size() : (m_replayIndex + 1);
    int total = m_totalSteps;

    m_panel->showStepExplanation(ev.explanation, stepNum, total > 0 ? total : stepNum);
    m_panel->highlightSection(ev.component);
    m_panel->setCurrentFlowNode(ev.component);

    if (ev.component == "InodeBitmap" && ev.targetIndex >= 0) {
        QColor c = ev.action == "allocate" ? QColor(60, 220, 60) :
                   ev.action == "free" ? QColor(220, 60, 60) :
                   ev.action == "scan" ? QColor(74, 220, 255) : QColor(100, 180, 255);
        m_panel->setCellHighlight(0, ev.targetIndex, c);
    }
    if (ev.component == "BlockBitmap" && ev.targetIndex >= 0) {
        QColor c = ev.action == "allocate" ? QColor(60, 220, 60) :
                   ev.action == "free" ? QColor(220, 60, 60) : QColor(74, 220, 255);
        m_panel->setCellHighlight(1, ev.targetIndex, c);
    }

    m_panel->setStepProgress(ev.progressPercent);
    m_panel->refreshStats();
}

void VisualizationEngine::onReplayTick()
{
    if (m_replayIndex >= m_recordedSteps.size()) {
        m_timer.stop();
        m_replayActive = false;
        if (m_panel) {
            m_panel->completeAll();
        }
        emit playbackFinished();
        return;
    }

    applyStep(m_recordedSteps[m_replayIndex], false);
    m_replayIndex++;
}

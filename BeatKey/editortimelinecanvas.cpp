#include "editortimelinecanvas.h"
#include "theme.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

EditorTimelineCanvas::EditorTimelineCanvas(QWidget *parent)
    : QWidget(parent)
    , m_bpm(120.0)
    , m_totalDurationMs(120000)
    , m_currentTimeMs(0)
    , m_scrollTimeMs(0)
    , m_selectedNoteIndex(-1)
    , m_activeTool(EditorTool::Place)
    , m_mouseInside(false)
    , m_pixelsPerSecond(150.0)
{
    setMinimumHeight(240);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void EditorTimelineCanvas::setNotes(const QVector<NoteData> &notes)
{
    m_notes = notes;
    update();
}

void EditorTimelineCanvas::setBpm(double bpm)
{
    m_bpm = (bpm > 0.0) ? bpm : 120.0;
    update();
}

void EditorTimelineCanvas::setTotalDuration(int durationMs)
{
    m_totalDurationMs = qMax(1000, durationMs);
    update();
}

void EditorTimelineCanvas::setCurrentTime(int timeMs)
{
    m_currentTimeMs = qBound(0, timeMs, m_totalDurationMs);

    // Auto-scroll to keep playhead in view during playback
    double px = timeToX(m_currentTimeMs);
    if (px > width() - 80) {
        m_scrollTimeMs = qMax(0, m_currentTimeMs - 100);
    } else if (px < 0) {
        m_scrollTimeMs = qMax(0, m_currentTimeMs - 50);
    }

    update();
}

void EditorTimelineCanvas::setActiveTool(EditorTool tool)
{
    m_activeTool = tool;
    update();
}

void EditorTimelineCanvas::setSelectedNoteIndex(int index)
{
    m_selectedNoteIndex = index;
    update();
}

double EditorTimelineCanvas::timeToX(int timeMs) const
{
    return (timeMs - m_scrollTimeMs) * (m_pixelsPerSecond / 1000.0);
}

int EditorTimelineCanvas::xToTime(double x) const
{
    return m_scrollTimeMs + static_cast<int>(x / (m_pixelsPerSecond / 1000.0));
}

int EditorTimelineCanvas::snapTime(int timeMs) const
{
    if (timeMs < 0) return 0;
    double beatMs = (m_bpm > 0.0) ? (60000.0 / m_bpm) : 500.0;
    double snapInterval = beatMs / 4.0; // 1/4 beat snap
    if (snapInterval < 15.0) snapInterval = 50.0;

    int steps = qRound(timeMs / snapInterval);
    return qMax(0, static_cast<int>(steps * snapInterval));
}

int EditorTimelineCanvas::findNoteNear(double x, double tolerancePx) const
{
    int closestIndex = -1;
    double minDiff = tolerancePx;

    for (int i = 0; i < m_notes.size(); ++i) {
        double nx = timeToX(m_notes[i].timeMs);
        double diff = qAbs(nx - x);
        if (diff < minDiff) {
            minDiff = diff;
            closestIndex = i;
        }
    }

    return closestIndex;
}

void EditorTimelineCanvas::drawDiamond(QPainter &painter, double cx, double cy, double radius, bool isSelected, bool isHover)
{
    QPointF pts[4] = {
        QPointF(cx, cy - radius),
        QPointF(cx + radius, cy),
        QPointF(cx, cy + radius),
        QPointF(cx - radius, cy)
    };

    if (isHover) {
        painter.setPen(QPen(QColor(187, 154, 247, 180), 2, Qt::DashLine));
        painter.setBrush(QColor(162, 89, 255, 100));
        painter.drawPolygon(pts, 4);
        return;
    }

    if (isSelected) {
        // Outer halo
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 204, 0, 80));
        painter.drawEllipse(QPointF(cx, cy), radius + 6, radius + 6);
    }

    // Body gradient
    QLinearGradient grad(cx - radius, cy - radius, cx + radius, cy + radius);
    if (isSelected) {
        grad.setColorAt(0.0, QColor(255, 220, 60));
        grad.setColorAt(1.0, QColor(210, 140, 10));
    } else {
        grad.setColorAt(0.0, QColor(162, 89, 255));
        grad.setColorAt(1.0, QColor(92, 43, 184));
    }

    painter.setBrush(grad);
    painter.setPen(QPen(isSelected ? QColor(255, 255, 200) : QColor(255, 255, 255, 230), isSelected ? 3 : 2));
    painter.drawPolygon(pts, 4);

    // Inner diamond core
    double innerR = radius * 0.42;
    QPointF innerPts[4] = {
        QPointF(cx, cy - innerR),
        QPointF(cx + innerR, cy),
        QPointF(cx, cy + innerR),
        QPointF(cx - innerR, cy)
    };
    painter.setPen(Qt::NoPen);
    painter.setBrush(isSelected ? QColor(255, 255, 255, 240) : QColor(230, 210, 255, 200));
    painter.drawPolygon(innerPts, 4);
}

void EditorTimelineCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    const double rulerHeight = 34.0;
    const double trackCenterY = rulerHeight + (h - rulerHeight) / 2.0;

    // 1. Canvas background (lighter palette)
    painter.fillRect(rect(), QColor(28, 20, 48));

    // 2. Track lane background
    QRectF trackRect(0, rulerHeight, w, h - rulerHeight);
    painter.fillRect(trackRect, QColor(24, 16, 42));

    // Center guide line
    painter.setPen(QPen(QColor(85, 52, 130), 1, Qt::DashLine));
    painter.drawLine(0, static_cast<int>(trackCenterY), w, static_cast<int>(trackCenterY));

    // 3. Grid lines (Seconds & Subdivisions)
    int startMs = qMax(0, xToTime(0));
    int endMs = xToTime(w) + 1000;

    int startSec = startMs / 1000;
    int endSec = endMs / 1000;

    // Minor ticks (every 250ms)
    painter.setPen(QPen(QColor(55, 36, 88), 1));
    for (int ms = (startMs / 250) * 250; ms <= endMs; ms += 250) {
        if (ms % 1000 == 0) continue;
        double x = timeToX(ms);
        if (x >= 0 && x <= w) {
            painter.drawLine(QPointF(x, rulerHeight), QPointF(x, h));
        }
    }

    // Major seconds ticks & labels
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    for (int s = startSec; s <= endSec; ++s) {
        int ms = s * 1000;
        double x = timeToX(ms);
        if (x < 0 || x > w) continue;

        // Grid line through track
        painter.setPen(QPen(QColor(95, 62, 148), 1));
        painter.drawLine(QPointF(x, rulerHeight), QPointF(x, h));

        // Ruler mark
        painter.setPen(QPen(QColor(187, 154, 247), 2));
        painter.drawLine(QPointF(x, rulerHeight - 12), QPointF(x, rulerHeight));

        // Seconds label
        int minutes = s / 60;
        int seconds = s % 60;
        QString secText = QStringLiteral("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));

        painter.setPen(QColor(235, 228, 255));
        painter.drawText(QRectF(x - 30, 4, 60, rulerHeight - 14), Qt::AlignCenter, secText);
    }

    // Ruler bar background and bottom border
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRectF(0, 0, w, rulerHeight), QColor(38, 26, 62, 230));
    painter.setPen(QPen(QColor(125, 68, 207), 1));
    painter.drawLine(0, static_cast<int>(rulerHeight), w, static_cast<int>(rulerHeight));

    // 4. Draw placed diamond notes
    for (int i = 0; i < m_notes.size(); ++i) {
        double nx = timeToX(m_notes[i].timeMs);
        if (nx >= -30 && nx <= w + 30) {
            bool selected = (i == m_selectedNoteIndex);
            drawDiamond(painter, nx, trackCenterY, 14.0, selected, false);

            // Small timestamp text under diamond
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
            painter.setPen(selected ? QColor(255, 220, 60) : QColor(187, 175, 215));
            double noteSec = m_notes[i].timeMs / 1000.0;
            QString noteTimeStr = QString::number(noteSec, 'f', 2) + QStringLiteral("с");
            painter.drawText(QRectF(nx - 30, trackCenterY + 16, 60, 16), Qt::AlignCenter, noteTimeStr);
        }
    }

    // 5. Hover preview diamond (if Place tool is active)
    if (m_mouseInside && m_activeTool == EditorTool::Place && m_mousePos.y() >= rulerHeight) {
        int snappedTime = snapTime(xToTime(m_mousePos.x()));
        double hx = timeToX(snappedTime);
        if (hx >= 0 && hx <= w) {
            drawDiamond(painter, hx, trackCenterY, 14.0, false, true);

            // Snapped timestamp text
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
            painter.setPen(QColor(187, 154, 247));
            double snapSec = snappedTime / 1000.0;
            QString snapStr = QString::number(snapSec, 'f', 2) + QStringLiteral("с");
            painter.drawText(QRectF(hx - 30, trackCenterY + 16, 60, 16), Qt::AlignCenter, snapStr);
        }
    }

    // 6. Playhead cursor
    double playheadX = timeToX(m_currentTimeMs);
    if (playheadX >= 0 && playheadX <= w) {
        // Vertical line
        painter.setPen(QPen(QColor(255, 85, 119), 2));
        painter.drawLine(QPointF(playheadX, 0), QPointF(playheadX, h));

        // Playhead diamond tip on the ruler
        QPointF tipPts[4] = {
            QPointF(playheadX, 2),
            QPointF(playheadX + 7, 12),
            QPointF(playheadX, 22),
            QPointF(playheadX - 7, 12)
        };
        painter.setPen(QPen(QColor(255, 255, 255), 1));
        painter.setBrush(QColor(255, 85, 119));
        painter.drawPolygon(tipPts, 4);
    }
}

void EditorTimelineCanvas::mousePressEvent(QMouseEvent *event)
{
    const double rulerHeight = 34.0;

    if (event->button() == Qt::LeftButton) {
        if (event->position().y() < rulerHeight) {
            // Clicked on ruler: seek to time
            int seekTime = qBound(0, xToTime(event->position().x()), m_totalDurationMs);
            emit seekRequested(seekTime);
        } else {
            // Clicked on track area
            if (m_activeTool == EditorTool::Place) {
                int addTime = snapTime(xToTime(event->position().x()));
                emit noteAddRequested(addTime);
            } else {
                // Select tool
                int nearIndex = findNoteNear(event->position().x());
                emit noteSelected(nearIndex);

                if (nearIndex == -1) {
                    int seekTime = qBound(0, xToTime(event->position().x()), m_totalDurationMs);
                    emit seekRequested(seekTime);
                }
            }
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        // Right click: delete note if clicked near one
        int nearIndex = findNoteNear(event->position().x());
        if (nearIndex != -1) {
            emit noteDeleteRequested(nearIndex);
        }
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void EditorTimelineCanvas::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->position();
    m_mouseInside = true;
    update();
    QWidget::mouseMoveEvent(event);
}

void EditorTimelineCanvas::leaveEvent(QEvent *event)
{
    m_mouseInside = false;
    update();
    QWidget::leaveEvent(event);
}

void EditorTimelineCanvas::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    int timeShift = static_cast<int>(-delta * 1.5);
    m_scrollTimeMs = qBound(0, m_scrollTimeMs + timeShift, qMax(0, m_totalDurationMs - 1000));
    update();
    event->accept();
}

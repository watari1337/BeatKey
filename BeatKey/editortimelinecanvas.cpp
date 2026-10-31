#include "editortimelinecanvas.h"
#include "theme.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QtMath>
#include <cmath>
#include <algorithm>

EditorTimelineCanvas::EditorTimelineCanvas(QWidget *parent)
    : QWidget(parent)
    , m_bpm(120.0)
    , m_audioOffsetMs(0)
    , m_quantDivisor(4) // Default 1/4 measure (quarter beat in standard 4/4)
    , m_totalDurationMs(120000)
    , m_currentTimeMs(0)
    , m_scrollTimeMs(0)
    , m_waveform(nullptr)
    , m_activeTool(EditorTool::Place)
    , m_lastClickedIndex(-1)
    , m_mouseInside(false)
    , m_isScrubbingRuler(false)
    , m_isMarqueeSelecting(false)
    , m_isPanning(false)
    , m_panStartMouseX(0.0)
    , m_panStartScrollTime(0)
    , m_dragPending(false)
    , m_isDraggingNotes(false)
    , m_dragAnchorTimeMs(0)
    , m_dragCurrentDeltaMs(0)
    , m_isBrushingPlace(false)
    , m_lastBrushedTimeMs(-1)
    , m_isBrushingErase(false)
    , m_lastEraseTimeMs(-1)
    , m_pixelsPerSecond(180.0)
{
    setMinimumHeight(260);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void EditorTimelineCanvas::setNotes(const QVector<NoteData> &notes)
{
    m_notes = notes;
    // Prune invalid selected indices
    QSet<int> valid;
    for (int idx : m_selectedIndices) {
        if (idx >= 0 && idx < m_notes.size()) {
            valid.insert(idx);
        }
    }
    m_selectedIndices = valid;
    update();
}

void EditorTimelineCanvas::setBpm(double bpm)
{
    m_bpm = qBound(30.0, bpm, 300.0);
    update();
}

void EditorTimelineCanvas::setAudioOffset(int offsetMs)
{
    m_audioOffsetMs = offsetMs;
    update();
}

void EditorTimelineCanvas::setQuantDivisor(int divisor)
{
    m_quantDivisor = qBound(1, divisor, 64);
    update();
}

void EditorTimelineCanvas::setTotalDuration(int durationMs)
{
    m_totalDurationMs = qMax(1000, durationMs);
    updateScrollBounds();
    update();
}

void EditorTimelineCanvas::setCurrentTime(int timeMs)
{
    m_currentTimeMs = qBound(0, timeMs, m_totalDurationMs);
    update();
}

int EditorTimelineCanvas::visibleTimeMs() const
{
    double pxPerMs = m_pixelsPerSecond / 1000.0;
    if (pxPerMs <= 0.0001) return 1000;
    return static_cast<int>(width() / pxPerMs);
}

int EditorTimelineCanvas::maxScrollTime() const
{
    return qMax(0, m_totalDurationMs - visibleTimeMs());
}

void EditorTimelineCanvas::setScrollTime(int timeMs)
{
    int maxScroll = maxScrollTime();
    int newScroll = qBound(0, timeMs, maxScroll);
    if (newScroll != m_scrollTimeMs) {
        m_scrollTimeMs = newScroll;
        emit scrollChanged(m_scrollTimeMs, maxScroll, visibleTimeMs());
        updateHoverInfo();
        update();
    }
}

void EditorTimelineCanvas::setActiveTool(EditorTool tool)
{
    m_activeTool = tool;
    m_dragPending = false;
    m_isDraggingNotes = false;
    m_isBrushingPlace = false;
    m_isBrushingErase = false;
    m_isPanning = false;
    unsetCursor();
    update();
}

void EditorTimelineCanvas::setWaveform(const AudioWaveform *waveform)
{
    if (m_waveform == waveform) {
        update();
        return;
    }
    if (m_waveform) {
        disconnect(m_waveform, &AudioWaveform::waveformReady, this, nullptr);
    }
    m_waveform = waveform;
    if (m_waveform) {
        connect(m_waveform, &AudioWaveform::waveformReady, this, [this]() {
            update();
        });
    }
    update();
}

void EditorTimelineCanvas::setSelectedNoteIndex(int index)
{
    m_selectedIndices.clear();
    if (index >= 0 && index < m_notes.size()) {
        m_selectedIndices.insert(index);
        m_lastClickedIndex = index;
    } else {
        m_lastClickedIndex = -1;
    }
    emit selectionChanged(m_selectedIndices.values().toVector());
    update();
}

void EditorTimelineCanvas::setSelectedNoteIndices(const QSet<int> &indices)
{
    m_selectedIndices = indices;
    emit selectionChanged(m_selectedIndices.values().toVector());
    update();
}

void EditorTimelineCanvas::selectAll()
{
    m_selectedIndices.clear();
    for (int i = 0; i < m_notes.size(); ++i) {
        m_selectedIndices.insert(i);
    }
    emit selectionChanged(m_selectedIndices.values().toVector());
    update();
}

void EditorTimelineCanvas::clearSelection()
{
    m_selectedIndices.clear();
    m_lastClickedIndex = -1;
    emit selectionChanged(QVector<int>());
    update();
}

int EditorTimelineCanvas::primarySelectedIndex() const
{
    if (m_selectedIndices.isEmpty()) return -1;
    if (m_selectedIndices.contains(m_lastClickedIndex)) return m_lastClickedIndex;
    return *m_selectedIndices.begin();
}

double EditorTimelineCanvas::measureDurationMs() const
{
    double b = (m_bpm > 0.0) ? m_bpm : 120.0;
    return 240000.0 / b; // Standard 4/4: 4 beats per measure
}

double EditorTimelineCanvas::quantIntervalMs() const
{
    int div = (m_quantDivisor > 0) ? m_quantDivisor : 4;
    return measureDurationMs() / static_cast<double>(div);
}

int EditorTimelineCanvas::snapTime(int timeMs) const
{
    double interval = quantIntervalMs();
    if (interval < 5.0) interval = 25.0;
    double rel = timeMs - m_audioOffsetMs;
    int steps = qRound(rel / interval);
    int snapped = m_audioOffsetMs + static_cast<int>(steps * interval);
    return qMax(0, snapped);
}

double EditorTimelineCanvas::timeToX(int timeMs) const
{
    return (timeMs - m_scrollTimeMs) * (m_pixelsPerSecond / 1000.0);
}

int EditorTimelineCanvas::xToTime(double x) const
{
    return m_scrollTimeMs + static_cast<int>(x / (m_pixelsPerSecond / 1000.0));
}

void EditorTimelineCanvas::triggerTapFlash(int timeMs)
{
    TapFlash flash;
    flash.timeMs = timeMs;
    flash.alpha = 1.0f;
    flash.radius = 16.0;
    m_tapFlashes.append(flash);
    update();
}

void EditorTimelineCanvas::updateScrollBounds()
{
    int maxScroll = maxScrollTime();
    if (m_scrollTimeMs > maxScroll) {
        m_scrollTimeMs = maxScroll;
    }
    emit scrollChanged(m_scrollTimeMs, maxScroll, visibleTimeMs());
}

void EditorTimelineCanvas::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScrollBounds();
}

int EditorTimelineCanvas::findNoteNear(double x, double y, double tolerancePx) const
{
    int closestIndex = -1;
    double minDistanceSq = tolerancePx * tolerancePx;
    const double rulerH = 36.0;
    const double trackH = height() - rulerH;
    const double trackCenterY = rulerH + trackH / 2.0;

    for (int i = 0; i < m_notes.size(); ++i) {
        int t = m_notes[i].timeMs;
        if (m_isDraggingNotes && m_selectedIndices.contains(i)) {
            t += m_dragCurrentDeltaMs;
        }
        double nx = timeToX(t);
        double dx = nx - x;
        double dy = trackCenterY - y;
        double distSq = dx * dx + dy * dy;
        if (distSq <= minDistanceSq) {
            minDistanceSq = distSq;
            closestIndex = i;
        }
    }

    return closestIndex;
}

void EditorTimelineCanvas::updateHoverInfo()
{
    if (!m_mouseInside) return;
    int hoverMs = qBound(0, xToTime(m_mousePos.x()), m_totalDurationMs);
    double mDur = measureDurationMs();
    int mIdx = qFloor((hoverMs - m_audioOffsetMs) / mDur) + 1;
    double beatInMeasure = ((hoverMs - m_audioOffsetMs - (mIdx - 1) * mDur) / (mDur / 4.0)) + 1.0;
    emit hoverInfoChanged(hoverMs, mIdx, beatInMeasure);
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
        painter.setPen(QPen(QColor(187, 154, 247, 200), 1.5, Qt::DashLine));
        painter.setBrush(QColor(162, 89, 255, 75));
        painter.drawPolygon(pts, 4);
        return;
    }

    if (isSelected) {
        // Crisp golden glow
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 215, 64, 70));
        painter.drawEllipse(QPointF(cx, cy), radius + 5, radius + 5);

        // Selected body gradient: Bright Golden
        QLinearGradient selGrad(cx - radius, cy - radius, cx + radius, cy + radius);
        selGrad.setColorAt(0.0, QColor(255, 235, 110));
        selGrad.setColorAt(1.0, QColor(245, 160, 20));
        painter.setBrush(selGrad);
        painter.setPen(QPen(QColor(255, 255, 255), 2.0));
        painter.drawPolygon(pts, 4);

        // Core highlight
        double innerR = radius * 0.42;
        QPointF innerPts[4] = {
            QPointF(cx, cy - innerR),
            QPointF(cx + innerR, cy),
            QPointF(cx, cy + innerR),
            QPointF(cx - innerR, cy)
        };
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 240));
        painter.drawPolygon(innerPts, 4);
    } else {
        // Normal body gradient: Electric Violet
        QLinearGradient normGrad(cx - radius, cy - radius, cx + radius, cy + radius);
        normGrad.setColorAt(0.0, QColor(175, 110, 255));
        normGrad.setColorAt(1.0, QColor(105, 45, 195));
        painter.setBrush(normGrad);
        painter.setPen(QPen(QColor(225, 200, 255, 230), 1.5));
        painter.drawPolygon(pts, 4);

        // Inner diamond core
        double innerR = radius * 0.40;
        QPointF innerPts[4] = {
            QPointF(cx, cy - innerR),
            QPointF(cx + innerR, cy),
            QPointF(cx, cy + innerR),
            QPointF(cx - innerR, cy)
        };
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(240, 225, 255, 190));
        painter.drawPolygon(innerPts, 4);
    }
}

void EditorTimelineCanvas::drawWaveform(QPainter &painter, int startMs, int endMs, double trackY, double trackH)
{
    if (!m_waveform || !m_waveform->isReady() || m_waveform->peaks().isEmpty()) {
        return;
    }

    const auto &peaks = m_waveform->peaks();
    const int totalBuckets = peaks.size();
    const double centerY = trackY + trackH / 2.0;
    const double maxAmpH = (trackH * 0.42);

    int startBucket = qMax(0, startMs / 10);
    int endBucket = qMin(totalBuckets - 1, (endMs / 10) + 1);

    if (startBucket > endBucket) return;

    double bucketWidthPx = (10.0 * (m_pixelsPerSecond / 1000.0));
    int step = 1;
    if (bucketWidthPx < 1.0) {
        step = qMax(1, static_cast<int>(1.0 / bucketWidthPx));
    }

    for (int b = startBucket; b <= endBucket; b += step) {
        float amp = peaks[b];
        if (step > 1) {
            for (int k = 1; k < step && (b + k) <= endBucket; ++k) {
                if (peaks[b + k] > amp) amp = peaks[b + k];
            }
        }

        if (amp < 0.01f) continue;

        int timeMs = b * 10;
        double x = timeToX(timeMs);
        double barH = amp * maxAmpH;

        QLinearGradient barGrad(x, centerY - barH, x, centerY + barH);
        barGrad.setColorAt(0.0, QColor(140, 80, 240, 140));
        barGrad.setColorAt(0.5, QColor(80, 225, 255, 180));
        barGrad.setColorAt(1.0, QColor(140, 80, 240, 140));

        painter.setPen(QPen(barGrad, qMax(1.0, bucketWidthPx * step)));
        painter.drawLine(QPointF(x, centerY - barH), QPointF(x, centerY + barH));
    }
}

void EditorTimelineCanvas::drawGrid(QPainter &painter, int startMs, int endMs, int w, int h, double rulerH, double trackY)
{
    Q_UNUSED(trackY);
    double interval = quantIntervalMs();
    if (interval < 5.0) interval = 25.0;

    double measureDur = measureDurationMs();

    // 1. Highlight active measure/beat
    int inspectTime = (m_mouseInside && m_mousePos.y() >= rulerH) ? xToTime(m_mousePos.x()) : m_currentTimeMs;
    int activeMeasureIdx = qFloor((inspectTime - m_audioOffsetMs) / measureDur);
    double activeMeasureStart = m_audioOffsetMs + activeMeasureIdx * measureDur;
    double activeMeasureEnd = activeMeasureStart + measureDur;

    double hxStart = timeToX(static_cast<int>(activeMeasureStart));
    double hxEnd = timeToX(static_cast<int>(activeMeasureEnd));
    if (hxEnd >= 0 && hxStart <= w) {
        QRectF activeMeasureRect(hxStart, rulerH, hxEnd - hxStart, h - rulerH);
        painter.fillRect(activeMeasureRect, QColor(162, 89, 255, 20));
        painter.setPen(QPen(QColor(162, 89, 255, 60), 1, Qt::DashLine));
        painter.drawRect(activeMeasureRect);
    }

    // 2. Iterate through grid lines in visible range
    int kStart = qFloor((startMs - m_audioOffsetMs) / interval) - 1;
    int kEnd = qCeil((endMs - m_audioOffsetMs) / interval) + 1;

    for (int k = kStart; k <= kEnd; ++k) {
        int t = qRound(m_audioOffsetMs + k * interval);
        if (t < 0) continue;

        double x = timeToX(t);
        if (x < -20 || x > w + 20) continue;

        bool isMeasureBoundary = (m_quantDivisor > 0) && (k % m_quantDivisor == 0);
        bool isBeat = false;
        if (!isMeasureBoundary && m_quantDivisor >= 4) {
            int beatStep = m_quantDivisor / 4;
            if (beatStep > 0 && (k % beatStep == 0)) {
                isBeat = true;
            }
        }

        if (isMeasureBoundary) {
            // Measure line: Bright solid line
            painter.setPen(QPen(QColor(255, 255, 255, 220), 2));
            painter.drawLine(QPointF(x, rulerH), QPointF(x, h));

            // Ruler measure mark & label
            painter.setPen(QPen(QColor(255, 255, 255), 2));
            painter.drawLine(QPointF(x, rulerH - 14), QPointF(x, rulerH));

            int measureNumber = (k / m_quantDivisor) + 1;
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
            painter.setPen(QColor(255, 255, 255));
            painter.drawText(QRectF(x - 30, 4, 60, rulerH - 16), Qt::AlignCenter, QStringLiteral("M%1").arg(measureNumber));
        } else if (isBeat) {
            // Quarter beat line
            painter.setPen(QPen(QColor(130, 170, 255, 170), 1.5));
            painter.drawLine(QPointF(x, rulerH), QPointF(x, h));

            painter.setPen(QPen(QColor(130, 170, 255, 170), 1.5));
            painter.drawLine(QPointF(x, rulerH - 8), QPointF(x, rulerH));
        } else {
            // Subdivision lines
            QColor subCol = (m_quantDivisor == 8 || m_quantDivisor == 16)
                ? QColor(255, 117, 151, 100)
                : QColor(255, 204, 0, 90);
            painter.setPen(QPen(subCol, 1, Qt::DotLine));
            painter.drawLine(QPointF(x, rulerH), QPointF(x, h));

            painter.setPen(QPen(subCol, 1));
            painter.drawLine(QPointF(x, rulerH - 4), QPointF(x, rulerH));
        }
    }

    // Major seconds marks and timecode labels on ruler
    int secStart = qMax(0, startMs / 1000);
    int secEnd = (endMs / 1000) + 1;

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 9));
    for (int s = secStart; s <= secEnd; ++s) {
        int ms = s * 1000;
        double x = timeToX(ms);
        if (x < 0 || x > w) continue;

        painter.setPen(QPen(QColor(187, 154, 247, 180), 1));
        painter.drawLine(QPointF(x, rulerH - 6), QPointF(x, rulerH));

        int minutes = s / 60;
        int seconds = s % 60;
        QString timeText = QStringLiteral("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'));

        painter.setPen(QColor(187, 154, 247, 210));
        painter.drawText(QRectF(x - 26, rulerH - 18, 52, 14), Qt::AlignCenter, timeText);
    }
}

void EditorTimelineCanvas::drawPlayhead(QPainter &painter, int w, int h, double rulerH)
{
    Q_UNUSED(w);
    double playheadX = timeToX(m_currentTimeMs);

    // Playhead vertical line
    painter.setPen(QPen(QColor(255, 85, 119), 2));
    painter.drawLine(QPointF(playheadX, 0), QPointF(playheadX, h));

    // Playhead diamond head on ruler
    double rTip = 8.0;
    QPointF tipPts[4] = {
        QPointF(playheadX, 2),
        QPointF(playheadX + rTip, rulerH * 0.5),
        QPointF(playheadX, rulerH - 2),
        QPointF(playheadX - rTip, rulerH * 0.5)
    };
    painter.setPen(QPen(QColor(255, 255, 255), 1.5));
    painter.setBrush(QColor(255, 85, 119));
    painter.drawPolygon(tipPts, 4);
}

void EditorTimelineCanvas::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int w = width();
    const int h = height();
    const double rulerH = 36.0;
    const double trackH = h - rulerH;
    const double trackY = rulerH;
    const double trackCenterY = trackY + trackH / 2.0;

    // 1. Overall canvas background with translucency to reveal editor background art
    painter.fillRect(rect(), QColor(20, 13, 34, 160));

    // 2. Track area background
    QRectF trackRect(0, trackY, w, trackH);
    painter.fillRect(trackRect, QColor(16, 10, 28, 140));

    // Center guide line
    painter.setPen(QPen(QColor(125, 68, 207, 120), 1, Qt::DashLine));
    painter.drawLine(0, static_cast<int>(trackCenterY), w, static_cast<int>(trackCenterY));

    // Visible time bounds
    int startMs = qMax(0, xToTime(0) - 200);
    int endMs = xToTime(w) + 200;

    // 3. Audio Waveform layer
    drawWaveform(painter, startMs, endMs, trackY, trackH);

    // 4. Measure & Quantization Beat Grid
    drawGrid(painter, startMs, endMs, w, h, rulerH, trackY);

    // 5. Ruler top header bar
    painter.setPen(Qt::NoPen);
    painter.fillRect(QRectF(0, 0, w, rulerH), QColor(28, 17, 48, 200));
    painter.setPen(QPen(QColor(125, 68, 207), 1));
    painter.drawLine(0, static_cast<int>(rulerH), w, static_cast<int>(rulerH));

    // 6. Draw Tap Record Flashes
    for (int i = m_tapFlashes.size() - 1; i >= 0; --i) {
        auto &f = m_tapFlashes[i];
        double fx = timeToX(f.timeMs);
        if (fx >= -40 && fx <= w + 40) {
            painter.setPen(QPen(QColor(80, 225, 255, static_cast<int>(255 * f.alpha)), 2));
            painter.setBrush(QColor(80, 225, 255, static_cast<int>(60 * f.alpha)));
            painter.drawEllipse(QPointF(fx, trackCenterY), f.radius, f.radius);
        }
        f.radius += 2.0;
        f.alpha -= 0.08f;
        if (f.alpha <= 0.0f) {
            m_tapFlashes.removeAt(i);
        }
    }

    // 7. Draw Notes
    for (int i = 0; i < m_notes.size(); ++i) {
        int noteTime = m_notes[i].timeMs;
        if (m_isDraggingNotes && m_selectedIndices.contains(i)) {
            noteTime += m_dragCurrentDeltaMs;
        }

        double nx = timeToX(noteTime);
        if (nx >= -30 && nx <= w + 30) {
            bool selected = m_selectedIndices.contains(i);
            drawDiamond(painter, nx, trackCenterY, 14.0, selected, false);

            // Small timestamp caption
            painter.setFont(QFont(QStringLiteral("Segoe UI"), 8));
            painter.setPen(selected ? QColor(255, 220, 60) : QColor(187, 175, 215));
            double noteSec = noteTime / 1000.0;
            QString noteTimeStr = QString::number(noteSec, 'f', 2) + QStringLiteral("с");
            painter.drawText(QRectF(nx - 30, trackCenterY + 16, 60, 16), Qt::AlignCenter, noteTimeStr);
        }
    }

    // 8. Hover note preview (if Place tool is active and not brushing/dragging)
    if (m_mouseInside && m_activeTool == EditorTool::Place && m_mousePos.y() >= rulerH && !m_isDraggingNotes && !m_isMarqueeSelecting && !m_isBrushingPlace && !m_isPanning) {
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

    // 9. Marquee Selection Box
    if (m_isMarqueeSelecting) {
        QRectF box = QRectF(m_marqueeStart, m_marqueeCurrent).normalized();
        painter.setPen(QPen(QColor(187, 154, 247, 220), 1.5, Qt::DashLine));
        painter.setBrush(QColor(162, 89, 255, 45));
        painter.drawRect(box);
    }

    // 10. Playhead Cursor
    drawPlayhead(painter, w, h, rulerH);
}

void EditorTimelineCanvas::mousePressEvent(QMouseEvent *event)
{
    const double rulerH = 36.0;

    // Middle-mouse-button (MMB) panning
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_panStartMouseX = event->position().x();
        m_panStartScrollTime = m_scrollTimeMs;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (event->position().y() < rulerH) {
            // Click on ruler: seek playhead
            m_isScrubbingRuler = true;
            int seekTime = qBound(0, xToTime(event->position().x()), m_totalDurationMs);
            emit seekRequested(seekTime);
            event->accept();
            return;
        }

        // Track area click: 2D hit test against note diamonds
        int clickedIndex = findNoteNear(event->position().x(), event->position().y(), 18.0);
        bool ctrlHeld = event->modifiers() & Qt::ControlModifier;
        bool shiftHeld = event->modifiers() & Qt::ShiftModifier;

        if (clickedIndex != -1) {
            // Clicked directly on an existing note
            if (ctrlHeld) {
                if (m_selectedIndices.contains(clickedIndex)) {
                    m_selectedIndices.remove(clickedIndex);
                } else {
                    m_selectedIndices.insert(clickedIndex);
                }
                m_lastClickedIndex = clickedIndex;
            } else if (shiftHeld && m_lastClickedIndex != -1) {
                int low = qMin(m_lastClickedIndex, clickedIndex);
                int high = qMax(m_lastClickedIndex, clickedIndex);
                for (int i = low; i <= high; ++i) {
                    m_selectedIndices.insert(i);
                }
            } else {
                if (!m_selectedIndices.contains(clickedIndex)) {
                    m_selectedIndices.clear();
                    m_selectedIndices.insert(clickedIndex);
                    m_lastClickedIndex = clickedIndex;
                }
            }
            emit selectionChanged(m_selectedIndices.values().toVector());

            // Prepare pending drag (only moves if dragged past threshold)
            m_dragPending = true;
            m_dragStartPos = event->position();
            m_dragAnchorTimeMs = m_notes[clickedIndex].timeMs;
            m_dragCurrentDeltaMs = 0;
            m_dragOriginalTimes.clear();
            for (int idx : m_selectedIndices) {
                m_dragOriginalTimes.append(m_notes[idx].timeMs);
            }

            update();
            event->accept();
            return;
        }

        // Clicked on empty track space (or above/below notes)
        if (m_activeTool == EditorTool::Place && !ctrlHeld && !shiftHeld) {
            // Start continuous brush placement
            int addTime = snapTime(xToTime(event->position().x()));
            m_isBrushingPlace = true;
            m_lastBrushedTimeMs = addTime;
            m_brushSessionAddedNotes.clear();

            // Check if note already exists at this snap tick
            bool exists = false;
            for (int i = 0; i < m_notes.size(); ++i) {
                if (qAbs(m_notes[i].timeMs - addTime) <= 15) {
                    exists = true;
                    setSelectedNoteIndex(i);
                    break;
                }
            }
            if (!exists) {
                NoteData nd;
                nd.timeMs = addTime;
                nd.type = QStringLiteral("single");
                m_notes.append(nd);
                std::sort(m_notes.begin(), m_notes.end(), [](const NoteData &a, const NoteData &b) {
                    return a.timeMs < b.timeMs;
                });
                m_brushSessionAddedNotes.append(nd);
                for (int i = 0; i < m_notes.size(); ++i) {
                    if (m_notes[i].timeMs == addTime) {
                        setSelectedNoteIndex(i);
                        break;
                    }
                }
            }

            update();
            event->accept();
            return;
        }

        // Tool 1 (Select) or Ctrl clicked on empty space: begin Marquee selection immediately!
        m_isMarqueeSelecting = true;
        m_marqueeStart = event->position();
        m_marqueeCurrent = event->position();
        if (!ctrlHeld) {
            m_selectedIndices.clear();
            m_lastClickedIndex = -1;
            emit selectionChanged(QVector<int>());
        }

        int seekTime = qBound(0, xToTime(event->position().x()), m_totalDurationMs);
        emit seekRequested(seekTime);

        update();
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        // Continuous brush eraser start
        m_isBrushingErase = true;
        m_brushSessionErasedNotes.clear();
        m_lastEraseTimeMs = xToTime(event->position().x());

        int clickedIndex = findNoteNear(event->position().x(), event->position().y(), 20.0);
        if (clickedIndex != -1) {
            m_brushSessionErasedNotes.append(m_notes[clickedIndex]);
            m_notes.removeAt(clickedIndex);
            clearSelection();
            update();
        }
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void EditorTimelineCanvas::mouseMoveEvent(QMouseEvent *event)
{
    m_mousePos = event->position();

    if (!rect().contains(event->pos())) {
        m_mouseInside = false;
        if (m_isBrushingPlace) {
            m_isBrushingPlace = false;
            if (!m_brushSessionAddedNotes.isEmpty()) {
                emit notesBatchAddRequested(m_brushSessionAddedNotes);
                m_brushSessionAddedNotes.clear();
            }
        }
        if (m_isBrushingErase) {
            m_isBrushingErase = false;
            if (!m_brushSessionErasedNotes.isEmpty()) {
                emit notesBatchDeleteRequested(m_brushSessionErasedNotes);
                m_brushSessionErasedNotes.clear();
            }
        }
        update();
        if (!m_isPanning) {
            return;
        }
    } else {
        m_mouseInside = true;
    }

    // MMB Panning
    if (m_isPanning) {
        double dx = event->position().x() - m_panStartMouseX;
        int timeShift = static_cast<int>(dx / (m_pixelsPerSecond / 1000.0));
        setScrollTime(m_panStartScrollTime - timeShift);
        event->accept();
        return;
    }

    // Hover info computation
    updateHoverInfo();

    // Scrubbing ruler
    if (m_isScrubbingRuler) {
        int seekTime = qBound(0, xToTime(event->position().x()), m_totalDurationMs);
        emit seekRequested(seekTime);
        update();
        event->accept();
        return;
    }

    // Continuous brush placement: zero-gap interpolation across all grid ticks
    if (m_isBrushingPlace) {
        int currentSnapTime = snapTime(xToTime(event->position().x()));
        if (currentSnapTime >= 0 && currentSnapTime <= m_totalDurationMs) {
            int tMin = qMin(m_lastBrushedTimeMs, currentSnapTime);
            int tMax = qMax(m_lastBrushedTimeMs, currentSnapTime);
            double interval = quantIntervalMs();
            if (interval < 5.0) interval = 25.0;

            int kStart = qFloor((tMin - m_audioOffsetMs) / interval);
            int kEnd = qCeil((tMax - m_audioOffsetMs) / interval);
            bool addedAny = false;

            for (int k = kStart; k <= kEnd; ++k) {
                int tick = qRound(m_audioOffsetMs + k * interval);
                if (tick < 0 || tick > m_totalDurationMs) continue;
                if (tick < tMin || tick > tMax) continue;

                // Deduplicate check
                bool exists = false;
                for (const auto &n : m_notes) {
                    if (qAbs(n.timeMs - tick) <= 15) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) {
                    for (const auto &n : m_brushSessionAddedNotes) {
                        if (qAbs(n.timeMs - tick) <= 15) {
                            exists = true;
                            break;
                        }
                    }
                }

                if (!exists) {
                    NoteData nd;
                    nd.timeMs = tick;
                    nd.type = QStringLiteral("single");
                    m_notes.append(nd);
                    m_brushSessionAddedNotes.append(nd);
                    addedAny = true;
                }
            }

            if (addedAny) {
                std::sort(m_notes.begin(), m_notes.end(), [](const NoteData &a, const NoteData &b) {
                    return a.timeMs < b.timeMs;
                });
            }
            m_lastBrushedTimeMs = currentSnapTime;
        }
        update();
        event->accept();
        return;
    }

    // Continuous brush eraser: delete notes across mouse trajectory
    if (m_isBrushingErase) {
        int curTime = xToTime(event->position().x());
        int tMin = qMin(m_lastEraseTimeMs, curTime) - 20;
        int tMax = qMax(m_lastEraseTimeMs, curTime) + 20;
        const double rulerH = 36.0;
        const double trackCenterY = rulerH + (height() - rulerH) / 2.0;

        if (qAbs(event->position().y() - trackCenterY) < 32.0) {
            bool removedAny = false;
            for (int i = m_notes.size() - 1; i >= 0; --i) {
                if (m_notes[i].timeMs >= tMin && m_notes[i].timeMs <= tMax) {
                    m_brushSessionErasedNotes.append(m_notes[i]);
                    m_notes.removeAt(i);
                    removedAny = true;
                }
            }
            if (removedAny) {
                clearSelection();
            }
        }
        m_lastEraseTimeMs = curTime;
        update();
        event->accept();
        return;
    }

    // Drag threshold check
    if (m_dragPending) {
        if ((event->position() - m_dragStartPos).manhattanLength() >= QApplication::startDragDistance()) {
            m_isDraggingNotes = true;
            m_dragPending = false;
        }
    }

    // Dragging notes with Snap to Grid
    if (m_isDraggingNotes) {
        int currentSnapTime = snapTime(xToTime(event->position().x()));
        int initialSnapTime = snapTime(xToTime(m_dragStartPos.x()));
        int rawDelta = currentSnapTime - initialSnapTime;

        // Prevent moving any note below 0 ms
        int minOrig = 99999999;
        for (int t : m_dragOriginalTimes) {
            if (t < minOrig) minOrig = t;
        }
        if (minOrig + rawDelta < 0) {
            rawDelta = -minOrig;
        }

        m_dragCurrentDeltaMs = rawDelta;
        update();
        event->accept();
        return;
    }

    // Marquee selection
    if (m_isMarqueeSelecting) {
        m_marqueeCurrent = event->position();
        QRectF box = QRectF(m_marqueeStart, m_marqueeCurrent).normalized();
        const double rulerH = 36.0;
        const double trackCenterY = rulerH + (height() - rulerH) / 2.0;

        for (int i = 0; i < m_notes.size(); ++i) {
            double nx = timeToX(m_notes[i].timeMs);
            if (box.contains(QPointF(nx, trackCenterY))) {
                m_selectedIndices.insert(i);
            }
        }
        emit selectionChanged(m_selectedIndices.values().toVector());
        update();
        event->accept();
        return;
    }

    update();
    QWidget::mouseMoveEvent(event);
}

void EditorTimelineCanvas::mouseReleaseEvent(QMouseEvent *event)
{
    // MMB Panning release
    if (m_isPanning && event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    m_isScrubbingRuler = false;

    // Brush placement completed: emit batch command to QUndoStack
    if (m_isBrushingPlace) {
        m_isBrushingPlace = false;
        if (!m_brushSessionAddedNotes.isEmpty()) {
            emit notesBatchAddRequested(m_brushSessionAddedNotes);
            m_brushSessionAddedNotes.clear();
        }
    }

    // Continuous eraser completed: emit batch command to QUndoStack
    if (m_isBrushingErase) {
        m_isBrushingErase = false;
        if (!m_brushSessionErasedNotes.isEmpty()) {
            emit notesBatchDeleteRequested(m_brushSessionErasedNotes);
            m_brushSessionErasedNotes.clear();
        }
    }

    m_dragPending = false;

    // Drag notes completed
    if (m_isDraggingNotes) {
        m_isDraggingNotes = false;
        if (m_dragCurrentDeltaMs != 0 && !m_dragOriginalTimes.isEmpty()) {
            emit notesMoved(m_dragOriginalTimes, m_dragCurrentDeltaMs);
        }
        m_dragCurrentDeltaMs = 0;
        m_dragOriginalTimes.clear();
        update();
        event->accept();
        return;
    }

    // Marquee selection completed
    if (m_isMarqueeSelecting) {
        m_isMarqueeSelecting = false;
        update();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void EditorTimelineCanvas::leaveEvent(QEvent *event)
{
    m_mouseInside = false;
    if (m_isBrushingPlace) {
        m_isBrushingPlace = false;
        if (!m_brushSessionAddedNotes.isEmpty()) {
            emit notesBatchAddRequested(m_brushSessionAddedNotes);
            m_brushSessionAddedNotes.clear();
        }
    }
    if (m_isBrushingErase) {
        m_isBrushingErase = false;
        if (!m_brushSessionErasedNotes.isEmpty()) {
            emit notesBatchDeleteRequested(m_brushSessionErasedNotes);
            m_brushSessionErasedNotes.clear();
        }
    }
    if (m_isPanning) {
        m_isPanning = false;
        unsetCursor();
    }
    m_mouseInside = false;
    update();
    QWidget::leaveEvent(event);
}

void EditorTimelineCanvas::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    if (delta == 0) delta = event->angleDelta().x();
    if (delta == 0) return;

    if (event->modifiers() & Qt::ControlModifier) {
        // Zooming in/out centered around mouse cursor position (Card 12)
        double mouseX = event->position().x();
        int timeUnderMouse = xToTime(mouseX);

        double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);
        double newPixelsPerSecond = qBound(40.0, m_pixelsPerSecond * factor, 1200.0);

        if (qAbs(newPixelsPerSecond - m_pixelsPerSecond) > 0.1) {
            m_pixelsPerSecond = newPixelsPerSecond;
            m_scrollTimeMs = qMax(0, timeUnderMouse - static_cast<int>(mouseX / (m_pixelsPerSecond / 1000.0)));
            updateScrollBounds();
            emit zoomChanged(m_pixelsPerSecond);
            updateHoverInfo();
            update();
        }
        event->accept();
        return;
    }

    // Horizontal timeline scrolling scaled dynamically with zoom:
    // Moves a consistent screen pixel distance per wheel tick,
    // so zooming out allows rapidly traversing the track.
    // Shift modifier provides 2x speed boost for rapid navigation.
    double pxPerMs = m_pixelsPerSecond / 1000.0;
    if (pxPerMs < 0.001) pxPerMs = 0.001;

    double pixelsToScroll = -delta * 0.55;
    if (event->modifiers() & Qt::ShiftModifier) {
        pixelsToScroll *= 2.0;
    }

    int timeShift = static_cast<int>(std::round(pixelsToScroll / pxPerMs));
    setScrollTime(m_scrollTimeMs + timeShift);
    updateHoverInfo();
    event->accept();
}
#ifndef EDITORTIMELINECANVAS_H
#define EDITORTIMELINECANVAS_H

#include <QWidget>
#include <QVector>
#include <QSet>
#include <QPointF>
#include <QRectF>

#include "beatmap.h"
#include "audiowaveform.h"

enum class EditorTool {
    Select = 0,
    Place = 1
};

/**
 * @brief EditorTimelineCanvas renders the interactive rhythmic track with:
 * - 2D distance-checked hit testing (allows marquee selection above/below notes)
 * - Continuous brush placement with zero frame-skip gaps
 * - Continuous brush eraser (hold RMB and drag)
 * - Middle-mouse-button (MMB) panning
 * - Multi-selection and Snap-to-Grid dragging
 * - Quantization grid (1/1 to 1/64)
 * - Audio waveform background
 */
class EditorTimelineCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit EditorTimelineCanvas(QWidget *parent = nullptr);
    ~EditorTimelineCanvas() override = default;

    // Data setters
    void setNotes(const QVector<NoteData> &notes);
    void setBpm(double bpm);
    void setAudioOffset(int offsetMs);
    void setQuantDivisor(int divisor); // 1 to 64
    void setTotalDuration(int durationMs);
    void setCurrentTime(int timeMs);
    void setScrollTime(int timeMs);
    void setActiveTool(EditorTool tool);
    void setWaveform(const AudioWaveform *waveform);

    // Selection
    void setSelectedNoteIndex(int index);
    void setSelectedNoteIndices(const QSet<int> &indices);
    void selectAll();
    void clearSelection();
    const QSet<int>& selectedNoteIndices() const { return m_selectedIndices; }
    int primarySelectedIndex() const;

    // Getters
    double bpm() const { return m_bpm; }
    int audioOffset() const { return m_audioOffsetMs; }
    int quantDivisor() const { return m_quantDivisor; }
    int currentTime() const { return m_currentTimeMs; }
    int scrollTime() const { return m_scrollTimeMs; }
    int totalDuration() const { return m_totalDurationMs; }
    double pixelsPerSecond() const { return m_pixelsPerSecond; }
    EditorTool activeTool() const { return m_activeTool; }
    int snapTime(int timeMs) const;
    double quantIntervalMs() const;
    double measureDurationMs() const;

    // Visual feedback
    void triggerTapFlash(int timeMs);

    // Helpers
    double timeToX(int timeMs) const;
    int xToTime(double x) const;

signals:
    void seekRequested(int timeMs);
    void noteAddRequested(int timeMs);
    void notesBatchAddRequested(const QVector<NoteData> &notes);
    void noteDeleteRequested(int noteIndex);
    void notesDeleteRequested(const QVector<int> &indices);
    void notesBatchDeleteRequested(const QVector<NoteData> &notes);
    void notesMoved(const QVector<int> &originalTimes, int deltaMs);
    void selectionChanged(const QVector<int> &selectedIndices);
    void scrollChanged(int scrollTimeMs, int totalDurationMs, int visibleMs);
    void zoomChanged(double pixelsPerSecond);
    void hoverInfoChanged(int timeMs, int measure, double beatInMeasure);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int findNoteNear(double x, double y, double tolerancePx = 18.0) const;
    void drawDiamond(QPainter &painter, double cx, double cy, double radius, bool isSelected, bool isHover);
    void drawWaveform(QPainter &painter, int startMs, int endMs, double trackY, double trackH);
    void drawGrid(QPainter &painter, int startMs, int endMs, int w, int h, double rulerH, double trackY);
    void drawPlayhead(QPainter &painter, int w, int h, double rulerH);
    void updateScrollBounds();

    // Data
    QVector<NoteData> m_notes;
    double m_bpm;
    int m_audioOffsetMs;
    int m_quantDivisor; // 1 to 64
    int m_totalDurationMs;
    int m_currentTimeMs;
    int m_scrollTimeMs;

    // Waveform
    const AudioWaveform *m_waveform;

    // Tools & Selection
    EditorTool m_activeTool;
    QSet<int> m_selectedIndices;
    int m_lastClickedIndex;

    // Mouse interactions
    QPointF m_mousePos;
    bool m_mouseInside;
    bool m_isScrubbingRuler;
    bool m_isMarqueeSelecting;
    QPointF m_marqueeStart;
    QPointF m_marqueeCurrent;

    // Middle-mouse-button (MMB) canvas panning
    bool m_isPanning;
    double m_panStartMouseX;
    int m_panStartScrollTime;

    // Drag-and-drop notes with threshold & live snap
    bool m_dragPending;
    bool m_isDraggingNotes;
    QPointF m_dragStartPos;
    int m_dragAnchorTimeMs;
    int m_dragCurrentDeltaMs;
    QVector<int> m_dragOriginalTimes;

    // Continuous brush placement (hold LMB and drag)
    bool m_isBrushingPlace;
    int m_lastBrushedTimeMs;
    QVector<NoteData> m_brushSessionAddedNotes;

    // Continuous brush eraser (hold RMB and drag)
    bool m_isBrushingErase;
    int m_lastEraseTimeMs;
    QVector<NoteData> m_brushSessionErasedNotes;

    // Visual animation
    struct TapFlash {
        int timeMs;
        float alpha;
        double radius;
    };
    QVector<TapFlash> m_tapFlashes;

    // Viewport
    double m_pixelsPerSecond;
};

#endif // EDITORTIMELINECANVAS_H

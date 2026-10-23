#ifndef EDITORTIMELINECANVAS_H
#define EDITORTIMELINECANVAS_H

#include <QWidget>
#include <QVector>
#include <QPointF>
#include "beatmap.h"

enum class EditorTool {
    Select = 0,
    Place = 1
};

/**
 * @brief EditorTimelineCanvas renders the interactive rhythmic track with seconds grid,
 * playhead cursor, and diamond-shaped notes per BeatKey design.
 */
class EditorTimelineCanvas : public QWidget
{
    Q_OBJECT

public:
    explicit EditorTimelineCanvas(QWidget *parent = nullptr);
    ~EditorTimelineCanvas() override = default;

    void setNotes(const QVector<NoteData> &notes);
    void setBpm(double bpm);
    void setTotalDuration(int durationMs);
    void setCurrentTime(int timeMs);
    void setActiveTool(EditorTool tool);
    void setSelectedNoteIndex(int index);

    int currentTime() const { return m_currentTimeMs; }
    int scrollTime() const { return m_scrollTimeMs; }
    int selectedNoteIndex() const { return m_selectedNoteIndex; }

signals:
    void seekRequested(int timeMs);
    void noteAddRequested(int timeMs);
    void noteSelected(int noteIndex);
    void noteDeleteRequested(int noteIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    double timeToX(int timeMs) const;
    int xToTime(double x) const;
    int snapTime(int timeMs) const;
    int findNoteNear(double x, double tolerancePx = 18.0) const;

    void drawDiamond(QPainter &painter, double cx, double cy, double radius, bool isSelected, bool isHover);

    QVector<NoteData> m_notes;
    double m_bpm;
    int m_totalDurationMs;
    int m_currentTimeMs;
    int m_scrollTimeMs;
    int m_selectedNoteIndex;

    EditorTool m_activeTool;
    QPointF m_mousePos;
    bool m_mouseInside;

    double m_pixelsPerSecond;
};

#endif // EDITORTIMELINECANVAS_H

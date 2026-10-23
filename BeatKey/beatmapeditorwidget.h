#ifndef BEATMAPEDITORWIDGET_H
#define BEATMAPEDITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QButtonGroup>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>

#include "beatmap.h"
#include "editortimelinecanvas.h"

/**
 * @brief BeatmapEditorWidget provides the chart editing interface.
 *
 * Implements tool panel (select/place), central interactive timeline canvas
 * with seconds grid and diamonds, right-side properties panel, and top playback scrub bar.
 */
class BeatmapEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BeatmapEditorWidget(QWidget *parent = nullptr);
    ~BeatmapEditorWidget() override = default;

    void loadBeatmap(const Beatmap &beatmap, const QString &audioPath, const QString &coverPath);
    void stopPlayback();

signals:
    void backRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void handlePlayPauseToggle();
    void handleStop();
    void handleSliderMoved(int position);
    void handleSliderReleased();
    void handleSeekRequested(int timeMs);
    void handleNoteAddRequested(int timeMs);
    void handleNoteSelected(int noteIndex);
    void handleNoteDeleteRequested(int noteIndex);
    void handleDeleteCurrentNoteClicked();
    void handleToolChanged(int id);
    void handleSaveMap();
    void updatePlaybackTick();

private:
    void setupUi();
    void updateTimeDisplay(int currentMs);
    void updatePropertiesPanel(int selectedIndex);
    QString formatMsToTime(int ms) const;

    // Data
    Beatmap m_currentBeatmap;
    QString m_sourceAudioPath;
    QString m_sourceCoverPath;
    int m_totalDurationMs;
    bool m_isScrubbing;

    // Audio
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QTimer *m_playbackTimer;

    // Top Header & Playback
    QPushButton *m_backBtn;
    QLabel *m_mapTitleLabel;
    QPushButton *m_playBtn;
    QPushButton *m_stopBtn;
    QLabel *m_timeLabel;
    QPushButton *m_saveBtn;
    QLabel *m_saveStatusLabel;
    QSlider *m_scrubSlider;

    // Left Tool Panel
    QButtonGroup *m_toolGroup;
    QPushButton *m_selectToolBtn;
    QPushButton *m_placeToolBtn;

    // Center Canvas
    EditorTimelineCanvas *m_timelineCanvas;

    // Right Properties Panel
    QLabel *m_propDurationValue;
    QLabel *m_propCurrentTimeValue;
    QLabel *m_propBpmValue;
    QLabel *m_propNoteTypeValue;
    QLabel *m_propNoteTimeValue;
    QPushButton *m_propDeleteNoteBtn;
    QLabel *m_propTotalNotesValue;
    QLabel *m_propActiveToolValue;
};

#endif // BEATMAPEDITORWIDGET_H

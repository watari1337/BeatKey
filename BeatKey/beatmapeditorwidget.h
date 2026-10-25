#ifndef BEATMAPEDITORWIDGET_H
#define BEATMAPEDITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QScrollBar>
#include <QScrollArea>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QComboBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QElapsedTimer>
#include <QUndoStack>
#include <QResizeEvent>

#include "beatmap.h"
#include "editortimelinecanvas.h"
#include "audiowaveform.h"

/**
 * @brief BeatmapEditorWidget provides the chart editing interface.
 *
 * Implements toolbar, interactive timeline canvas with snap grid & waveform,
 * Tap Record mode (R key toggle), Undo/Redo stack, scrollable properties panel,
 * custom integer grid quantization, and validated .bkey export.
 */
class BeatmapEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BeatmapEditorWidget(QWidget *parent = nullptr);
    ~BeatmapEditorWidget() override;

    void loadBeatmap(const Beatmap &beatmap, const QString &audioPath, const QString &coverPath);
    void stopPlayback();
    Beatmap currentBeatmap() const { return m_currentBeatmap; }
    bool maybeSaveAndExit();
    bool hasUnsavedChanges() const;
    bool isMetadataModified() const;
    int backgroundDim() const { return m_backgroundDimPercent; }
    void setBackgroundDim(int percent) {
        m_backgroundDimPercent = qBound(0, percent, 100);
        update();
    }

    // Undo/Redo executor hooks called by command objects
    void executeAddNote(const NoteData &note);
    void executeRemoveNote(const NoteData &note);
    void executeDeleteNotes(const QVector<NoteData> &notes);
    void executeRestoreNotes(const QVector<NoteData> &notes);
    void executeMoveNotes(const QVector<int> &originalTimes, int deltaMs);
    void executeBatchAddNotes(const QVector<NoteData> &notes);
    void executeBatchRemoveNotes(const QVector<NoteData> &notes);

signals:
    void backRequested();

protected:
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    // Playback
    void handlePlayPauseToggle();
    void handleStop();
    void handleStepBackward();
    void handleStepForward();
    void handleSpeedChanged(int index);
    void handleSliderMoved(int position);
    void handleSliderReleased();
    void updatePlaybackTick();

    // Timeline events
    void handleSeekRequested(int timeMs);
    void handleNoteAddRequested(int timeMs);
    void handleNotesBatchAddRequested(const QVector<NoteData> &notes);
    void handleNoteDeleteRequested(int noteIndex);
    void handleNotesDeleteRequested(const QVector<int> &indices);
    void handleNotesBatchDeleteRequested(const QVector<NoteData> &notes);
    void handleNotesMoved(const QVector<int> &originalTimes, int deltaMs);
    void handleSelectionChanged(const QVector<int> &selectedIndices);
    void handleScrollChanged(int scrollTimeMs, int totalDurationMs, int visibleMs);
    void handleHoverInfoChanged(int timeMs, int measure, double beatInMeasure);

    // Modes & Tools
    void handleToolChanged(int id);
    void handleToggleTapRecord();
    void handleShowHelp();

    // Map Metadata & Settings
    void handleTitleChanged(const QString &title);
    void handleMapperChanged(const QString &mapper);
    void handleArtistChanged(const QString &artist);
    void handleDifficultyChanged(int index);
    void handleBpmChanged(double bpm);
    void handleOffsetChanged(int offsetMs);
    void handleQuantPresetChanged(int index);
    void handleQuantCustomSpinChanged(int value);
    void handleChooseCover();
    void handleChooseAudio();

    // Actions & Export
    void handleSaveMap();
    void handleExportBkey();
    void handleDeleteSelection();
    void handleUndo();
    void handleRedo();
    void handleBackButtonClicked();
    void handleTabChanged(int index);
    void handleBgDimSliderChanged(int value);
    void handleBgBlurSliderChanged(int value);
    void handleBgDesatSliderChanged(int value);
    void handleBgVignetteSliderChanged(int value);

private:
    void setupUi();
    void setupShortcuts();
    void updateTimeDisplay(int currentMs);
    void updatePropertiesPanel();
    void updateStats();
    bool validateMapData(QString *errorMessage);
    QString formatMsToTime(int ms) const;
    QWidget* createEditorPage();
    QWidget* createDescriptionPage();
    QWidget* createPlaceholderPage(const QString &title, const QString &subtitle);
    void updateCoverPreview();
    void updateCachedBackground();
    void updateStatusNotesCount();
    int calculateLastNoteTime() const;
    void recalculateMapDuration(int audioDurationMs = 0);
    void setMapDuration(int durationMs);

    // Data
    Beatmap m_currentBeatmap;
    QString m_sourceAudioPath;
    QString m_sourceCoverPath;
    BeatmapMeta m_initialMeta;
    QString m_initialCoverPath;
    QString m_initialAudioPath;
    int m_initialDurationMs;
    bool m_isDurationManuallyEdited;
    int m_backgroundDimPercent;
    int m_backgroundBlurRadius;
    int m_desaturationPercent;
    int m_vignettePercent;
    QPixmap m_cachedBackgroundPixmap;
    int m_audioDurationMs;
    int m_totalDurationMs;
    bool m_isScrubbing;
    bool m_userScrolledAway;
    bool m_isAutoScrolling;
    int m_playheadInViewMs;

    // Audio & Waveform
    QMediaPlayer *m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    QTimer *m_playbackTimer;
    AudioWaveform *m_waveform;
    QElapsedTimer m_audioClock;
    qint64 m_clockBaseMs;
    qint64 m_lastMediaPos;

    // Undo / Redo
    QUndoStack m_undoStack;

    // Tap Record mode session
    bool m_isMetadataModified;
    bool m_isTapRecording;
    QVector<NoteData> m_tapRecordSessionNotes;

    // Section Navigation Tabs & Stack
    QButtonGroup *m_tabGroup;
    QPushButton *m_tabEditorBtn;
    QPushButton *m_tabDescriptionBtn;
    QPushButton *m_tabTimingBtn;
    QPushButton *m_tabTestBtn;
    QStackedWidget *m_editorStack;

    // Description Page Widgets
    QLabel *m_descCoverPreview;
    QLabel *m_descAudioPathLabel;
    QSlider *m_descBgDimSlider;
    QLabel *m_descBgDimValLabel;
    QSlider *m_descBgBlurSlider;
    QLabel *m_descBgBlurValLabel;
    QSlider *m_descBgDesatSlider;
    QLabel *m_descBgDesatValLabel;
    QSlider *m_descBgVignetteSlider;
    QLabel *m_descBgVignetteValLabel;

    // Top Header & Playback
    QFrame *m_headerContainer;
    QFrame *m_editorToolbar;
    QScrollArea *m_topScrollArea;
    QPushButton *m_backBtn;
    QLabel *m_mapTitleLabel;
    QPushButton *m_stepBackBtn;
    QPushButton *m_playBtn;
    QPushButton *m_stepForwardBtn;
    QPushButton *m_stopBtn;
    QComboBox *m_speedCombo;
    QLabel *m_timeLabel;
    QPushButton *m_undoBtn;
    QPushButton *m_redoBtn;
    QPushButton *m_tapRecordBtn;
    QPushButton *m_helpBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_exportBtn;
    QLabel *m_saveStatusLabel;
    QSlider *m_scrubSlider;

    // Left Tool Panel (1: Select, 2: Place)
    QButtonGroup *m_toolGroup;
    QPushButton *m_selectToolBtn;
    QPushButton *m_placeToolBtn;

    // Center Workspace
    EditorTimelineCanvas *m_timelineCanvas;
    QLabel *m_postprocessorLabel;
    QLabel *m_statusNotesCountLabel;
    QLabel *m_zoomLabel;

    // Right Properties Panel: ScrollArea Container
    QScrollArea *m_rightScrollArea;

    // Metadata Widgets
    QLineEdit *m_metaTitleEdit;
    QLineEdit *m_metaMapperEdit;
    QLineEdit *m_metaArtistEdit;
    QComboBox *m_metaDiffCombo;
    QDoubleSpinBox *m_metaBpmSpin;
    QSpinBox *m_metaOffsetSpin;
    QSpinBox *m_metaDurationSpin;
    QPushButton *m_autoDurationBtn;
    QPushButton *m_metaChooseCoverBtn;
    QLabel *m_metaCoverLabel;
    QPushButton *m_metaChooseAudioBtn;
    QLabel *m_metaAudioLabel;

    // Quantization Grid Widgets
    QComboBox *m_quantPresetCombo;
    QSpinBox *m_quantCustomSpin;

    // Selection Widgets
    QLabel *m_selCountLabel;
    QLabel *m_selTimeLabel;
    QPushButton *m_selDeleteBtn;

    // Statistics Widgets
    QLabel *m_statTotalNotes;
    QLabel *m_statDuration;
    QLabel *m_statNps;
};

#endif // BEATMAPEDITORWIDGET_H

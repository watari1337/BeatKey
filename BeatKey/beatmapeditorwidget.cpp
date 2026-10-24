#include "beatmapeditorwidget.h"
#include "beatmapmanager.h"
#include "audiomanager.h"
#include "settingsmanager.h"
#include "editorcommands.h"
#include "editorhelpdialog.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QKeyEvent>
#include <QShortcut>
#include <QApplication>
#include <QUrl>
#include <algorithm>

BeatmapEditorWidget::BeatmapEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_totalDurationMs(120000)
    , m_isScrubbing(false)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playbackTimer(new QTimer(this))
    , m_waveform(new AudioWaveform(this))
    , m_clockBaseMs(0)
    , m_lastMediaPos(-1)
    , m_isTapRecording(false)
    , m_topScrollArea(nullptr)
    , m_backBtn(nullptr)
    , m_mapTitleLabel(nullptr)
    , m_stepBackBtn(nullptr)
    , m_playBtn(nullptr)
    , m_stepForwardBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_speedCombo(nullptr)
    , m_timeLabel(nullptr)
    , m_undoBtn(nullptr)
    , m_redoBtn(nullptr)
    , m_tapRecordBtn(nullptr)
    , m_helpBtn(nullptr)
    , m_saveBtn(nullptr)
    , m_exportBtn(nullptr)
    , m_saveStatusLabel(nullptr)
    , m_scrubSlider(nullptr)
    , m_toolGroup(nullptr)
    , m_selectToolBtn(nullptr)
    , m_placeToolBtn(nullptr)
    , m_timelineCanvas(nullptr)
    , m_timelineScrollBar(nullptr)
    , m_postprocessorLabel(nullptr)
    , m_zoomLabel(nullptr)
    , m_rightScrollArea(nullptr)
    , m_metaTitleEdit(nullptr)
    , m_metaMapperEdit(nullptr)
    , m_metaArtistEdit(nullptr)
    , m_metaDiffCombo(nullptr)
    , m_metaBpmSpin(nullptr)
    , m_metaOffsetSpin(nullptr)
    , m_metaChooseCoverBtn(nullptr)
    , m_metaCoverLabel(nullptr)
    , m_metaChooseAudioBtn(nullptr)
    , m_metaAudioLabel(nullptr)
    , m_quantPresetCombo(nullptr)
    , m_quantCustomSpin(nullptr)
    , m_selCountLabel(nullptr)
    , m_selTimeLabel(nullptr)
    , m_selDeleteBtn(nullptr)
    , m_statTotalNotes(nullptr)
    , m_statDuration(nullptr)
    , m_statNps(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setFocusPolicy(Qt::StrongFocus);

    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(SettingsManager::instance().musicVolume() / 100.0f);

    connect(m_playbackTimer, &QTimer::timeout, this, &BeatmapEditorWidget::updatePlaybackTick);

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 d) {
        if (d > 0) {
            m_totalDurationMs = static_cast<int>(d);
            m_scrubSlider->setRange(0, m_totalDurationMs);
            m_timelineCanvas->setTotalDuration(m_totalDurationMs);
            updateTimeDisplay(static_cast<int>(m_mediaPlayer->position()));
            updateStats();
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (!m_isScrubbing) {
            int currentMs = static_cast<int>(pos);
            m_scrubSlider->setValue(currentMs);
            m_timelineCanvas->setCurrentTime(currentMs);
            updateTimeDisplay(currentMs);
        }
    });

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            m_playBtn->setText(QStringLiteral("❚❚ Пауза"));
            m_audioClock.restart();
            m_clockBaseMs = m_mediaPlayer->position();
            m_lastMediaPos = m_clockBaseMs;
            m_playbackTimer->start(16);
        } else {
            m_playBtn->setText(QStringLiteral("▶ Плей"));
            m_playbackTimer->stop();
            m_audioClock.invalidate();
            if (m_isTapRecording) {
                // If stopped while recording, commit session
                handleToggleTapRecord();
            }
        }
    });

    // Undo stack state updates
    connect(&m_undoStack, &QUndoStack::canUndoChanged, this, [this](bool canUndo) {
        if (m_undoBtn) m_undoBtn->setEnabled(canUndo);
    });
    connect(&m_undoStack, &QUndoStack::canRedoChanged, this, [this](bool canRedo) {
        if (m_redoBtn) m_redoBtn->setEnabled(canRedo);
    });

    setupUi();
    setupShortcuts();

    // Install application-wide or widget-wide event filter for reliable Space capture
    qApp->installEventFilter(this);
}

void BeatmapEditorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    Theme::paintWindowBackground(&painter, rect());
}

bool BeatmapEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
    // Horizontal mouse wheel scrolling for top bar scroll area
    if (m_topScrollArea && (watched == m_topScrollArea || watched == m_topScrollArea->viewport())) {
        if (event->type() == QEvent::Wheel) {
            auto *wheelEvent = static_cast<QWheelEvent *>(event);
            int delta = wheelEvent->angleDelta().y();
            if (delta == 0) delta = wheelEvent->angleDelta().x();
            if (delta != 0) {
                m_topScrollArea->horizontalScrollBar()->setValue(
                    m_topScrollArea->horizontalScrollBar()->value() - delta
                );
                return true;
            }
        }
    }

    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);

        // Check whether focus or target is inside any text/numeric input widget
        QWidget *w = qobject_cast<QWidget *>(watched);
        QWidget *focused = QApplication::focusWidget();
        bool isTyping = false;

        for (QWidget *testWidget : { w, focused }) {
            QWidget *curr = testWidget;
            while (curr) {
                if (qobject_cast<QLineEdit *>(curr) ||
                    qobject_cast<QAbstractSpinBox *>(curr) ||
                    qobject_cast<QComboBox *>(curr)) {
                    isTyping = true;
                    break;
                }
                curr = curr->parentWidget();
            }
            if (isTyping) break;
        }

        if (!isTyping) {
            if (keyEvent->key() == Qt::Key_Space) {
                handlePlayPauseToggle();
                return true; // Consume event completely
            }
            if (keyEvent->key() == Qt::Key_1) {
                m_selectToolBtn->click();
                return true;
            }
            if (keyEvent->key() == Qt::Key_2) {
                m_placeToolBtn->click();
                return true;
            }
            if (keyEvent->key() == Qt::Key_R) {
                handleToggleTapRecord();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void BeatmapEditorWidget::setupShortcuts()
{
    // Quick Save
    auto *scSave = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(scSave, &QShortcut::activated, this, &BeatmapEditorWidget::handleSaveMap);

    // Undo
    auto *scUndo = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z), this);
    connect(scUndo, &QShortcut::activated, this, &BeatmapEditorWidget::handleUndo);

    // Redo
    auto *scRedo1 = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y), this);
    connect(scRedo1, &QShortcut::activated, this, &BeatmapEditorWidget::handleRedo);

    auto *scRedo2 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), this);
    connect(scRedo2, &QShortcut::activated, this, &BeatmapEditorWidget::handleRedo);

    // Delete
    auto *scDel = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(scDel, &QShortcut::activated, this, &BeatmapEditorWidget::handleDeleteSelection);

    auto *scBksp = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    connect(scBksp, &QShortcut::activated, this, &BeatmapEditorWidget::handleDeleteSelection);

    // Step navigation
    auto *scLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(scLeft, &QShortcut::activated, this, &BeatmapEditorWidget::handleStepBackward);

    auto *scRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(scRight, &QShortcut::activated, this, &BeatmapEditorWidget::handleStepForward);

    // Help (F1)
    auto *scHelp = new QShortcut(QKeySequence(Qt::Key_F1), this);
    connect(scHelp, &QShortcut::activated, this, &BeatmapEditorWidget::handleShowHelp);

    // Select All
    auto *scAll = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_A), this);
    connect(scAll, &QShortcut::activated, this, [this]() {
        m_timelineCanvas->selectAll();
    });
}

void BeatmapEditorWidget::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(16, 12, 16, 14);
    rootLayout->setSpacing(8);

    // ========================================================================
    // Top Bar (Controls, Navigation, Playback, Speed, Help, Save, Export)
    // ========================================================================
    auto *topBarContainer = new QFrame(this);
    topBarContainer->setStyleSheet(Theme::editorCardStyle());

    auto *topBarLayout = new QVBoxLayout(topBarContainer);
    topBarLayout->setContentsMargins(10, 6, 10, 6);
    topBarLayout->setSpacing(4);

    // Row 1: Action Controls wrapped inside horizontal QScrollArea (prevents text clipping on any resolution)
    m_topScrollArea = new QScrollArea(topBarContainer);
    m_topScrollArea->setWidgetResizable(true);
    m_topScrollArea->setFrameShape(QFrame::NoFrame);
    m_topScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_topScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_topScrollArea->setFixedHeight(54);
    m_topScrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:horizontal { background: rgba(20, 14, 34, 180); height: 6px; border-radius: 3px; margin: 0px; }"
        "QScrollBar::handle:horizontal { background: #5a388f; border-radius: 3px; min-width: 28px; }"
        "QScrollBar::handle:horizontal:hover { background: #a259ff; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
    ));
    m_topScrollArea->viewport()->installEventFilter(this);

    auto *topScrollContent = new QWidget(m_topScrollArea);
    topScrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *controlsRow = new QHBoxLayout(topScrollContent);
    controlsRow->setContentsMargins(4, 2, 4, 8);
    controlsRow->setSpacing(8);
    controlsRow->setSizeConstraint(QLayout::SetMinimumSize);

    m_backBtn = new QPushButton(QStringLiteral("← В хаб карт"), topScrollContent);
    m_backBtn->setFocusPolicy(Qt::NoFocus);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_backBtn->setFixedSize(110, 32);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        stopPlayback();
        emit backRequested();
    });
    controlsRow->addWidget(m_backBtn);

    m_mapTitleLabel = new QLabel(QStringLiteral("Редактор карты"), topScrollContent);
    m_mapTitleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 14px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "padding: 0 4px;"
    ));
    m_mapTitleLabel->setFixedHeight(32);
    controlsRow->addWidget(m_mapTitleLabel);

    // Step Backward
    m_stepBackBtn = new QPushButton(QStringLiteral("|◀"), topScrollContent);
    m_stepBackBtn->setFocusPolicy(Qt::NoFocus);
    m_stepBackBtn->setCursor(Qt::PointingHandCursor);
    m_stepBackBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stepBackBtn->setFixedSize(34, 32);
    m_stepBackBtn->setToolTip(QStringLiteral("Шаг назад (←)"));
    connect(m_stepBackBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStepBackward);
    controlsRow->addWidget(m_stepBackBtn);

    // Play/Pause
    m_playBtn = new QPushButton(QStringLiteral("▶ Плей"), topScrollContent);
    m_playBtn->setFocusPolicy(Qt::NoFocus);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_playBtn->setFixedSize(92, 32);
    m_playBtn->setToolTip(QStringLiteral("Воспроизведение / Пауза (Пробел)"));
    connect(m_playBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handlePlayPauseToggle);
    controlsRow->addWidget(m_playBtn);

    // Step Forward
    m_stepForwardBtn = new QPushButton(QStringLiteral("▶|"), topScrollContent);
    m_stepForwardBtn->setFocusPolicy(Qt::NoFocus);
    m_stepForwardBtn->setCursor(Qt::PointingHandCursor);
    m_stepForwardBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stepForwardBtn->setFixedSize(34, 32);
    m_stepForwardBtn->setToolTip(QStringLiteral("Шаг вперед (→)"));
    connect(m_stepForwardBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStepForward);
    controlsRow->addWidget(m_stepForwardBtn);

    // Stop
    m_stopBtn = new QPushButton(QStringLiteral("■"), topScrollContent);
    m_stopBtn->setFocusPolicy(Qt::NoFocus);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stopBtn->setFixedSize(34, 32);
    m_stopBtn->setToolTip(QStringLiteral("Остановить"));
    connect(m_stopBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStop);
    controlsRow->addWidget(m_stopBtn);

    // Speed Selector (Card 35)
    m_speedCombo = new QComboBox(topScrollContent);
    m_speedCombo->setFocusPolicy(Qt::NoFocus);
    m_speedCombo->addItems({
        QStringLiteral("0.25x"),
        QStringLiteral("0.50x"),
        QStringLiteral("0.75x"),
        QStringLiteral("1.00x")
    });
    m_speedCombo->setCurrentIndex(3); // 1.00x
    m_speedCombo->setStyleSheet(Theme::comboBoxStyle());
    m_speedCombo->setFixedSize(74, 32);
    m_speedCombo->setCursor(Qt::PointingHandCursor);
    m_speedCombo->setToolTip(QStringLiteral("Скорость воспроизведения трека"));
    connect(m_speedCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleSpeedChanged);
    controlsRow->addWidget(m_speedCombo);

    // Timecode display (Generous width to prevent truncation)
    m_timeLabel = new QLabel(QStringLiteral("00:00.000 / 02:00.000"), topScrollContent);
    m_timeLabel->setFixedSize(176, 32);
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Consolas', 'Courier New', monospace;"
        "font-size: 13px;"
        "color: #bb9af7;"
        "background: rgba(24, 16, 42, 180);"
        "border: 1px solid #4a2d75;"
        "border-radius: 4px;"
        "padding: 4px 6px;"
    ));
    controlsRow->addWidget(m_timeLabel);

    controlsRow->addStretch(1);

    // Undo / Redo
    m_undoBtn = new QPushButton(QStringLiteral("↺ Отмена"), topScrollContent);
    m_undoBtn->setFocusPolicy(Qt::NoFocus);
    m_undoBtn->setCursor(Qt::PointingHandCursor);
    m_undoBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_undoBtn->setFixedSize(86, 32);
    m_undoBtn->setToolTip(QStringLiteral("Отменить действие (Ctrl+Z)"));
    m_undoBtn->setEnabled(false);
    connect(m_undoBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleUndo);
    controlsRow->addWidget(m_undoBtn);

    m_redoBtn = new QPushButton(QStringLiteral("↻ Повтор"), topScrollContent);
    m_redoBtn->setFocusPolicy(Qt::NoFocus);
    m_redoBtn->setCursor(Qt::PointingHandCursor);
    m_redoBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_redoBtn->setFixedSize(86, 32);
    m_redoBtn->setToolTip(QStringLiteral("Повторить действие (Ctrl+Y)"));
    m_redoBtn->setEnabled(false);
    connect(m_redoBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleRedo);
    controlsRow->addWidget(m_redoBtn);

    // Tap Record button (Clean text, no emoji)
    m_tapRecordBtn = new QPushButton(QStringLiteral("REC (R)"), topScrollContent);
    m_tapRecordBtn->setFocusPolicy(Qt::NoFocus);
    m_tapRecordBtn->setCheckable(true);
    m_tapRecordBtn->setCursor(Qt::PointingHandCursor);
    m_tapRecordBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_tapRecordBtn->setFixedSize(82, 32);
    m_tapRecordBtn->setToolTip(QStringLiteral("Режим записи на лету (Горячая клавиша: R)"));
    connect(m_tapRecordBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleToggleTapRecord);
    controlsRow->addWidget(m_tapRecordBtn);

    // Help Button
    m_helpBtn = new QPushButton(QStringLiteral("? Справка"), topScrollContent);
    m_helpBtn->setFocusPolicy(Qt::NoFocus);
    m_helpBtn->setCursor(Qt::PointingHandCursor);
    m_helpBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_helpBtn->setFixedSize(88, 32);
    m_helpBtn->setToolTip(QStringLiteral("Справка по горячим клавишам и редактору (F1)"));
    connect(m_helpBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleShowHelp);
    controlsRow->addWidget(m_helpBtn);

    // Save Button
    m_saveBtn = new QPushButton(QStringLiteral("Сохранить"), topScrollContent);
    m_saveBtn->setFocusPolicy(Qt::NoFocus);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_saveBtn->setFixedSize(98, 32);
    m_saveBtn->setToolTip(QStringLiteral("Сохранить карту (Ctrl+S)"));
    connect(m_saveBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleSaveMap);
    controlsRow->addWidget(m_saveBtn);

    // Export Button
    m_exportBtn = new QPushButton(QStringLiteral("Экспорт"), topScrollContent);
    m_exportBtn->setFocusPolicy(Qt::NoFocus);
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    m_exportBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_exportBtn->setFixedSize(88, 32);
    m_exportBtn->setToolTip(QStringLiteral("Экспортировать карту в файл .bkey"));
    connect(m_exportBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleExportBkey);
    controlsRow->addWidget(m_exportBtn);

    m_topScrollArea->setWidget(topScrollContent);
    topBarLayout->addWidget(m_topScrollArea);

    // Row 2: Full-width Dedicated Scrub Slider (Spans 100% width)
    auto *sliderRow = new QHBoxLayout();
    sliderRow->setContentsMargins(0, 4, 0, 0);
    sliderRow->setSpacing(8);

    m_scrubSlider = new QSlider(Qt::Horizontal, topBarContainer);
    m_scrubSlider->setFocusPolicy(Qt::NoFocus);
    m_scrubSlider->setRange(0, m_totalDurationMs);
    m_scrubSlider->setValue(0);
    m_scrubSlider->setCursor(Qt::PointingHandCursor);
    m_scrubSlider->setStyleSheet(Theme::sliderStyle());
    connect(m_scrubSlider, &QSlider::sliderMoved, this, &BeatmapEditorWidget::handleSliderMoved);
    connect(m_scrubSlider, &QSlider::sliderReleased, this, &BeatmapEditorWidget::handleSliderReleased);
    connect(m_scrubSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_isScrubbing) {
            handleSliderMoved(value);
        }
    });
    sliderRow->addWidget(m_scrubSlider, 1);

    m_saveStatusLabel = new QLabel(topBarContainer);
    m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #2ecc71; font-weight: bold;"));
    m_saveStatusLabel->setVisible(false);
    sliderRow->addWidget(m_saveStatusLabel);

    topBarLayout->addLayout(sliderRow);
    rootLayout->addWidget(topBarContainer);

    // ========================================================================
    // Main Workspace: Left Toolstrip + Center Canvas + Right Properties Panel
    // ========================================================================
    auto *workLayout = new QHBoxLayout();
    workLayout->setSpacing(10);

    // 1. Left Toolstrip (Keys 1 and 2)
    auto *toolPanel = new QFrame(this);
    toolPanel->setStyleSheet(Theme::editorCardStyle());
    toolPanel->setFixedWidth(54);

    auto *toolLayout = new QVBoxLayout(toolPanel);
    toolLayout->setContentsMargins(6, 12, 6, 12);
    toolLayout->setSpacing(10);
    toolLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    m_selectToolBtn = new QPushButton(QStringLiteral("1 ↖"), toolPanel);
    m_selectToolBtn->setFocusPolicy(Qt::NoFocus);
    m_selectToolBtn->setCheckable(true);
    m_selectToolBtn->setFixedSize(42, 38);
    m_selectToolBtn->setCursor(Qt::PointingHandCursor);
    m_selectToolBtn->setStyleSheet(Theme::toolButtonStyle());
    m_selectToolBtn->setToolTip(QStringLiteral("Выделение нот (Клавиша 1)"));

    m_placeToolBtn = new QPushButton(QStringLiteral("2 ◆"), toolPanel);
    m_placeToolBtn->setFocusPolicy(Qt::NoFocus);
    m_placeToolBtn->setCheckable(true);
    m_placeToolBtn->setChecked(true);
    m_placeToolBtn->setFixedSize(42, 38);
    m_placeToolBtn->setCursor(Qt::PointingHandCursor);
    m_placeToolBtn->setStyleSheet(Theme::toolButtonStyle());
    m_placeToolBtn->setToolTip(QStringLiteral("Размещение нот (Клавиша 2)\nЗажмите ЛКМ и тяните: рисование\nЗажмите ПКМ и тяните: ластик"));

    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->addButton(m_selectToolBtn, static_cast<int>(EditorTool::Select));
    m_toolGroup->addButton(m_placeToolBtn, static_cast<int>(EditorTool::Place));
    m_toolGroup->setExclusive(true);
    connect(m_toolGroup, &QButtonGroup::idClicked, this, &BeatmapEditorWidget::handleToolChanged);

    toolLayout->addWidget(m_selectToolBtn);
    toolLayout->addWidget(m_placeToolBtn);
    workLayout->addWidget(toolPanel);

    // 2. Center Timeline Canvas Container
    auto *centerContainer = new QFrame(this);
    centerContainer->setStyleSheet(Theme::editorCardStyle());
    auto *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(4, 4, 4, 6);
    centerLayout->setSpacing(4);

    m_timelineCanvas = new EditorTimelineCanvas(centerContainer);
    m_timelineCanvas->setWaveform(m_waveform);
    centerLayout->addWidget(m_timelineCanvas, 1);

    // Horizontal Scrollbar synced with canvas (Card 33)
    m_timelineScrollBar = new QScrollBar(Qt::Horizontal, centerContainer);
    m_timelineScrollBar->setFocusPolicy(Qt::NoFocus);
    m_timelineScrollBar->setStyleSheet(Theme::sliderStyle());
    m_timelineScrollBar->setCursor(Qt::PointingHandCursor);
    connect(m_timelineScrollBar, &QScrollBar::valueChanged, this, &BeatmapEditorWidget::handleCanvasScrollbarMoved);
    centerLayout->addWidget(m_timelineScrollBar);

    // Canvas Status Strip (Cursor Postprocessor, Measure, Beat, Zoom)
    auto *statusStrip = new QHBoxLayout();
    statusStrip->setContentsMargins(8, 2, 8, 2);

    m_postprocessorLabel = new QLabel(QStringLiteral("Курсор: 00:00.000 | Такт 1, Доля 1.00"), centerContainer);
    m_postprocessorLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    statusStrip->addWidget(m_postprocessorLabel);

    statusStrip->addStretch(1);

    m_zoomLabel = new QLabel(QStringLiteral("Масштаб: 100% (Ctrl+Колесо)"), centerContainer);
    m_zoomLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    statusStrip->addWidget(m_zoomLabel);

    centerLayout->addLayout(statusStrip);

    // Timeline event connections
    connect(m_timelineCanvas, &EditorTimelineCanvas::seekRequested, this, &BeatmapEditorWidget::handleSeekRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::noteAddRequested, this, &BeatmapEditorWidget::handleNoteAddRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::notesBatchAddRequested, this, &BeatmapEditorWidget::handleNotesBatchAddRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::noteDeleteRequested, this, &BeatmapEditorWidget::handleNoteDeleteRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::notesDeleteRequested, this, &BeatmapEditorWidget::handleNotesDeleteRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::notesBatchDeleteRequested, this, &BeatmapEditorWidget::handleNotesBatchDeleteRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::notesMoved, this, &BeatmapEditorWidget::handleNotesMoved);
    connect(m_timelineCanvas, &EditorTimelineCanvas::selectionChanged, this, &BeatmapEditorWidget::handleSelectionChanged);
    connect(m_timelineCanvas, &EditorTimelineCanvas::scrollChanged, this, &BeatmapEditorWidget::handleScrollChanged);
    connect(m_timelineCanvas, &EditorTimelineCanvas::hoverInfoChanged, this, &BeatmapEditorWidget::handleHoverInfoChanged);
    connect(m_timelineCanvas, &EditorTimelineCanvas::zoomChanged, this, [this](double pxSec) {
        int zoomPercent = qRound((pxSec / 180.0) * 100.0);
        m_zoomLabel->setText(QStringLiteral("Масштаб: %1% (Ctrl+Колесо)").arg(zoomPercent));
    });

    workLayout->addWidget(centerContainer, 1);

    // 3. Right Properties Panel wrapped inside a dedicated QScrollArea (Fixes layout clipping)
    auto *rightPanelContainer = new QFrame(this);
    rightPanelContainer->setStyleSheet(Theme::editorCardStyle());
    rightPanelContainer->setFixedWidth(310);

    auto *panelContainerLayout = new QVBoxLayout(rightPanelContainer);
    panelContainerLayout->setContentsMargins(4, 4, 4, 4);

    m_rightScrollArea = new QScrollArea(rightPanelContainer);
    m_rightScrollArea->setWidgetResizable(true);
    m_rightScrollArea->setFrameShape(QFrame::NoFrame);
    m_rightScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rightScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_rightScrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical { background: rgba(20, 14, 34, 180); width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #5a388f; border-radius: 4px; min-height: 24px; }"
        "QScrollBar::handle:vertical:hover { background: #a259ff; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
    ));

    auto *scrollContent = new QWidget(m_rightScrollArea);
    scrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *rightLayout = new QVBoxLayout(scrollContent);
    rightLayout->setContentsMargins(10, 8, 12, 12);
    rightLayout->setSpacing(8);
    rightLayout->setAlignment(Qt::AlignTop);

    auto *propTitle = new QLabel(QStringLiteral("Свойства и параметры"), scrollContent);
    propTitle->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 15px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));
    rightLayout->addWidget(propTitle);

    // Section 1: Map Metadata
    auto *metaSec = new QLabel(QStringLiteral("ПАРАМЕТРЫ КАРТЫ"), scrollContent);
    metaSec->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold; margin-top: 4px;"));
    rightLayout->addWidget(metaSec);

    // Title (Mandatory)
    auto *titleLbl = new QLabel(QStringLiteral("Название карты (обязательно):"), scrollContent);
    titleLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaTitleEdit = new QLineEdit(scrollContent);
    m_metaTitleEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaTitleEdit->setPlaceholderText(QStringLiteral("Название трека"));
    connect(m_metaTitleEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleTitleChanged);
    rightLayout->addWidget(titleLbl);
    rightLayout->addWidget(m_metaTitleEdit);

    // Mapper (Mandatory)
    auto *mapperLbl = new QLabel(QStringLiteral("Автор карты (обязательно):"), scrollContent);
    mapperLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaMapperEdit = new QLineEdit(scrollContent);
    m_metaMapperEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaMapperEdit->setPlaceholderText(QStringLiteral("Никнейм автора"));
    connect(m_metaMapperEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleMapperChanged);
    rightLayout->addWidget(mapperLbl);
    rightLayout->addWidget(m_metaMapperEdit);

    // Artist (Optional)
    auto *artistLbl = new QLabel(QStringLiteral("Исполнитель трека:"), scrollContent);
    artistLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaArtistEdit = new QLineEdit(scrollContent);
    m_metaArtistEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaArtistEdit->setPlaceholderText(QStringLiteral("Исполнитель музыки"));
    connect(m_metaArtistEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleArtistChanged);
    rightLayout->addWidget(artistLbl);
    rightLayout->addWidget(m_metaArtistEdit);

    // Difficulty
    auto *diffLbl = new QLabel(QStringLiteral("Сложность:"), scrollContent);
    diffLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaDiffCombo = new QComboBox(scrollContent);
    m_metaDiffCombo->addItems({
        QStringLiteral("Легко"),
        QStringLiteral("Нормально"),
        QStringLiteral("Сложно"),
        QStringLiteral("Эксперт"),
        QStringLiteral("Мастер")
    });
    m_metaDiffCombo->setStyleSheet(Theme::comboBoxStyle());
    connect(m_metaDiffCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleDifficultyChanged);
    rightLayout->addWidget(diffLbl);
    rightLayout->addWidget(m_metaDiffCombo);

    // BPM
    auto *bpmLbl = new QLabel(QStringLiteral("Темп (BPM, 30.0 - 300.0):"), scrollContent);
    bpmLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaBpmSpin = new QDoubleSpinBox(scrollContent);
    m_metaBpmSpin->setRange(30.0, 300.0);
    m_metaBpmSpin->setValue(120.0);
    m_metaBpmSpin->setSingleStep(1.0);
    m_metaBpmSpin->setDecimals(1);
    m_metaBpmSpin->setAlignment(Qt::AlignCenter);
    m_metaBpmSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_metaBpmSpin, &QDoubleSpinBox::valueChanged, this, &BeatmapEditorWidget::handleBpmChanged);
    rightLayout->addWidget(bpmLbl);
    rightLayout->addWidget(m_metaBpmSpin);

    // Audio Offset
    auto *offsetLbl = new QLabel(QStringLiteral("Смещение первого такта (Offset, мс):"), scrollContent);
    offsetLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_metaOffsetSpin = new QSpinBox(scrollContent);
    m_metaOffsetSpin->setRange(-5000, 5000);
    m_metaOffsetSpin->setValue(0);
    m_metaOffsetSpin->setAlignment(Qt::AlignCenter);
    m_metaOffsetSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_metaOffsetSpin, &QSpinBox::valueChanged, this, &BeatmapEditorWidget::handleOffsetChanged);
    rightLayout->addWidget(offsetLbl);
    rightLayout->addWidget(m_metaOffsetSpin);

    // Choose Cover
    auto *coverLbl = new QLabel(QStringLiteral("Обложка карты:"), scrollContent);
    coverLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    rightLayout->addWidget(coverLbl);

    auto *coverRow = new QHBoxLayout();
    m_metaChooseCoverBtn = new QPushButton(QStringLiteral("Выбрать..."), scrollContent);
    m_metaChooseCoverBtn->setFocusPolicy(Qt::NoFocus);
    m_metaChooseCoverBtn->setCursor(Qt::PointingHandCursor);
    m_metaChooseCoverBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_metaChooseCoverBtn->setFixedSize(90, 30);
    connect(m_metaChooseCoverBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleChooseCover);

    m_metaCoverLabel = new QLabel(QStringLiteral("Не выбрана"), scrollContent);
    m_metaCoverLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));

    coverRow->addWidget(m_metaChooseCoverBtn);
    coverRow->addWidget(m_metaCoverLabel, 1);
    rightLayout->addLayout(coverRow);

    // Choose Audio
    auto *audioLbl = new QLabel(QStringLiteral("Аудиофайл:"), scrollContent);
    audioLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    rightLayout->addWidget(audioLbl);

    auto *audioRow = new QHBoxLayout();
    m_metaChooseAudioBtn = new QPushButton(QStringLiteral("Выбрать..."), scrollContent);
    m_metaChooseAudioBtn->setFocusPolicy(Qt::NoFocus);
    m_metaChooseAudioBtn->setCursor(Qt::PointingHandCursor);
    m_metaChooseAudioBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_metaChooseAudioBtn->setFixedSize(90, 30);
    connect(m_metaChooseAudioBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleChooseAudio);

    m_metaAudioLabel = new QLabel(QStringLiteral("Не выбран"), scrollContent);
    m_metaAudioLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));

    audioRow->addWidget(m_metaChooseAudioBtn);
    audioRow->addWidget(m_metaAudioLabel, 1);
    rightLayout->addLayout(audioRow);

    // Section 2: Quantization Grid Selector & Custom Integer Divisor
    auto *sep1 = new QFrame(scrollContent);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep1);

    auto *quantSec = new QLabel(QStringLiteral("СЕТКА КВАНТОВАНИЯ"), scrollContent);
    quantSec->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(quantSec);

    m_quantPresetCombo = new QComboBox(scrollContent);
    m_quantPresetCombo->addItems({
        QStringLiteral("1/1 такта (Целый такт)"),
        QStringLiteral("1/2 такта (Половинная)"),
        QStringLiteral("1/3 такта (Триоль)"),
        QStringLiteral("1/4 такта (Четвертная доля)"),
        QStringLiteral("1/8 такта (Восьмая доля)"),
        QStringLiteral("1/16 такта (Шестнадцатая)"),
        QStringLiteral("Свой шаг (1/N такта)...")
    });
    m_quantPresetCombo->setCurrentIndex(3); // 1/4 такта (четверть)
    m_quantPresetCombo->setStyleSheet(Theme::comboBoxStyle());
    m_quantPresetCombo->setCursor(Qt::PointingHandCursor);
    connect(m_quantPresetCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleQuantPresetChanged);
    rightLayout->addWidget(m_quantPresetCombo);

    // Custom Integer Divisor Spinbox (Allows user to enter any integer divisor 1 to 64)
    auto *customRow = new QHBoxLayout();
    auto *customLbl = new QLabel(QStringLiteral("Шаг доли 1/N (N):"), scrollContent);
    customLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_quantCustomSpin = new QSpinBox(scrollContent);
    m_quantCustomSpin->setRange(1, 64);
    m_quantCustomSpin->setValue(4);
    m_quantCustomSpin->setAlignment(Qt::AlignCenter);
    m_quantCustomSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_quantCustomSpin, &QSpinBox::valueChanged, this, &BeatmapEditorWidget::handleQuantCustomSpinChanged);
    customRow->addWidget(customLbl);
    customRow->addStretch(1);
    customRow->addWidget(m_quantCustomSpin);
    rightLayout->addLayout(customRow);

    // Section 3: Selection Properties & Delete
    auto *sep2 = new QFrame(scrollContent);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep2);

    auto *selSec = new QLabel(QStringLiteral("СВОЙСТВА ВЫДЕЛЕНИЯ"), scrollContent);
    selSec->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(selSec);

    auto *selCountRow = new QHBoxLayout();
    auto *selCountText = new QLabel(QStringLiteral("Выделено нот:"), scrollContent);
    selCountText->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_selCountLabel = new QLabel(QStringLiteral("0"), scrollContent);
    m_selCountLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11px; font-weight: bold;"));
    selCountRow->addWidget(selCountText);
    selCountRow->addStretch(1);
    selCountRow->addWidget(m_selCountLabel);
    rightLayout->addLayout(selCountRow);

    auto *selTimeRow = new QHBoxLayout();
    auto *selTimeText = new QLabel(QStringLiteral("Время удара:"), scrollContent);
    selTimeText->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_selTimeLabel = new QLabel(QStringLiteral("—"), scrollContent);
    m_selTimeLabel->setStyleSheet(QStringLiteral("color: #ffcc00; font-size: 11px; font-weight: bold;"));
    selTimeRow->addWidget(selTimeText);
    selTimeRow->addStretch(1);
    selTimeRow->addWidget(m_selTimeLabel);
    rightLayout->addLayout(selTimeRow);

    m_selDeleteBtn = new QPushButton(QStringLiteral("Удалить выделенные (Del)"), scrollContent);
    m_selDeleteBtn->setFocusPolicy(Qt::NoFocus);
    m_selDeleteBtn->setCursor(Qt::PointingHandCursor);
    m_selDeleteBtn->setStyleSheet(Theme::dangerButtonStyle());
    m_selDeleteBtn->setFixedHeight(32);
    m_selDeleteBtn->setEnabled(false);
    connect(m_selDeleteBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleDeleteSelection);
    rightLayout->addWidget(m_selDeleteBtn);

    // Section 4: Map Statistics
    auto *sep3 = new QFrame(scrollContent);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep3);

    auto *statSec = new QLabel(QStringLiteral("СТАТИСТИКА КАРТЫ"), scrollContent);
    statSec->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(statSec);

    auto *statRow1 = new QHBoxLayout();
    auto *totLbl = new QLabel(QStringLiteral("Всего нот:"), scrollContent);
    totLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_statTotalNotes = new QLabel(QStringLiteral("0"), scrollContent);
    m_statTotalNotes->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 11px; font-weight: bold;"));
    statRow1->addWidget(totLbl);
    statRow1->addStretch(1);
    statRow1->addWidget(m_statTotalNotes);
    rightLayout->addLayout(statRow1);

    auto *statRow2 = new QHBoxLayout();
    auto *durLbl = new QLabel(QStringLiteral("Длительность:"), scrollContent);
    durLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_statDuration = new QLabel(QStringLiteral("00:00"), scrollContent);
    m_statDuration->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11px; font-weight: bold;"));
    statRow2->addWidget(durLbl);
    statRow2->addStretch(1);
    statRow2->addWidget(m_statDuration);
    rightLayout->addLayout(statRow2);

    auto *statRow3 = new QHBoxLayout();
    auto *npsLbl = new QLabel(QStringLiteral("Плотность (NPS):"), scrollContent);
    npsLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px;"));
    m_statNps = new QLabel(QStringLiteral("0.0"), scrollContent);
    m_statNps->setStyleSheet(QStringLiteral("color: #70d6ff; font-size: 11px; font-weight: bold;"));
    statRow3->addWidget(npsLbl);
    statRow3->addStretch(1);
    statRow3->addWidget(m_statNps);
    rightLayout->addLayout(statRow3);

    rightLayout->addStretch(1);
    m_rightScrollArea->setWidget(scrollContent);
    panelContainerLayout->addWidget(m_rightScrollArea);

    workLayout->addWidget(rightPanelContainer);
    rootLayout->addLayout(workLayout, 1);
}

void BeatmapEditorWidget::loadBeatmap(const Beatmap &beatmap, const QString &audioPath, const QString &coverPath)
{
    stopPlayback();
    m_undoStack.clear();

    m_currentBeatmap = beatmap;
    m_sourceAudioPath = audioPath.isEmpty() ? beatmap.extractedAudioPath : audioPath;
    m_sourceCoverPath = coverPath.isEmpty() ? beatmap.meta.coverFileName : coverPath;

    // Load metadata into widgets
    m_metaTitleEdit->setText(m_currentBeatmap.meta.title);
    m_metaMapperEdit->setText(m_currentBeatmap.meta.mapper);
    m_metaArtistEdit->setText(m_currentBeatmap.meta.artist);

    int diffIdx = m_metaDiffCombo->findText(m_currentBeatmap.meta.difficultyName);
    if (diffIdx != -1) {
        m_metaDiffCombo->setCurrentIndex(diffIdx);
    }

    m_metaBpmSpin->setValue(m_currentBeatmap.meta.bpm);
    m_metaOffsetSpin->setValue(m_currentBeatmap.meta.audioOffset);

    if (!m_sourceCoverPath.isEmpty()) {
        m_metaCoverLabel->setText(QFileInfo(m_sourceCoverPath).fileName());
    } else {
        m_metaCoverLabel->setText(QStringLiteral("Не выбрана"));
    }

    if (!m_sourceAudioPath.isEmpty()) {
        m_metaAudioLabel->setText(QFileInfo(m_sourceAudioPath).fileName());
    } else {
        m_metaAudioLabel->setText(QStringLiteral("Не выбран"));
    }

    QString titleText = m_currentBeatmap.meta.title.isEmpty()
        ? QStringLiteral("Новая карта")
        : m_currentBeatmap.meta.title;
    QString diffText = m_currentBeatmap.meta.difficultyName.isEmpty()
        ? QStringLiteral("Нормально")
        : m_currentBeatmap.meta.difficultyName;
    m_mapTitleLabel->setText(QStringLiteral("%1 [%2]").arg(titleText, diffText));

    // Load Audio Source & Waveform
    if (!m_sourceAudioPath.isEmpty() && QFile::exists(m_sourceAudioPath)) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_sourceAudioPath));
        m_waveform->load(m_sourceAudioPath);
    } else {
        m_waveform->clear();
    }

    // Configure timeline
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    m_timelineCanvas->setBpm(m_currentBeatmap.meta.bpm);
    m_timelineCanvas->setAudioOffset(m_currentBeatmap.meta.audioOffset);
    m_timelineCanvas->setCurrentTime(0);
    m_timelineCanvas->clearSelection();

    m_scrubSlider->setValue(0);
    updateTimeDisplay(0);
    updatePropertiesPanel();
    updateStats();
}

void BeatmapEditorWidget::stopPlayback()
{
    m_mediaPlayer->stop();
    m_playbackTimer->stop();
    m_audioClock.invalidate();
    m_clockBaseMs = 0;
    m_lastMediaPos = 0;
    m_timelineCanvas->setCurrentTime(0);
    m_scrubSlider->setValue(0);
    m_playBtn->setText(QStringLiteral("▶ Плей"));
    updateTimeDisplay(0);
}

void BeatmapEditorWidget::handlePlayPauseToggle()
{
    AudioManager::instance().playClickSound();
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    } else {
        m_mediaPlayer->play();
    }
}

void BeatmapEditorWidget::handleStop()
{
    AudioManager::instance().playClickSound();
    stopPlayback();
}

void BeatmapEditorWidget::handleStepBackward()
{
    AudioManager::instance().playClickSound();
    int currentMs = static_cast<int>(m_mediaPlayer->position());
    double step = m_timelineCanvas->quantIntervalMs();
    int targetMs = qMax(0, static_cast<int>(m_timelineCanvas->snapTime(currentMs - static_cast<int>(step + 2))));
    handleSeekRequested(targetMs);
}

void BeatmapEditorWidget::handleStepForward()
{
    AudioManager::instance().playClickSound();
    int currentMs = static_cast<int>(m_mediaPlayer->position());
    double step = m_timelineCanvas->quantIntervalMs();
    int targetMs = qMin(m_totalDurationMs, static_cast<int>(m_timelineCanvas->snapTime(currentMs + static_cast<int>(step + 2))));
    handleSeekRequested(targetMs);
}

void BeatmapEditorWidget::handleSpeedChanged(int index)
{
    AudioManager::instance().playClickSound();
    qreal speed = 1.0;
    switch (index) {
        case 0: speed = 0.25; break;
        case 1: speed = 0.50; break;
        case 2: speed = 0.75; break;
        case 3: speed = 1.00; break;
        default: speed = 1.00; break;
    }
    m_mediaPlayer->setPlaybackRate(speed);
}

void BeatmapEditorWidget::handleSliderMoved(int position)
{
    m_isScrubbing = true;
    m_timelineCanvas->setCurrentTime(position);
    updateTimeDisplay(position);
}

void BeatmapEditorWidget::handleSliderReleased()
{
    m_isScrubbing = false;
    int targetMs = m_scrubSlider->value();
    m_clockBaseMs = targetMs;
    m_lastMediaPos = targetMs;
    if (m_audioClock.isValid()) {
        m_audioClock.restart();
    }
    m_mediaPlayer->setPosition(targetMs);
    m_timelineCanvas->setCurrentTime(targetMs);
    updateTimeDisplay(targetMs);
}

void BeatmapEditorWidget::handleSeekRequested(int timeMs)
{
    m_clockBaseMs = timeMs;
    m_lastMediaPos = timeMs;
    if (m_audioClock.isValid()) {
        m_audioClock.restart();
    }
    m_timelineCanvas->setCurrentTime(timeMs);
    m_scrubSlider->setValue(timeMs);
    m_mediaPlayer->setPosition(timeMs);
    updateTimeDisplay(timeMs);
}

void BeatmapEditorWidget::handleScrollChanged(int scrollTimeMs, int totalDurationMs, int visibleMs)
{
    m_timelineScrollBar->blockSignals(true);
    int maxScroll = qMax(0, totalDurationMs - visibleMs);
    m_timelineScrollBar->setRange(0, maxScroll);
    m_timelineScrollBar->setPageStep(visibleMs);
    m_timelineScrollBar->setValue(scrollTimeMs);
    m_timelineScrollBar->blockSignals(false);
}

void BeatmapEditorWidget::handleCanvasScrollbarMoved(int value)
{
    m_timelineCanvas->setScrollTime(value);
}

void BeatmapEditorWidget::handleHoverInfoChanged(int timeMs, int measure, double beatInMeasure)
{
    QString timeStr = formatMsToTime(timeMs);
    m_postprocessorLabel->setText(QStringLiteral("Курсор: %1 (%2 мс) | Такт %3, Доля %4 | Сетка: 1/%5")
        .arg(timeStr)
        .arg(timeMs)
        .arg(measure)
        .arg(QString::number(beatInMeasure, 'f', 2))
        .arg(m_quantCustomSpin->value()));
}

// ============================================================================
// Note Operations (Routed through QUndoStack with deduplication)
// ============================================================================

void BeatmapEditorWidget::handleNoteAddRequested(int timeMs)
{
    // Strict deduplication: never allow notes at identical or overlapping time (< 15ms)
    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (qAbs(m_currentBeatmap.notes[i].timeMs - timeMs) <= 15) {
            m_timelineCanvas->setSelectedNoteIndex(i);
            return;
        }
    }

    NoteData note;
    note.timeMs = timeMs;
    note.type = QStringLiteral("single");

    m_undoStack.push(new AddNoteCommand(this, note));
}

void BeatmapEditorWidget::handleNotesBatchAddRequested(const QVector<NoteData> &notes)
{
    if (notes.isEmpty()) return;
    // Deduplicate against existing notes in m_currentBeatmap
    QVector<NoteData> deduplicated;
    for (const auto &n : notes) {
        bool exists = false;
        for (const auto &cn : m_currentBeatmap.notes) {
            if (qAbs(cn.timeMs - n.timeMs) <= 15) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            deduplicated.append(n);
        }
    }
    if (!deduplicated.isEmpty()) {
        m_undoStack.push(new BatchAddNotesCommand(this, deduplicated));
    }
}

void BeatmapEditorWidget::handleNoteDeleteRequested(int noteIndex)
{
    if (noteIndex >= 0 && noteIndex < m_currentBeatmap.notes.size()) {
        QVector<NoteData> toDelete;
        toDelete.append(m_currentBeatmap.notes[noteIndex]);
        m_undoStack.push(new DeleteNotesCommand(this, toDelete));
    }
}

void BeatmapEditorWidget::handleNotesDeleteRequested(const QVector<int> &indices)
{
    QVector<NoteData> toDelete;
    for (int idx : indices) {
        if (idx >= 0 && idx < m_currentBeatmap.notes.size()) {
            toDelete.append(m_currentBeatmap.notes[idx]);
        }
    }
    if (!toDelete.isEmpty()) {
        m_undoStack.push(new DeleteNotesCommand(this, toDelete));
    }
}

void BeatmapEditorWidget::handleNotesBatchDeleteRequested(const QVector<NoteData> &notes)
{
    if (notes.isEmpty()) return;
    m_undoStack.push(new DeleteNotesCommand(this, notes));
}

void BeatmapEditorWidget::handleNotesMoved(const QVector<int> &originalTimes, int deltaMs)
{
    if (deltaMs == 0 || originalTimes.isEmpty()) return;
    m_undoStack.push(new MoveNotesCommand(this, originalTimes, deltaMs));
}

void BeatmapEditorWidget::handleDeleteSelection()
{
    const auto &sel = m_timelineCanvas->selectedNoteIndices();
    if (!sel.isEmpty()) {
        handleNotesDeleteRequested(sel.values().toVector());
    }
}

void BeatmapEditorWidget::handleSelectionChanged(const QVector<int> &selectedIndices)
{
    Q_UNUSED(selectedIndices);
    updatePropertiesPanel();
}

void BeatmapEditorWidget::handleUndo()
{
    if (m_undoStack.canUndo()) {
        AudioManager::instance().playClickSound();
        m_undoStack.undo();
    }
}

void BeatmapEditorWidget::handleRedo()
{
    if (m_undoStack.canRedo()) {
        AudioManager::instance().playClickSound();
        m_undoStack.redo();
    }
}

// ============================================================================
// Undo/Redo Executors
// ============================================================================

void BeatmapEditorWidget::executeAddNote(const NoteData &note)
{
    AudioManager::instance().playClickSound();
    m_currentBeatmap.notes.append(note);
    std::sort(m_currentBeatmap.notes.begin(), m_currentBeatmap.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });

    m_timelineCanvas->setNotes(m_currentBeatmap.notes);

    // Select the newly added note
    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (m_currentBeatmap.notes[i].timeMs == note.timeMs) {
            m_timelineCanvas->setSelectedNoteIndex(i);
            break;
        }
    }
    updateStats();
}

void BeatmapEditorWidget::executeRemoveNote(const NoteData &note)
{
    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (m_currentBeatmap.notes[i].timeMs == note.timeMs) {
            m_currentBeatmap.notes.removeAt(i);
            break;
        }
    }
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    m_timelineCanvas->clearSelection();
    updateStats();
}

void BeatmapEditorWidget::executeDeleteNotes(const QVector<NoteData> &notes)
{
    AudioManager::instance().playClickSound();
    QSet<int> timesToRemove;
    for (const auto &n : notes) timesToRemove.insert(n.timeMs);

    QVector<NoteData> remaining;
    for (const auto &n : m_currentBeatmap.notes) {
        if (!timesToRemove.contains(n.timeMs)) {
            remaining.append(n);
        }
    }
    m_currentBeatmap.notes = remaining;
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    m_timelineCanvas->clearSelection();
    updateStats();
}

void BeatmapEditorWidget::executeRestoreNotes(const QVector<NoteData> &notes)
{
    for (const auto &n : notes) {
        m_currentBeatmap.notes.append(n);
    }
    std::sort(m_currentBeatmap.notes.begin(), m_currentBeatmap.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);

    // Re-select restored notes
    QSet<int> sel;
    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        for (const auto &n : notes) {
            if (m_currentBeatmap.notes[i].timeMs == n.timeMs) {
                sel.insert(i);
            }
        }
    }
    m_timelineCanvas->setSelectedNoteIndices(sel);
    updateStats();
}

void BeatmapEditorWidget::executeMoveNotes(const QVector<int> &originalTimes, int deltaMs)
{
    QSet<int> origSet = QSet<int>(originalTimes.begin(), originalTimes.end());

    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (origSet.contains(m_currentBeatmap.notes[i].timeMs)) {
            m_currentBeatmap.notes[i].timeMs = qMax(0, m_currentBeatmap.notes[i].timeMs + deltaMs);
        }
    }

    std::sort(m_currentBeatmap.notes.begin(), m_currentBeatmap.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });

    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    updateStats();
}

void BeatmapEditorWidget::executeBatchAddNotes(const QVector<NoteData> &notes)
{
    for (const auto &n : notes) {
        m_currentBeatmap.notes.append(n);
    }
    std::sort(m_currentBeatmap.notes.begin(), m_currentBeatmap.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    updateStats();
}

void BeatmapEditorWidget::executeBatchRemoveNotes(const QVector<NoteData> &notes)
{
    QSet<int> timesToRemove;
    for (const auto &n : notes) timesToRemove.insert(n.timeMs);

    QVector<NoteData> remaining;
    for (const auto &n : m_currentBeatmap.notes) {
        if (!timesToRemove.contains(n.timeMs)) {
            remaining.append(n);
        }
    }
    m_currentBeatmap.notes = remaining;
    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    m_timelineCanvas->clearSelection();
    updateStats();
}

// ============================================================================
// Tools, Modes, Tap Record, Help
// ============================================================================

void BeatmapEditorWidget::handleToolChanged(int id)
{
    AudioManager::instance().playClickSound();
    auto tool = static_cast<EditorTool>(id);
    m_timelineCanvas->setActiveTool(tool);
}

void BeatmapEditorWidget::handleToggleTapRecord()
{
    AudioManager::instance().playClickSound();
    m_isTapRecording = !m_isTapRecording;

    if (m_isTapRecording) {
        m_tapRecordSessionNotes.clear();
        m_tapRecordBtn->setChecked(true);
        m_tapRecordBtn->setText(QStringLiteral("REC • (R)"));
        m_tapRecordBtn->setStyleSheet(QStringLiteral(
            "QPushButton { background: #b82b4a; color: #ffffff; font-weight: bold; border: 1px solid #ff5577; border-radius: 6px; }"
            "QPushButton:hover { background: #d9536f; }"
        ));

        m_saveStatusLabel->setText(QStringLiteral("ЗАПИСЬ АКТИВНА: нажимайте Z, X или Пробел"));
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #ff5577; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);

        // Turn on playback automatically
        if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState) {
            m_mediaPlayer->play();
        }
    } else {
        m_tapRecordBtn->setChecked(false);
        m_tapRecordBtn->setText(QStringLiteral("REC (R)"));
        m_tapRecordBtn->setStyleSheet(Theme::secondaryButtonStyle());
        m_saveStatusLabel->setVisible(false);

        // Turn on pause automatically
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_mediaPlayer->pause();
        }

        // Commit recorded notes to Undo stack
        if (!m_tapRecordSessionNotes.isEmpty()) {
            m_undoStack.push(new BatchAddNotesCommand(this, m_tapRecordSessionNotes));
            m_tapRecordSessionNotes.clear();
        }
    }
}

void BeatmapEditorWidget::handleShowHelp()
{
    AudioManager::instance().playClickSound();
    EditorHelpDialog dlg(this);
    dlg.exec();
}

// ============================================================================
// Map Metadata & Quantization Settings
// ============================================================================

void BeatmapEditorWidget::handleTitleChanged(const QString &title)
{
    m_currentBeatmap.meta.title = title.trimmed();
    if (m_currentBeatmap.meta.title.isEmpty()) {
        m_metaTitleEdit->setStyleSheet(Theme::errorLineEditStyle());
    } else {
        m_metaTitleEdit->setStyleSheet(Theme::lineEditStyle());
    }
    QString diff = m_currentBeatmap.meta.difficultyName.isEmpty() ? QStringLiteral("Нормально") : m_currentBeatmap.meta.difficultyName;
    m_mapTitleLabel->setText(QStringLiteral("%1 [%2]").arg(m_currentBeatmap.meta.title, diff));
}

void BeatmapEditorWidget::handleMapperChanged(const QString &mapper)
{
    m_currentBeatmap.meta.mapper = mapper.trimmed();
    if (m_currentBeatmap.meta.mapper.isEmpty()) {
        m_metaMapperEdit->setStyleSheet(Theme::errorLineEditStyle());
    } else {
        m_metaMapperEdit->setStyleSheet(Theme::lineEditStyle());
    }
}

void BeatmapEditorWidget::handleArtistChanged(const QString &artist)
{
    m_currentBeatmap.meta.artist = artist.trimmed();
}

void BeatmapEditorWidget::handleDifficultyChanged(int index)
{
    Q_UNUSED(index);
    m_currentBeatmap.meta.difficultyName = m_metaDiffCombo->currentText();
    QString title = m_currentBeatmap.meta.title.isEmpty() ? QStringLiteral("Новая карта") : m_currentBeatmap.meta.title;
    m_mapTitleLabel->setText(QStringLiteral("%1 [%2]").arg(title, m_currentBeatmap.meta.difficultyName));
}

void BeatmapEditorWidget::handleBpmChanged(double bpm)
{
    m_currentBeatmap.meta.bpm = bpm;
    m_timelineCanvas->setBpm(bpm); // Instant dynamic grid re-render (Card 11)
}

void BeatmapEditorWidget::handleOffsetChanged(int offsetMs)
{
    m_currentBeatmap.meta.audioOffset = offsetMs;
    m_timelineCanvas->setAudioOffset(offsetMs); // Instant dynamic grid re-render (Card 11)
}

void BeatmapEditorWidget::handleQuantPresetChanged(int index)
{
    int divisor = 4;
    switch (index) {
        case 0: divisor = 1;  break; // 1/1
        case 1: divisor = 2;  break; // 1/2
        case 2: divisor = 3;  break; // 1/3
        case 3: divisor = 4;  break; // 1/4
        case 4: divisor = 8;  break; // 1/8
        case 5: divisor = 16; break; // 1/16
        case 6: divisor = m_quantCustomSpin->value(); break; // Custom
        default: divisor = 4; break;
    }

    m_quantCustomSpin->blockSignals(true);
    m_quantCustomSpin->setValue(divisor);
    m_quantCustomSpin->blockSignals(false);

    m_timelineCanvas->setQuantDivisor(divisor);
}

void BeatmapEditorWidget::handleQuantCustomSpinChanged(int value)
{
    m_quantPresetCombo->blockSignals(true);
    switch (value) {
        case 1:  m_quantPresetCombo->setCurrentIndex(0); break;
        case 2:  m_quantPresetCombo->setCurrentIndex(1); break;
        case 3:  m_quantPresetCombo->setCurrentIndex(2); break;
        case 4:  m_quantPresetCombo->setCurrentIndex(3); break;
        case 8:  m_quantPresetCombo->setCurrentIndex(4); break;
        case 16: m_quantPresetCombo->setCurrentIndex(5); break;
        default: m_quantPresetCombo->setCurrentIndex(6); break; // Custom
    }
    m_quantPresetCombo->blockSignals(false);

    m_timelineCanvas->setQuantDivisor(value);
}

void BeatmapEditorWidget::handleChooseCover()
{
    AudioManager::instance().playClickSound();
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите файл обложки карты"),
        QString(),
        QStringLiteral("Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)")
    );
    if (!filePath.isEmpty()) {
        m_sourceCoverPath = filePath;
        m_currentBeatmap.meta.coverFileName = QFileInfo(filePath).fileName();
        m_currentBeatmap.coverImage.load(filePath);
        m_metaCoverLabel->setText(QFileInfo(filePath).fileName());
        m_saveStatusLabel->setText(QStringLiteral("Обложка карты выбрана!"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(2500, this, [this]() { m_saveStatusLabel->setVisible(false); });
    }
}

void BeatmapEditorWidget::handleChooseAudio()
{
    AudioManager::instance().playClickSound();
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите аудиофайл для карты"),
        QString(),
        QStringLiteral("Музыкальные файлы (*.wav *.mp3 *.ogg);;Все файлы (*.*)")
    );
    if (!filePath.isEmpty()) {
        m_sourceAudioPath = filePath;
        m_currentBeatmap.meta.audioFileName = QFileInfo(filePath).fileName();
        m_metaAudioLabel->setText(QFileInfo(filePath).fileName());
        m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        m_waveform->load(filePath);
        m_saveStatusLabel->setText(QStringLiteral("Аудиофайл обновлен!"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(2500, this, [this]() { m_saveStatusLabel->setVisible(false); });
    }
}

// ============================================================================
// Data Structure Validation & Export
// ============================================================================

bool BeatmapEditorWidget::validateMapData(QString *errorMessage)
{
    if (m_currentBeatmap.meta.title.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Поле 'Название карты' обязательно для заполнения.");
        m_metaTitleEdit->setStyleSheet(Theme::errorLineEditStyle());
        m_metaTitleEdit->setFocus();
        return false;
    }

    if (m_currentBeatmap.meta.mapper.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Поле 'Автор карты' обязательно для заполнения.");
        m_metaMapperEdit->setStyleSheet(Theme::errorLineEditStyle());
        m_metaMapperEdit->setFocus();
        return false;
    }

    if (m_currentBeatmap.meta.bpm < 30.0 || m_currentBeatmap.meta.bpm > 300.0) {
        if (errorMessage) *errorMessage = QStringLiteral("Значение BPM должно находиться в диапазоне от 30.0 до 300.0.");
        m_metaBpmSpin->setFocus();
        return false;
    }

    if (m_sourceAudioPath.isEmpty() || !QFile::exists(m_sourceAudioPath)) {
        if (errorMessage) *errorMessage = QStringLiteral("Аудиофайл карты не найден или не выбран.");
        return false;
    }

    QString audioExt = QFileInfo(m_sourceAudioPath).suffix().toLower();
    if (audioExt != QStringLiteral("mp3") && audioExt != QStringLiteral("wav") && audioExt != QStringLiteral("ogg")) {
        if (errorMessage) *errorMessage = QStringLiteral("Неподдерживаемый аудиоформат '%1'. Поддерживаются .mp3, .wav, .ogg.").arg(audioExt);
        return false;
    }

    if (m_currentBeatmap.notes.isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("Карта не содержит нот. Разместите хотя бы одну ноту на таймлайне.");
        return false;
    }

    return true;
}

void BeatmapEditorWidget::handleSaveMap()
{
    AudioManager::instance().playClickSound();

    QString validationError;
    if (!validateMapData(&validationError)) {
        QMessageBox::warning(this, QStringLiteral("Ошибка проверки структуры карты"), validationError);
        m_saveStatusLabel->setText(validationError);
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #e57388; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);
        return;
    }

    QString outBkeyPath = m_currentBeatmap.bkeyFilePath;
    if (outBkeyPath.isEmpty()) {
        outBkeyPath = BeatmapManager::instance().generateUniqueBkeyPath(m_currentBeatmap.meta.title);
        m_currentBeatmap.bkeyFilePath = outBkeyPath;
    }

    QString error;
    bool ok = BeatmapManager::instance().exportToFile(
        outBkeyPath,
        m_currentBeatmap,
        m_sourceAudioPath,
        m_sourceCoverPath,
        &error
    );

    if (ok) {
        m_saveStatusLabel->setText(QStringLiteral("Карта успешно сохранена!"));
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #2ecc71; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(3500, this, [this]() {
            m_saveStatusLabel->setVisible(false);
        });
    } else {
        m_saveStatusLabel->setText(QStringLiteral("Ошибка сохранения: %1").arg(error));
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #e57388; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);
    }
}

void BeatmapEditorWidget::handleExportBkey()
{
    AudioManager::instance().playClickSound();

    QString validationError;
    if (!validateMapData(&validationError)) {
        QMessageBox::warning(this, QStringLiteral("Ошибка перед экспортом"), validationError);
        return;
    }

    QString defaultName = m_currentBeatmap.meta.title.trimmed();
    defaultName.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\s]")), QStringLiteral("_"));
    if (defaultName.isEmpty()) defaultName = QStringLiteral("map");
    defaultName += QStringLiteral(".bkey");

    QString savePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Экспорт карты в файл .bkey"),
        defaultName,
        QStringLiteral("BeatKey Map Package (*.bkey)")
    );

    if (savePath.isEmpty()) return;

    QString error;
    bool ok = BeatmapManager::instance().exportToFile(
        savePath,
        m_currentBeatmap,
        m_sourceAudioPath,
        m_sourceCoverPath,
        &error
    );

    if (ok) {
        QMessageBox::information(this, QStringLiteral("Экспорт завершен"),
            QStringLiteral("Пакет карты успешно экспортирован:\n%1").arg(savePath));
    } else {
        QMessageBox::critical(this, QStringLiteral("Ошибка экспорта"),
            QStringLiteral("Не удалось экспортировать карту:\n%1").arg(error));
    }
}

// ============================================================================
// Keyboard Shortcuts & Events
// ============================================================================

void BeatmapEditorWidget::keyPressEvent(QKeyEvent *event)
{
    // Tap Record active: capture rhythm keys (Z, X, Space)
    if (m_isTapRecording) {
        if (event->key() == Qt::Key_Z ||
            event->key() == Qt::Key_X ||
            event->key() == Qt::Key_Space) {

            int currentAudioMs = static_cast<int>(m_mediaPlayer->position());
            int snappedMs = m_timelineCanvas->snapTime(currentAudioMs);

            // Avoid duplicate note at the same exact snap tick
            bool exists = false;
            for (const auto &n : m_currentBeatmap.notes) {
                if (qAbs(n.timeMs - snappedMs) <= 15) { exists = true; break; }
            }
            for (const auto &n : m_tapRecordSessionNotes) {
                if (qAbs(n.timeMs - snappedMs) <= 15) { exists = true; break; }
            }

            if (!exists) {
                NoteData newNote;
                newNote.timeMs = snappedMs;
                newNote.type = QStringLiteral("single");

                m_tapRecordSessionNotes.append(newNote);
                executeAddNote(newNote);
                m_timelineCanvas->triggerTapFlash(snappedMs);
                AudioManager::instance().playHitSound();
            }

            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void BeatmapEditorWidget::updatePlaybackTick()
{
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState && m_audioClock.isValid()) {
        qreal rate = m_mediaPlayer->playbackRate();
        qint64 currentMediaPos = m_mediaPlayer->position();
        if (currentMediaPos != m_lastMediaPos) {
            m_lastMediaPos = currentMediaPos;
            m_clockBaseMs = currentMediaPos;
            m_audioClock.restart();
        }
        int interpolatedMs = m_clockBaseMs + static_cast<int>(m_audioClock.elapsed() * rate);
        interpolatedMs = qBound(0, interpolatedMs, m_totalDurationMs);
        m_timelineCanvas->setCurrentTime(interpolatedMs);
        if (!m_isScrubbing) {
            m_scrubSlider->setValue(interpolatedMs);
            updateTimeDisplay(interpolatedMs);
        }
    } else {
        int currentMs = static_cast<int>(m_mediaPlayer->position());
        m_timelineCanvas->setCurrentTime(currentMs);
        if (!m_isScrubbing) {
            m_scrubSlider->setValue(currentMs);
            updateTimeDisplay(currentMs);
        }
    }
}

void BeatmapEditorWidget::updateTimeDisplay(int currentMs)
{
    m_timeLabel->setText(QStringLiteral("%1 / %2")
        .arg(formatMsToTime(currentMs), formatMsToTime(m_totalDurationMs)));
}

void BeatmapEditorWidget::updatePropertiesPanel()
{
    const auto &sel = m_timelineCanvas->selectedNoteIndices();
    int count = sel.size();
    m_selCountLabel->setText(QString::number(count));
    m_selDeleteBtn->setEnabled(count > 0);

    if (count == 1) {
        int idx = *sel.begin();
        if (idx >= 0 && idx < m_currentBeatmap.notes.size()) {
            int t = m_currentBeatmap.notes[idx].timeMs;
            m_selTimeLabel->setText(QStringLiteral("%1 (%2 мс)").arg(formatMsToTime(t)).arg(t));
        } else {
            m_selTimeLabel->setText(QStringLiteral("—"));
        }
    } else if (count > 1) {
        m_selTimeLabel->setText(QStringLiteral("%1 нот").arg(count));
    } else {
        m_selTimeLabel->setText(QStringLiteral("Не выбрана"));
    }
}

void BeatmapEditorWidget::updateStats()
{
    int count = m_currentBeatmap.notes.size();
    m_statTotalNotes->setText(QString::number(count));

    int durSec = m_totalDurationMs / 1000;
    int m = durSec / 60;
    int s = durSec % 60;
    m_statDuration->setText(QStringLiteral("%1:%2").arg(m, 2, 10, QChar('0')).arg(s, 2, 10, QChar('0')));

    if (durSec > 0) {
        double nps = static_cast<double>(count) / durSec;
        m_statNps->setText(QString::number(nps, 'f', 1));
    } else {
        m_statNps->setText(QStringLiteral("0.0"));
    }
}

QString BeatmapEditorWidget::formatMsToTime(int ms) const
{
    if (ms < 0) ms = 0;
    int totalSeconds = ms / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int millis = ms % 1000;

    return QStringLiteral("%1:%2.%3")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(millis, 3, 10, QChar('0'));
}

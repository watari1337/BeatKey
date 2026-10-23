#include "beatmapeditorwidget.h"
#include "beatmapmanager.h"
#include "audiomanager.h"
#include "settingsmanager.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QDir>
#include <QCoreApplication>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

BeatmapEditorWidget::BeatmapEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_totalDurationMs(120000)
    , m_isScrubbing(false)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playbackTimer(new QTimer(this))
    , m_backBtn(nullptr)
    , m_mapTitleLabel(nullptr)
    , m_playBtn(nullptr)
    , m_stopBtn(nullptr)
    , m_timeLabel(nullptr)
    , m_saveBtn(nullptr)
    , m_saveStatusLabel(nullptr)
    , m_scrubSlider(nullptr)
    , m_toolGroup(nullptr)
    , m_selectToolBtn(nullptr)
    , m_placeToolBtn(nullptr)
    , m_timelineCanvas(nullptr)
    , m_propDurationValue(nullptr)
    , m_propCurrentTimeValue(nullptr)
    , m_propBpmValue(nullptr)
    , m_propNoteTypeValue(nullptr)
    , m_propNoteTimeValue(nullptr)
    , m_propDeleteNoteBtn(nullptr)
    , m_propTotalNotesValue(nullptr)
    , m_propActiveToolValue(nullptr)
{
    setStyleSheet(QStringLiteral("BeatmapEditorWidget { background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #18112c, stop:1 #24143e); }"));

    setAttribute(Qt::WA_StyledBackground, true);
    m_mediaPlayer->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(SettingsManager::instance().musicVolume() / 100.0f);

    connect(m_playbackTimer, &QTimer::timeout, this, &BeatmapEditorWidget::updatePlaybackTick);

    connect(m_mediaPlayer, &QMediaPlayer::durationChanged, this, [this](qint64 d) {
        if (d > 0) {
            m_totalDurationMs = static_cast<int>(d);
            m_scrubSlider->setRange(0, m_totalDurationMs);
            m_timelineCanvas->setTotalDuration(m_totalDurationMs);
            m_propDurationValue->setText(formatMsToTime(m_totalDurationMs));
            updateTimeDisplay(m_mediaPlayer->position());
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
            m_playBtn->setText(QStringLiteral("⏸ Пауза"));
            m_playbackTimer->start(16);
        } else {
            m_playBtn->setText(QStringLiteral("▶ Плей"));
            m_playbackTimer->stop();
        }
    });

    setupUi();
}


void BeatmapEditorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    Theme::paintWindowBackground(&painter, rect());
}

void BeatmapEditorWidget::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(20, 16, 20, 20);
    rootLayout->setSpacing(12);

    // ========================================================================
    // Top Bar (Controls & Playback)
    // ========================================================================
    auto *topBarContainer = new QFrame(this);
    topBarContainer->setStyleSheet(Theme::editorCardStyle());

    auto *topBarLayout = new QVBoxLayout(topBarContainer);
    topBarLayout->setContentsMargins(16, 12, 16, 12);
    topBarLayout->setSpacing(10);

    // Row 1: Navigation, Title, Play/Stop, Time, Save
    auto *navRow = new QHBoxLayout();
    navRow->setSpacing(14);

    m_backBtn = new QPushButton(QStringLiteral("← В хаб карт"), topBarContainer);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_backBtn->setFixedSize(120, 36);
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        stopPlayback();
        emit backRequested();
    });
    navRow->addWidget(m_backBtn);

    m_mapTitleLabel = new QLabel(QStringLiteral("Редактор карты"), topBarContainer);
    m_mapTitleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));
    navRow->addWidget(m_mapTitleLabel);

    navRow->addStretch(1);

    m_playBtn = new QPushButton(QStringLiteral("▶ Плей"), topBarContainer);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_playBtn->setFixedSize(100, 36);
    connect(m_playBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handlePlayPauseToggle);
    navRow->addWidget(m_playBtn);

    m_stopBtn = new QPushButton(QStringLiteral("■ Стоп"), topBarContainer);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stopBtn->setFixedSize(80, 36);
    connect(m_stopBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStop);
    navRow->addWidget(m_stopBtn);

    m_timeLabel = new QLabel(QStringLiteral("00:00.000 / 02:00.000"), topBarContainer);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Consolas', 'Courier New', monospace;"
        "font-size: 14px;"
        "color: #bb9af7;"
        "padding: 0 8px;"
    ));
    navRow->addWidget(m_timeLabel);

    navRow->addStretch(1);

    m_saveStatusLabel = new QLabel(topBarContainer);
    m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #2ecc71; font-weight: bold;"));
    m_saveStatusLabel->setVisible(false);
    navRow->addWidget(m_saveStatusLabel);

    m_saveBtn = new QPushButton(QStringLiteral("Сохранить карту"), topBarContainer);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(Theme::successButtonStyle());
    m_saveBtn->setFixedSize(140, 36);
    connect(m_saveBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleSaveMap);
    navRow->addWidget(m_saveBtn);

    topBarLayout->addLayout(navRow);

    // Row 2: Scrub Slider
    m_scrubSlider = new QSlider(Qt::Horizontal, topBarContainer);
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

    topBarLayout->addWidget(m_scrubSlider);
    rootLayout->addWidget(topBarContainer);

    // ========================================================================
    // Main Workspace: Left Toolstrip + Center Canvas + Right Properties
    // ========================================================================
    auto *workLayout = new QHBoxLayout();
    workLayout->setSpacing(14);

    // 1. Left Tool Panel (Small compact symbols)
    auto *toolPanel = new QFrame(this);
    toolPanel->setStyleSheet(Theme::editorCardStyle());
    toolPanel->setFixedWidth(56);
    auto *toolLayout = new QVBoxLayout(toolPanel);
    toolLayout->setContentsMargins(8, 12, 8, 12);
    toolLayout->setSpacing(10);
    toolLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    m_selectToolBtn = new QPushButton(QStringLiteral("↖"), toolPanel);
    m_selectToolBtn->setCheckable(true);
    m_selectToolBtn->setFixedSize(38, 38);
    m_selectToolBtn->setCursor(Qt::PointingHandCursor);
    m_selectToolBtn->setStyleSheet(Theme::toolButtonStyle());
    m_selectToolBtn->setToolTip(QStringLiteral("Выделить (Select)"));

    m_placeToolBtn = new QPushButton(QStringLiteral("◆"), toolPanel);
    m_placeToolBtn->setCheckable(true);
    m_placeToolBtn->setChecked(true);
    m_placeToolBtn->setFixedSize(38, 38);
    m_placeToolBtn->setCursor(Qt::PointingHandCursor);
    m_placeToolBtn->setStyleSheet(Theme::toolButtonStyle());
    m_placeToolBtn->setToolTip(QStringLiteral("Размещать ноты (Place Note)"));

    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->addButton(m_selectToolBtn, static_cast<int>(EditorTool::Select));
    m_toolGroup->addButton(m_placeToolBtn, static_cast<int>(EditorTool::Place));
    m_toolGroup->setExclusive(true);
    connect(m_toolGroup, &QButtonGroup::idClicked, this, &BeatmapEditorWidget::handleToolChanged);

    toolLayout->addWidget(m_selectToolBtn);
    toolLayout->addWidget(m_placeToolBtn);
    workLayout->addWidget(toolPanel);

    // 2. Central Timeline Canvas (Field with seconds grid and diamond notes)
    auto *centerContainer = new QFrame(this);
    centerContainer->setStyleSheet(Theme::editorCardStyle());
    auto *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(4, 4, 4, 8);
    centerLayout->setSpacing(6);

    m_timelineCanvas = new EditorTimelineCanvas(centerContainer);
    centerLayout->addWidget(m_timelineCanvas, 1);

    auto *hintLabel = new QLabel(
        QStringLiteral("Колесо мыши: прокрутка времени | ЛКМ: поставить / выбрать ноту | ПКМ: удалить ноту"),
        centerContainer
    );
    hintLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px; padding-left: 8px;"));
    centerLayout->addWidget(hintLabel);

    connect(m_timelineCanvas, &EditorTimelineCanvas::seekRequested, this, &BeatmapEditorWidget::handleSeekRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::noteAddRequested, this, &BeatmapEditorWidget::handleNoteAddRequested);
    connect(m_timelineCanvas, &EditorTimelineCanvas::noteSelected, this, &BeatmapEditorWidget::handleNoteSelected);
    connect(m_timelineCanvas, &EditorTimelineCanvas::noteDeleteRequested, this, &BeatmapEditorWidget::handleNoteDeleteRequested);

    workLayout->addWidget(centerContainer, 1);

    // 3. Right Properties Panel
    auto *rightPanel = new QFrame(this);
    rightPanel->setStyleSheet(Theme::editorCardStyle());
    rightPanel->setFixedWidth(260);

    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(18, 16, 18, 16);
    rightLayout->setSpacing(14);
    rightLayout->setAlignment(Qt::AlignTop);

    auto *propHeader = new QLabel(QStringLiteral("Свойства"), rightPanel);
    propHeader->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));
    rightLayout->addWidget(propHeader);

    // Section 1: Track Properties
    auto *trackSecLbl = new QLabel(QStringLiteral("ПАРАМЕТРЫ ТРЕКА"), rightPanel);
    trackSecLbl->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(trackSecLbl);

    auto *durRow = new QHBoxLayout();
    auto *durLbl = new QLabel(QStringLiteral("Длительность:"), rightPanel);
    durLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propDurationValue = new QLabel(QStringLiteral("02:00.000"), rightPanel);
    m_propDurationValue->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 12px; font-weight: bold;"));
    durRow->addWidget(durLbl);
    durRow->addStretch(1);
    durRow->addWidget(m_propDurationValue);
    rightLayout->addLayout(durRow);

    auto *curRow = new QHBoxLayout();
    auto *curLbl = new QLabel(QStringLiteral("Текущее время:"), rightPanel);
    curLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propCurrentTimeValue = new QLabel(QStringLiteral("00:00.000"), rightPanel);
    m_propCurrentTimeValue->setStyleSheet(QStringLiteral("color: #ff5577; font-size: 12px; font-weight: bold;"));
    curRow->addWidget(curLbl);
    curRow->addStretch(1);
    curRow->addWidget(m_propCurrentTimeValue);
    rightLayout->addLayout(curRow);

    auto *bpmRow = new QHBoxLayout();
    auto *bpmLbl = new QLabel(QStringLiteral("Темп (BPM):"), rightPanel);
    bpmLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propBpmValue = new QLabel(QStringLiteral("120.0"), rightPanel);
    m_propBpmValue->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 12px; font-weight: bold;"));
    bpmRow->addWidget(bpmLbl);
    bpmRow->addStretch(1);
    bpmRow->addWidget(m_propBpmValue);
    rightLayout->addLayout(bpmRow);

    // Section 2: Note Properties
    auto *sep1 = new QFrame(rightPanel);
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep1);

    auto *noteSecLbl = new QLabel(QStringLiteral("СВОЙСТВА НОТЫ"), rightPanel);
    noteSecLbl->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(noteSecLbl);

    auto *typeRow = new QHBoxLayout();
    auto *typeLbl = new QLabel(QStringLiteral("Тип ноты:"), rightPanel);
    typeLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propNoteTypeValue = new QLabel(QStringLiteral("Одиночная (Single)"), rightPanel);
    m_propNoteTypeValue->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 12px;"));
    typeRow->addWidget(typeLbl);
    typeRow->addStretch(1);
    typeRow->addWidget(m_propNoteTypeValue);
    rightLayout->addLayout(typeRow);

    auto *timeRow = new QHBoxLayout();
    auto *timeLbl = new QLabel(QStringLiteral("Время удара:"), rightPanel);
    timeLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propNoteTimeValue = new QLabel(QStringLiteral("Не выбрана"), rightPanel);
    m_propNoteTimeValue->setStyleSheet(QStringLiteral("color: #ffcc00; font-size: 12px; font-weight: bold;"));
    timeRow->addWidget(timeLbl);
    timeRow->addStretch(1);
    timeRow->addWidget(m_propNoteTimeValue);
    rightLayout->addLayout(timeRow);

    m_propDeleteNoteBtn = new QPushButton(QStringLiteral("Удалить ноту"), rightPanel);
    m_propDeleteNoteBtn->setCursor(Qt::PointingHandCursor);
    m_propDeleteNoteBtn->setStyleSheet(Theme::dangerButtonStyle());
    m_propDeleteNoteBtn->setFixedHeight(32);
    m_propDeleteNoteBtn->setEnabled(false);
    connect(m_propDeleteNoteBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleDeleteCurrentNoteClicked);
    rightLayout->addWidget(m_propDeleteNoteBtn);

    // Section 3: Extra Info
    auto *sep2 = new QFrame(rightPanel);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep2);

    auto *extraSecLbl = new QLabel(QStringLiteral("ДОПОЛНИТЕЛЬНО"), rightPanel);
    extraSecLbl->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold;"));
    rightLayout->addWidget(extraSecLbl);

    auto *countRow = new QHBoxLayout();
    auto *countLbl = new QLabel(QStringLiteral("Всего нот:"), rightPanel);
    countLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propTotalNotesValue = new QLabel(QStringLiteral("0"), rightPanel);
    m_propTotalNotesValue->setStyleSheet(QStringLiteral("color: #2ecc71; font-size: 12px; font-weight: bold;"));
    countRow->addWidget(countLbl);
    countRow->addStretch(1);
    countRow->addWidget(m_propTotalNotesValue);
    rightLayout->addLayout(countRow);

    auto *toolRow = new QHBoxLayout();
    auto *toolLbl = new QLabel(QStringLiteral("Инструмент:"), rightPanel);
    toolLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    m_propActiveToolValue = new QLabel(QStringLiteral("Размещение"), rightPanel);
    m_propActiveToolValue->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 12px;"));
    toolRow->addWidget(toolLbl);
    toolRow->addStretch(1);
    toolRow->addWidget(m_propActiveToolValue);
    rightLayout->addLayout(toolRow);

    auto *snapRow = new QHBoxLayout();
    auto *snapLbl = new QLabel(QStringLiteral("Сетка квантования:"), rightPanel);
    snapLbl->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 12px;"));
    auto *snapVal = new QLabel(QStringLiteral("1/4 такта"), rightPanel);
    snapVal->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 12px; font-weight: bold;"));
    snapRow->addWidget(snapLbl);
    snapRow->addStretch(1);
    snapRow->addWidget(snapVal);
    rightLayout->addLayout(snapRow);

    rightLayout->addStretch(1);
    workLayout->addWidget(rightPanel);

    rootLayout->addLayout(workLayout, 1);
}

void BeatmapEditorWidget::loadBeatmap(const Beatmap &beatmap, const QString &audioPath, const QString &coverPath)
{
    stopPlayback();

    m_currentBeatmap = beatmap;
    m_sourceAudioPath = audioPath.isEmpty() ? beatmap.extractedAudioPath : audioPath;
    m_sourceCoverPath = coverPath;

    QString titleText = m_currentBeatmap.meta.title.isEmpty()
        ? QStringLiteral("Без названия")
        : m_currentBeatmap.meta.title;
    QString diffText = m_currentBeatmap.meta.difficultyName.isEmpty()
        ? QStringLiteral("Нормально")
        : m_currentBeatmap.meta.difficultyName;

    m_mapTitleLabel->setText(QStringLiteral("%1 [%2]").arg(titleText, diffText));
    m_propBpmValue->setText(QString::number(m_currentBeatmap.meta.bpm, 'f', 1));

    if (!m_sourceAudioPath.isEmpty() && QFile::exists(m_sourceAudioPath)) {
        m_mediaPlayer->setSource(QUrl::fromLocalFile(m_sourceAudioPath));
    }

    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    m_timelineCanvas->setBpm(m_currentBeatmap.meta.bpm);
    m_timelineCanvas->setCurrentTime(0);
    m_timelineCanvas->setSelectedNoteIndex(-1);

    m_scrubSlider->setValue(0);
    updateTimeDisplay(0);
    updatePropertiesPanel(-1);
}

void BeatmapEditorWidget::stopPlayback()
{
    m_mediaPlayer->stop();
    m_playbackTimer->stop();
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
    m_mediaPlayer->setPosition(targetMs);
    m_timelineCanvas->setCurrentTime(targetMs);
    updateTimeDisplay(targetMs);
}

void BeatmapEditorWidget::handleSeekRequested(int timeMs)
{
    m_timelineCanvas->setCurrentTime(timeMs);
    m_scrubSlider->setValue(timeMs);
    m_mediaPlayer->setPosition(timeMs);
    updateTimeDisplay(timeMs);
}

void BeatmapEditorWidget::handleNoteAddRequested(int timeMs)
{
    AudioManager::instance().playClickSound();

    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (qAbs(m_currentBeatmap.notes[i].timeMs - timeMs) <= 15) {
            handleNoteSelected(i);
            return;
        }
    }

    NoteData newNote;
    newNote.timeMs = timeMs;
    newNote.type = QStringLiteral("single");

    m_currentBeatmap.notes.append(newNote);
    std::sort(m_currentBeatmap.notes.begin(), m_currentBeatmap.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });

    int newIndex = -1;
    for (int i = 0; i < m_currentBeatmap.notes.size(); ++i) {
        if (m_currentBeatmap.notes[i].timeMs == timeMs) {
            newIndex = i;
            break;
        }
    }

    m_timelineCanvas->setNotes(m_currentBeatmap.notes);
    handleNoteSelected(newIndex);
}

void BeatmapEditorWidget::handleNoteSelected(int noteIndex)
{
    m_timelineCanvas->setSelectedNoteIndex(noteIndex);
    updatePropertiesPanel(noteIndex);
}

void BeatmapEditorWidget::handleNoteDeleteRequested(int noteIndex)
{
    if (noteIndex >= 0 && noteIndex < m_currentBeatmap.notes.size()) {
        AudioManager::instance().playClickSound();
        m_currentBeatmap.notes.removeAt(noteIndex);
        m_timelineCanvas->setNotes(m_currentBeatmap.notes);
        handleNoteSelected(-1);
    }
}

void BeatmapEditorWidget::handleDeleteCurrentNoteClicked()
{
    int selIndex = m_timelineCanvas->selectedNoteIndex();
    if (selIndex != -1) {
        handleNoteDeleteRequested(selIndex);
    }
}

void BeatmapEditorWidget::handleToolChanged(int id)
{
    AudioManager::instance().playClickSound();
    auto tool = static_cast<EditorTool>(id);
    m_timelineCanvas->setActiveTool(tool);
    m_propActiveToolValue->setText(tool == EditorTool::Place
        ? QStringLiteral("Размещение")
        : QStringLiteral("Выделение"));
}

void BeatmapEditorWidget::handleSaveMap()
{
    AudioManager::instance().playClickSound();

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
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #2ecc71; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(3500, this, [this]() {
            m_saveStatusLabel->setVisible(false);
        });
    } else {
        m_saveStatusLabel->setText(QStringLiteral("Ошибка сохранения: %1").arg(error));
        m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #e57388; font-weight: bold;"));
        m_saveStatusLabel->setVisible(true);
    }
}

void BeatmapEditorWidget::updatePlaybackTick()
{
    int currentMs = static_cast<int>(m_mediaPlayer->position());
    m_timelineCanvas->setCurrentTime(currentMs);
    if (!m_isScrubbing) {
        m_scrubSlider->setValue(currentMs);
        updateTimeDisplay(currentMs);
    }
}

void BeatmapEditorWidget::updateTimeDisplay(int currentMs)
{
    m_timeLabel->setText(QStringLiteral("%1 / %2")
        .arg(formatMsToTime(currentMs), formatMsToTime(m_totalDurationMs)));
    m_propCurrentTimeValue->setText(formatMsToTime(currentMs));
}

void BeatmapEditorWidget::updatePropertiesPanel(int selectedIndex)
{
    m_propTotalNotesValue->setText(QString::number(m_currentBeatmap.notes.size()));

    if (selectedIndex >= 0 && selectedIndex < m_currentBeatmap.notes.size()) {
        const NoteData &note = m_currentBeatmap.notes[selectedIndex];
        double sec = note.timeMs / 1000.0;
        m_propNoteTimeValue->setText(QStringLiteral("%1 с (%2 мс)").arg(QString::number(sec, 'f', 3)).arg(note.timeMs));
        m_propNoteTypeValue->setText(QStringLiteral("Одиночная (Single)"));
        m_propDeleteNoteBtn->setEnabled(true);
    } else {
        m_propNoteTimeValue->setText(QStringLiteral("Не выбрана"));
        m_propNoteTypeValue->setText(QStringLiteral("—"));
        m_propDeleteNoteBtn->setEnabled(false);
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

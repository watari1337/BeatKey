#include "beatmapeditorwidget.h"
#include "savepromptdialog.h"
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
#include <QStandardPaths>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QMessageBox>
#include <QKeyEvent>
#include <QShortcut>
#include <QApplication>
#include <QUrl>
#include <algorithm>

BeatmapEditorWidget::BeatmapEditorWidget(QWidget *parent)
    : QWidget(parent)
    , m_initialDurationMs(0)
    , m_isDurationManuallyEdited(false)
    , m_backgroundDimPercent(50)
    , m_backgroundBlurRadius(0)
    , m_desaturationPercent(0)
    , m_vignettePercent(50)
    , m_audioDurationMs(120000)
    , m_totalDurationMs(120000)
    , m_isScrubbing(false)
    , m_userScrolledAway(false)
    , m_isAutoScrolling(false)
    , m_playheadInViewMs(0)
    , m_mediaPlayer(new QMediaPlayer(this))
    , m_audioOutput(new QAudioOutput(this))
    , m_playbackTimer(new QTimer(this))
    , m_waveform(new AudioWaveform(this))
    , m_clockBaseMs(0)
    , m_lastMediaPos(-1)
    , m_isMetadataModified(false)
    , m_isTapRecording(false)
    , m_tabGroup(nullptr)
    , m_tabEditorBtn(nullptr)
    , m_tabDescriptionBtn(nullptr)
    , m_tabTimingBtn(nullptr)
    , m_tabTestBtn(nullptr)
    , m_editorStack(nullptr)
    , m_descCoverPreview(nullptr)
    , m_descAudioPathLabel(nullptr)
    , m_descBgDimSlider(nullptr)
    , m_descBgDimValLabel(nullptr)
    , m_descBgBlurSlider(nullptr)
    , m_descBgBlurValLabel(nullptr)
    , m_descBgDesatSlider(nullptr)
    , m_descBgDesatValLabel(nullptr)
    , m_descBgVignetteSlider(nullptr)
    , m_descBgVignetteValLabel(nullptr)
    , m_headerContainer(nullptr)
    , m_editorToolbar(nullptr)
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
    , m_postprocessorLabel(nullptr)
    , m_statusNotesCountLabel(nullptr)
    , m_zoomLabel(nullptr)
    , m_rightScrollArea(nullptr)
    , m_metaTitleEdit(nullptr)
    , m_metaMapperEdit(nullptr)
    , m_metaArtistEdit(nullptr)
    , m_metaDiffCombo(nullptr)
    , m_metaBpmSpin(nullptr)
    , m_metaOffsetSpin(nullptr)
    , m_metaDurationSpin(nullptr)
    , m_autoDurationBtn(nullptr)
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
            recalculateMapDuration(static_cast<int>(d));
            if (!m_isDurationManuallyEdited) {
                m_initialDurationMs = m_totalDurationMs;
            }
        }
    });

    connect(m_waveform, &AudioWaveform::waveformReady, this, [this]() {
        if (m_waveform && m_waveform->durationMs() > 0) {
            recalculateMapDuration(m_waveform->durationMs());
            if (!m_isDurationManuallyEdited) {
                m_initialDurationMs = m_totalDurationMs;
            }
        }
        m_timelineCanvas->update();
    });

    connect(m_mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        int currentMs = static_cast<int>(pos);
        m_timelineCanvas->setCurrentTime(currentMs);
        updateTimeDisplay(currentMs);
    });

    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState state) {
        if (state == QMediaPlayer::PlayingState) {
            m_playBtn->setText(QStringLiteral("||"));
            m_audioClock.restart();
            m_clockBaseMs = m_mediaPlayer->position();
            m_lastMediaPos = m_clockBaseMs;
            m_playbackTimer->start(16);
        } else {
            m_playBtn->setText(QStringLiteral("▶"));
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

BeatmapEditorWidget::~BeatmapEditorWidget()
{
    if (qApp) {
        qApp->removeEventFilter(this);
    }
    stopPlayback();
}

void BeatmapEditorWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_tabEditorBtn) {
        m_tabEditorBtn->setChecked(true);
    }
    if (m_editorStack) {
        m_editorStack->setCurrentIndex(0);
    }
    if (m_editorToolbar) {
        m_editorToolbar->setVisible(true);
    }
    if (m_timelineCanvas) {
        m_timelineCanvas->setFocus();
    }
}

void BeatmapEditorWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateCachedBackground();
}

void BeatmapEditorWidget::updateCachedBackground()
{
    if (width() <= 0 || height() <= 0) return;

    QPixmap bgPixmap = m_currentBeatmap.coverImage;
    if (bgPixmap.isNull() && !m_sourceCoverPath.isEmpty() && QFile::exists(m_sourceCoverPath)) {
        m_currentBeatmap.coverImage.load(m_sourceCoverPath);
        bgPixmap = m_currentBeatmap.coverImage;
    }
    if (bgPixmap.isNull()) {
        static QPixmap defaultBg(QStringLiteral(":/assets/images/standard_bg2.jpg"));
        bgPixmap = defaultBg;
    }
    if (bgPixmap.isNull()) {
        m_cachedBackgroundPixmap = QPixmap();
        return;
    }

    QPixmap scaled = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int cropX = (scaled.width() - width()) / 2;
    int cropY = (scaled.height() - height()) / 2;
    QImage img = scaled.copy(cropX, cropY, width(), height()).toImage();

    // Cinematic Blur: Fast Box-Filter Downsample & Upsample
    if (m_backgroundBlurRadius > 0) {
        int downFactor = 1 + (m_backgroundBlurRadius * 2);
        int dw = qMax(16, width() / downFactor);
        int dh = qMax(16, height() / downFactor);
        QImage small = img.scaled(dw, dh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        img = small.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Desaturation: Blend RGB toward Greyscale
    if (m_desaturationPercent > 0) {
        img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        int desatFactor = qBound(0, m_desaturationPercent, 100);
        int invFactor = 100 - desatFactor;
        int h = img.height();
        int w = img.width();
        for (int y = 0; y < h; ++y) {
            auto *scanLine = reinterpret_cast<QRgb*>(img.scanLine(y));
            for (int x = 0; x < w; ++x) {
                QRgb p = scanLine[x];
                int r = qRed(p);
                int g = qGreen(p);
                int b = qBlue(p);
                int gray = (r * 77 + g * 150 + b * 29) >> 8;
                int nr = (r * invFactor + gray * desatFactor) / 100;
                int ng = (g * invFactor + gray * desatFactor) / 100;
                int nb = (b * invFactor + gray * desatFactor) / 100;
                scanLine[x] = qRgba(nr, ng, nb, qAlpha(p));
            }
        }
    }

    m_cachedBackgroundPixmap = QPixmap::fromImage(img);
}

void BeatmapEditorWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (m_cachedBackgroundPixmap.isNull() || m_cachedBackgroundPixmap.size() != size()) {
        updateCachedBackground();
    }

    if (!m_cachedBackgroundPixmap.isNull()) {
        painter.drawPixmap(0, 0, m_cachedBackgroundPixmap);

        // 1. Dark dimming overlay based on configurable background dim percent
        int dimAlpha = qBound(0, static_cast<int>(m_backgroundDimPercent * 2.55), 255);
        if (dimAlpha > 0) {
            painter.fillRect(rect(), QColor(16, 10, 28, dimAlpha));
        }

        // 2. Cinematic Vignette (Radial gradient focusing attention on center)
        if (m_vignettePercent > 0) {
            double vigStrength = m_vignettePercent / 100.0;
            QRadialGradient radial(rect().center(), qMax(width(), height()) * 0.72);
            radial.setColorAt(0.0, QColor(0, 0, 0, 0));
            radial.setColorAt(0.45, QColor(0, 0, 0, static_cast<int>(35 * vigStrength)));
            radial.setColorAt(1.0, QColor(10, 6, 20, static_cast<int>(230 * vigStrength)));
            painter.fillRect(rect(), radial);
        }
    } else {
        Theme::paintWindowBackground(&painter, rect());
    }
}
bool BeatmapEditorWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (!isVisible()) {
        return QWidget::eventFilter(watched, event);
    }

    QWidget *targetWidget = qobject_cast<QWidget *>(watched);
    if (targetWidget && !this->isAncestorOf(targetWidget) && targetWidget != this) {
        return QWidget::eventFilter(watched, event);
    }

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
    // Quick Save (Ctrl+S / Cmd+S)
    auto *scSave = new QShortcut(QKeySequence::Save, this);
    scSave->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scSave, &QShortcut::activated, this, &BeatmapEditorWidget::handleSaveMap);

    // Undo
    auto *scUndo = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z), this);
    scUndo->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scUndo, &QShortcut::activated, this, &BeatmapEditorWidget::handleUndo);

    // Redo
    auto *scRedo1 = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y), this);
    scRedo1->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scRedo1, &QShortcut::activated, this, &BeatmapEditorWidget::handleRedo);

    auto *scRedo2 = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z), this);
    scRedo2->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scRedo2, &QShortcut::activated, this, &BeatmapEditorWidget::handleRedo);

    // Delete
    auto *scDel = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    scDel->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scDel, &QShortcut::activated, this, &BeatmapEditorWidget::handleDeleteSelection);

    auto *scBksp = new QShortcut(QKeySequence(Qt::Key_Backspace), this);
    scBksp->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scBksp, &QShortcut::activated, this, &BeatmapEditorWidget::handleDeleteSelection);

    // Step navigation
    auto *scLeft = new QShortcut(QKeySequence(Qt::Key_Left), this);
    scLeft->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scLeft, &QShortcut::activated, this, &BeatmapEditorWidget::handleStepBackward);

    auto *scRight = new QShortcut(QKeySequence(Qt::Key_Right), this);
    scRight->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scRight, &QShortcut::activated, this, &BeatmapEditorWidget::handleStepForward);

    // Help (F1)
    auto *scHelp = new QShortcut(QKeySequence(Qt::Key_F1), this);
    scHelp->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scHelp, &QShortcut::activated, this, &BeatmapEditorWidget::handleShowHelp);

    // Select All
    auto *scAll = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_A), this);
    scAll->setContext(Qt::WidgetWithChildrenShortcut);
    connect(scAll, &QShortcut::activated, this, [this]() {
        m_timelineCanvas->selectAll();
    });
}

void BeatmapEditorWidget::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 10, 12, 12);
    rootLayout->setSpacing(8);

    // ========================================================================
    // Top Bar 1: Header Navigation (Back + Section Tabs + Title + Save/Export Actions)
    // Placed separately and distinctly above the editor controls bar
    // ========================================================================
    m_headerContainer = new QFrame(this);
    m_headerContainer->setStyleSheet(Theme::editorCardStyle());
    m_headerContainer->setFixedHeight(46);

    auto *headerLayout = new QHBoxLayout(m_headerContainer);
    headerLayout->setContentsMargins(10, 6, 10, 6);
    headerLayout->setSpacing(8);
    headerLayout->setAlignment(Qt::AlignVCenter);

    // 1. Back button
    m_backBtn = new QPushButton(QStringLiteral("← Назад"), m_headerContainer);
    m_backBtn->setFocusPolicy(Qt::NoFocus);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_backBtn->setFixedSize(76, 32);
    connect(m_backBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleBackButtonClicked);
    headerLayout->addWidget(m_backBtn);

    // Separator
    auto *sepNav = new QFrame(m_headerContainer);
    sepNav->setFrameShape(QFrame::VLine);
    sepNav->setStyleSheet(QStringLiteral("background: #5a388f; max-width: 1px; margin: 4px 2px;"));
    headerLayout->addWidget(sepNav);

    // 2. Section Navigation Tabs: [Редактор] [Описание] [Тайминг] [Тест]
    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    auto *tabsContainer = new QWidget(m_headerContainer);
    tabsContainer->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *tabsLayout = new QHBoxLayout(tabsContainer);
    tabsLayout->setContentsMargins(0, 0, 0, 0);
    tabsLayout->setSpacing(6);

    m_tabEditorBtn = new QPushButton(QStringLiteral("Редактор"), tabsContainer);
    m_tabEditorBtn->setFocusPolicy(Qt::NoFocus);
    m_tabEditorBtn->setCheckable(true);
    m_tabEditorBtn->setChecked(true);
    m_tabEditorBtn->setCursor(Qt::PointingHandCursor);
    m_tabEditorBtn->setStyleSheet(Theme::tabButtonStyle());
    m_tabEditorBtn->setFixedHeight(32);
    m_tabGroup->addButton(m_tabEditorBtn, 0);
    tabsLayout->addWidget(m_tabEditorBtn);

    m_tabDescriptionBtn = new QPushButton(QStringLiteral("Описание"), tabsContainer);
    m_tabDescriptionBtn->setFocusPolicy(Qt::NoFocus);
    m_tabDescriptionBtn->setCheckable(true);
    m_tabDescriptionBtn->setCursor(Qt::PointingHandCursor);
    m_tabDescriptionBtn->setStyleSheet(Theme::tabButtonStyle());
    m_tabDescriptionBtn->setFixedHeight(32);
    m_tabGroup->addButton(m_tabDescriptionBtn, 1);
    tabsLayout->addWidget(m_tabDescriptionBtn);

    m_tabTimingBtn = new QPushButton(QStringLiteral("Тайминг"), tabsContainer);
    m_tabTimingBtn->setFocusPolicy(Qt::NoFocus);
    m_tabTimingBtn->setCheckable(true);
    m_tabTimingBtn->setCursor(Qt::PointingHandCursor);
    m_tabTimingBtn->setStyleSheet(Theme::tabButtonStyle());
    m_tabTimingBtn->setFixedHeight(32);
    m_tabGroup->addButton(m_tabTimingBtn, 2);
    tabsLayout->addWidget(m_tabTimingBtn);

    m_tabTestBtn = new QPushButton(QStringLiteral("Тест"), tabsContainer);
    m_tabTestBtn->setFocusPolicy(Qt::NoFocus);
    m_tabTestBtn->setCheckable(true);
    m_tabTestBtn->setCursor(Qt::PointingHandCursor);
    m_tabTestBtn->setStyleSheet(Theme::tabButtonStyle());
    m_tabTestBtn->setFixedHeight(32);
    m_tabGroup->addButton(m_tabTestBtn, 3);
    tabsLayout->addWidget(m_tabTestBtn);

    connect(m_tabGroup, &QButtonGroup::idClicked, this, &BeatmapEditorWidget::handleTabChanged);
    headerLayout->addWidget(tabsContainer);

    headerLayout->addStretch(1);

    // Map Title display in header
    m_mapTitleLabel = new QLabel(QStringLiteral("Новая карта"), m_headerContainer);
    m_mapTitleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 13px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "padding: 0 8px;"
    ));
    headerLayout->addWidget(m_mapTitleLabel);

    // Save Button
    m_saveBtn = new QPushButton(QStringLiteral("Сохранить"), m_headerContainer);
    m_saveBtn->setFocusPolicy(Qt::NoFocus);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_saveBtn->setFixedSize(98, 32);
    m_saveBtn->setToolTip(QStringLiteral("Сохранить карту (Ctrl+S)"));
    m_saveBtn->setShortcut(QKeySequence::Save);
    connect(m_saveBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleSaveMap);
    headerLayout->addWidget(m_saveBtn);

    // Export Button
    m_exportBtn = new QPushButton(QStringLiteral("Экспорт"), m_headerContainer);
    m_exportBtn->setFocusPolicy(Qt::NoFocus);
    m_exportBtn->setCursor(Qt::PointingHandCursor);
    m_exportBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_exportBtn->setFixedSize(88, 32);
    m_exportBtn->setToolTip(QStringLiteral("Экспортировать карту в файл .bkey"));
    connect(m_exportBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleExportBkey);
    headerLayout->addWidget(m_exportBtn);

    m_saveStatusLabel = new QLabel(m_headerContainer);
    m_saveStatusLabel->setStyleSheet(QStringLiteral("font-size: 11px; color: #2ecc71; font-weight: bold; margin-left: 4px;"));
    m_saveStatusLabel->setVisible(false);
    headerLayout->addWidget(m_saveStatusLabel);

    rootLayout->addWidget(m_headerContainer);

    // ========================================================================
    // Top Bar 2: Editor Toolbar (Transport, Speed, Time, Undo/Redo, Tap Record, Help)
    // Sits directly below the tabs header
    // ========================================================================
    m_editorToolbar = new QFrame(this);
    m_editorToolbar->setStyleSheet(Theme::editorCardStyle());
    m_editorToolbar->setFixedHeight(44);

    auto *toolbarContainerLayout = new QVBoxLayout(m_editorToolbar);
    toolbarContainerLayout->setContentsMargins(6, 4, 6, 4);
    toolbarContainerLayout->setSpacing(0);

    m_topScrollArea = new QScrollArea(m_editorToolbar);
    m_topScrollArea->setWidgetResizable(true);
    m_topScrollArea->setFrameShape(QFrame::NoFrame);
    m_topScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_topScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_topScrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:horizontal { height: 4px; background: rgba(20, 14, 34, 180); border-radius: 2px; }"
        "QScrollBar::handle:horizontal { background: #5a388f; border-radius: 2px; min-width: 20px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
    ));

    auto *toolbarScrollContent = new QWidget(m_topScrollArea);
    toolbarScrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *controlsRow = new QHBoxLayout(toolbarScrollContent);
    controlsRow->setContentsMargins(0, 0, 0, 0);
    controlsRow->setSpacing(6);
    controlsRow->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    // Playback Transport Controls
    m_stepBackBtn = new QPushButton(QStringLiteral("◄◄"), toolbarScrollContent);
    m_stepBackBtn->setFocusPolicy(Qt::NoFocus);
    m_stepBackBtn->setCursor(Qt::PointingHandCursor);
    m_stepBackBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stepBackBtn->setFixedSize(36, 32);
    m_stepBackBtn->setToolTip(QStringLiteral("Шаг назад (←)"));
    connect(m_stepBackBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStepBackward);
    controlsRow->addWidget(m_stepBackBtn);

    m_playBtn = new QPushButton(QStringLiteral("▶"), toolbarScrollContent);
    m_playBtn->setFocusPolicy(Qt::NoFocus);
    m_playBtn->setCursor(Qt::PointingHandCursor);
    m_playBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_playBtn->setFixedSize(40, 32);
    m_playBtn->setToolTip(QStringLiteral("Воспроизведение / Пауза (Пробел)"));
    connect(m_playBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handlePlayPauseToggle);
    controlsRow->addWidget(m_playBtn);

    m_stepForwardBtn = new QPushButton(QStringLiteral("►►"), toolbarScrollContent);
    m_stepForwardBtn->setFocusPolicy(Qt::NoFocus);
    m_stepForwardBtn->setCursor(Qt::PointingHandCursor);
    m_stepForwardBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stepForwardBtn->setFixedSize(36, 32);
    m_stepForwardBtn->setToolTip(QStringLiteral("Шаг вперед (→)"));
    connect(m_stepForwardBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStepForward);
    controlsRow->addWidget(m_stepForwardBtn);

    m_stopBtn = new QPushButton(QStringLiteral("■"), toolbarScrollContent);
    m_stopBtn->setFocusPolicy(Qt::NoFocus);
    m_stopBtn->setCursor(Qt::PointingHandCursor);
    m_stopBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_stopBtn->setFixedSize(36, 32);
    m_stopBtn->setToolTip(QStringLiteral("Остановить"));
    connect(m_stopBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleStop);
    controlsRow->addWidget(m_stopBtn);

    m_speedCombo = new QComboBox(toolbarScrollContent);
    m_speedCombo->addItem(QStringLiteral("0.5x"), 0.5);
    m_speedCombo->addItem(QStringLiteral("0.75x"), 0.75);
    m_speedCombo->addItem(QStringLiteral("1.0x"), 1.0);
    m_speedCombo->setCurrentIndex(2);
    m_speedCombo->setFocusPolicy(Qt::NoFocus);
    m_speedCombo->setCursor(Qt::PointingHandCursor);
    m_speedCombo->setFixedSize(68, 32);
    m_speedCombo->setStyleSheet(Theme::comboBoxStyle());
    connect(m_speedCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleSpeedChanged);
    controlsRow->addWidget(m_speedCombo);

    m_timeLabel = new QLabel(QStringLiteral("00:00.000 / 02:00.000"), toolbarScrollContent);
    m_timeLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Consolas', 'Courier New', monospace;"
        "font-size: 11px;"
        "color: #bb9af7;"
        "padding: 0 4px;"
    ));
    controlsRow->addWidget(m_timeLabel);

    controlsRow->addStretch(1);

    // Undo / Redo
    m_undoBtn = new QPushButton(QStringLiteral("↺ Отмена"), toolbarScrollContent);
    m_undoBtn->setFocusPolicy(Qt::NoFocus);
    m_undoBtn->setCursor(Qt::PointingHandCursor);
    m_undoBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_undoBtn->setFixedSize(86, 32);
    m_undoBtn->setToolTip(QStringLiteral("Отменить действие (Ctrl+Z)"));
    m_undoBtn->setEnabled(false);
    connect(m_undoBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleUndo);
    controlsRow->addWidget(m_undoBtn);

    m_redoBtn = new QPushButton(QStringLiteral("↻ Повтор"), toolbarScrollContent);
    m_redoBtn->setFocusPolicy(Qt::NoFocus);
    m_redoBtn->setCursor(Qt::PointingHandCursor);
    m_redoBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_redoBtn->setFixedSize(86, 32);
    m_redoBtn->setToolTip(QStringLiteral("Повторить действие (Ctrl+Y)"));
    m_redoBtn->setEnabled(false);
    connect(m_redoBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleRedo);
    controlsRow->addWidget(m_redoBtn);

    // Tap Record button
    m_tapRecordBtn = new QPushButton(QStringLiteral("REC (R)"), toolbarScrollContent);
    m_tapRecordBtn->setFocusPolicy(Qt::NoFocus);
    m_tapRecordBtn->setCheckable(true);
    m_tapRecordBtn->setCursor(Qt::PointingHandCursor);
    m_tapRecordBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_tapRecordBtn->setFixedSize(82, 32);
    m_tapRecordBtn->setToolTip(QStringLiteral("Режим записи на лету (Горячая клавиша: R)"));
    connect(m_tapRecordBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleToggleTapRecord);
    controlsRow->addWidget(m_tapRecordBtn);

    // Help Button
    m_helpBtn = new QPushButton(QStringLiteral("? Справка"), toolbarScrollContent);
    m_helpBtn->setFocusPolicy(Qt::NoFocus);
    m_helpBtn->setCursor(Qt::PointingHandCursor);
    m_helpBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_helpBtn->setFixedSize(88, 32);
    m_helpBtn->setToolTip(QStringLiteral("Справка по горячим клавишам и редактору (F1)"));
    connect(m_helpBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleShowHelp);
    controlsRow->addWidget(m_helpBtn);

    m_topScrollArea->setWidget(toolbarScrollContent);
    toolbarContainerLayout->addWidget(m_topScrollArea);
    rootLayout->addWidget(m_editorToolbar);

    // ========================================================================
    // Stack of Sections: Editor, Description, Timing, Test
    // ========================================================================
    m_editorStack = new QStackedWidget(this);
    m_editorStack->addWidget(createEditorPage());
    m_editorStack->addWidget(createDescriptionPage());
    m_editorStack->addWidget(createPlaceholderPage(
        QStringLiteral("Раздел «Тайминг»"),
        QStringLiteral("Раздел «Тайминг» будет доступен в будущем")
    ));
    m_editorStack->addWidget(createPlaceholderPage(
        QStringLiteral("Раздел «Тестирование»"),
        QStringLiteral("Раздел «Тестирование» будет доступен в будущем")
    ));

    rootLayout->addWidget(m_editorStack, 1);
}

QWidget* BeatmapEditorWidget::createEditorPage()
{
    auto *page = new QWidget(this);
    auto *workLayout = new QHBoxLayout(page);
    workLayout->setContentsMargins(0, 0, 0, 0);
    workLayout->setSpacing(10);

    // 1. Left Toolstrip (Keys 1 and 2)
    auto *toolPanel = new QFrame(page);
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
    m_placeToolBtn->setToolTip(QStringLiteral("Размещение нот (Клавиша 2)"));

    m_toolGroup = new QButtonGroup(this);
    m_toolGroup->addButton(m_selectToolBtn, static_cast<int>(EditorTool::Select));
    m_toolGroup->addButton(m_placeToolBtn, static_cast<int>(EditorTool::Place));
    m_toolGroup->setExclusive(true);
    connect(m_toolGroup, &QButtonGroup::idClicked, this, &BeatmapEditorWidget::handleToolChanged);

    toolLayout->addWidget(m_selectToolBtn);
    toolLayout->addWidget(m_placeToolBtn);
    workLayout->addWidget(toolPanel);

    // 2. Center Timeline Canvas Container
    auto *centerContainer = new QFrame(page);
    centerContainer->setStyleSheet(Theme::editorCardStyle());
    auto *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(6, 6, 6, 6);
    centerLayout->setSpacing(4);

    // Scrubber above notes canvas
    m_scrubSlider = new ClickableSlider(Qt::Horizontal, centerContainer);
    m_scrubSlider->setFocusPolicy(Qt::NoFocus);
    m_scrubSlider->setRange(0, m_totalDurationMs);
    m_scrubSlider->setValue(0);
    m_scrubSlider->setCursor(Qt::PointingHandCursor);
    m_scrubSlider->setStyleSheet(Theme::sliderStyle());
    connect(m_scrubSlider, &QSlider::sliderPressed, this, [this]() {
        m_isScrubbing = true;
        m_userScrolledAway = true;
    });
    connect(m_scrubSlider, &QSlider::sliderMoved, this, &BeatmapEditorWidget::handleSliderMoved);
    connect(m_scrubSlider, &QSlider::sliderReleased, this, &BeatmapEditorWidget::handleSliderReleased);
    connect(m_scrubSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_isScrubbing) {
            handleSliderMoved(value);
        }
    });
    centerLayout->addWidget(m_scrubSlider);

    m_timelineCanvas = new EditorTimelineCanvas(centerContainer);
    m_timelineCanvas->setWaveform(m_waveform);
    centerLayout->addWidget(m_timelineCanvas, 1);

    // Canvas Status Strip (Cursor, Notes Count, Zoom)
    auto *statusStrip = new QHBoxLayout();
    statusStrip->setContentsMargins(8, 2, 8, 2);

    m_postprocessorLabel = new QLabel(QStringLiteral("Курсор: 00:00.000 | Такт 1, Доля 1.00 | Сетка: 1/4"), centerContainer);
    m_postprocessorLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold; background: transparent; border: none;"));
    statusStrip->addWidget(m_postprocessorLabel);

    statusStrip->addStretch(1);

    m_statusNotesCountLabel = new QLabel(QStringLiteral("Всего нот: 0"), centerContainer);
    m_statusNotesCountLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-size: 11px; font-weight: bold; background: transparent; border: none;"));
    statusStrip->addWidget(m_statusNotesCountLabel);

    statusStrip->addStretch(1);

    m_zoomLabel = new QLabel(QStringLiteral("Масштаб: 100%"), centerContainer);
    m_zoomLabel->setStyleSheet(QStringLiteral("color: #a09cb0; font-size: 11px; background: transparent; border: none;"));
    m_zoomLabel->setToolTip(QStringLiteral(
        "Горячие клавиши управления таймлайном:\n"
        "• Ctrl + Колёсико мыши — плавный масштаб\n"
        "• Shift + Колёсико мыши — ускоренная прокрутка (в 2 раза быстрее)\n"
        "• Колёсико мыши — горизонтальная прокрутка"
    ));
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
        m_zoomLabel->setText(QStringLiteral("Масштаб: %1%").arg(zoomPercent));
    });

    workLayout->addWidget(centerContainer, 1);

    // 3. Right Properties Panel
    auto *rightPanelContainer = new QFrame(page);
    rightPanelContainer->setStyleSheet(Theme::editorCardStyle());
    rightPanelContainer->setFixedWidth(290);

    auto *panelContainerLayout = new QVBoxLayout(rightPanelContainer);
    panelContainerLayout->setContentsMargins(4, 4, 4, 4);

    m_rightScrollArea = new QScrollArea(rightPanelContainer);
    m_rightScrollArea->setWidgetResizable(true);
    m_rightScrollArea->setFrameShape(QFrame::NoFrame);
    m_rightScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_rightScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_rightScrollArea->setStyleSheet(Theme::scrollAreaStyle());

    auto *scrollContent = new QWidget(m_rightScrollArea);
    scrollContent->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *rightLayout = new QVBoxLayout(scrollContent);
    rightLayout->setContentsMargins(10, 8, 12, 12);
    rightLayout->setSpacing(8);
    rightLayout->setAlignment(Qt::AlignTop);

    auto *propTitle = new QLabel(QStringLiteral("Параметры и инструменты"), scrollContent);
    propTitle->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 15px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));
    rightLayout->addWidget(propTitle);

    // Section 1: Quantization Grid
    auto *quantSec = new QLabel(QStringLiteral("СЕТКА КВАНТОВАНИЯ"), scrollContent);
    quantSec->setStyleSheet(Theme::cardHeaderStyle());
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
    m_quantPresetCombo->setCurrentIndex(3);
    m_quantPresetCombo->setStyleSheet(Theme::comboBoxStyle());
    m_quantPresetCombo->setCursor(Qt::PointingHandCursor);
    connect(m_quantPresetCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleQuantPresetChanged);
    rightLayout->addWidget(m_quantPresetCombo);

    auto *customRow = new QHBoxLayout();
    auto *customLbl = new QLabel(QStringLiteral("Шаг доли 1/N (N):"), scrollContent);
    customLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_quantCustomSpin = new QSpinBox(scrollContent);
    m_quantCustomSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_quantCustomSpin->setRange(1, 64);
    m_quantCustomSpin->setValue(4);
    m_quantCustomSpin->setAlignment(Qt::AlignCenter);
    m_quantCustomSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_quantCustomSpin, &QSpinBox::valueChanged, this, &BeatmapEditorWidget::handleQuantCustomSpinChanged);
    customRow->addWidget(customLbl);
    customRow->addStretch(1);
    customRow->addWidget(m_quantCustomSpin);
    rightLayout->addLayout(customRow);

    // Section 2: Selection Properties & Delete
    auto *sep2 = new QFrame(scrollContent);
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep2);

    auto *selSec = new QLabel(QStringLiteral("СВОЙСТВА ВЫДЕЛЕНИЯ"), scrollContent);
    selSec->setStyleSheet(Theme::cardHeaderStyle());
    rightLayout->addWidget(selSec);

    auto *selCountRow = new QHBoxLayout();
    auto *selCountText = new QLabel(QStringLiteral("Выделено нот:"), scrollContent);
    selCountText->setStyleSheet(Theme::secondaryLabelStyle());
    m_selCountLabel = new QLabel(QStringLiteral("0"), scrollContent);
    m_selCountLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11px; font-weight: bold;"));
    selCountRow->addWidget(selCountText);
    selCountRow->addStretch(1);
    selCountRow->addWidget(m_selCountLabel);
    rightLayout->addLayout(selCountRow);

    auto *selTimeRow = new QHBoxLayout();
    auto *selTimeText = new QLabel(QStringLiteral("Время удара:"), scrollContent);
    selTimeText->setStyleSheet(Theme::secondaryLabelStyle());
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

    // Section 3: Map Statistics
    auto *sep3 = new QFrame(scrollContent);
    sep3->setFrameShape(QFrame::HLine);
    sep3->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    rightLayout->addWidget(sep3);

    auto *statSec = new QLabel(QStringLiteral("СТАТИСТИКА КАРТЫ"), scrollContent);
    statSec->setStyleSheet(Theme::cardHeaderStyle());
    rightLayout->addWidget(statSec);

    auto *statRow1 = new QHBoxLayout();
    auto *totLbl = new QLabel(QStringLiteral("Всего нот:"), scrollContent);
    totLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_statTotalNotes = new QLabel(QStringLiteral("0"), scrollContent);
    m_statTotalNotes->setStyleSheet(Theme::accentLabelStyle());
    statRow1->addWidget(totLbl);
    statRow1->addStretch(1);
    statRow1->addWidget(m_statTotalNotes);
    rightLayout->addLayout(statRow1);

    auto *statRow2 = new QHBoxLayout();
    auto *durLbl = new QLabel(QStringLiteral("Длительность:"), scrollContent);
    durLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_statDuration = new QLabel(QStringLiteral("00:00"), scrollContent);
    m_statDuration->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 11px; font-weight: bold;"));
    statRow2->addWidget(durLbl);
    statRow2->addStretch(1);
    statRow2->addWidget(m_statDuration);
    rightLayout->addLayout(statRow2);

    auto *statRow3 = new QHBoxLayout();
    auto *npsLbl = new QLabel(QStringLiteral("Плотность (NPS):"), scrollContent);
    npsLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_statNps = new QLabel(QStringLiteral("0.0"), scrollContent);
    m_statNps->setStyleSheet(QStringLiteral("color: #ffaa00; font-size: 11px; font-weight: bold;"));
    statRow3->addWidget(npsLbl);
    statRow3->addStretch(1);
    statRow3->addWidget(m_statNps);
    rightLayout->addLayout(statRow3);

    m_rightScrollArea->setWidget(scrollContent);
    panelContainerLayout->addWidget(m_rightScrollArea);
    workLayout->addWidget(rightPanelContainer);

    return page;
}

QWidget* BeatmapEditorWidget::createDescriptionPage()
{
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet(Theme::scrollAreaStyle());

    auto *content = new QWidget(scrollArea);
    content->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(24, 20, 24, 24);
    mainLayout->setSpacing(16);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *headerLabel = new QLabel(QStringLiteral("Описание и метаданные карты"), content);
    headerLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #ffffff; font-family: 'Segoe UI', sans-serif;"));
    headerLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(headerLabel);

    auto *cardsRow = new QHBoxLayout();
    cardsRow->setSpacing(20);
    cardsRow->setAlignment(Qt::AlignCenter);

    // Left Card: Media & Visuals
    auto *leftCard = new QFrame(content);
    leftCard->setStyleSheet(Theme::editorCardStyle());
    leftCard->setFixedWidth(360);
    auto *leftLayout = new QVBoxLayout(leftCard);
    leftLayout->setContentsMargins(18, 16, 18, 18);
    leftLayout->setSpacing(12);

    auto *mediaTitle = new QLabel(QStringLiteral("МЕДИАФАЙЛЫ И ОБЛОЖКА"), leftCard);
    mediaTitle->setStyleSheet(Theme::cardHeaderStyle());
    leftLayout->addWidget(mediaTitle);

    m_descCoverPreview = new QLabel(leftCard);
    m_descCoverPreview->setFixedSize(200, 200);
    m_descCoverPreview->setAlignment(Qt::AlignCenter);
    m_descCoverPreview->setStyleSheet(QStringLiteral(
        "border: 2px solid #7d44cf; border-radius: 12px; background: rgba(20, 14, 34, 180);"
    ));
    auto *coverPreviewRow = new QHBoxLayout();
    coverPreviewRow->setAlignment(Qt::AlignCenter);
    coverPreviewRow->addWidget(m_descCoverPreview);
    leftLayout->addLayout(coverPreviewRow);

    m_metaChooseCoverBtn = new QPushButton(QStringLiteral("Выбрать обложку..."), leftCard);
    m_metaChooseCoverBtn->setFocusPolicy(Qt::NoFocus);
    m_metaChooseCoverBtn->setCursor(Qt::PointingHandCursor);
    m_metaChooseCoverBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_metaChooseCoverBtn->setFixedHeight(34);
    connect(m_metaChooseCoverBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleChooseCover);
    leftLayout->addWidget(m_metaChooseCoverBtn);

    m_metaCoverLabel = new QLabel(QStringLiteral("Выбрана автоматическая обложка"), leftCard);
    m_metaCoverLabel->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaCoverLabel->setAlignment(Qt::AlignCenter);
    leftLayout->addWidget(m_metaCoverLabel);

    auto *audioSep = new QFrame(leftCard);
    audioSep->setFrameShape(QFrame::HLine);
    audioSep->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    leftLayout->addWidget(audioSep);

    auto *audioHeading = new QLabel(QStringLiteral("АУДИОФАЙЛ КАРТЫ"), leftCard);
    audioHeading->setStyleSheet(Theme::cardHeaderStyle());
    leftLayout->addWidget(audioHeading);

    m_metaChooseAudioBtn = new QPushButton(QStringLiteral("Выбрать аудио..."), leftCard);
    m_metaChooseAudioBtn->setFocusPolicy(Qt::NoFocus);
    m_metaChooseAudioBtn->setCursor(Qt::PointingHandCursor);
    m_metaChooseAudioBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_metaChooseAudioBtn->setFixedHeight(34);
    connect(m_metaChooseAudioBtn, &QPushButton::clicked, this, &BeatmapEditorWidget::handleChooseAudio);
    leftLayout->addWidget(m_metaChooseAudioBtn);

    m_descAudioPathLabel = new QLabel(QStringLiteral("Не выбран"), leftCard);
    m_descAudioPathLabel->setStyleSheet(Theme::secondaryLabelStyle());
    m_descAudioPathLabel->setWordWrap(true);
    leftLayout->addWidget(m_descAudioPathLabel);

    auto *dimSep = new QFrame(leftCard);
    dimSep->setFrameShape(QFrame::HLine);
    dimSep->setStyleSheet(QStringLiteral("background: #5a388f; max-height: 1px;"));
    leftLayout->addWidget(dimSep);

    auto *visualHeading = new QLabel(QStringLiteral("ВИЗУАЛЬНЫЕ НАСТРОЙКИ ФОНА"), leftCard);
    visualHeading->setStyleSheet(Theme::cardHeaderStyle());
    leftLayout->addWidget(visualHeading);

    // 1. Background Dim Slider
    auto *dimLbl = new QLabel(QStringLiteral("Затемнение фона (0% - 100%):"), leftCard);
    dimLbl->setStyleSheet(Theme::secondaryLabelStyle());
    leftLayout->addWidget(dimLbl);

    auto *dimRow = new QHBoxLayout();
    m_descBgDimSlider = new ClickableSlider(Qt::Horizontal, leftCard);
    m_descBgDimSlider->setRange(0, 100);
    m_descBgDimSlider->setValue(m_backgroundDimPercent);
    m_descBgDimSlider->setStyleSheet(Theme::sliderStyle());
    m_descBgDimSlider->setToolTip(QStringLiteral("0% — яркий оригинальный фон, 100% — полностью черный фон"));
    connect(m_descBgDimSlider, &QSlider::valueChanged, this, &BeatmapEditorWidget::handleBgDimSliderChanged);

    m_descBgDimValLabel = new QLabel(QStringLiteral("%1%").arg(m_backgroundDimPercent), leftCard);
    m_descBgDimValLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-weight: bold; min-width: 42px; background: transparent; border: none;"));
    m_descBgDimValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    dimRow->addWidget(m_descBgDimSlider, 1);
    dimRow->addWidget(m_descBgDimValLabel);
    leftLayout->addLayout(dimRow);

    // 2. Background Blur Slider
    auto *blurLbl = new QLabel(QStringLiteral("Размытие фона (0 - 20 px):"), leftCard);
    blurLbl->setStyleSheet(Theme::secondaryLabelStyle());
    leftLayout->addWidget(blurLbl);

    auto *blurRow = new QHBoxLayout();
    m_descBgBlurSlider = new ClickableSlider(Qt::Horizontal, leftCard);
    m_descBgBlurSlider->setRange(0, 20);
    m_descBgBlurSlider->setValue(m_backgroundBlurRadius);
    m_descBgBlurSlider->setStyleSheet(Theme::sliderStyle());
    m_descBgBlurSlider->setToolTip(QStringLiteral("Размывает мелкие детали обложки для максимальной читаемости нот и звуковой волны"));
    connect(m_descBgBlurSlider, &QSlider::valueChanged, this, &BeatmapEditorWidget::handleBgBlurSliderChanged);

    m_descBgBlurValLabel = new QLabel(QStringLiteral("%1 px").arg(m_backgroundBlurRadius), leftCard);
    m_descBgBlurValLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-weight: bold; min-width: 42px; background: transparent; border: none;"));
    m_descBgBlurValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    blurRow->addWidget(m_descBgBlurSlider, 1);
    blurRow->addWidget(m_descBgBlurValLabel);
    leftLayout->addLayout(blurRow);

    // 3. Background Desaturation Slider
    auto *desatLbl = new QLabel(QStringLiteral("Приглушение насыщенности (0% - 100%):"), leftCard);
    desatLbl->setStyleSheet(Theme::secondaryLabelStyle());
    leftLayout->addWidget(desatLbl);

    auto *desatRow = new QHBoxLayout();
    m_descBgDesatSlider = new ClickableSlider(Qt::Horizontal, leftCard);
    m_descBgDesatSlider->setRange(0, 100);
    m_descBgDesatSlider->setValue(m_desaturationPercent);
    m_descBgDesatSlider->setStyleSheet(Theme::sliderStyle());
    m_descBgDesatSlider->setToolTip(QStringLiteral("Уменьшает цветность ярких фонов, чтобы они не перебивали фиолетовые и золотые акценты"));
    connect(m_descBgDesatSlider, &QSlider::valueChanged, this, &BeatmapEditorWidget::handleBgDesatSliderChanged);

    m_descBgDesatValLabel = new QLabel(QStringLiteral("%1%").arg(m_desaturationPercent), leftCard);
    m_descBgDesatValLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-weight: bold; min-width: 42px; background: transparent; border: none;"));
    m_descBgDesatValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    desatRow->addWidget(m_descBgDesatSlider, 1);
    desatRow->addWidget(m_descBgDesatValLabel);
    leftLayout->addLayout(desatRow);

    // 4. Vignette Slider
    auto *vigLbl = new QLabel(QStringLiteral("Виньетка по краям (0% - 100%):"), leftCard);
    vigLbl->setStyleSheet(Theme::secondaryLabelStyle());
    leftLayout->addWidget(vigLbl);

    auto *vigRow = new QHBoxLayout();
    m_descBgVignetteSlider = new ClickableSlider(Qt::Horizontal, leftCard);
    m_descBgVignetteSlider->setRange(0, 100);
    m_descBgVignetteSlider->setValue(m_vignettePercent);
    m_descBgVignetteSlider->setStyleSheet(Theme::sliderStyle());
    m_descBgVignetteSlider->setToolTip(QStringLiteral("Мягкое затемнение по краям и углам экрана для фокусировки внимания на нотах"));
    connect(m_descBgVignetteSlider, &QSlider::valueChanged, this, &BeatmapEditorWidget::handleBgVignetteSliderChanged);

    m_descBgVignetteValLabel = new QLabel(QStringLiteral("%1%").arg(m_vignettePercent), leftCard);
    m_descBgVignetteValLabel->setStyleSheet(QStringLiteral("color: #bb9af7; font-weight: bold; min-width: 42px; background: transparent; border: none;"));
    m_descBgVignetteValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    vigRow->addWidget(m_descBgVignetteSlider, 1);
    vigRow->addWidget(m_descBgVignetteValLabel);
    leftLayout->addLayout(vigRow);

    cardsRow->addWidget(leftCard);

    // Right Card: Metadata Form
    auto *rightCard = new QFrame(content);
    rightCard->setStyleSheet(Theme::editorCardStyle());
    rightCard->setFixedWidth(460);
    auto *formLayout = new QVBoxLayout(rightCard);
    formLayout->setContentsMargins(20, 16, 20, 18);
    formLayout->setSpacing(10);

    auto *metaTitle = new QLabel(QStringLiteral("СВЕДЕНИЯ О ТРЕКЕ"), rightCard);
    metaTitle->setStyleSheet(Theme::cardHeaderStyle());
    formLayout->addWidget(metaTitle);

    // Title
    auto *titleLbl = new QLabel(QStringLiteral("Название трека (Title):"), rightCard);
    titleLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaTitleEdit = new QLineEdit(rightCard);
    m_metaTitleEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaTitleEdit->setPlaceholderText(QStringLiteral("Название трека"));
    connect(m_metaTitleEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleTitleChanged);
    formLayout->addWidget(titleLbl);
    formLayout->addWidget(m_metaTitleEdit);

    // Artist
    auto *artistLbl = new QLabel(QStringLiteral("Исполнитель (Artist):"), rightCard);
    artistLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaArtistEdit = new QLineEdit(rightCard);
    m_metaArtistEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaArtistEdit->setPlaceholderText(QStringLiteral("Исполнитель композиции"));
    connect(m_metaArtistEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleArtistChanged);
    formLayout->addWidget(artistLbl);
    formLayout->addWidget(m_metaArtistEdit);

    // Mapper
    auto *mapperLbl = new QLabel(QStringLiteral("Автор карты (Mapper):"), rightCard);
    mapperLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaMapperEdit = new QLineEdit(rightCard);
    m_metaMapperEdit->setStyleSheet(Theme::lineEditStyle());
    m_metaMapperEdit->setPlaceholderText(QStringLiteral("Никнейм картодела"));
    connect(m_metaMapperEdit, &QLineEdit::textChanged, this, &BeatmapEditorWidget::handleMapperChanged);
    formLayout->addWidget(mapperLbl);
    formLayout->addWidget(m_metaMapperEdit);

    // Difficulty
    auto *diffLbl = new QLabel(QStringLiteral("Сложность карты:"), rightCard);
    diffLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaDiffCombo = new QComboBox(rightCard);
    m_metaDiffCombo->addItems({
        QStringLiteral("Легкая"),
        QStringLiteral("Нормальная"),
        QStringLiteral("Сложная"),
        QStringLiteral("Эксперт"),
        QStringLiteral("Безумная")
    });
    m_metaDiffCombo->setStyleSheet(Theme::comboBoxStyle());
    m_metaDiffCombo->setCursor(Qt::PointingHandCursor);
    connect(m_metaDiffCombo, &QComboBox::currentIndexChanged, this, &BeatmapEditorWidget::handleDifficultyChanged);
    formLayout->addWidget(diffLbl);
    formLayout->addWidget(m_metaDiffCombo);

    // BPM & Offset Row
    auto *tempoRow = new QHBoxLayout();
    tempoRow->setSpacing(14);

    auto *bpmCol = new QVBoxLayout();
    auto *bpmLbl = new QLabel(QStringLiteral("Темп (BPM):"), rightCard);
    bpmLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaBpmSpin = new QDoubleSpinBox(rightCard);
    m_metaBpmSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_metaBpmSpin->setRange(30.0, 300.0);
    m_metaBpmSpin->setValue(120.0);
    m_metaBpmSpin->setSingleStep(1.0);
    m_metaBpmSpin->setAlignment(Qt::AlignCenter);
    m_metaBpmSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_metaBpmSpin, &QDoubleSpinBox::valueChanged, this, &BeatmapEditorWidget::handleBpmChanged);
    bpmCol->addWidget(bpmLbl);
    bpmCol->addWidget(m_metaBpmSpin);
    tempoRow->addLayout(bpmCol, 1);

    auto *offsetCol = new QVBoxLayout();
    auto *offsetLbl = new QLabel(QStringLiteral("Смещение (Offset, мс):"), rightCard);
    offsetLbl->setStyleSheet(Theme::secondaryLabelStyle());
    m_metaOffsetSpin = new QSpinBox(rightCard);
    m_metaOffsetSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_metaOffsetSpin->setRange(-5000, 5000);
    m_metaOffsetSpin->setValue(0);
    m_metaOffsetSpin->setAlignment(Qt::AlignCenter);
    m_metaOffsetSpin->setStyleSheet(Theme::spinBoxStyle());
    connect(m_metaOffsetSpin, &QSpinBox::valueChanged, this, &BeatmapEditorWidget::handleOffsetChanged);
    offsetCol->addWidget(offsetLbl);
    offsetCol->addWidget(m_metaOffsetSpin);
    tempoRow->addLayout(offsetCol, 1);

    formLayout->addLayout(tempoRow);

    // Duration Row (Seconds with Auto-detect button)
    auto *durCol = new QVBoxLayout();
    auto *durLbl = new QLabel(QStringLiteral("Длительность карты (сек):"), rightCard);
    durLbl->setStyleSheet(Theme::secondaryLabelStyle());
    durCol->addWidget(durLbl);

    auto *durControlRow = new QHBoxLayout();
    durControlRow->setSpacing(8);

    m_metaDurationSpin = new QSpinBox(rightCard);
    m_metaDurationSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_metaDurationSpin->setRange(5, 3600);
    m_metaDurationSpin->setValue(m_totalDurationMs / 1000);
    m_metaDurationSpin->setAlignment(Qt::AlignCenter);
    m_metaDurationSpin->setStyleSheet(Theme::spinBoxStyle());
    m_metaDurationSpin->setToolTip(QStringLiteral("Общая длительность карты. Автоматически рассчитывается по треку и нотам, но может быть изменена вручную."));
    connect(m_metaDurationSpin, &QSpinBox::editingFinished, this, [this]() {
        m_isDurationManuallyEdited = true;
        setMapDuration(m_metaDurationSpin->value() * 1000);
    });
    durControlRow->addWidget(m_metaDurationSpin, 1);

    m_autoDurationBtn = new QPushButton(QStringLiteral("Авто"), rightCard);
    m_autoDurationBtn->setFocusPolicy(Qt::NoFocus);
    m_autoDurationBtn->setCursor(Qt::PointingHandCursor);
    m_autoDurationBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_autoDurationBtn->setMinimumSize(76, 32);
    m_autoDurationBtn->setToolTip(QStringLiteral("Автоматически рассчитать длительность по аудиофайлу и последней ноте"));
    connect(m_autoDurationBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        recalculateMapDuration(m_audioDurationMs);
    });
    durControlRow->addWidget(m_autoDurationBtn);

    durCol->addLayout(durControlRow);
    formLayout->addLayout(durCol);
    formLayout->addStretch(1);

    cardsRow->addWidget(rightCard);
    mainLayout->addLayout(cardsRow);
    mainLayout->addStretch(1);

    scrollArea->setWidget(content);
    return scrollArea;
}

QWidget* BeatmapEditorWidget::createPlaceholderPage(const QString &title, const QString &subtitle)
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);

    auto *card = new QFrame(page);
    card->setStyleSheet(Theme::editorCardStyle());
    card->setFixedSize(480, 240);

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setAlignment(Qt::AlignCenter);
    cardLayout->setSpacing(14);
    cardLayout->setContentsMargins(24, 24, 24, 24);

    auto *titleLbl = new QLabel(title, card);
    titleLbl->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #ffffff; font-family: 'Segoe UI', sans-serif;"));
    titleLbl->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(titleLbl);

    auto *subLbl = new QLabel(subtitle, card);
    subLbl->setStyleSheet(QStringLiteral("font-size: 14px; color: #a09cb0; font-family: 'Segoe UI', sans-serif;"));
    subLbl->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subLbl);

    auto *backBtn = new QPushButton(QStringLiteral("← Перейти в редактор"), card);
    backBtn->setFocusPolicy(Qt::NoFocus);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    backBtn->setFixedSize(200, 36);
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        m_tabEditorBtn->setChecked(true);
        handleTabChanged(0);
    });
    cardLayout->addWidget(backBtn, 0, Qt::AlignCenter);

    layout->addWidget(card);
    return page;
}

void BeatmapEditorWidget::updateCoverPreview()
{
    if (!m_descCoverPreview) return;

    QPixmap pix = m_currentBeatmap.coverImage;
    if (pix.isNull() && !m_sourceCoverPath.isEmpty() && QFile::exists(m_sourceCoverPath)) {
        m_currentBeatmap.coverImage.load(m_sourceCoverPath);
        pix = m_currentBeatmap.coverImage;
    }
    if (pix.isNull()) {
        static QPixmap defaultBg(QStringLiteral(":/assets/images/standard_bg2.jpg"));
        pix = defaultBg;
    }

    if (!pix.isNull()) {
        QPixmap scaled = pix.scaled(200, 200, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int cx = (scaled.width() - 200) / 2;
        int cy = (scaled.height() - 200) / 2;
        QPixmap cropped = scaled.copy(cx, cy, 200, 200);
        m_descCoverPreview->setPixmap(cropped);
    } else {
        m_descCoverPreview->clear();
        m_descCoverPreview->setText(QStringLiteral("Нет обложки"));
    }
}

void BeatmapEditorWidget::handleTabChanged(int index)
{
    AudioManager::instance().playClickSound();
    m_editorStack->setCurrentIndex(index);
    if (m_editorToolbar) {
        m_editorToolbar->setVisible(index == 0);
    }
    if (index == 0) {
        m_timelineCanvas->setFocus();
    } else if (index == 1) {
        updateCoverPreview();
    }
}

void BeatmapEditorWidget::updateStatusNotesCount()
{
    if (!m_statusNotesCountLabel) return;
    int total = m_currentBeatmap.notes.size();
    int selCount = m_timelineCanvas ? m_timelineCanvas->selectedNoteIndices().size() : 0;
    if (selCount > 0) {
        m_statusNotesCountLabel->setText(QStringLiteral("Всего нот: %1 | Выделено: %2").arg(total).arg(selCount));
    } else {
        m_statusNotesCountLabel->setText(QStringLiteral("Всего нот: %1").arg(total));
    }
}

void BeatmapEditorWidget::handleBgDimSliderChanged(int value)
{
    m_backgroundDimPercent = qBound(0, value, 100);
    if (m_descBgDimValLabel) {
        m_descBgDimValLabel->setText(QStringLiteral("%1%").arg(m_backgroundDimPercent));
    }
    update();
}

void BeatmapEditorWidget::handleBgBlurSliderChanged(int value)
{
    m_backgroundBlurRadius = qBound(0, value, 20);
    if (m_descBgBlurValLabel) {
        m_descBgBlurValLabel->setText(QStringLiteral("%1 px").arg(m_backgroundBlurRadius));
    }
    updateCachedBackground();
    update();
}

void BeatmapEditorWidget::handleBgDesatSliderChanged(int value)
{
    m_desaturationPercent = qBound(0, value, 100);
    if (m_descBgDesatValLabel) {
        m_descBgDesatValLabel->setText(QStringLiteral("%1%").arg(m_desaturationPercent));
    }
    updateCachedBackground();
    update();
}

void BeatmapEditorWidget::handleBgVignetteSliderChanged(int value)
{
    m_vignettePercent = qBound(0, value, 100);
    if (m_descBgVignetteValLabel) {
        m_descBgVignetteValLabel->setText(QStringLiteral("%1%").arg(m_vignettePercent));
    }
    update();
}
bool BeatmapEditorWidget::isMetadataModified() const
{
    if (m_currentBeatmap.meta.title != m_initialMeta.title) return true;
    if (m_currentBeatmap.meta.artist != m_initialMeta.artist) return true;
    if (m_currentBeatmap.meta.mapper != m_initialMeta.mapper) return true;
    if (m_currentBeatmap.meta.difficultyName != m_initialMeta.difficultyName) return true;
    if (qAbs(m_currentBeatmap.meta.bpm - m_initialMeta.bpm) > 0.001) return true;
    if (m_currentBeatmap.meta.audioOffset != m_initialMeta.audioOffset) return true;
    if (m_sourceCoverPath != m_initialCoverPath) return true;
    if (m_sourceAudioPath != m_initialAudioPath) return true;
    if (m_isDurationManuallyEdited && m_initialDurationMs > 0 && m_totalDurationMs != m_initialDurationMs) return true;
    return false;
}

bool BeatmapEditorWidget::hasUnsavedChanges() const
{
    return isMetadataModified() || !m_undoStack.isClean();
}

bool BeatmapEditorWidget::maybeSaveAndExit()
{
    if (!hasUnsavedChanges()) {
        emit backRequested();
        return true;
    }

    SavePromptDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        if (dialog.promptResult() == SavePromptDialog::Result::Save) {
            QString validationError;
            if (!validateMapData(&validationError)) {
                QMessageBox::warning(this, QStringLiteral("Ошибка проверки структуры карты"), validationError);
                return false;
            }
            handleSaveMap();
            emit backRequested();
            return true;
        } else if (dialog.promptResult() == SavePromptDialog::Result::Discard) {
            emit backRequested();
            return true;
        }
    }

    return false;
}

void BeatmapEditorWidget::handleBackButtonClicked()
{
    AudioManager::instance().playClickSound();
    maybeSaveAndExit();
}

int BeatmapEditorWidget::calculateLastNoteTime() const
{
    int lastTime = 0;
    for (const auto &n : m_currentBeatmap.notes) {
        if (n.timeMs > lastTime) {
            lastTime = n.timeMs;
        }
    }
    return lastTime;
}

void BeatmapEditorWidget::recalculateMapDuration(int audioDurationMs)
{
    if (audioDurationMs > 0) {
        m_audioDurationMs = audioDurationMs;
    }

    int lastNoteMs = calculateLastNoteTime();
    int minDurationByNotes = (lastNoteMs > 0) ? (lastNoteMs + 1000) : 0;

    // Automatic map duration: max between audio duration, notes duration, and 10 seconds minimum
    int targetDuration = std::max({ m_audioDurationMs, minDurationByNotes, 10000 });

    setMapDuration(targetDuration);
}

void BeatmapEditorWidget::setMapDuration(int durationMs)
{
    int lastNoteMs = calculateLastNoteTime();
    // Ensure we never cut off existing notes (give at least 500ms breathing room past last note)
    int clamped = std::max({ durationMs, lastNoteMs + (lastNoteMs > 0 ? 500 : 0), 5000 });

    m_totalDurationMs = clamped;
    m_timelineCanvas->setTotalDuration(m_totalDurationMs);

    if (m_scrubSlider) {
        m_scrubSlider->setRange(0, m_totalDurationMs);
    }
    if (m_metaDurationSpin) {
        m_metaDurationSpin->blockSignals(true);
        m_metaDurationSpin->setValue(m_totalDurationMs / 1000);
        m_metaDurationSpin->blockSignals(false);
    }

    updateTimeDisplay(static_cast<int>(m_mediaPlayer->position()));
    updateStats();
    update();
}

void BeatmapEditorWidget::loadBeatmap(const Beatmap &beatmap, const QString &audioPath, const QString &coverPath)
{
    stopPlayback();
    if (m_tabEditorBtn) {
        m_tabEditorBtn->setChecked(true);
    }
    if (m_editorStack) {
        m_editorStack->setCurrentIndex(0);
    }
    if (m_editorToolbar) {
        m_editorToolbar->setVisible(true);
    }
    if (m_timelineCanvas) {
        m_timelineCanvas->setFocus();
    }
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

    if (m_currentBeatmap.meta.bpm <= 0.0) {
        m_currentBeatmap.meta.bpm = 120.0;
    }
    m_metaBpmSpin->setValue(m_currentBeatmap.meta.bpm);
    m_metaOffsetSpin->setValue(m_currentBeatmap.meta.audioOffset);

    if (m_descBgDimSlider) {
        m_descBgDimSlider->setValue(m_backgroundDimPercent);
    }
    if (m_descBgDimValLabel) {
        m_descBgDimValLabel->setText(QStringLiteral("%1%").arg(m_backgroundDimPercent));
    }
    if (m_descBgBlurSlider) {
        m_descBgBlurSlider->setValue(m_backgroundBlurRadius);
    }
    if (m_descBgBlurValLabel) {
        m_descBgBlurValLabel->setText(QStringLiteral("%1 px").arg(m_backgroundBlurRadius));
    }
    if (m_descBgDesatSlider) {
        m_descBgDesatSlider->setValue(m_desaturationPercent);
    }
    if (m_descBgDesatValLabel) {
        m_descBgDesatValLabel->setText(QStringLiteral("%1%").arg(m_desaturationPercent));
    }
    if (m_descBgVignetteSlider) {
        m_descBgVignetteSlider->setValue(m_vignettePercent);
    }
    if (m_descBgVignetteValLabel) {
        m_descBgVignetteValLabel->setText(QStringLiteral("%1%").arg(m_vignettePercent));
    }
    updateCachedBackground();
    updateStatusNotesCount();
    if (m_descAudioPathLabel) {
        m_descAudioPathLabel->setText(m_currentBeatmap.meta.audioFileName.isEmpty()
            ? QStringLiteral("Не выбран")
            : m_currentBeatmap.meta.audioFileName);
    }
    updateCoverPreview();

    if (m_metaCoverLabel) {
        if (!m_sourceCoverPath.isEmpty()) {
            m_metaCoverLabel->setText(QFileInfo(m_sourceCoverPath).fileName());
        } else {
            m_metaCoverLabel->setText(QStringLiteral("Выбрана автоматическая обложка"));
        }
    }

    if (m_metaAudioLabel) {
        if (!m_sourceAudioPath.isEmpty()) {
            m_metaAudioLabel->setText(QFileInfo(m_sourceAudioPath).fileName());
        } else {
            m_metaAudioLabel->setText(QStringLiteral("Не выбран"));
        }
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
    m_userScrolledAway = false;
    m_isAutoScrolling = false;
    m_timelineCanvas->setScrollTime(0);
    m_timelineCanvas->clearSelection();

    recalculateMapDuration(m_audioDurationMs);

    m_scrubSlider->setValue(0);
    updateTimeDisplay(0);
    updatePropertiesPanel();
    updateStats();

    m_initialMeta = m_currentBeatmap.meta;
    m_initialCoverPath = m_sourceCoverPath;
    m_initialAudioPath = m_sourceAudioPath;
    m_initialDurationMs = m_totalDurationMs;
    m_isDurationManuallyEdited = false;
    m_isMetadataModified = false;
    m_undoStack.clear();
    m_undoStack.setClean();
}

void BeatmapEditorWidget::stopPlayback()
{
    m_mediaPlayer->stop();
    m_playbackTimer->stop();
    m_audioClock.invalidate();
    m_clockBaseMs = 0;
    m_lastMediaPos = 0;
    m_timelineCanvas->setCurrentTime(0);
    m_userScrolledAway = false;
    m_playheadInViewMs = 0;
    m_isAutoScrolling = true;
    m_timelineCanvas->setScrollTime(0);
    m_isAutoScrolling = false;
    m_scrubSlider->setValue(0);
    m_playBtn->setText(QStringLiteral("▶"));
    updateTimeDisplay(0);
}

void BeatmapEditorWidget::handlePlayPauseToggle()
{
    AudioManager::instance().playClickSound();
    if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer->pause();
    } else {
        m_userScrolledAway = false;
        m_playheadInViewMs = 0;
        int currentMs = static_cast<int>(m_mediaPlayer->position());
        double w = m_timelineCanvas->width();
        double pxPerMs = m_timelineCanvas->pixelsPerSecond() / 1000.0;
        double playheadX = m_timelineCanvas->timeToX(currentMs);

        // If red line is currently outside the normal viewing zone, page it into view at 15% from left
        if (w > 100 && pxPerMs > 0.0001 && (playheadX < 0 || playheadX > 0.85 * w)) {
            int newScroll = qMax(0, currentMs - static_cast<int>((0.15 * w) / pxPerMs));
            m_isAutoScrolling = true;
            m_timelineCanvas->setScrollTime(newScroll);
            m_isAutoScrolling = false;
        }

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
    m_userScrolledAway = true;
    m_timelineCanvas->setScrollTime(position);
}

void BeatmapEditorWidget::handleSliderReleased()
{
    m_isScrubbing = false;
    m_userScrolledAway = true;
    m_timelineCanvas->setScrollTime(m_scrubSlider->value());
}

void BeatmapEditorWidget::handleSeekRequested(int timeMs)
{
    m_clockBaseMs = timeMs;
    m_lastMediaPos = timeMs;
    if (m_audioClock.isValid()) {
        m_audioClock.restart();
    }
    m_timelineCanvas->setCurrentTime(timeMs);
    m_mediaPlayer->setPosition(timeMs);
    updateTimeDisplay(timeMs);

    m_userScrolledAway = false;
    m_playheadInViewMs = 0;
    double w = m_timelineCanvas->width();
    double pxPerMs = m_timelineCanvas->pixelsPerSecond() / 1000.0;
    double playheadX = m_timelineCanvas->timeToX(timeMs);

    // If seeking puts red line outside normal viewing zone, page it into view at 15% from left
    if (w > 100 && pxPerMs > 0.0001 && (playheadX < 0 || playheadX > 0.85 * w)) {
        int newScroll = qMax(0, timeMs - static_cast<int>((0.15 * w) / pxPerMs));
        m_isAutoScrolling = true;
        m_timelineCanvas->setScrollTime(newScroll);
        m_isAutoScrolling = false;
    }
}

void BeatmapEditorWidget::handleScrollChanged(int scrollTimeMs, int totalDurationMs, int visibleMs)
{
    Q_UNUSED(visibleMs);
    if (!m_isAutoScrolling) {
        m_userScrolledAway = true;
    }
    if (m_totalDurationMs > 0 && m_scrubSlider && !m_isScrubbing) {
        m_scrubSlider->blockSignals(true);
        m_scrubSlider->setRange(0, totalDurationMs);
        m_scrubSlider->setValue(scrollTimeMs);
        m_scrubSlider->blockSignals(false);
    }
}

void BeatmapEditorWidget::handleHoverInfoChanged(int timeMs, int measure, double beatInMeasure)
{
    QString timeStr = formatMsToTime(timeMs);
    m_postprocessorLabel->setText(QStringLiteral("Курсор: %1 | Такт %2, Доля %3 | Сетка: 1/%4")
        .arg(timeStr)
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
    updateStatusNotesCount();
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

        Qt::Key k1 = SettingsManager::instance().primaryKey();
        Qt::Key k2 = SettingsManager::instance().secondaryKey();
        QString k1Str = SettingsManager::keyToString(k1);
        QString k2Str = SettingsManager::keyToString(k2);
        m_saveStatusLabel->setText(QStringLiteral("ЗАПИСЬ АКТИВНА: нажимайте клавиши %1 или %2").arg(k1Str, k2Str));
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
    m_isMetadataModified = true;
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
    m_isMetadataModified = true;
    m_currentBeatmap.meta.mapper = mapper.trimmed();
    if (m_currentBeatmap.meta.mapper.isEmpty()) {
        m_metaMapperEdit->setStyleSheet(Theme::errorLineEditStyle());
    } else {
        m_metaMapperEdit->setStyleSheet(Theme::lineEditStyle());
    }
}

void BeatmapEditorWidget::handleArtistChanged(const QString &artist)
{
    m_isMetadataModified = true;
    m_currentBeatmap.meta.artist = artist.trimmed();
}

void BeatmapEditorWidget::handleDifficultyChanged(int index)
{
    m_isMetadataModified = true;
    Q_UNUSED(index);
    m_currentBeatmap.meta.difficultyName = m_metaDiffCombo->currentText();
    QString title = m_currentBeatmap.meta.title.isEmpty() ? QStringLiteral("Новая карта") : m_currentBeatmap.meta.title;
    m_mapTitleLabel->setText(QStringLiteral("%1 [%2]").arg(title, m_currentBeatmap.meta.difficultyName));
}

void BeatmapEditorWidget::handleBpmChanged(double bpm)
{
    m_isMetadataModified = true;
    m_currentBeatmap.meta.bpm = bpm;
    m_timelineCanvas->setBpm(bpm); // Instant dynamic grid re-render (Card 11)
}

void BeatmapEditorWidget::handleOffsetChanged(int offsetMs)
{
    m_isMetadataModified = true;
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
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите файл обложки карты"),
        desktopPath,
        QStringLiteral("Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)")
    );
    if (!filePath.isEmpty()) {
        m_isMetadataModified = true;
        m_sourceCoverPath = filePath;
        m_currentBeatmap.meta.coverFileName = QFileInfo(filePath).fileName();
        m_currentBeatmap.coverImage.load(filePath);
        m_metaCoverLabel->setText(QFileInfo(filePath).fileName());
        updateCoverPreview();
        updateCachedBackground();
        m_saveStatusLabel->setText(QStringLiteral("Обложка карты выбрана!"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(2500, this, [this]() { m_saveStatusLabel->setVisible(false); });
        update();
    }
}

void BeatmapEditorWidget::handleChooseAudio()
{
    AudioManager::instance().playClickSound();
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString filePath = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Выберите аудиофайл для карты"),
        desktopPath,
        QStringLiteral("Музыкальные файлы (*.wav *.mp3 *.ogg);;Все файлы (*.*)")
    );
    if (!filePath.isEmpty()) {
        m_isMetadataModified = true;
        m_sourceAudioPath = filePath;
        m_currentBeatmap.meta.audioFileName = QFileInfo(filePath).fileName();
        if (m_metaAudioLabel) {
            m_metaAudioLabel->setText(QFileInfo(filePath).fileName());
        }
        if (m_descAudioPathLabel) {
            m_descAudioPathLabel->setText(QFileInfo(filePath).fileName());
        }

        // Stop current playback if active
        if (m_mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            m_mediaPlayer->stop();
        }

        // Load new audio source into player and waveform decoder
        m_mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        m_waveform->load(filePath);

        m_saveStatusLabel->setText(QStringLiteral("Аудиофайл карты обновлен!"));
        m_saveStatusLabel->setVisible(true);
        QTimer::singleShot(2500, this, [this]() { m_saveStatusLabel->setVisible(false); });
        update();
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
        m_initialMeta = m_currentBeatmap.meta;
        m_initialCoverPath = m_sourceCoverPath;
        m_initialAudioPath = m_sourceAudioPath;
        m_initialDurationMs = m_totalDurationMs;
        m_isDurationManuallyEdited = false;
        m_isMetadataModified = false;
        m_undoStack.setClean();
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

    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString initialSavePath = QDir(desktopPath).filePath(defaultName);
    QString savePath = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Экспорт карты в файл .bkey"),
        initialSavePath,
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
    // Ctrl+S quick save
    if ((event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_S) {
        handleSaveMap();
        event->accept();
        return;
    }

    // Tap Record active: capture configured rhythm keys and Space
    if (m_isTapRecording) {
        Qt::Key k1 = SettingsManager::instance().primaryKey();
        Qt::Key k2 = SettingsManager::instance().secondaryKey();

        if (event->key() == k1 ||
            event->key() == k2) {

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
        updateTimeDisplay(interpolatedMs);

        double w = m_timelineCanvas->width();
        double pxPerMs = m_timelineCanvas->pixelsPerSecond() / 1000.0;
        if (w > 100 && pxPerMs > 0.0001 && !m_isScrubbing && !m_timelineCanvas->isInteracting()) {
            double playheadX = m_timelineCanvas->timeToX(interpolatedMs);

            if (m_userScrolledAway) {
                // User navigated away. Check if they returned and are viewing the red line:
                bool inViewingZone = (playheadX >= 0.05 * w && playheadX <= 0.85 * w);
                if (inViewingZone) {
                    m_playheadInViewMs += 16;
                    if (m_playheadInViewMs >= 700) {
                        // Watched the red line for ~0.7s: seamlessly re-magnetize!
                        m_userScrolledAway = false;
                        m_playheadInViewMs = 0;
                    }
                } else if (playheadX > 0.85 * w && playheadX < w + 80 && m_playheadInViewMs > 100) {
                    // Reached the right threshold while in view: page forward and re-magnetize
                    m_userScrolledAway = false;
                    m_playheadInViewMs = 0;
                    int newScroll = qMax(0, interpolatedMs - static_cast<int>((0.15 * w) / pxPerMs));
                    m_isAutoScrolling = true;
                    m_timelineCanvas->setScrollTime(newScroll);
                    m_isAutoScrolling = false;
                } else {
                    m_playheadInViewMs = 0;
                }
            } else {
                // Active auto-following mode (Page Scroll):
                // Red line moves across screen. When it reaches ~85% of screen width (or is off-screen),
                // page screen forward so that red line is comfortably placed at 15% from left.
                if (playheadX > 0.85 * w || playheadX < 0) {
                    int newScroll = qMax(0, interpolatedMs - static_cast<int>((0.15 * w) / pxPerMs));
                    m_isAutoScrolling = true;
                    m_timelineCanvas->setScrollTime(newScroll);
                    m_isAutoScrolling = false;
                }
            }
        }
    } else {
        int currentMs = static_cast<int>(m_mediaPlayer->position());
        m_timelineCanvas->setCurrentTime(currentMs);
        updateTimeDisplay(currentMs);
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
            m_selTimeLabel->setText(formatMsToTime(t));
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
    updateStatusNotesCount();
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

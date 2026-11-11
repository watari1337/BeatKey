#include "gameplaywidget.h"
#include "settingsmanager.h"
#include "audiomanager.h"
#include "covermanager.h"
#include "theme.h"

#include <QPainter>
#include <QPaintEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QGraphicsDropShadowEffect>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <cmath>
#include <algorithm>

// ============================================================================
// InGamePauseDialog Implementation
// ============================================================================
class InGamePauseDialog : public QDialog
{
public:
    enum Action { ActionResume, ActionRestart, ActionExit };

    explicit InGamePauseDialog(GameplayWidget *gameplay, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_gameplay(gameplay)
        , m_action(ActionResume)
        , m_isDraggingBgSlider(false)
        , m_centerCard(nullptr)
        , m_bgCard(nullptr)
        , m_previewTimer(new QTimer(this))
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);

        m_previewTimer->setSingleShot(true);
        connect(m_previewTimer, &QTimer::timeout, this, [this]() {
            setBgPreviewMode(false);
        });

        if (m_gameplay) {
            m_gameplay->installEventFilter(this);
        }

        buildUi();
    }

    ~InGamePauseDialog() override
    {
        if (m_gameplay) {
            m_gameplay->removeEventFilter(this);
        }
    }

    Action chosenAction() const { return m_action; }

protected:
    void showEvent(QShowEvent *event) override
    {
        QDialog::showEvent(event);
        syncGeometryWithGameplay();
    }

    void resizeEvent(QResizeEvent *event) override
    {
        QDialog::resizeEvent(event);
        updatePositions();
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_gameplay && event->type() == QEvent::Resize) {
            syncGeometryWithGameplay();
        }
        return QDialog::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        if (!m_isDraggingBgSlider) {
            QPainter p(this);
            p.fillRect(rect(), Theme::Colors::PauseOverlayDim);
        }
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape) {
            m_action = ActionResume;
            accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    void syncGeometryWithGameplay()
    {
        if (!m_gameplay) return;
        QPoint globalPos = m_gameplay->mapToGlobal(QPoint(0, 0));
        setGeometry(globalPos.x(), globalPos.y(), m_gameplay->width(), m_gameplay->height());
        updatePositions();
    }

    void updatePositions()
    {
        if (m_centerCard) {
            int cx = (width() - m_centerCard->width()) / 2;
            int cy = (height() - m_centerCard->height()) / 2;
            m_centerCard->move(cx, cy);
        }
        if (m_bgCard) {
            int bx = 28;
            int by = height() - m_bgCard->height() - 28;
            m_bgCard->move(bx, by);
        }
    }

    void setBgPreviewMode(bool preview)
    {
        if (m_isDraggingBgSlider == preview) return;
        m_isDraggingBgSlider = preview;
        if (m_centerCard) {
            m_centerCard->setVisible(!preview);
        }
        update();
    }

    void buildUi()
    {
        // 1. Центральная карточка: Заголовок "ПАУЗА", 3 кнопки и настройки звука
        m_centerCard = new QWidget(this);
        m_centerCard->setObjectName(QStringLiteral("pauseCenterCard"));
        m_centerCard->setStyleSheet(Theme::pauseCenterCardStyle());
        m_centerCard->setFixedSize(500, 340);

        auto *shadow = new QGraphicsDropShadowEffect(m_centerCard);
        shadow->setBlurRadius(30);
        shadow->setColor(QColor(0, 0, 0, 200));
        shadow->setOffset(0, 8);
        m_centerCard->setGraphicsEffect(shadow);

        auto *centerLayout = new QVBoxLayout(m_centerCard);
        centerLayout->setContentsMargins(28, 22, 28, 24);
        centerLayout->setSpacing(12);

        auto *titleLabel = new QLabel(QStringLiteral("ПАУЗА"), m_centerCard);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(Theme::dialogTitleStyle());
        centerLayout->addWidget(titleLabel);

        auto *buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(10);

        auto *resumeBtn = new QPushButton(QStringLiteral("Продолжить"), m_centerCard);
        resumeBtn->setCursor(Qt::PointingHandCursor);
        resumeBtn->setStyleSheet(Theme::primaryButtonStyle());
        resumeBtn->setFixedHeight(38);
        connect(resumeBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionResume;
            accept();
        });
        buttonsLayout->addWidget(resumeBtn);

        auto *retryBtn = new QPushButton(QStringLiteral("Начать заново"), m_centerCard);
        retryBtn->setCursor(Qt::PointingHandCursor);
        retryBtn->setStyleSheet(Theme::secondaryButtonStyle());
        retryBtn->setFixedHeight(38);
        connect(retryBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionRestart;
            accept();
        });
        buttonsLayout->addWidget(retryBtn);

        auto *exitBtn = new QPushButton(QStringLiteral("К выбору карт"), m_centerCard);
        exitBtn->setCursor(Qt::PointingHandCursor);
        exitBtn->setStyleSheet(Theme::dangerButtonStyle());
        exitBtn->setFixedHeight(38);
        connect(exitBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionExit;
            accept();
        });
        buttonsLayout->addWidget(exitBtn);

        centerLayout->addLayout(buttonsLayout);

        auto *sep = new QFrame(m_centerCard);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet(QStringLiteral("background: rgba(125, 68, 207, 120); max-height: 1px;"));
        centerLayout->addWidget(sep);

        auto *soundHeader = new QLabel(QStringLiteral("Настройки звука"), m_centerCard);
        soundHeader->setStyleSheet(Theme::accentLabelStyle());
        centerLayout->addWidget(soundHeader);

        auto addSoundSliderRow = [&](const QString &name, int val, int minV, int maxV, auto onChange) {
            auto *lbl = new QLabel(QStringLiteral("%1: %2%").arg(name).arg(val), m_centerCard);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, m_centerCard);
            slider->setRange(minV, maxV);
            slider->setValue(val);
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, m_centerCard, [lbl, name, onChange](int v) {
                lbl->setText(QStringLiteral("%1: %2%").arg(name).arg(v));
                onChange(v);
            });
            centerLayout->addWidget(lbl);
            centerLayout->addWidget(slider);
        };

        auto &mgr = SettingsManager::instance();
        addSoundSliderRow(QStringLiteral("Общая громкость"), mgr.masterVolume(), 0, 100, [&mgr](int v) {
            mgr.setMasterVolume(v);
        });
        addSoundSliderRow(QStringLiteral("Музыка"), mgr.musicVolume(), 0, 100, [&mgr](int v) {
            mgr.setMusicVolume(v);
        });
        addSoundSliderRow(QStringLiteral("Эффекты"), mgr.sfxVolume(), 0, 100, [&mgr](int v) {
            mgr.setSfxVolume(v);
        });

        // 2. Карточка в левом нижнем углу: Настройки фона с почти прозрачным фоном
        m_bgCard = new QWidget(this);
        m_bgCard->setObjectName(QStringLiteral("pauseGlassCard"));
        m_bgCard->setStyleSheet(Theme::pauseGlassCardStyle());
        m_bgCard->setFixedSize(320, 240);

        auto *bgShadow = new QGraphicsDropShadowEffect(m_bgCard);
        bgShadow->setBlurRadius(20);
        bgShadow->setColor(QColor(0, 0, 0, 140));
        bgShadow->setOffset(0, 4);
        m_bgCard->setGraphicsEffect(bgShadow);

        auto *bgLayout = new QVBoxLayout(m_bgCard);
        bgLayout->setContentsMargins(16, 12, 16, 14);
        bgLayout->setSpacing(4);

        auto *bgHeader = new QLabel(QStringLiteral("Отображение фона"), m_bgCard);
        bgHeader->setStyleSheet(Theme::accentLabelStyle());
        bgLayout->addWidget(bgHeader);

        auto setupBgSlider = [this](QSlider *slider) {
            connect(slider, &QSlider::sliderPressed, this, [this]() {
                m_previewTimer->stop();
                setBgPreviewMode(true);
            });
            connect(slider, &QSlider::sliderReleased, this, [this]() {
                m_previewTimer->stop();
                setBgPreviewMode(false);
            });
        };

        // Затемнение
        {
            auto *lbl = new QLabel(QStringLiteral("Затемнение: %1%").arg(m_gameplay->bgDimPercent()), m_bgCard);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, m_bgCard);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgDimPercent());
            slider->setStyleSheet(Theme::sliderStyle());
            setupBgSlider(slider);
            connect(slider, &QSlider::valueChanged, m_bgCard, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Затемнение: %1%").arg(v));
                m_gameplay->setBgDimPercent(v);
                SettingsManager::instance().setBgDimPercent(v);
                SettingsManager::instance().saveSettings();
                if (!m_isDraggingBgSlider) {
                    setBgPreviewMode(true);
                }
                m_previewTimer->start(600);
            });
            bgLayout->addWidget(lbl);
            bgLayout->addWidget(slider);
        }

        // Размытие
        {
            auto *lbl = new QLabel(QStringLiteral("Размытие: %1 px").arg(m_gameplay->bgBlurRadius()), m_bgCard);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, m_bgCard);
            slider->setRange(0, 20);
            slider->setValue(m_gameplay->bgBlurRadius());
            slider->setStyleSheet(Theme::sliderStyle());
            setupBgSlider(slider);
            connect(slider, &QSlider::valueChanged, m_bgCard, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Размытие: %1 px").arg(v));
                m_gameplay->setBgBlurRadius(v);
                SettingsManager::instance().setBgBlurRadius(v);
                SettingsManager::instance().saveSettings();
                if (!m_isDraggingBgSlider) {
                    setBgPreviewMode(true);
                }
                m_previewTimer->start(600);
            });
            bgLayout->addWidget(lbl);
            bgLayout->addWidget(slider);
        }

        // Приглушение насыщенности
        {
            auto *lbl = new QLabel(QStringLiteral("Приглушение цвета: %1%").arg(m_gameplay->bgDesatPercent()), m_bgCard);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, m_bgCard);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgDesatPercent());
            slider->setStyleSheet(Theme::sliderStyle());
            setupBgSlider(slider);
            connect(slider, &QSlider::valueChanged, m_bgCard, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Приглушение цвета: %1%").arg(v));
                m_gameplay->setBgDesatPercent(v);
                if (!m_isDraggingBgSlider) {
                    setBgPreviewMode(true);
                }
                m_previewTimer->start(600);
            });
            bgLayout->addWidget(lbl);
            bgLayout->addWidget(slider);
        }

        // Виньетка
        {
            auto *lbl = new QLabel(QStringLiteral("Виньетка: %1%").arg(m_gameplay->bgVignettePercent()), m_bgCard);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, m_bgCard);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgVignettePercent());
            slider->setStyleSheet(Theme::sliderStyle());
            setupBgSlider(slider);
            connect(slider, &QSlider::valueChanged, m_bgCard, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Виньетка: %1%").arg(v));
                m_gameplay->setBgVignettePercent(v);
                if (!m_isDraggingBgSlider) {
                    setBgPreviewMode(true);
                }
                m_previewTimer->start(600);
            });
            bgLayout->addWidget(lbl);
            bgLayout->addWidget(slider);
        }
    }

    GameplayWidget *m_gameplay;
    Action m_action;
    bool m_isDraggingBgSlider;
    QWidget *m_centerCard;
    QWidget *m_bgCard;
    QTimer *m_previewTimer;
};

// InGameFailDialog Implementation (TZ 4.7.9)
// Диалоговое окно поражения при HP <= 0 с кнопками «Повторить» и «Выход в меню»
// ============================================================================
class InGameFailDialog : public QDialog
{
public:
    enum Action { ActionRetry, ActionExit };

    explicit InGameFailDialog(int score, double accuracy, int maxCombo, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_action(ActionRetry)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);

        auto *outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(20, 20, 20, 20);

        auto *container = new QWidget(this);
        container->setObjectName(QStringLiteral("failDialogContainer"));
        container->setStyleSheet(Theme::failDialogContainerStyle());
        container->setFixedSize(500, 310);

        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(35);
        shadow->setColor(QColor(0, 0, 0, 220));
        shadow->setOffset(0, 8);
        container->setGraphicsEffect(shadow);

        auto *innerLayout = new QVBoxLayout(container);
        innerLayout->setContentsMargins(32, 26, 32, 28);
        innerLayout->setSpacing(16);

        // 1. Заголовок "ПОРАЖЕНИЕ"
        auto *titleLabel = new QLabel(QStringLiteral("ПОРАЖЕНИЕ"), container);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(Theme::failDialogTitleStyle());
        innerLayout->addWidget(titleLabel);

        auto *subLabel = new QLabel(QStringLiteral("Уровень здоровья упал до 0 единиц"), container);
        subLabel->setAlignment(Qt::AlignCenter);
        subLabel->setStyleSheet(Theme::secondaryLabelStyle());
        innerLayout->addWidget(subLabel);

        // Блок статистики попытки
        auto *statsLayout = new QHBoxLayout();
        statsLayout->setSpacing(12);

        auto createStatBox = [container](const QString &title, const QString &val) {
            auto *box = new QWidget(container);
            box->setStyleSheet(Theme::failStatBoxStyle());
            auto *ly = new QVBoxLayout(box);
            ly->setContentsMargins(10, 8, 10, 8);
            ly->setSpacing(2);
            auto *vLbl = new QLabel(val, box);
            vLbl->setAlignment(Qt::AlignCenter);
            vLbl->setStyleSheet(Theme::failStatValueStyle());
            auto *tLbl = new QLabel(title, box);
            tLbl->setAlignment(Qt::AlignCenter);
            tLbl->setStyleSheet(Theme::failStatTitleStyle());
            ly->addWidget(vLbl);
            ly->addWidget(tLbl);
            return box;
        };

        statsLayout->addWidget(createStatBox(QStringLiteral("СЧЕТ"), QString::number(score)));
        statsLayout->addWidget(createStatBox(QStringLiteral("ТОЧНОСТЬ"), QString::asprintf("%.2f%%", accuracy)));
        statsLayout->addWidget(createStatBox(QStringLiteral("МАКС. КОМБО"), QStringLiteral("%1x").arg(maxCombo)));
        innerLayout->addLayout(statsLayout);

        innerLayout->addStretch(1);

        // 2. Кнопки: «Повторить» и «Выход в меню»
        auto *buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(16);

        auto *retryBtn = new QPushButton(QStringLiteral("Повторить"), container);
        retryBtn->setCursor(Qt::PointingHandCursor);
        retryBtn->setStyleSheet(Theme::primaryButtonStyle());
        retryBtn->setFixedHeight(38);
        connect(retryBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionRetry;
            accept();
        });
        buttonsLayout->addWidget(retryBtn);

        auto *exitBtn = new QPushButton(QStringLiteral("Выход в меню"), container);
        exitBtn->setCursor(Qt::PointingHandCursor);
        exitBtn->setStyleSheet(Theme::secondaryButtonStyle());
        exitBtn->setFixedHeight(38);
        connect(exitBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionExit;
            accept();
        });
        buttonsLayout->addWidget(exitBtn);

        innerLayout->addLayout(buttonsLayout);
        outerLayout->addWidget(container, 0, Qt::AlignCenter);
    }

    Action chosenAction() const { return m_action; }

private:
    Action m_action;
};

// ============================================================================
// GameplayWidget Implementation
// ============================================================================

GameplayWidget::GameplayWidget(QWidget *parent)
    : QWidget(parent)
    , m_gameLoopTimer(new QTimer(this))
    , m_countdownTimer(new QTimer(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    // Загрузка текстур ноты и рамки зоны нажатия (скины из assets/ с fallback-ресурсами)
    loadNoteTexture();
    loadHitTargetTexture();

    m_gameLoopTimer->setTimerType(Qt::PreciseTimer);
    m_gameLoopTimer->setInterval(1); // 1 ms высокоточный опрос для максимальной плавности под любую герцовку (60/144/240 Гц)
    connect(m_gameLoopTimer, &QTimer::timeout, this, &GameplayWidget::onGameTick);

    connect(m_countdownTimer, &QTimer::timeout, this, &GameplayWidget::onCountdownTick);
}

GameplayWidget::~GameplayWidget()
{
    stopGame();
}

void GameplayWidget::startMap(const Beatmap &beatmap)
{
    stopGame();

    if (beatmap.notes.isEmpty() || beatmap.totalNotes() == 0) {
        emit exitRequested();
        return;
    }

    m_beatmap = beatmap;
    m_notes.clear();
    m_lastNoteTimeMs = 0;

    m_notes.reserve(beatmap.notes.size());
    for (const auto &noteData : beatmap.notes) {
        GameplayNote gn;
        gn.data = noteData;
        gn.isHit = false;
        gn.isMissed = false;
        m_notes.append(gn);
        if (noteData.timeMs > m_lastNoteTimeMs) {
            m_lastNoteTimeMs = noteData.timeMs;
        }
    }

    m_score = 0;
    m_combo = 0;
    m_maxCombo = 0;
    m_hp = 50;
    m_count300 = 0;
    m_count200 = 0;
    m_count100 = 0;
    m_countMiss = 0;

    m_isFailed = false;
    m_isFinished = false;
    m_isPaused = false;
    m_floatingJudgments.clear();

    m_clockBaseMs = 0;
    m_lastMediaPos = -1;
    m_pausedElapsedMs = 0.0;

    m_bgDimPercent = SettingsManager::instance().bgDimPercent();
    m_bgBlurRadius = SettingsManager::instance().bgBlurRadius();

    loadNoteTexture();
    loadHitTargetTexture();
    updateTrackGeometry();
    updateCachedBackground();

    // Вступительная задержка в 1 секунду (1000 мс) перед стартом музыки:
    // Ноты уже двигаются с правого края экрана, чтобы игрок успел подготовиться к первым нотам
    m_isLeadIn = true;
    m_leadInElapsedMs = 0;
    m_leadInClock.restart();
    m_lastRenderedTrackTimeMs = -1000.0;

    m_gameLoopTimer->start();
    setFocus();
    update();
}

void GameplayWidget::pauseGame()
{
    cancelCountdown();
    if (m_isPaused || m_isFinished || m_isFailed) return;

    if (m_isLeadIn) {
        m_leadInElapsedMs += (m_leadInClock.isValid() ? m_leadInClock.elapsed() : 0);
        m_pausedElapsedMs = static_cast<double>(m_leadInElapsedMs - LEAD_IN_DURATION_MS);
    } else {
        m_pausedElapsedMs = currentTrackTimePreciseMs();
        AudioManager::instance().pauseMusic();
    }

    m_isPaused = true;
    m_gameLoopTimer->stop();
    update();
}

void GameplayWidget::resumeGame()
{
    if (!m_isPaused) return;

    m_isPaused = false;
    if (m_isLeadIn) {
        m_leadInClock.restart();
    } else {
        double offset = static_cast<double>(SettingsManager::instance().audioOffsetMs());
        m_audioElapsedTimer.restart();
        m_audioClock.restart();
        m_clockBaseMs = static_cast<qint64>(std::floor(m_pausedElapsedMs + offset));
        m_lastMediaPos = m_clockBaseMs;
        m_lastRenderedTrackTimeMs = m_pausedElapsedMs;
        AudioManager::instance().resumeMusic();
    }
    m_gameLoopTimer->start();
    update();
}

void GameplayWidget::stopGame()
{
    cancelCountdown();
    m_gameLoopTimer->stop();
    AudioManager::instance().stopMusic();
    m_notes.clear();
    m_floatingJudgments.clear();
    m_isFinished = true;
    m_isPaused = false;
    m_isLeadIn = false;
    m_leadInElapsedMs = 0;
    m_lastRenderedTrackTimeMs = -999999.0;
    m_clockBaseMs = 0;
    m_lastMediaPos = -1;
    m_pausedElapsedMs = 0.0;
}

qint64 GameplayWidget::currentTrackTimeMs() const
{
    return static_cast<qint64>(std::floor(currentTrackTimePreciseMs()));
}

double GameplayWidget::currentTrackTimePreciseMs() const
{
    if (m_isPaused) {
        return static_cast<double>(m_pausedElapsedMs);
    }

    if (m_isLeadIn) {
        double elapsed = static_cast<double>(m_leadInElapsedMs) +
            (m_leadInClock.isValid() ? (m_leadInClock.nsecsElapsed() / 1000000.0) : 0.0);
        if (elapsed < static_cast<double>(LEAD_IN_DURATION_MS)) {
            double cur = elapsed - static_cast<double>(LEAD_IN_DURATION_MS);
            if (cur < m_lastRenderedTrackTimeMs) cur = m_lastRenderedTrackTimeMs;
            m_lastRenderedTrackTimeMs = cur;
            return cur;
        }
        return 0.0;
    }

    double offset = static_cast<double>(SettingsManager::instance().audioOffsetMs());

    if (AudioManager::instance().isMusicPlaying()) {
        qint64 curPos = AudioManager::instance().currentPositionMs();
        if (curPos != m_lastMediaPos) {
            m_lastMediaPos = curPos;
            double currentEst = static_cast<double>(m_clockBaseMs) +
                (m_audioClock.isValid() ? (m_audioClock.nsecsElapsed() / 1000000.0) : 0.0);
            double diff = static_cast<double>(curPos) - currentEst;
            if (qAbs(diff) > 300.0) {
                m_clockBaseMs = curPos;
                m_audioClock.restart();
            }
        }
        double elapsedSinceMediaUpdate = m_audioClock.isValid() ?
            (m_audioClock.nsecsElapsed() / 1000000.0) : 0.0;
        double curTime = (static_cast<double>(m_clockBaseMs) + elapsedSinceMediaUpdate) - offset;
        if (curTime < m_lastRenderedTrackTimeMs) {
            curTime = m_lastRenderedTrackTimeMs;
        }
        m_lastRenderedTrackTimeMs = curTime;
        return curTime;
    }

    if (m_audioElapsedTimer.isValid()) {
        return static_cast<double>(m_pausedElapsedMs + m_audioElapsedTimer.elapsed()) - offset;
    }
    return 0.0;
}

void GameplayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTrackGeometry();
    updateCachedBackground();
}

void GameplayWidget::updateTrackGeometry()
{
    const qreal marginX = 60.0;
    const qreal trackHeight = 110.0;
    const qreal centerY = height() * 0.52;

    m_trackRect = QRectF(marginX, centerY - trackHeight / 2.0, width() - marginX * 2.0, trackHeight);
    m_hitLineX = m_trackRect.left() + 160.0;
}

void GameplayWidget::updateCachedBackground()
{
    QPixmap bgPixmap = m_beatmap.coverImage;
    if (bgPixmap.isNull()) {
        bgPixmap = CoverManager::standardCoverPixmap(CoverManager::StandardCover::Standard2);
    }
    if (bgPixmap.isNull() || size().isEmpty() || width() <= 0 || height() <= 0) {
        m_cachedBackgroundPixmap = QPixmap();
        return;
    }

    QPixmap scaled = bgPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int cropX = (scaled.width() - width()) / 2;
    int cropY = (scaled.height() - height()) / 2;
    QImage img = scaled.copy(cropX, cropY, width(), height()).toImage();

    // Размытие фона (0 - 20 px)
    if (m_bgBlurRadius > 0) {
        int downFactor = 1 + (m_bgBlurRadius * 2);
        int dw = qMax(16, width() / downFactor);
        int dh = qMax(16, height() / downFactor);
        QImage small = img.scaled(dw, dh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        img = small.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    // Приглушение насыщенности (0% - 100%)
    if (m_bgDesatPercent > 0) {
        img = img.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        int desatFactor = qBound(0, m_bgDesatPercent, 100);
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

NoteRenderObject GameplayWidget::getNoteRenderObject(const NoteData &note,
                                                    double currentTimeMs,
                                                    const QRectF &trackRect,
                                                    qreal hitLineX) const
{
    NoteRenderObject obj;
    double diffMs = static_cast<double>(note.timeMs) - currentTimeMs;

    if (diffMs > m_travelTimeMs + 200.0 || diffMs < -200.0) {
        obj.isVisible = false;
        return obj;
    }

    qreal distance = width() - hitLineX;
    qreal pixelsPerMs = distance / m_travelTimeMs;
    qreal x = hitLineX + (diffMs * pixelsPerMs);

    obj.rect = QRectF(x - NOTE_WIDTH / 2.0,
                      trackRect.center().y() - NOTE_HEIGHT / 2.0,
                      NOTE_WIDTH,
                      NOTE_HEIGHT);
    obj.pixmap = m_scaledNotePixmap.isNull() ? m_notePixmap : m_scaledNotePixmap;
    obj.isVisible = true;
    obj.opacity = 1.0;
    return obj;
}

void GameplayWidget::onGameTick()
{
    if (m_isPaused || m_isFinished || m_isFailed) {
        return;
    }

    if (m_isLeadIn) {
        qint64 elapsed = m_leadInElapsedMs + (m_leadInClock.isValid() ? m_leadInClock.elapsed() : 0);
        if (elapsed >= LEAD_IN_DURATION_MS) {
            m_isLeadIn = false;
            if (!m_beatmap.extractedAudioPath.isEmpty()) {
                AudioManager::instance().stopMusic();
                AudioManager::instance().playTrack(m_beatmap.extractedAudioPath, false);
            }
            m_clockBaseMs = 0;
            m_lastMediaPos = -1;
            m_audioClock.restart();
            m_audioElapsedTimer.restart();
            m_pausedElapsedMs = 0;
        }
    }

    qint64 currentMs = currentTrackTimeMs();

    if (!m_isLeadIn) {
        checkAutoMisses(currentMs);
        checkGameCompletion(currentMs);
    }

    // Анимация всплывающих надписей точности
    for (auto it = m_floatingJudgments.begin(); it != m_floatingJudgments.end(); ) {
        it->yOffset += 1.8;
        it->opacity -= 0.035;
        if (it->opacity <= 0.0) {
            it = m_floatingJudgments.erase(it);
        } else {
            ++it;
        }
    }

    update();
}

void GameplayWidget::loadNoteTexture()
{
    m_notePixmap = QPixmap();

    // Поиск пользовательского скина в каталогах assets/
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/assets/note.png"),
        appDir + QStringLiteral("/assets/skins/note.png"),
        appDir + QStringLiteral("/assets/images/game_note.png"),
        QDir::currentPath() + QStringLiteral("/assets/note.png"),
        QStringLiteral(":/assets/images/game_note.png")
    };

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            QPixmap pm(path);
            if (!pm.isNull()) {
                m_notePixmap = pm;
                break;
            }
        }
    }

    if (!m_notePixmap.isNull()) {
        m_scaledNotePixmap = m_notePixmap.scaled(NOTE_WIDTH, NOTE_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        m_scaledNotePixmap = QPixmap();
    }
}

void GameplayWidget::loadHitTargetTexture()
{
    m_hitTargetPixmap = QPixmap();

    // Поиск пользовательского скина рамки зоны нажатия в каталогах assets/
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + QStringLiteral("/assets/hit_target.png"),
        appDir + QStringLiteral("/assets/target.png"),
        appDir + QStringLiteral("/assets/skins/hit_target.png"),
        appDir + QStringLiteral("/assets/images/hit_target.png"),
        QDir::currentPath() + QStringLiteral("/assets/hit_target.png"),
        QDir::currentPath() + QStringLiteral("/assets/target.png"),
        QStringLiteral(":/assets/images/hit_target.png")
    };

    for (const QString &path : candidates) {
        if (QFile::exists(path)) {
            QPixmap pm(path);
            if (!pm.isNull()) {
                m_hitTargetPixmap = pm;
                break;
            }
        }
    }

    if (!m_hitTargetPixmap.isNull()) {
        m_scaledHitTargetPixmap = m_hitTargetPixmap.scaled(NOTE_WIDTH, NOTE_HEIGHT, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        m_scaledHitTargetPixmap = QPixmap();
    }
}

void GameplayWidget::handleHitAction()
{
    if (m_isPaused || m_isFinished || m_isFailed || m_isCountingDown) {
        return;
    }

    qint64 currentMs = currentTrackTimeMs();

    // 4.7.1.2. Вступительный период: за 100 мс до момента первой ноты нажатия игнорируются
    if (!m_notes.isEmpty()) {
        int firstNoteTime = m_notes[0].data.timeMs;
        if (currentMs < firstNoteTime - 100) {
            return;
        }
    }

    int bestIndex = -1;
    qint64 bestDiff = 999999;

    for (int i = 0; i < m_notes.size(); ++i) {
        if (m_notes[i].isHit || m_notes[i].isMissed) continue;
        qint64 diff = std::abs(m_notes[i].data.timeMs - currentMs);
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIndex = i;
        }
    }

    // 4.7.3. Окна точности (<= 100 мс)
    if (bestIndex != -1 && bestDiff <= 100) {
        m_notes[bestIndex].isHit = true;
        AudioManager::instance().playHitSound();

        int baseScore = 0;
        int hpDelta = 0;
        QString text;
        QColor color;

        if (bestDiff <= 25) {
            m_count300++;
            baseScore = 300;
            hpDelta = 2; // +2 HP по ТЗ 4.7.3.1
            text = QStringLiteral("ИДЕАЛЬНО");
            color = Theme::Colors::AccentPurpleLight;
        } else if (bestDiff <= 50) {
            m_count200++;
            baseScore = 200;
            hpDelta = 1; // +1 HP по ТЗ 4.7.3.2
            text = QStringLiteral("ХОРОШО");
            color = Theme::Colors::Success;
        } else {
            m_count100++;
            baseScore = 100;
            hpDelta = 0; // 0 HP по ТЗ 4.7.3.3
            text = QStringLiteral("СОЙДЕТ");
            color = Theme::Colors::Warning;
        }

        m_combo++;
        if (m_combo > m_maxCombo) m_maxCombo = m_combo;
        int comboMultiplier = 1 + std::min(m_combo, 100) / 25;
        m_score += baseScore * comboMultiplier;

        m_hp = qBound(0, m_hp + hpDelta, 100);
        spawnFloatingJudgment(text, color);

        if (m_hp <= 0) {
            finishGame(true);
        }
    } else {
        // 4.7.5. Защита от спама клавиш вне зоны активных нот: -5 HP, сброс комбо, miss.wav
        if (m_combo >= 5) {
            AudioManager::instance().playComboBreakSound();
        }
        m_combo = 0;
        m_hp = qMax(0, m_hp - 5);
        spawnFloatingJudgment(QStringLiteral("ПРОМАХ"), Theme::Colors::Danger);
        AudioManager::instance().playMissSound();

        if (m_hp <= 0) {
            finishGame(true);
        }
    }
}

void GameplayWidget::checkAutoMisses(qint64 currentMs)
{
    for (int i = 0; i < m_notes.size(); ++i) {
        if (!m_notes[i].isHit && !m_notes[i].isMissed) {
            if (currentMs - m_notes[i].data.timeMs > 120) {
                m_notes[i].isMissed = true;
                m_countMiss++;
                if (m_combo >= 5) {
                    AudioManager::instance().playComboBreakSound();
                }
                m_combo = 0;
                m_hp = qMax(0, m_hp - 5); // Списание 5 HP по ТЗ 4.7.3.4
                spawnFloatingJudgment(QStringLiteral("ПРОМАХ"), Theme::Colors::Danger);
                AudioManager::instance().playMissSound();

                if (m_hp <= 0) {
                    finishGame(true);
                    return;
                }
            }
        }
    }
}

void GameplayWidget::checkGameCompletion(qint64 currentMs)
{
    // Игра длится до последней ноты плюс 2 секунды
    if (m_lastNoteTimeMs > 0 && currentMs >= m_lastNoteTimeMs + 2000) {
        finishGame(false);
    }
}

void GameplayWidget::spawnFloatingJudgment(const QString &text, const QColor &color)
{
    FloatingJudgment fj;
    fj.text = text;
    fj.color = color;
    fj.yOffset = 0.0;
    fj.opacity = 1.0;
    m_floatingJudgments.append(fj);
}

void GameplayWidget::finishGame(bool failed)
{
    m_isFinished = true;
    m_isFailed = failed;
    m_gameLoopTimer->stop();
    AudioManager::instance().stopMusic();

    int totalHits = m_count300 + m_count200 + m_count100 + m_countMiss;
    double accuracy = 0.0;
    if (totalHits > 0) {
        accuracy = ((300.0 * m_count300 + 200.0 * m_count200 + 100.0 * m_count100) / (300.0 * totalHits)) * 100.0;
    }

    if (failed) {
        AudioManager::instance().playFailSound();

        // 4.7.9. Диалоговое окно поражения при HP <= 0 («Повторить» / «Выход в меню»)
        InGameFailDialog dialog(m_score, accuracy, m_maxCombo, this);
        dialog.exec();

        if (dialog.chosenAction() == InGameFailDialog::ActionRetry) {
            startMap(m_beatmap);
        } else {
            stopGame();
            emit exitRequested();
        }
        return;
    }

    // 4.7.9. Успешное завершение раунда (Clear):
    GameResult result;
    result.beatmap = m_beatmap;
    result.score = m_score;
    result.maxCombo = m_maxCombo;
    result.count300 = m_count300;
    result.count200 = m_count200;
    result.count100 = m_count100;
    result.countMiss = m_countMiss;
    result.isClear = true;
    result.accuracy = accuracy;

    // 4.7.10. Градация рангов мастерства
    if (m_count200 == 0 && m_count100 == 0 && m_countMiss == 0 && result.accuracy >= 99.999) {
        result.rank = QStringLiteral("SS");
    } else if (result.accuracy >= 95.0 && m_countMiss == 0) {
        result.rank = QStringLiteral("S");
    } else if (result.accuracy >= 90.0) {
        result.rank = QStringLiteral("A");
    } else if (result.accuracy >= 80.0) {
        result.rank = QStringLiteral("B");
    } else if (result.accuracy >= 70.0) {
        result.rank = QStringLiteral("C");
    } else {
        result.rank = QStringLiteral("D");
    }

    AudioManager::instance().playClearSound();
    emit gameplayFinished(result);
}

void GameplayWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    if (m_isCountingDown) {
        if (event->key() == Qt::Key_Escape) {
            cancelCountdown();
            showPauseDialog();
        }
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        showPauseDialog();
        return;
    }

    // Проверяем соответствие клавишам из настроек
    Qt::Key primary = SettingsManager::instance().primaryKey();
    Qt::Key secondary = SettingsManager::instance().secondaryKey();

    if (SettingsManager::isKeyMatching(event, primary) || SettingsManager::isKeyMatching(event, secondary)) {
        qint64 curMs = currentTrackTimeMs();
        // Вступительный период (ТЗ 4.2): игнорирование нажатий и защита от штрафов
        if (curMs < -100) {
            return;
        }
        handleHitAction();
        return;
    }

    QWidget::keyPressEvent(event);
}

void GameplayWidget::showPauseDialog()
{
    pauseGame();

    InGamePauseDialog dialog(this, this);
    dialog.exec();

    if (dialog.chosenAction() == InGamePauseDialog::ActionResume) {
        startResumeCountdown(3);
    } else if (dialog.chosenAction() == InGamePauseDialog::ActionRestart) {
        startMap(m_beatmap);
    } else if (dialog.chosenAction() == InGamePauseDialog::ActionExit) {
        stopGame();
        emit exitRequested();
    }
}

void GameplayWidget::startResumeCountdown(int count)
{
    m_isCountingDown = true;
    m_countdownNumber = count;
    AudioManager::instance().playClickSound();
    m_countdownTimer->start(800);
    update();
}

void GameplayWidget::cancelCountdown()
{
    if (m_countdownTimer && m_countdownTimer->isActive()) {
        m_countdownTimer->stop();
    }
    m_isCountingDown = false;
    m_countdownNumber = 0;
}

void GameplayWidget::onCountdownTick()
{
    m_countdownNumber--;
    if (m_countdownNumber <= 0) {
        cancelCountdown();
        resumeGame();
    } else {
        AudioManager::instance().playClickSound();
        update();
    }
}

void GameplayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 1. Отрисовка фона с кинематографическими настройками (Затемнение, Размытие, Обесцвечивание, Виньетка)
    if (!m_cachedBackgroundPixmap.isNull()) {
        painter.drawPixmap(0, 0, m_cachedBackgroundPixmap);

        int dimAlpha = qBound(0, static_cast<int>(m_bgDimPercent * 2.55), 255);
        if (dimAlpha > 0) {
            painter.fillRect(rect(), QColor(16, 10, 28, dimAlpha));
        }

        if (m_bgVignettePercent > 0) {
            double vigStrength = m_bgVignettePercent / 100.0;
            QRadialGradient radial(rect().center(), qMax(width(), height()) * 0.72);
            radial.setColorAt(0.0, QColor(0, 0, 0, 0));
            radial.setColorAt(0.45, QColor(0, 0, 0, static_cast<int>(35 * vigStrength)));
            radial.setColorAt(1.0, QColor(10, 6, 20, static_cast<int>(230 * vigStrength)));
            painter.fillRect(rect(), radial);
        }
    } else {
        Theme::paintWindowBackground(&painter, rect());
    }

    double currentPreciseMs = currentTrackTimePreciseMs();
    qint64 currentMs = static_cast<qint64>(std::floor(currentPreciseMs));

    // 2. Горизонтальная белая полоска от края до края экрана (вместо фиолетового фона дорожки)
    qreal centerY = m_trackRect.center().y();
    painter.setPen(QPen(Theme::Colors::TrackLineWhite, 1.5, Qt::SolidLine, Qt::FlatCap));
    painter.drawLine(QPointF(0, centerY), QPointF(width(), centerY));

    // 3. Рамка зоны нажатия (полый прямоугольник той же формы, что и нота)
    QRectF hitTargetRect(m_hitLineX - NOTE_WIDTH / 2.0,
                         centerY - NOTE_HEIGHT / 2.0,
                         NOTE_WIDTH,
                         NOTE_HEIGHT);

    if (!m_scaledHitTargetPixmap.isNull()) {
        painter.drawPixmap(hitTargetRect, m_scaledHitTargetPixmap, m_scaledHitTargetPixmap.rect());
    } else {
        // Векторный fallback: пустой внутри прямоугольник с закругленными углами
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Theme::Colors::HitTargetBorder, 2.0));
        painter.drawRoundedRect(hitTargetRect, 6, 6);
    }

    // Подсказка назначенных клавиш под полым прямоугольником
    QString keyText = QStringLiteral("%1 / %2")
        .arg(SettingsManager::keyToString(SettingsManager::instance().primaryKey()),
             SettingsManager::keyToString(SettingsManager::instance().secondaryKey()));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    painter.setPen(Theme::Colors::AccentPurpleLight);
    painter.drawText(QRectF(m_hitLineX - 60, centerY + NOTE_HEIGHT / 2.0 + 8, 120, 24), Qt::AlignCenter, keyText);

    // 4. Отрисовка летящих нот (вызов единой функции getNoteRenderObject со спрайтом из ресурсов)
    for (const auto &note : m_notes) {
        if (note.isHit) continue;

        NoteRenderObject obj = getNoteRenderObject(note.data, currentPreciseMs, m_trackRect, m_hitLineX);
        if (!obj.isVisible) continue;

        if (!obj.pixmap.isNull()) {
            painter.drawPixmap(obj.rect, obj.pixmap, obj.pixmap.rect());
        } else {
            // Векторный fallback-рендеринг ноты
            QLinearGradient grad(obj.rect.topLeft(), obj.rect.bottomLeft());
            grad.setColorAt(0.0, Theme::Colors::NoteCyan);
            grad.setColorAt(1.0, Theme::Colors::NoteCyanDark);
            painter.setBrush(grad);
            painter.setPen(QPen(Theme::Colors::TextPrimary, 2.0));
            painter.drawRoundedRect(obj.rect, 6, 6);
        }
    }

    // 5. Всплывающие оценки точности над линией нажатия
    for (const auto &fj : m_floatingJudgments) {
        painter.save();
        QFont font(QStringLiteral("Segoe UI"), 15, QFont::Black);
        painter.setFont(font);

        QColor textColor = fj.color;
        textColor.setAlphaF(fj.opacity);
        painter.setPen(textColor);

        QRectF textRect(m_hitLineX - 80,
                        centerY - NOTE_HEIGHT / 2.0 - 24 - fj.yOffset,
                        160,
                        30);
        painter.drawText(textRect, Qt::AlignCenter, fj.text);
        painter.restore();
    }

    // 6. Верхний HUD: Информация о треке (слева), Счет и Точность (по центру), HP Bar (справа)
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Bold));
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(40, 42, m_beatmap.meta.title.isEmpty() ? QStringLiteral("BeatKey Track") : m_beatmap.meta.title);

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11));
    painter.setPen(Theme::Colors::TextMuted);
    QString subtitle = QStringLiteral("%1 | %2 | BPM %3")
        .arg(m_beatmap.meta.artist.isEmpty() ? QStringLiteral("Unknown") : m_beatmap.meta.artist,
             m_beatmap.meta.difficultyName.isEmpty() ? QStringLiteral("Normal") : m_beatmap.meta.difficultyName)
        .arg(static_cast<int>(m_beatmap.meta.bpm));
    painter.drawText(40, 64, subtitle);

    // Прогресс-бар прохождения трека (п. 4.7.6 ТЗ)
    qint64 totalMs = qMax(static_cast<qint64>(1), static_cast<qint64>(m_lastNoteTimeMs + 1000));
    qreal progress = qBound(0.0, static_cast<double>(currentMs) / totalMs, 1.0);
    QRectF progBg(40, 76, width() - 80, 3);
    painter.fillRect(progBg, Theme::Colors::TrackProgressBarBg);
    if (progress > 0.0) {
        QRectF progFill(40, 76, (width() - 80) * progress, 3);
        painter.fillRect(progFill, Theme::Colors::AccentPurple);
    }

    // Центр: Счет и Точность
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 22, QFont::Bold));
    painter.setPen(QColor(255, 255, 255));
    QString scoreStr = QString::asprintf("%08d", m_score);
    painter.drawText(QRectF(width() / 2.0 - 150, 22, 300, 32), Qt::AlignCenter, scoreStr);

    int totalProcessed = m_count300 + m_count200 + m_count100 + m_countMiss;
    double currentAccuracy = 100.0;
    if (totalProcessed > 0) {
        currentAccuracy = ((300.0 * m_count300 + 200.0 * m_count200 + 100.0 * m_count100) / (300.0 * totalProcessed)) * 100.0;
    }
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::DemiBold));
    painter.setPen(QColor(187, 154, 247));
    painter.drawText(QRectF(width() / 2.0 - 100, 56, 200, 20), Qt::AlignCenter,
                     QString::asprintf("%.2f%%", currentAccuracy));

    // Справа: Шкала здоровья (HP)
    const qreal hpWidth = 240.0;
    const qreal hpHeight = 16.0;
    const qreal hpX = width() - hpWidth - 40.0;
    const qreal hpY = 32.0;
    QRectF hpBgRect(hpX, hpY, hpWidth, hpHeight);

    painter.setBrush(QColor(28, 20, 48, 220));
    painter.setPen(QPen(QColor(125, 68, 207), 1.5));
    painter.drawRoundedRect(hpBgRect, 8, 8);

    qreal hpRatio = qBound(0.0, m_hp / 100.0, 1.0);
    if (hpRatio > 0.0) {
        QRectF hpFillRect(hpX + 2, hpY + 2, (hpWidth - 4) * hpRatio, hpHeight - 4);
        QColor hpColor = Theme::Colors::Success;
        if (m_hp <= 25) {
            hpColor = Theme::Colors::Danger;
        } else if (m_hp <= 50) {
            hpColor = Theme::Colors::Warning;
        }
        painter.setBrush(hpColor);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(hpFillRect, 6, 6);
    }

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
    painter.setPen(QColor(255, 255, 255));
    painter.drawText(QRectF(hpX, hpY + hpHeight + 4, hpWidth, 18), Qt::AlignRight,
                     QStringLiteral("HP %1%").arg(m_hp));

    // 7. Комбо-счетчик
    if (m_combo > 0) {
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 24, QFont::Black));
        painter.setPen(Theme::Colors::TextPrimary);
        painter.drawText(QRectF(m_hitLineX - 100, centerY - NOTE_HEIGHT / 2.0 - 75, 200, 32), Qt::AlignCenter,
                         QStringLiteral("%1x").arg(m_combo));

        painter.setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
        painter.setPen(Theme::Colors::AccentPurpleLight);
        painter.drawText(QRectF(m_hitLineX - 100, centerY - NOTE_HEIGHT / 2.0 - 48, 200, 18), Qt::AlignCenter,
                         QStringLiteral("КОМБО"));
    }

    // 8. Отрисовка обратного отсчета (только при продолжении игры после паузы)
    if (m_isCountingDown) {
        // Полупрозрачное затемнение (ноты и дорожка видны сквозь него)
        painter.fillRect(rect(), Theme::Colors::CountdownOverlayDim);

        QPointF center = rect().center();
        qreal radius = 65.0;

        // Круговой бейдж с мягкой полупрозрачностью
        painter.setPen(QPen(Theme::Colors::CountdownBadgeBorder, 2.5));
        painter.setBrush(Theme::Colors::CountdownBadgeBg);
        painter.drawEllipse(center, radius, radius);

        // Число обратного отсчета
        painter.setPen(QColor(255, 255, 255, 220));
        QFont countFont(QStringLiteral("Segoe UI"), 42, QFont::Black);
        painter.setFont(countFont);

        QString countStr = QString::number(m_countdownNumber);
        QRectF numRect(center.x() - radius, center.y() - radius - 4, radius * 2, radius * 2);
        painter.drawText(numRect, Qt::AlignCenter, countStr);

        // Подпись под кругом
        painter.setPen(QColor(187, 154, 247, 200));
        QFont hintFont(QStringLiteral("Segoe UI"), 13, QFont::DemiBold);
        painter.setFont(hintFont);
        QRectF hintRect(center.x() - 150, center.y() + radius + 15, 300, 30);
        painter.drawText(hintRect, Qt::AlignCenter, QStringLiteral("Приготовьтесь..."));
    }


    // 9. Вступительное затемнение при старте: экран темный в первой половине секунды (-1000..-500мс)
    // и затем плавно осветляется за 0.5 секунды (-500..0мс)
    // Кинематографический старт: мягкое затемнение в начале (-1000..-500мс)
    // Не полное (полупрозрачное, макс. 110 из 255), плавно осветляется вдвое быстрее за ~0.5 с
    if (currentPreciseMs < -500.0) {
        double t = (-currentPreciseMs - 500.0) / 500.0;
        int fadeAlpha = qBound(0, static_cast<int>(t * 110.0), 110);
        if (fadeAlpha > 0) {
            painter.fillRect(rect(), QColor(10, 6, 20, fadeAlpha));
        }
    }
}
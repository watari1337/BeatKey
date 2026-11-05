#include "gameplaywidget.h"
#include "settingsmanager.h"
#include "audiomanager.h"
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
#include <cmath>
#include <algorithm>

// ============================================================================
// InGamePauseDialog Implementation
// Полупрозрачное окно паузы с прямым доступом к настройкам звука и фона
// ============================================================================
class InGamePauseDialog : public QDialog
{
public:
    enum Action { ActionResume, ActionRestart, ActionExit };

    explicit InGamePauseDialog(GameplayWidget *gameplay, QWidget *parent = nullptr)
        : QDialog(parent)
        , m_gameplay(gameplay)
        , m_action(ActionResume)
    {
        setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setModal(true);

        auto *outerLayout = new QVBoxLayout(this);
        outerLayout->setContentsMargins(20, 20, 20, 20);

        auto *container = new QWidget(this);
        container->setObjectName(QStringLiteral("pauseDialogContainer"));
        container->setStyleSheet(QStringLiteral(
            "#pauseDialogContainer {"
            "  background: rgba(18, 12, 34, 235);"
            "  border: 1.5px solid rgba(125, 68, 207, 210);"
            "  border-radius: 16px;"
            "}"
        ));
        container->setFixedSize(620, 480);

        auto *shadow = new QGraphicsDropShadowEffect(this);
        shadow->setBlurRadius(30);
        shadow->setColor(QColor(0, 0, 0, 200));
        shadow->setOffset(0, 8);
        container->setGraphicsEffect(shadow);

        auto *innerLayout = new QVBoxLayout(container);
        innerLayout->setContentsMargins(28, 22, 28, 24);
        innerLayout->setSpacing(14);

        // 1. Шапка: Заголовок "ПАУЗА"
        auto *titleLabel = new QLabel(QStringLiteral("ПАУЗА"), container);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet(Theme::dialogTitleStyle());
        innerLayout->addWidget(titleLabel);

        // 2. Блок основных кнопок действий
        auto *buttonsLayout = new QHBoxLayout();
        buttonsLayout->setSpacing(12);

        auto *resumeBtn = new QPushButton(QStringLiteral("Продолжить"), container);
        resumeBtn->setCursor(Qt::PointingHandCursor);
        resumeBtn->setStyleSheet(Theme::primaryButtonStyle());
        resumeBtn->setFixedHeight(38);
        connect(resumeBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionResume;
            accept();
        });
        buttonsLayout->addWidget(resumeBtn);

        auto *retryBtn = new QPushButton(QStringLiteral("Начать заново"), container);
        retryBtn->setCursor(Qt::PointingHandCursor);
        retryBtn->setStyleSheet(Theme::secondaryButtonStyle());
        retryBtn->setFixedHeight(38);
        connect(retryBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionRestart;
            accept();
        });
        buttonsLayout->addWidget(retryBtn);

        auto *exitBtn = new QPushButton(QStringLiteral("К выбору карт"), container);
        exitBtn->setCursor(Qt::PointingHandCursor);
        exitBtn->setStyleSheet(Theme::dangerButtonStyle());
        exitBtn->setFixedHeight(38);
        connect(exitBtn, &QPushButton::clicked, this, [this]() {
            m_action = ActionExit;
            accept();
        });
        buttonsLayout->addWidget(exitBtn);

        innerLayout->addLayout(buttonsLayout);

        // Разделительная линия
        auto *sep = new QFrame(container);
        sep->setFrameShape(QFrame::HLine);
        sep->setStyleSheet(QStringLiteral("background: rgba(125, 68, 207, 120); max-height: 1px;"));
        innerLayout->addWidget(sep);

        // 3. Двухколоночный блок настроек: Звук (слева) и Визуальное оформление фона (справа)
        auto *settingsColumnsLayout = new QHBoxLayout();
        settingsColumnsLayout->setSpacing(24);

        // Левая колонка: Настройки звука
        auto *soundCol = new QVBoxLayout();
        soundCol->setSpacing(8);

        auto *soundHeader = new QLabel(QStringLiteral("Настройки звука"), container);
        soundHeader->setStyleSheet(Theme::accentLabelStyle());
        soundCol->addWidget(soundHeader);

        auto addSliderRow = [&](QVBoxLayout *col, const QString &name, int val, int minV, int maxV, auto onChange) {
            auto *lbl = new QLabel(QStringLiteral("%1: %2%").arg(name).arg(val), container);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(minV, maxV);
            slider->setValue(val);
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, container, [lbl, name, onChange](int v) {
                lbl->setText(QStringLiteral("%1: %2%").arg(name).arg(v));
                onChange(v);
            });
            col->addWidget(lbl);
            col->addWidget(slider);
        };

        auto &mgr = SettingsManager::instance();
        addSliderRow(soundCol, QStringLiteral("Общая громкость"), mgr.masterVolume(), 0, 100, [&mgr](int v) {
            mgr.setMasterVolume(v);
        });
        addSliderRow(soundCol, QStringLiteral("Музыка"), mgr.musicVolume(), 0, 100, [&mgr](int v) {
            mgr.setMusicVolume(v);
        });
        addSliderRow(soundCol, QStringLiteral("Эффекты"), mgr.sfxVolume(), 0, 100, [&mgr](int v) {
            mgr.setSfxVolume(v);
        });
        soundCol->addStretch(1);
        settingsColumnsLayout->addLayout(soundCol, 1);

        // Правая колонка: Настройки фона (как в редакторе)
        auto *bgCol = new QVBoxLayout();
        bgCol->setSpacing(8);

        auto *bgHeader = new QLabel(QStringLiteral("Отображение фона"), container);
        bgHeader->setStyleSheet(Theme::accentLabelStyle());
        bgCol->addWidget(bgHeader);

        // Затемнение
        {
            auto *lbl = new QLabel(QStringLiteral("Затемнение: %1%").arg(m_gameplay->bgDimPercent()), container);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgDimPercent());
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, container, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Затемнение: %1%").arg(v));
                m_gameplay->setBgDimPercent(v);
            });
            bgCol->addWidget(lbl);
            bgCol->addWidget(slider);
        }

        // Размытие
        {
            auto *lbl = new QLabel(QStringLiteral("Размытие: %1 px").arg(m_gameplay->bgBlurRadius()), container);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(0, 20);
            slider->setValue(m_gameplay->bgBlurRadius());
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, container, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Размытие: %1 px").arg(v));
                m_gameplay->setBgBlurRadius(v);
            });
            bgCol->addWidget(lbl);
            bgCol->addWidget(slider);
        }

        // Приглушение насыщенности
        {
            auto *lbl = new QLabel(QStringLiteral("Приглушение цвета: %1%").arg(m_gameplay->bgDesatPercent()), container);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgDesatPercent());
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, container, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Приглушение цвета: %1%").arg(v));
                m_gameplay->setBgDesatPercent(v);
            });
            bgCol->addWidget(lbl);
            bgCol->addWidget(slider);
        }

        // Виньетка
        {
            auto *lbl = new QLabel(QStringLiteral("Виньетка: %1%").arg(m_gameplay->bgVignettePercent()), container);
            lbl->setStyleSheet(Theme::secondaryLabelStyle());
            auto *slider = new QSlider(Qt::Horizontal, container);
            slider->setRange(0, 100);
            slider->setValue(m_gameplay->bgVignettePercent());
            slider->setStyleSheet(Theme::sliderStyle());
            connect(slider, &QSlider::valueChanged, container, [this, lbl](int v) {
                lbl->setText(QStringLiteral("Виньетка: %1%").arg(v));
                m_gameplay->setBgVignettePercent(v);
            });
            bgCol->addWidget(lbl);
            bgCol->addWidget(slider);
        }
        bgCol->addStretch(1);
        settingsColumnsLayout->addLayout(bgCol, 1);

        innerLayout->addLayout(settingsColumnsLayout);
        outerLayout->addWidget(container);
    }

    Action chosenAction() const { return m_action; }

protected:
    void keyPressEvent(QKeyEvent *event) override
    {
        // По нажатию Esc окно закрывается и продолжается обратный отсчет так же, как кнопка "Продолжить"
        if (event->key() == Qt::Key_Escape) {
            m_action = ActionResume;
            accept();
            return;
        }
        QDialog::keyPressEvent(event);
    }

private:
    GameplayWidget *m_gameplay;
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

    // Загрузка спрайта ноты из ресурсов
    m_notePixmap = QPixmap(QStringLiteral(":/assets/images/game_note.png"));

    m_gameLoopTimer->setInterval(16); // ~60 FPS
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

    updateTrackGeometry();
    updateCachedBackground();

    // Запуск обратного отсчета перед началом матча (3.. 2.. 1..), чтобы игрок мог собраться
    startResumeCountdown(3, true);

    m_gameLoopTimer->start();
    setFocus();
    update();
}

void GameplayWidget::pauseGame()
{
    cancelCountdown();
    if (m_isPaused || m_isFinished || m_isFailed) return;

    m_isPaused = true;
    m_gameLoopTimer->stop();
    m_pausedElapsedMs = currentTrackTimeMs();
    AudioManager::instance().pauseMusic();
    update();
}

void GameplayWidget::resumeGame()
{
    if (!m_isPaused) return;

    m_isPaused = false;
    m_audioElapsedTimer.restart();
    AudioManager::instance().resumeMusic();
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
    m_isStartingCountdown = false;
}

qint64 GameplayWidget::currentTrackTimeMs() const
{
    if (m_isStartingCountdown) {
        return 0;
    }
    if (!m_audioElapsedTimer.isValid()) return 0;

    qint64 audioPos = AudioManager::instance().syncedPositionMs();
    qint64 elapsedMs = m_pausedElapsedMs + (m_isPaused ? 0 : m_audioElapsedTimer.elapsed());

    if (audioPos > 0) {
        return audioPos;
    }
    return elapsedMs;
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
    if (m_beatmap.coverImage.isNull()) {
        m_cachedBackgroundPixmap = QPixmap();
        return;
    }

    QPixmap scaled = m_beatmap.coverImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
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
                                                    qint64 currentTimeMs,
                                                    const QRectF &trackRect,
                                                    qreal hitLineX) const
{
    NoteRenderObject obj;
    qint64 diffMs = note.timeMs - currentTimeMs;

    if (diffMs > m_travelTimeMs + 200 || diffMs < -200) {
        obj.isVisible = false;
        return obj;
    }

    qreal pixelsPerMs = (trackRect.width() - hitLineX) / m_travelTimeMs;
    qreal x = hitLineX + (diffMs * pixelsPerMs);

    const qreal noteWidth = 56.0;
    const qreal noteHeight = 36.0;

    obj.rect = QRectF(x - noteWidth / 2.0,
                      trackRect.center().y() - noteHeight / 2.0,
                      noteWidth,
                      noteHeight);
    obj.pixmap = m_notePixmap;
    obj.isVisible = true;
    obj.opacity = 1.0;
    return obj;
}

void GameplayWidget::onGameTick()
{
    if (m_isPaused || m_isFinished || m_isFailed) {
        return;
    }

    qint64 currentMs = currentTrackTimeMs();

    if (!m_isStartingCountdown) {
        checkAutoMisses(currentMs);
        checkGameCompletion(currentMs);
    }

    // Затухание вспышки нажатия
    if (m_keyPressGlow > 0.0) {
        m_keyPressGlow = qMax(0.0, m_keyPressGlow - 0.08);
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

void GameplayWidget::handleHitAction()
{
    if (m_isPaused || m_isFinished || m_isFailed || m_isStartingCountdown) {
        return;
    }

    m_keyPressGlow = 1.0;

    qint64 currentMs = currentTrackTimeMs();
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

    if (bestIndex != -1 && bestDiff <= 120) {
        m_notes[bestIndex].isHit = true;
        AudioManager::instance().playHitSound();

        int baseScore = 0;
        int hpDelta = 0;
        QString text;
        QColor color;

        if (bestDiff <= 25) {
            m_count300++;
            baseScore = 300;
            hpDelta = 4;
            text = QStringLiteral("ИДЕАЛЬНО!");
            color = Theme::Colors::AccentPurpleLight;
        } else if (bestDiff <= 50) {
            m_count200++;
            baseScore = 200;
            hpDelta = 2;
            text = QStringLiteral("ХОРОШО");
            color = Theme::Colors::Success;
        } else if (bestDiff <= 100) {
            m_count100++;
            baseScore = 100;
            hpDelta = 0;
            text = QStringLiteral("СОЙДЕТ");
            color = Theme::Colors::Warning;
        } else {
            m_countMiss++;
            baseScore = 0;
            hpDelta = -6;
            text = QStringLiteral("ПРОМАХ");
            color = Theme::Colors::Danger;
        }

        if (baseScore > 0) {
            m_combo++;
            if (m_combo > m_maxCombo) m_maxCombo = m_combo;
            int comboMultiplier = 1 + std::min(m_combo, 100) / 25;
            m_score += baseScore * comboMultiplier;
        } else {
            if (m_combo >= 5) {
                AudioManager::instance().playComboBreakSound();
            }
            m_combo = 0;
        }

        m_hp = qBound(0, m_hp + hpDelta, 100);
        spawnFloatingJudgment(text, color);

        if (m_hp <= 0) {
            finishGame(true);
        }
    } else {
        // Удар в пустоту (Ghost tap)
        AudioManager::instance().playMissSound();
        m_keyPressGlow = 0.5;
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
                m_hp = qBound(0, m_hp - 6, 100);
                spawnFloatingJudgment(QStringLiteral("ПРОМАХ"), Theme::Colors::Danger);

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

    GameResult result;
    result.beatmap = m_beatmap;
    result.score = m_score;
    result.maxCombo = m_maxCombo;
    result.count300 = m_count300;
    result.count200 = m_count200;
    result.count100 = m_count100;
    result.countMiss = m_countMiss;
    result.isClear = !failed;

    int totalHits = m_count300 + m_count200 + m_count100 + m_countMiss;
    if (totalHits > 0) {
        result.accuracy = ((300.0 * m_count300 + 200.0 * m_count200 + 100.0 * m_count100) / (300.0 * totalHits)) * 100.0;
    } else {
        result.accuracy = 0.0;
    }

    if (failed) {
        result.rank = QStringLiteral("FAIL");
        AudioManager::instance().playFailSound();
    } else {
        if (result.accuracy >= 98.0 && m_countMiss == 0) result.rank = QStringLiteral("SS");
        else if (result.accuracy >= 95.0 && m_countMiss == 0) result.rank = QStringLiteral("S");
        else if (result.accuracy >= 90.0) result.rank = QStringLiteral("A");
        else if (result.accuracy >= 80.0) result.rank = QStringLiteral("B");
        else if (result.accuracy >= 70.0) result.rank = QStringLiteral("C");
        else result.rank = QStringLiteral("D");
        AudioManager::instance().playClearSound();
    }

    emit gameplayFinished(result);
}

void GameplayWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->isAutoRepeat()) {
        return;
    }

    if (m_isCountingDown || m_isStartingCountdown) {
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

    // Независимо от того, нажата ли кнопка "Продолжить" или Esc,
    // игра возобновляется с обратным отсчетом
    if (dialog.chosenAction() == InGamePauseDialog::ActionResume) {
        startResumeCountdown(3, false);
    } else if (dialog.chosenAction() == InGamePauseDialog::ActionRestart) {
        startMap(m_beatmap);
    } else if (dialog.chosenAction() == InGamePauseDialog::ActionExit) {
        stopGame();
        emit exitRequested();
    }
}

void GameplayWidget::startResumeCountdown(int count, bool isInitial)
{
    m_isCountingDown = true;
    m_isInitialCountdown = isInitial;
    m_isStartingCountdown = isInitial;
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
        if (m_isInitialCountdown) {
            m_isInitialCountdown = false;
            m_isStartingCountdown = false;
            if (!m_beatmap.extractedAudioPath.isEmpty()) {
                AudioManager::instance().stopMusic();
                AudioManager::instance().playTrack(m_beatmap.extractedAudioPath, false);
            }
            m_audioElapsedTimer.restart();
            m_pausedElapsedMs = 0;
        } else {
            resumeGame();
        }
    } else {
        AudioManager::instance().playClickSound();
        update();
    }
}

void GameplayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

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

    qint64 currentMs = currentTrackTimeMs();

    // 2. Отрисовка горизонтальной дорожки нот
    QLinearGradient trackGrad(m_trackRect.topLeft(), m_trackRect.bottomLeft());
    trackGrad.setColorAt(0.0, QColor(26, 16, 48, 170));
    trackGrad.setColorAt(0.5, QColor(36, 22, 66, 200));
    trackGrad.setColorAt(1.0, QColor(26, 16, 48, 170));
    painter.setBrush(trackGrad);
    painter.setPen(QPen(QColor(125, 68, 207, 140), 1.5));
    painter.drawRoundedRect(m_trackRect, 10, 10);

    // Внутренняя центральная пунктирная линия дорожки
    painter.setPen(QPen(QColor(125, 68, 207, 90), 1.0, Qt::DashLine));
    painter.drawLine(QPointF(m_trackRect.left() + 10, m_trackRect.center().y()),
                     QPointF(m_trackRect.right() - 10, m_trackRect.center().y()));

    // 3. Линия нажатия (без внешней обводки/рамки)
    QColor hitLineColor = (m_keyPressGlow > 0.0) ? QColor(255, 255, 255) : QColor(187, 154, 247, 180);
    qreal hitLineWidth = (m_keyPressGlow > 0.0) ? 3.5 : 2.0;

    // Вспышка свечения вдоль линии при нажатии
    if (m_keyPressGlow > 0.0) {
        QColor glowColor(0, 229, 255, static_cast<int>(150 * m_keyPressGlow));
        painter.setPen(QPen(glowColor, 8.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(m_hitLineX, m_trackRect.top() + 4),
                         QPointF(m_hitLineX, m_trackRect.bottom() - 4));
    }

    painter.setPen(QPen(hitLineColor, hitLineWidth, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(m_hitLineX, m_trackRect.top() + 4),
                     QPointF(m_hitLineX, m_trackRect.bottom() - 4));

    // Подсказка назначенных клавиш под линией
    QString keyText = QStringLiteral("%1 / %2")
        .arg(SettingsManager::keyToString(SettingsManager::instance().primaryKey()),
             SettingsManager::keyToString(SettingsManager::instance().secondaryKey()));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
    painter.setPen(QColor(187, 154, 247));
    painter.drawText(QRectF(m_hitLineX - 60, m_trackRect.bottom() + 8, 120, 24), Qt::AlignCenter, keyText);

    // 4. Отрисовка летящих нот (вызов единой функции getNoteRenderObject со спрайтом из ресурсов)
    for (const auto &note : m_notes) {
        if (note.isHit) continue;

        NoteRenderObject obj = getNoteRenderObject(note.data, currentMs, m_trackRect, m_hitLineX);
        if (!obj.isVisible) continue;

        if (!obj.pixmap.isNull()) {
            painter.drawPixmap(obj.rect.toRect(), obj.pixmap);
        } else {
            painter.setBrush(QColor(0, 229, 255));
            painter.setPen(QPen(QColor(255, 255, 255), 2.0));
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
                        m_trackRect.top() - 36 - fj.yOffset,
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
    painter.setPen(QColor(160, 156, 176));
    QString subtitle = QStringLiteral("%1 | %2 | BPM %3")
        .arg(m_beatmap.meta.artist.isEmpty() ? QStringLiteral("Unknown") : m_beatmap.meta.artist,
             m_beatmap.meta.difficultyName.isEmpty() ? QStringLiteral("Normal") : m_beatmap.meta.difficultyName)
        .arg(static_cast<int>(m_beatmap.meta.bpm));
    painter.drawText(40, 64, subtitle);

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
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(QRectF(m_hitLineX - 100, m_trackRect.top() - 75, 200, 32), Qt::AlignCenter,
                         QStringLiteral("%1x").arg(m_combo));

        painter.setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
        painter.setPen(QColor(187, 154, 247));
        painter.drawText(QRectF(m_hitLineX - 100, m_trackRect.top() - 48, 200, 18), Qt::AlignCenter,
                         QStringLiteral("КОМБО"));
    }

    // 8. Отрисовка обратного отсчета (при старте трека и при возврате из паузы)
    if (m_isCountingDown) {
        painter.fillRect(rect(), QColor(16, 10, 28, 175));

        QPointF center = rect().center();
        qreal radius = 65.0;

        // Круговой бейдж с неоновым контуром
        painter.setPen(QPen(QColor(187, 154, 247), 3));
        painter.setBrush(QColor(26, 16, 48, 240));
        painter.drawEllipse(center, radius, radius);

        // Число обратного отсчета
        painter.setPen(QColor(255, 255, 255));
        QFont countFont(QStringLiteral("Segoe UI"), 42, QFont::Black);
        painter.setFont(countFont);

        QString countStr = QString::number(m_countdownNumber);
        QRectF numRect(center.x() - radius, center.y() - radius - 4, radius * 2, radius * 2);
        painter.drawText(numRect, Qt::AlignCenter, countStr);

        // Подпись под кругом
        painter.setPen(QColor(187, 154, 247));
        QFont hintFont(QStringLiteral("Segoe UI"), 13, QFont::DemiBold);
        painter.setFont(hintFont);
        QRectF hintRect(center.x() - 150, center.y() + radius + 15, 300, 30);
        painter.drawText(hintRect, Qt::AlignCenter, QStringLiteral("Приготовьтесь..."));
    }
}

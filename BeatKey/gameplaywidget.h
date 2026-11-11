#ifndef GAMEPLAYWIDGET_H
#define GAMEPLAYWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QRectF>
#include <QColor>
#include <QVector>
#include <QList>
#include <QPixmap>

#include "beatmap.h"
#include "gameresult.h"

/**
 * @brief NoteRenderObject представляет визуальные и геометрические параметры летящей ноты.
 */
struct NoteRenderObject {
    QRectF rect;
    QPixmap pixmap;
    QColor fillColor;
    QColor borderColor;
    bool isVisible = false;
    qreal opacity = 1.0;
};

/**
 * @brief GameplayNote хранит данные ритм-ноты и флаги ее отработки.
 */
struct GameplayNote {
    NoteData data;
    bool isHit = false;
    bool isMissed = false;
};

/**
 * @brief FloatingJudgment представляет всплывающий текст оценки точности попадания.
 */
struct FloatingJudgment {
    QString text;
    QColor color;
    qreal yOffset = 0.0;
    qreal opacity = 1.0;
};

/**
 * @brief GameplayWidget реализует интерактивный игровой процесс rhythm-игры.
 * Ноты летят справа налево в виде спрайтов-прямоугольников к фиксированной линии нажатия.
 */
class GameplayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameplayWidget(QWidget *parent = nullptr);
    ~GameplayWidget() override;

    /**
     * @brief Запустить раунд игры по выбранной карте.
     */
    void startMap(const Beatmap &beatmap);

    /**
     * @brief Приостановить игровой процесс.
     */
    void pauseGame();

    /**
     * @brief Возобновить игровой процесс.
     */
    void resumeGame();

    /**
     * @brief Остановить игровой процесс и сбросить состояние.
     */
    void stopGame();

    /**
     * @brief Единая функция, возвращающая объект отображаемой прямоугольной ноты.
     * Рассчитывает положение ноты на дорожке в зависимости от текущего времени воспроизведения.
     * @param note Данные ритм-ноты.
     * @param currentTimeMs Текущий момент времени композиции в миллисекундах.
     * @param trackRect Прямоугольник дорожки на экране.
     * @param hitLineX Координата X линии нажатия.
     */
    NoteRenderObject getNoteRenderObject(const NoteData &note,
                                        double currentTimeMs,
                                        const QRectF &trackRect,
                                        qreal hitLineX) const;
    double currentTrackTimePreciseMs() const;
    qint64 currentTrackTimeMs() const;

    // Настройки отображения фона
    int bgDimPercent() const { return m_bgDimPercent; }
    void setBgDimPercent(int v) { m_bgDimPercent = v; update(); }

    int bgBlurRadius() const { return m_bgBlurRadius; }
    void setBgBlurRadius(int v) { m_bgBlurRadius = v; updateCachedBackground(); update(); }

    int bgDesatPercent() const { return m_bgDesatPercent; }
    void setBgDesatPercent(int v) { m_bgDesatPercent = v; updateCachedBackground(); update(); }

    int bgVignettePercent() const { return m_bgVignettePercent; }
    void setBgVignettePercent(int v) { m_bgVignettePercent = v; update(); }

    void updateCachedBackground();

signals:
    void gameplayFinished(const GameResult &result);
    void exitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onGameTick();
    void onCountdownTick();

private:
    void updateTrackGeometry();
    void handleHitAction();
    void checkAutoMisses(qint64 currentMs);
    void checkGameCompletion(qint64 currentMs);
    void spawnFloatingJudgment(const QString &text, const QColor &color);
    void finishGame(bool failed);
    void showPauseDialog();
    void startResumeCountdown(int count = 3);
    void cancelCountdown();
    void loadNoteTexture();
    void loadHitTargetTexture();

    Beatmap m_beatmap;
    QVector<GameplayNote> m_notes;
    int m_lastNoteTimeMs = 0;

    static constexpr qreal NOTE_WIDTH = 32.0;
    static constexpr qreal NOTE_HEIGHT = 60.0;

    // Спрайт ноты и рамки зоны нажатия
    QPixmap m_notePixmap;
    QPixmap m_scaledNotePixmap;
    QPixmap m_hitTargetPixmap;
    QPixmap m_scaledHitTargetPixmap;

    // Визуальные настройки фона
    int m_bgDimPercent = 50;
    int m_bgBlurRadius = 0;
    int m_bgDesatPercent = 0;
    int m_bgVignettePercent = 50;
    QPixmap m_cachedBackgroundPixmap;

    // Игровая статистика раунда
    int m_score = 0;
    int m_combo = 0;
    int m_maxCombo = 0;
    int m_hp = 50; // Диапазон от 0 до 100 по ТЗ 4.7.8
    int m_count300 = 0; // Идеально
    int m_count200 = 0; // Хорошо
    int m_count100 = 0; // Сойдет
    int m_countMiss = 0;// Промахи

    bool m_isFailed = false;
    bool m_isFinished = false;
    bool m_isPaused = false;
    bool m_isCountingDown = false;
    int m_countdownNumber = 0;

    // Вступительная задержка (1 секунда до старта музыки со свободным движением нот)
    bool m_isLeadIn = false;
    qint64 m_leadInElapsedMs = 0;
    QElapsedTimer m_leadInClock;
    static constexpr qint64 LEAD_IN_DURATION_MS = 1000;

    // Таймеры анимации и синхронизации
    QTimer *m_gameLoopTimer;
    QTimer *m_countdownTimer = nullptr;
    QElapsedTimer m_audioElapsedTimer;
    mutable QElapsedTimer m_audioClock;
    mutable qint64 m_clockBaseMs = 0;
    mutable qint64 m_lastMediaPos = -1;
    double m_pausedElapsedMs = 0.0;

    // Геометрия игрового поля
    QRectF m_trackRect;
    qreal m_hitLineX = 160.0;
    qreal m_travelTimeMs = 1000.0;
    mutable double m_lastRenderedTrackTimeMs = -999999.0; // 1000 мс время долета ноты по ТЗ 4.7.1.3


    // Всплывающие оценки точности
    QList<FloatingJudgment> m_floatingJudgments;
};

#endif // GAMEPLAYWIDGET_H

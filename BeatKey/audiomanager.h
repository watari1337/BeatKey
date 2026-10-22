#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

#include <QObject>
#include <QString>
#include <QUrl>

class QMediaPlayer;
class QAudioOutput;
class QSoundEffect;

/**
 * @brief AudioPaths contains centralized constants for internal QRC assets and external song directories.
 */
namespace AudioPaths {
    // Встроенные ресурсы (.qrc внутри .exe)
    inline const QString MENU_THEME      = QStringLiteral("qrc:/assets/audio/menu_theme.mp3");
    inline const QString RESULTS_THEME   = QStringLiteral("qrc:/assets/audio/results_theme.mp3");

    inline const QString SFX_HIT         = QStringLiteral("qrc:/assets/audio/hit.wav");
    inline const QString SFX_MISS        = QStringLiteral("qrc:/assets/audio/miss.wav");
    inline const QString SFX_COMBO_BREAK = QStringLiteral("qrc:/assets/audio/combo_break.wav");
    inline const QString SFX_METRONOME   = QStringLiteral("qrc:/assets/audio/metronome.wav");
    inline const QString SFX_COUNTDOWN   = QStringLiteral("qrc:/assets/audio/countdown.wav");
    inline const QString SFX_FAIL        = QStringLiteral("qrc:/assets/audio/fail.wav");
    inline const QString SFX_CLEAR       = QStringLiteral("qrc:/assets/audio/clear.wav");
    inline const QString SFX_CLICK       = QStringLiteral("qrc:/assets/audio/button_click.wav");
    inline const QString SFX_HOVER       = QStringLiteral("qrc:/assets/audio/button_hover.wav");

    // Директория для внешних треков и карт на диске
    inline const QString SONGS_DIR       = QStringLiteral("assets/songs/");
}

/**
 * @brief AudioManager manages all background music playback, sound effects, and timing synchronization.
 *
 * Implemented as a thread-safe singleton, reactive to SettingsManager volume changes.
 */
class AudioManager : public QObject
{
    Q_OBJECT

public:
    static AudioManager& instance();

    // ========================================================================
    // Music Controls (BGM)
    // ========================================================================
    void playMenuMusic();
    void playResultsMusic();
    void playTrack(const QString &sourcePath, bool loop = false);
    void pauseMusic();
    void resumeMusic();
    void stopMusic();
    bool isMusicPlaying() const;

    // ========================================================================
    // Rhythm Synchronization & Timing
    // ========================================================================
    qint64 currentPositionMs() const;
    qint64 syncedPositionMs() const;
    qint64 trackDurationMs() const;
    void setPositionMs(qint64 positionMs);

    // ========================================================================
    // Sound Effects (Low-latency SFX)
    // ========================================================================
    void playHitSound();
    void playMissSound();
    void playComboBreakSound();
    void playMetronomeSound();
    void playCountdownSound();
    void playFailSound();
    void playClearSound();
    void playClickSound();
    void playHoverSound();

public slots:
    void updateVolumes();

signals:
    void trackFinished();
    void positionChanged(qint64 positionMs);

private:
    explicit AudioManager(QObject *parent = nullptr);
    ~AudioManager() override = default;
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void initMusicPlayer();
    void initSoundEffects();
    QUrl resolveUrl(const QString &sourcePath) const;

    // Music playback
    QMediaPlayer *m_musicPlayer;
    QAudioOutput *m_musicOutput;
    QString m_currentTrackSource;

    // Preloaded sound effects
    QSoundEffect *m_sfxHit;
    QSoundEffect *m_sfxMiss;
    QSoundEffect *m_sfxComboBreak;
    QSoundEffect *m_sfxMetronome;
    QSoundEffect *m_sfxCountdown;
    QSoundEffect *m_sfxFail;
    QSoundEffect *m_sfxClear;
    QSoundEffect *m_sfxClick;
    QSoundEffect *m_sfxHover;
};

#endif // AUDIOMANAGER_H

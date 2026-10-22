#include "audiomanager.h"
#include "settingsmanager.h"

#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>

AudioManager& AudioManager::instance()
{
    static AudioManager s_instance;
    return s_instance;
}

AudioManager::AudioManager(QObject *parent)
    : QObject(parent)
    , m_musicPlayer(nullptr)
    , m_musicOutput(nullptr)
    , m_sfxHit(nullptr)
    , m_sfxMiss(nullptr)
    , m_sfxComboBreak(nullptr)
    , m_sfxMetronome(nullptr)
    , m_sfxCountdown(nullptr)
    , m_sfxFail(nullptr)
    , m_sfxClear(nullptr)
    , m_sfxClick(nullptr)
    , m_sfxHover(nullptr)
{
    initMusicPlayer();
    initSoundEffects();

    // Подписываемся на реактивное изменение громкостей в настройках
    auto &settings = SettingsManager::instance();
    connect(&settings, &SettingsManager::masterVolumeChanged, this, &AudioManager::updateVolumes);
    connect(&settings, &SettingsManager::musicVolumeChanged,  this, &AudioManager::updateVolumes);
    connect(&settings, &SettingsManager::sfxVolumeChanged,    this, &AudioManager::updateVolumes);

    updateVolumes();
}

void AudioManager::initMusicPlayer()
{
    m_musicPlayer = new QMediaPlayer(this);
    m_musicOutput = new QAudioOutput(this);
    m_musicPlayer->setAudioOutput(m_musicOutput);

    connect(m_musicPlayer, &QMediaPlayer::positionChanged, this, &AudioManager::positionChanged);
    connect(m_musicPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            emit trackFinished();
        }
    });
}

static QSoundEffect* createSoundEffect(QObject *parent, const QString &sourcePath)
{
    auto *effect = new QSoundEffect(parent);
    effect->setSource(QUrl(sourcePath));
    return effect;
}

void AudioManager::initSoundEffects()
{
    m_sfxHit         = createSoundEffect(this, AudioPaths::SFX_HIT);
    m_sfxMiss        = createSoundEffect(this, AudioPaths::SFX_MISS);
    m_sfxComboBreak  = createSoundEffect(this, AudioPaths::SFX_COMBO_BREAK);
    m_sfxMetronome    = createSoundEffect(this, AudioPaths::SFX_METRONOME);
    m_sfxCountdown   = createSoundEffect(this, AudioPaths::SFX_COUNTDOWN);
    m_sfxFail        = createSoundEffect(this, AudioPaths::SFX_FAIL);
    m_sfxClear       = createSoundEffect(this, AudioPaths::SFX_CLEAR);
    m_sfxClick       = createSoundEffect(this, AudioPaths::SFX_CLICK);
    m_sfxHover       = createSoundEffect(this, AudioPaths::SFX_HOVER);
}

QUrl AudioManager::resolveUrl(const QString &sourcePath) const
{
    if (sourcePath.startsWith(QStringLiteral("qrc:/")) || sourcePath.startsWith(QStringLiteral(":/"))) {
        return QUrl(sourcePath);
    }

    if (QFileInfo::exists(sourcePath)) {
        return QUrl::fromLocalFile(sourcePath);
    }

    return QUrl(sourcePath);
}

// ============================================================================
// Music Controls (BGM)
// ============================================================================

void AudioManager::playMenuMusic()
{
    playTrack(AudioPaths::MENU_THEME, true);
}

void AudioManager::playResultsMusic()
{
    playTrack(AudioPaths::RESULTS_THEME, true);
}

void AudioManager::playTrack(const QString &sourcePath, bool loop)
{
    if (m_currentTrackSource == sourcePath && m_musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
        return; // Трек уже играет
    }

    m_currentTrackSource = sourcePath;
    m_musicPlayer->stop();
    m_musicPlayer->setSource(resolveUrl(sourcePath));
    m_musicPlayer->setLoops(loop ? QMediaPlayer::Infinite : 1);
    m_musicPlayer->play();
}

void AudioManager::pauseMusic()
{
    if (m_musicPlayer->playbackState() == QMediaPlayer::PlayingState) {
        m_musicPlayer->pause();
    }
}

void AudioManager::resumeMusic()
{
    if (m_musicPlayer->playbackState() == QMediaPlayer::PausedState) {
        m_musicPlayer->play();
    }
}

void AudioManager::stopMusic()
{
    m_musicPlayer->stop();
    m_currentTrackSource.clear();
}

bool AudioManager::isMusicPlaying() const
{
    return m_musicPlayer->playbackState() == QMediaPlayer::PlayingState;
}

// ============================================================================
// Rhythm Synchronization & Timing
// ============================================================================

qint64 AudioManager::currentPositionMs() const
{
    return m_musicPlayer->position();
}

qint64 AudioManager::syncedPositionMs() const
{
    qint64 rawPos = m_musicPlayer->position();
    int offset = SettingsManager::instance().audioOffsetMs();
    return rawPos - offset;
}

qint64 AudioManager::trackDurationMs() const
{
    return m_musicPlayer->duration();
}

void AudioManager::setPositionMs(qint64 positionMs)
{
    m_musicPlayer->setPosition(positionMs);
}

// ============================================================================
// Sound Effects
// ============================================================================

void AudioManager::playHitSound()
{
    if (m_sfxHit) m_sfxHit->play();
}

void AudioManager::playMissSound()
{
    if (m_sfxMiss) m_sfxMiss->play();
}

void AudioManager::playComboBreakSound()
{
    if (m_sfxComboBreak) m_sfxComboBreak->play();
}

void AudioManager::playMetronomeSound()
{
    if (m_sfxMetronome) m_sfxMetronome->play();
}

void AudioManager::playCountdownSound()
{
    if (m_sfxCountdown) m_sfxCountdown->play();
}

void AudioManager::playFailSound()
{
    if (m_sfxFail) m_sfxFail->play();
}

void AudioManager::playClearSound()
{
    if (m_sfxClear) m_sfxClear->play();
}

void AudioManager::playClickSound()
{
    if (m_sfxClick) m_sfxClick->play();
}

void AudioManager::playHoverSound()
{
    if (m_sfxHover) m_sfxHover->play();
}

void AudioManager::updateVolumes()
{
    auto &settings = SettingsManager::instance();
    float master = settings.masterVolume() / 100.0f;
    float music  = settings.musicVolume() / 100.0f;
    float sfx    = settings.sfxVolume() / 100.0f;

    float effectiveMusic = std::clamp(master * music, 0.0f, 1.0f);
    float effectiveSfx   = std::clamp(master * sfx, 0.0f, 1.0f);

    if (m_musicOutput) {
        m_musicOutput->setVolume(effectiveMusic);
    }

    QSoundEffect* allSfx[] = {
        m_sfxHit, m_sfxMiss, m_sfxComboBreak, m_sfxMetronome,
        m_sfxCountdown, m_sfxFail, m_sfxClear, m_sfxClick, m_sfxHover
    };

    for (auto *eff : allSfx) {
        if (eff) {
            eff->setVolume(effectiveSfx);
        }
    }
}

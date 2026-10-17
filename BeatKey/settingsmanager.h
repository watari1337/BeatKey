#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QKeySequence>
#include <QSettings>

/**
 * @brief SettingsManager handles loading, storing, and accessing game configuration.
 *
 * Config is persisted in "settings.ini" using QSettings format.
 * Implemented as a thread-safe singleton with Qt signals for reactive UI updates.
 */
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager& instance();

    // Default configuration values per TZ
    static constexpr int DEFAULT_MASTER_VOLUME = 80;
    static constexpr int DEFAULT_MUSIC_VOLUME = 80;
    static constexpr int DEFAULT_SFX_VOLUME = 80;
    static constexpr int DEFAULT_AUDIO_OFFSET_MS = 0;
    static constexpr int MIN_AUDIO_OFFSET_MS = -200;
    static constexpr int MAX_AUDIO_OFFSET_MS = 200;
    static constexpr Qt::Key DEFAULT_PRIMARY_KEY = Qt::Key_D;
    static constexpr Qt::Key DEFAULT_SECONDARY_KEY = Qt::Key_K;
    static inline const QString DEFAULT_NICKNAME = QStringLiteral("Player");
    static constexpr bool DEFAULT_FULLSCREEN = false;

    // Load / Save
    void loadSettings();
    void saveSettings();
    void resetToDefaults();

    // Player Profile
    QString nickname() const;
    void setNickname(const QString &nick);

    // Audio
    int masterVolume() const;
    void setMasterVolume(int volume);

    int musicVolume() const;
    void setMusicVolume(int volume);

    int sfxVolume() const;
    void setSfxVolume(int volume);

    int audioOffsetMs() const;
    void setAudioOffsetMs(int offsetMs);

    // Controls
    Qt::Key primaryKey() const;
    void setPrimaryKey(Qt::Key key);

    Qt::Key secondaryKey() const;
    void setSecondaryKey(Qt::Key key);

    // Video
    bool isFullscreen() const;
    void setFullscreen(bool fullscreen);

    int windowWidth() const;
    int windowHeight() const;
    void setWindowResolution(int width, int height);

    // Helpers
    static QString keyToString(Qt::Key key);
    static Qt::Key stringToKey(const QString &str);
    QString configFilePath() const;

signals:
    void nicknameChanged(const QString &nickname);
    void masterVolumeChanged(int volume);
    void musicVolumeChanged(int volume);
    void sfxVolumeChanged(int volume);
    void audioOffsetChanged(int offsetMs);
    void keyBindingsChanged(Qt::Key primary, Qt::Key secondary);
    void fullscreenChanged(bool fullscreen);
    void settingsReset();
    void settingsSaved();

private:
    explicit SettingsManager(QObject *parent = nullptr);
    ~SettingsManager() override = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    QString m_configPath;

    // Cached values
    QString m_nickname;
    int m_masterVolume;
    int m_musicVolume;
    int m_sfxVolume;
    int m_audioOffsetMs;
    Qt::Key m_primaryKey;
    Qt::Key m_secondaryKey;
    bool m_fullscreen;
    int m_windowWidth;
    int m_windowHeight;
};

#endif // SETTINGSMANAGER_H

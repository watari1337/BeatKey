#include "settingsmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <algorithm>

SettingsManager& SettingsManager::instance()
{
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
    , m_nickname(DEFAULT_NICKNAME)
    , m_masterVolume(DEFAULT_MASTER_VOLUME)
    , m_musicVolume(DEFAULT_MUSIC_VOLUME)
    , m_sfxVolume(DEFAULT_SFX_VOLUME)
    , m_audioOffsetMs(DEFAULT_AUDIO_OFFSET_MS)
    , m_primaryKey(DEFAULT_PRIMARY_KEY)
    , m_secondaryKey(DEFAULT_SECONDARY_KEY)
    , m_fullscreen(DEFAULT_FULLSCREEN)
    , m_windowWidth(1280)
    , m_windowHeight(720)
{
    QString appDir = QCoreApplication::applicationDirPath();
    m_configPath = appDir + QDir::separator() + QStringLiteral("settings.ini");
    loadSettings();
}

QString SettingsManager::configFilePath() const
{
    return m_configPath;
}

void SettingsManager::loadSettings()
{
    if (!QFile::exists(m_configPath)) {
        qDebug() << "[SettingsManager] Config file not found, creating default at" << m_configPath;
        saveSettings();
        return;
    }

    QSettings settings(m_configPath, QSettings::IniFormat);

    // Player
    m_nickname = settings.value(QStringLiteral("Player/Nickname"), DEFAULT_NICKNAME).toString().trimmed();
    if (m_nickname.isEmpty() || m_nickname.length() < 3) {
        m_nickname = DEFAULT_NICKNAME;
    }

    // Audio
    m_masterVolume = std::clamp(settings.value(QStringLiteral("Audio/MasterVolume"), DEFAULT_MASTER_VOLUME).toInt(), 0, 100);
    m_musicVolume = std::clamp(settings.value(QStringLiteral("Audio/MusicVolume"), DEFAULT_MUSIC_VOLUME).toInt(), 0, 100);
    m_sfxVolume = std::clamp(settings.value(QStringLiteral("Audio/SfxVolume"), DEFAULT_SFX_VOLUME).toInt(), 0, 100);
    m_audioOffsetMs = std::clamp(settings.value(QStringLiteral("Audio/AudioOffsetMs"), DEFAULT_AUDIO_OFFSET_MS).toInt(),
                                 MIN_AUDIO_OFFSET_MS, MAX_AUDIO_OFFSET_MS);

    // Controls
    QString primStr = settings.value(QStringLiteral("Controls/PrimaryKey"), QStringLiteral("D")).toString();
    QString secStr = settings.value(QStringLiteral("Controls/SecondaryKey"), QStringLiteral("K")).toString();
    m_primaryKey = stringToKey(primStr);
    m_secondaryKey = stringToKey(secStr);

    // Video
    m_fullscreen = settings.value(QStringLiteral("Video/Fullscreen"), DEFAULT_FULLSCREEN).toBool();
    m_windowWidth = settings.value(QStringLiteral("Video/WindowWidth"), 1280).toInt();
    m_windowHeight = settings.value(QStringLiteral("Video/WindowHeight"), 720).toInt();

    qDebug() << "[SettingsManager] Loaded config from" << m_configPath
             << "| Nickname:" << m_nickname
             << "| PrimaryKey:" << keyToString(m_primaryKey)
             << "| SecondaryKey:" << keyToString(m_secondaryKey)
             << "| Offset:" << m_audioOffsetMs << "ms";
}

void SettingsManager::saveSettings()
{
    QSettings settings(m_configPath, QSettings::IniFormat);

    settings.setValue(QStringLiteral("Player/Nickname"), m_nickname);

    settings.setValue(QStringLiteral("Audio/MasterVolume"), m_masterVolume);
    settings.setValue(QStringLiteral("Audio/MusicVolume"), m_musicVolume);
    settings.setValue(QStringLiteral("Audio/SfxVolume"), m_sfxVolume);
    settings.setValue(QStringLiteral("Audio/AudioOffsetMs"), m_audioOffsetMs);

    settings.setValue(QStringLiteral("Controls/PrimaryKey"), keyToString(m_primaryKey));
    settings.setValue(QStringLiteral("Controls/SecondaryKey"), keyToString(m_secondaryKey));

    settings.setValue(QStringLiteral("Video/Fullscreen"), m_fullscreen);
    settings.setValue(QStringLiteral("Video/WindowWidth"), m_windowWidth);
    settings.setValue(QStringLiteral("Video/WindowHeight"), m_windowHeight);

    settings.sync();
    qDebug() << "[SettingsManager] Settings successfully saved to" << m_configPath;
    emit settingsSaved();
}

void SettingsManager::resetToDefaults()
{
    m_nickname = DEFAULT_NICKNAME;
    m_masterVolume = DEFAULT_MASTER_VOLUME;
    m_musicVolume = DEFAULT_MUSIC_VOLUME;
    m_sfxVolume = DEFAULT_SFX_VOLUME;
    m_audioOffsetMs = DEFAULT_AUDIO_OFFSET_MS;
    m_primaryKey = DEFAULT_PRIMARY_KEY;
    m_secondaryKey = DEFAULT_SECONDARY_KEY;
    m_fullscreen = DEFAULT_FULLSCREEN;
    m_windowWidth = 1280;
    m_windowHeight = 720;

    saveSettings();

    emit nicknameChanged(m_nickname);
    emit masterVolumeChanged(m_masterVolume);
    emit musicVolumeChanged(m_musicVolume);
    emit sfxVolumeChanged(m_sfxVolume);
    emit audioOffsetChanged(m_audioOffsetMs);
    emit keyBindingsChanged(m_primaryKey, m_secondaryKey);
    emit fullscreenChanged(m_fullscreen);
    emit settingsReset();
}

// Getters & Setters
QString SettingsManager::nickname() const { return m_nickname; }
void SettingsManager::setNickname(const QString &nick)
{
    QString clean = nick.trimmed();
    if (clean.length() >= 3 && clean.length() <= 20 && m_nickname != clean) {
        m_nickname = clean;
        emit nicknameChanged(m_nickname);
    }
}

int SettingsManager::masterVolume() const { return m_masterVolume; }
void SettingsManager::setMasterVolume(int volume)
{
    int val = std::clamp(volume, 0, 100);
    if (m_masterVolume != val) {
        m_masterVolume = val;
        emit masterVolumeChanged(m_masterVolume);
    }
}

int SettingsManager::musicVolume() const { return m_musicVolume; }
void SettingsManager::setMusicVolume(int volume)
{
    int val = std::clamp(volume, 0, 100);
    if (m_musicVolume != val) {
        m_musicVolume = val;
        emit musicVolumeChanged(m_musicVolume);
    }
}

int SettingsManager::sfxVolume() const { return m_sfxVolume; }
void SettingsManager::setSfxVolume(int volume)
{
    int val = std::clamp(volume, 0, 100);
    if (m_sfxVolume != val) {
        m_sfxVolume = val;
        emit sfxVolumeChanged(m_sfxVolume);
    }
}

int SettingsManager::audioOffsetMs() const { return m_audioOffsetMs; }
void SettingsManager::setAudioOffsetMs(int offsetMs)
{
    int val = std::clamp(offsetMs, MIN_AUDIO_OFFSET_MS, MAX_AUDIO_OFFSET_MS);
    if (m_audioOffsetMs != val) {
        m_audioOffsetMs = val;
        emit audioOffsetChanged(m_audioOffsetMs);
    }
}

Qt::Key SettingsManager::primaryKey() const { return m_primaryKey; }
void SettingsManager::setPrimaryKey(Qt::Key key)
{
    if (m_primaryKey != key) {
        m_primaryKey = key;
        emit keyBindingsChanged(m_primaryKey, m_secondaryKey);
    }
}

Qt::Key SettingsManager::secondaryKey() const { return m_secondaryKey; }
void SettingsManager::setSecondaryKey(Qt::Key key)
{
    if (m_secondaryKey != key) {
        m_secondaryKey = key;
        emit keyBindingsChanged(m_primaryKey, m_secondaryKey);
    }
}

bool SettingsManager::isFullscreen() const { return m_fullscreen; }
void SettingsManager::setFullscreen(bool fullscreen)
{
    if (m_fullscreen != fullscreen) {
        m_fullscreen = fullscreen;
        emit fullscreenChanged(m_fullscreen);
    }
}

int SettingsManager::windowWidth() const { return m_windowWidth; }
int SettingsManager::windowHeight() const { return m_windowHeight; }
void SettingsManager::setWindowResolution(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
}

QString SettingsManager::keyToString(Qt::Key key)
{
    if (key == Qt::Key_Space) return QStringLiteral("Space");
    if (key == Qt::Key_Shift) return QStringLiteral("Shift");
    if (key == Qt::Key_Control) return QStringLiteral("Ctrl");
    if (key == Qt::Key_Alt) return QStringLiteral("Alt");
    if (key == Qt::Key_Return || key == Qt::Key_Enter) return QStringLiteral("Enter");

    QKeySequence seq(key);
    QString str = seq.toString(QKeySequence::NativeText);
    if (str.isEmpty()) {
        return QString::number(static_cast<int>(key));
    }
    return str;
}

Qt::Key SettingsManager::stringToKey(const QString &str)
{
    QString trimmed = str.trimmed();
    if (trimmed.compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0) return Qt::Key_Space;
    if (trimmed.compare(QStringLiteral("Shift"), Qt::CaseInsensitive) == 0) return Qt::Key_Shift;
    if (trimmed.compare(QStringLiteral("Ctrl"), Qt::CaseInsensitive) == 0) return Qt::Key_Control;
    if (trimmed.compare(QStringLiteral("Alt"), Qt::CaseInsensitive) == 0) return Qt::Key_Alt;
    if (trimmed.compare(QStringLiteral("Enter"), Qt::CaseInsensitive) == 0) return Qt::Key_Return;

    QKeySequence seq(trimmed);
    if (!seq.isEmpty()) {
        return static_cast<Qt::Key>(seq[0].toCombined());
    }

    bool ok = false;
    int code = trimmed.toInt(&ok);
    if (ok) return static_cast<Qt::Key>(code);

    return Qt::Key_unknown;
}

#include "settingsmanager.h"
#include <QCoreApplication>
#include <QDir>
#include <QDebug>
#include <QHash>
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
    , m_bgDimPercent(DEFAULT_BG_DIM_PERCENT)
    , m_bgBlurRadius(DEFAULT_BG_BLUR_RADIUS)
    , m_editorHitsoundsEnabled(true)
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

    // Graphics Background Display
    m_bgDimPercent = std::clamp(settings.value(QStringLiteral("Graphics/BgDim"), DEFAULT_BG_DIM_PERCENT).toInt(), 0, 100);
    m_bgBlurRadius = std::clamp(settings.value(QStringLiteral("Graphics/BgBlur"), DEFAULT_BG_BLUR_RADIUS).toInt(), 0, 20);

    // Editor Settings
    m_editorHitsoundsEnabled = settings.value(QStringLiteral("Editor/HitsoundsEnabled"), true).toBool();

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

    settings.setValue(QStringLiteral("Graphics/BgDim"), m_bgDimPercent);
    settings.setValue(QStringLiteral("Graphics/BgBlur"), m_bgBlurRadius);
    settings.setValue(QStringLiteral("Editor/HitsoundsEnabled"), m_editorHitsoundsEnabled);

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
    m_bgDimPercent = DEFAULT_BG_DIM_PERCENT;
    m_bgBlurRadius = DEFAULT_BG_BLUR_RADIUS;
    m_editorHitsoundsEnabled = true;

    saveSettings();

    emit nicknameChanged(m_nickname);
    emit masterVolumeChanged(m_masterVolume);
    emit musicVolumeChanged(m_musicVolume);
    emit sfxVolumeChanged(m_sfxVolume);
    emit audioOffsetChanged(m_audioOffsetMs);
    emit bgDimPercentChanged(m_bgDimPercent);
    emit bgBlurRadiusChanged(m_bgBlurRadius);
    emit editorHitsoundsEnabledChanged(m_editorHitsoundsEnabled);
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

Qt::Key SettingsManager::normalizeKey(const QKeyEvent *event)
{
    if (!event) {
        return Qt::Key_unknown;
    }

    // 1. Аппаратный код клавиши (Windows Virtual Key Code) не зависит от раскладки
    quint32 vk = event->nativeVirtualKey();
    if (vk >= 0x41 && vk <= 0x5A) { // 'A' .. 'Z'
        return static_cast<Qt::Key>(vk);
    }
    if (vk >= 0x30 && vk <= 0x39) { // '0' .. '9'
        return static_cast<Qt::Key>(vk);
    }
    if (vk == 0x20) { // VK_SPACE
        return Qt::Key_Space;
    }
    if (vk == 0x1B) { // VK_ESCAPE
        return Qt::Key_Escape;
    }
    if (vk == 0x0D) { // VK_RETURN
        return Qt::Key_Return;
    }
    if (vk == 0x08) { // VK_BACK
        return Qt::Key_Backspace;
    }
    if (vk == 0x2E) { // VK_DELETE
        return Qt::Key_Delete;
    }
    if (vk == 0x25) { // VK_LEFT
        return Qt::Key_Left;
    }
    if (vk == 0x26) { // VK_UP
        return Qt::Key_Up;
    }
    if (vk == 0x27) { // VK_RIGHT
        return Qt::Key_Right;
    }
    if (vk == 0x28) { // VK_DOWN
        return Qt::Key_Down;
    }

    // 2. Таблица прямого сопоставления кириллических символов стандартной клавиатуры
    int k = event->key();
    QString text = event->text();
    QChar ch = text.isEmpty() ? QChar(static_cast<ushort>(k)) : text.at(0);

    static const QHash<QChar, Qt::Key> cyrillicMap = {
        {QChar(u'й'), Qt::Key_Q}, {QChar(u'Й'), Qt::Key_Q},
        {QChar(u'ц'), Qt::Key_W}, {QChar(u'Ц'), Qt::Key_W},
        {QChar(u'у'), Qt::Key_E}, {QChar(u'У'), Qt::Key_E},
        {QChar(u'к'), Qt::Key_R}, {QChar(u'К'), Qt::Key_R},
        {QChar(u'е'), Qt::Key_T}, {QChar(u'Е'), Qt::Key_T},
        {QChar(u'н'), Qt::Key_Y}, {QChar(u'Н'), Qt::Key_Y},
        {QChar(u'г'), Qt::Key_U}, {QChar(u'Г'), Qt::Key_U},
        {QChar(u'ш'), Qt::Key_I}, {QChar(u'Ш'), Qt::Key_I},
        {QChar(u'щ'), Qt::Key_O}, {QChar(u'Щ'), Qt::Key_O},
        {QChar(u'з'), Qt::Key_P}, {QChar(u'З'), Qt::Key_P},
        {QChar(u'х'), Qt::Key_BracketLeft}, {QChar(u'Х'), Qt::Key_BracketLeft},
        {QChar(u'ъ'), Qt::Key_BracketRight}, {QChar(u'Ъ'), Qt::Key_BracketRight},
        {QChar(u'ф'), Qt::Key_A}, {QChar(u'Ф'), Qt::Key_A},
        {QChar(u'ы'), Qt::Key_S}, {QChar(u'Ы'), Qt::Key_S},
        {QChar(u'в'), Qt::Key_D}, {QChar(u'В'), Qt::Key_D},
        {QChar(u'а'), Qt::Key_F}, {QChar(u'А'), Qt::Key_F},
        {QChar(u'п'), Qt::Key_G}, {QChar(u'П'), Qt::Key_G},
        {QChar(u'р'), Qt::Key_H}, {QChar(u'Р'), Qt::Key_H},
        {QChar(u'о'), Qt::Key_J}, {QChar(u'О'), Qt::Key_J},
        {QChar(u'л'), Qt::Key_K}, {QChar(u'Л'), Qt::Key_K},
        {QChar(u'д'), Qt::Key_L}, {QChar(u'Д'), Qt::Key_L},
        {QChar(u'ж'), Qt::Key_Semicolon}, {QChar(u'Ж'), Qt::Key_Semicolon},
        {QChar(u'э'), Qt::Key_Apostrophe}, {QChar(u'Э'), Qt::Key_Apostrophe},
        {QChar(u'я'), Qt::Key_Z}, {QChar(u'Я'), Qt::Key_Z},
        {QChar(u'ч'), Qt::Key_X}, {QChar(u'Ч'), Qt::Key_X},
        {QChar(u'с'), Qt::Key_C}, {QChar(u'С'), Qt::Key_C},
        {QChar(u'м'), Qt::Key_V}, {QChar(u'М'), Qt::Key_V},
        {QChar(u'и'), Qt::Key_B}, {QChar(u'И'), Qt::Key_B},
        {QChar(u'т'), Qt::Key_N}, {QChar(u'Т'), Qt::Key_N},
        {QChar(u'ь'), Qt::Key_M}, {QChar(u'Ь'), Qt::Key_M},
        {QChar(u'б'), Qt::Key_Comma}, {QChar(u'Б'), Qt::Key_Comma},
        {QChar(u'ю'), Qt::Key_Period}, {QChar(u'Ю'), Qt::Key_Period},
        {QChar(u'ё'), Qt::Key_QuoteLeft}, {QChar(u'Ё'), Qt::Key_QuoteLeft}
    };

    if (cyrillicMap.contains(ch)) {
        return cyrillicMap.value(ch);
    }

    // 3. Стандартная клавиша Qt
    return static_cast<Qt::Key>(k);
}

bool SettingsManager::isKeyMatching(const QKeyEvent *event, Qt::Key expectedKey)
{
    if (!event) {
        return false;
    }
    if (normalizeKey(event) == expectedKey) {
        return true;
    }
    if (static_cast<Qt::Key>(event->key()) == expectedKey) {
        return true;
    }
    return false;
}

int SettingsManager::bgDimPercent() const
{
    return m_bgDimPercent;
}

void SettingsManager::setBgDimPercent(int percent)
{
    percent = std::clamp(percent, 0, 100);
    if (m_bgDimPercent != percent) {
        m_bgDimPercent = percent;
        emit bgDimPercentChanged(m_bgDimPercent);
    }
}

int SettingsManager::bgBlurRadius() const
{
    return m_bgBlurRadius;
}

void SettingsManager::setBgBlurRadius(int radius)
{
    radius = std::clamp(radius, 0, 20);
    if (m_bgBlurRadius != radius) {
        m_bgBlurRadius = radius;
        emit bgBlurRadiusChanged(m_bgBlurRadius);
    }
}

bool SettingsManager::editorHitsoundsEnabled() const
{
    return m_editorHitsoundsEnabled;
}

void SettingsManager::setEditorHitsoundsEnabled(bool enabled)
{
    if (m_editorHitsoundsEnabled != enabled) {
        m_editorHitsoundsEnabled = enabled;
        saveSettings();
        emit editorHitsoundsEnabledChanged(m_editorHitsoundsEnabled);
    }
}

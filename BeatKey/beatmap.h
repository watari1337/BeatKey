#ifndef BEATMAP_H
#define BEATMAP_H

#include <QString>
#include <QVector>
#include <QPixmap>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QStringList>

namespace BeatmapDifficulties {
    inline const QStringList ALL = {
        QStringLiteral("Легко"),
        QStringLiteral("Нормально"),
        QStringLiteral("Сложно"),
        QStringLiteral("Эксперт"),
        QStringLiteral("Мастер")
    };
    inline const QString DEFAULT_DIFFICULTY = QStringLiteral("Нормально");

    inline QString canonicalDifficulty(const QString &raw) {
        QString trimmed = raw.trimmed();
        if (ALL.contains(trimmed)) {
            return trimmed;
        }
        QString lower = trimmed.toLower();
        if (lower.startsWith(QStringLiteral("легк")) || lower == QStringLiteral("easy")) {
            return QStringLiteral("Легко");
        }
        if (lower.startsWith(QStringLiteral("норм")) || lower == QStringLiteral("normal")) {
            return QStringLiteral("Нормально");
        }
        if (lower.startsWith(QStringLiteral("сложн")) || lower == QStringLiteral("hard")) {
            return QStringLiteral("Сложно");
        }
        if (lower.startsWith(QStringLiteral("эксперт")) || lower == QStringLiteral("expert")) {
            return QStringLiteral("Эксперт");
        }
        if (lower.startsWith(QStringLiteral("мастер")) || lower.startsWith(QStringLiteral("безумн")) || lower == QStringLiteral("insane")) {
            return QStringLiteral("Мастер");
        }
        return DEFAULT_DIFFICULTY;
    }
}

/**
 * @brief NoteData represents a single rhythmic hit object on the track.
 */
struct NoteData {
    int timeMs = 0;              // Целевой момент удара ноты (в мс от старта)
    QString type = QStringLiteral("single"); // Тип ноты (в v1 фиксировано "single")

    QJsonObject toJson() const;
    static NoteData fromJson(const QJsonObject &obj);
};

/**
 * @brief BeatmapMeta contains all descriptive metadata about the song and chart.
 */
struct BeatmapMeta {
    QString title;               // Наименование композиции (1-100 символов)
    QString artist;              // Исполнитель (1-100 символов)
    QString mapper;              // Автор разметки карты (1-50 символов)
    QString difficultyName;      // Наименование сложности (Easy, Normal, Hard и т.д.)
    double bpm = 120.0;          // Темп в ударах в минуту (30.0 - 300.0)
    int audioOffset = 0;         // Смещение первого такта в мс
    QString audioFileName;       // Имя аудиофайла внутри пакета (.mp3, .wav, .ogg)
    QString coverFileName;       // Имя файла обложки внутри пакета (.png, .jpg)
    int previewTimeMs = 0;       // Момент начала предпрослушивания в меню (в мс)

    // Дополнительные параметры для обратной совместимости будущих версий:
    int songDurationMs = 0;      // Полная длительность трека в мс (0 = автоопределение)
    int leadInMs = 1500;         // Пауза перед стартом первой ноты (по умолчанию 1500 мс)

    QJsonObject toJson() const;
    static BeatmapMeta fromJson(const QJsonObject &obj);
};

/**
 * @brief BeatmapGameplay contains gameplay multipliers and speed settings.
 */
struct BeatmapGameplay {
    double scrollSpeed = 1.0;    // Скорость прокрутки нот (0.5 - 3.0)
    double hpDrainRate = 1.0;    // Множитель расхода здоровья при ошибках

    QJsonObject toJson() const;
    static BeatmapGameplay fromJson(const QJsonObject &obj);
};

/**
 * @brief Beatmap represents a fully parsed, validated playable rhythm chart.
 *
 * Designed with versioning to guarantee backward compatibility with future formats.
 */
class Beatmap {
public:
    static constexpr int CURRENT_FORMAT_VERSION = 1;

    Beatmap();

    // Core attributes per TZ 4.10
    int formatVersion = CURRENT_FORMAT_VERSION;
    QString id;                  // SHA-256 хеш карты (ключ для Firebase)
    BeatmapMeta meta;
    BeatmapGameplay gameplay;
    QVector<NoteData> notes;

    // Runtime loaded assets
    QPixmap coverImage;
    QString extractedAudioPath;  // Путь к извлеченному mp3 во временной папке для AudioManager
    QString bkeyFilePath;        // Исходный путь к архиву .bkey на диске

    // Status
    bool isValid = false;
    QString errorMessage;

    // Serialization
    QByteArray toJson(bool indented = true) const;
    static Beatmap fromJson(const QByteArray &jsonData, QString *error = nullptr);

    // Helpers
    int totalNotes() const { return notes.size(); }
    QString formattedBpm() const;
    QString formattedDuration() const;
};

#endif // BEATMAP_H

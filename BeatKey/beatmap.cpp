#include "beatmap.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <algorithm>

// ============================================================================
// NoteData Serialization
// ============================================================================

QJsonObject NoteData::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("time")] = timeMs;
    obj[QStringLiteral("type")] = type;
    return obj;
}

NoteData NoteData::fromJson(const QJsonObject &obj)
{
    NoteData note;
    note.timeMs = obj.value(QStringLiteral("time")).toInt(0);
    note.type = obj.value(QStringLiteral("type")).toString(QStringLiteral("single"));
    return note;
}

// ============================================================================
// BeatmapMeta Serialization
// ============================================================================

QJsonObject BeatmapMeta::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("title")] = title;
    obj[QStringLiteral("artist")] = artist;
    obj[QStringLiteral("mapper")] = mapper;
    obj[QStringLiteral("difficultyName")] = difficultyName;
    obj[QStringLiteral("bpm")] = bpm;
    obj[QStringLiteral("audioOffset")] = audioOffset;
    obj[QStringLiteral("audioFileName")] = audioFileName;
    obj[QStringLiteral("coverFileName")] = coverFileName;
    obj[QStringLiteral("previewTimeMs")] = previewTimeMs;

    // Optional future fields
    if (songDurationMs > 0) {
        obj[QStringLiteral("songDurationMs")] = songDurationMs;
    }
    if (leadInMs != 1500) {
        obj[QStringLiteral("leadInMs")] = leadInMs;
    }

    return obj;
}

BeatmapMeta BeatmapMeta::fromJson(const QJsonObject &obj)
{
    BeatmapMeta meta;
    meta.title = obj.value(QStringLiteral("title")).toString().trimmed();
    meta.artist = obj.value(QStringLiteral("artist")).toString().trimmed();
    meta.mapper = obj.value(QStringLiteral("mapper")).toString().trimmed();
    meta.difficultyName = obj.value(QStringLiteral("difficultyName")).toString().trimmed();
    meta.bpm = obj.value(QStringLiteral("bpm")).toDouble(120.0);
    meta.audioOffset = obj.value(QStringLiteral("audioOffset")).toInt(0);
    meta.audioFileName = obj.value(QStringLiteral("audioFileName")).toString().trimmed();
    meta.coverFileName = obj.value(QStringLiteral("coverFileName")).toString().trimmed();
    meta.previewTimeMs = obj.value(QStringLiteral("previewTimeMs")).toInt(0);

    // Backward-compatible fallback for optional fields
    meta.songDurationMs = obj.value(QStringLiteral("songDurationMs")).toInt(0);
    meta.leadInMs = obj.value(QStringLiteral("leadInMs")).toInt(1500);

    return meta;
}

// ============================================================================
// BeatmapGameplay Serialization
// ============================================================================

QJsonObject BeatmapGameplay::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("scrollSpeed")] = scrollSpeed;
    obj[QStringLiteral("hpDrainRate")] = hpDrainRate;
    return obj;
}

BeatmapGameplay BeatmapGameplay::fromJson(const QJsonObject &obj)
{
    BeatmapGameplay gp;
    gp.scrollSpeed = obj.value(QStringLiteral("scrollSpeed")).toDouble(1.0);
    gp.hpDrainRate = obj.value(QStringLiteral("hpDrainRate")).toDouble(1.0);
    return gp;
}

// ============================================================================
// Beatmap Core
// ============================================================================

Beatmap::Beatmap()
    : formatVersion(CURRENT_FORMAT_VERSION)
    , isValid(false)
{
}

QByteArray Beatmap::toJson(bool indented) const
{
    QJsonObject root;
    root[QStringLiteral("formatVersion")] = formatVersion;
    root[QStringLiteral("id")] = id;
    root[QStringLiteral("meta")] = meta.toJson();
    root[QStringLiteral("gameplay")] = gameplay.toJson();

    QJsonArray notesArray;
    for (const auto &note : notes) {
        notesArray.append(note.toJson());
    }
    root[QStringLiteral("notes")] = notesArray;

    QJsonDocument doc(root);
    return doc.toJson(indented ? QJsonDocument::Indented : QJsonDocument::Compact);
}

Beatmap Beatmap::fromJson(const QByteArray &jsonData, QString *error)
{
    Beatmap map;
    map.isValid = false;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QString err = QStringLiteral("Синтаксическая ошибка в map.json: %1").arg(parseError.errorString());
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    QJsonObject root = doc.object();

    // 1. Проверка версии формата (обратная совместимость)
    map.formatVersion = root.value(QStringLiteral("formatVersion")).toInt(1);
    if (map.formatVersion > CURRENT_FORMAT_VERSION) {
        QString err = QStringLiteral("Формат карты v%1 новее текущей версии игры (v%2). Требуется обновление игры.")
                          .arg(map.formatVersion).arg(CURRENT_FORMAT_VERSION);
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    // 2. ID (Хеш)
    map.id = root.value(QStringLiteral("id")).toString().trimmed();

    // 3. Метаданные (объект meta)
    if (!root.contains(QStringLiteral("meta")) || !root[QStringLiteral("meta")].isObject()) {
        QString err = QStringLiteral("В map.json отсутствует обязательный блок 'meta'.");
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }
    map.meta = BeatmapMeta::fromJson(root[QStringLiteral("meta")].toObject());

    // Валидация аудио: поддерживаются .mp3, .wav, .ogg
    QString audioExt = QFileInfo(map.meta.audioFileName).suffix().toLower();
    if (audioExt != QStringLiteral("mp3") && audioExt != QStringLiteral("wav") && audioExt != QStringLiteral("ogg")) {
        QString err = QStringLiteral("Неподдерживаемый аудиоформат '%1'. Поддерживаются .mp3, .wav, .ogg.")
                          .arg(map.meta.audioFileName);
        if (error) *error = err;
        map.errorMessage = err;
        map.isValid = false;
        return map;
    }

    // 4. Геймплейные параметры (объект gameplay)
    if (root.contains(QStringLiteral("gameplay")) && root[QStringLiteral("gameplay")].isObject()) {
        map.gameplay = BeatmapGameplay::fromJson(root[QStringLiteral("gameplay")].toObject());
    } else {
        // Дефолтные параметры для совместимости
        map.gameplay = BeatmapGameplay();
    }

    // 5. Ноты (массив notes)
    if (!root.contains(QStringLiteral("notes")) || !root[QStringLiteral("notes")].isArray()) {
        QString err = QStringLiteral("В map.json отсутствует обязательный массив 'notes'.");
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    QJsonArray notesArray = root[QStringLiteral("notes")].toArray();
    map.notes.reserve(notesArray.size());
    for (const auto &val : notesArray) {
        if (val.isObject()) {
            map.notes.append(NoteData::fromJson(val.toObject()));
        }
    }

    // Сортировка нот по возрастанию времени для гарантированной консистентности геймплея
    std::sort(map.notes.begin(), map.notes.end(), [](const NoteData &a, const NoteData &b) {
        return a.timeMs < b.timeMs;
    });

    map.isValid = true;
    return map;
}

QString Beatmap::formattedBpm() const
{
    return QStringLiteral("%1 BPM").arg(QString::number(meta.bpm, 'f', (meta.bpm == (int)meta.bpm) ? 0 : 1));
}

QString Beatmap::formattedDuration() const
{
    int durMs = meta.songDurationMs;
    if (durMs <= 0 && !notes.isEmpty()) {
        durMs = notes.last().timeMs + 2000; // приблизительно по последней ноте
    }
    int totalSec = durMs / 1000;
    int min = totalSec / 60;
    int sec = totalSec % 60;
    return QStringLiteral("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
}

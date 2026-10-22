#include "beatmapmanager.h"

#include <QtCore/private/qzipreader_p.h>
#include <QtCore/private/qzipwriter_p.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDebug>

BeatmapManager& BeatmapManager::instance()
{
    static BeatmapManager s_instance;
    return s_instance;
}

BeatmapManager::BeatmapManager(QObject *parent)
    : QObject(parent)
{
    // Инициализация директории кэша для распакованных аудиофайлов
    QString baseCache = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (baseCache.isEmpty()) {
        baseCache = QDir::tempPath() + QStringLiteral("/BeatKey");
    }
    m_cacheDir = baseCache + QStringLiteral("/songs_cache");
    QDir().mkpath(m_cacheDir);
}

QString BeatmapManager::cacheDirectory() const
{
    return m_cacheDir;
}

void BeatmapManager::clearCache()
{
    QDir dir(m_cacheDir);
    if (dir.exists()) {
        dir.removeRecursively();
        dir.mkpath(m_cacheDir);
    }
}

// ============================================================================
// Map Hash Calculation (TZ 4.10.3)
// ============================================================================

QString BeatmapManager::calculateMapHash(const QByteArray &mapJsonBytes, const QByteArray &audioBytes)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(mapJsonBytes);
    hash.addData(audioBytes);
    return QString::fromLatin1(hash.result().toHex());
}

// ============================================================================
// Batch Directory Loading
// ============================================================================

QVector<Beatmap> BeatmapManager::loadAllFromDirectory(const QString &dirPath, QStringList *errors)
{
    QVector<Beatmap> result;

    QDir dir(dirPath);
    if (!dir.exists()) {
        QString err = QStringLiteral("Директория '%1' не найдена.").arg(dirPath);
        qWarning() << "[BeatmapManager]" << err;
        if (errors) errors->append(err);
        return result;
    }

    // Сканируем только файлы с расширением .bkey
    QStringList filters;
    filters << QStringLiteral("*.bkey");
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    result.reserve(fileList.size());
    for (const QFileInfo &fileInfo : fileList) {
        QString loadError;
        Beatmap map = loadFromFile(fileInfo.absoluteFilePath(), &loadError);
        if (map.isValid) {
            result.append(map);
        } else {
            QString fullErrMsg = QStringLiteral("[%1] %2").arg(fileInfo.fileName(), loadError);
            qWarning() << "[BeatmapManager] Ошибка загрузки карты:" << fullErrMsg;
            if (errors) errors->append(fullErrMsg);
        }
    }

    qDebug() << "[BeatmapManager] Успешно загружено" << result.size() << "карт из" << dirPath;
    return result;
}

// ============================================================================
// Single File Loading & Validation (TZ 4.10.4)
// ============================================================================

Beatmap BeatmapManager::loadFromFile(const QString &bkeyFilePath, QString *error)
{
    Beatmap map;
    map.isValid = false;
    map.bkeyFilePath = bkeyFilePath;

    QFileInfo fileInfo(bkeyFilePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: файл '%1' не найден.").arg(bkeyFilePath);
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    // 1. Проверка целостности архива ZIP
    QZipReader reader(bkeyFilePath);
    if (!reader.isReadable()) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: архив поврежден или не является валидным ZIP.");
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    // 2. Наличие в корне архива файла map.json
    QByteArray mapJsonBytes = reader.fileData(QStringLiteral("map.json"));
    if (mapJsonBytes.isEmpty()) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: в корне архива отсутствует файл map.json.");
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    // 3. Синтаксическая корректность JSON и проверка обязательных полей
    QString jsonErr;
    map = Beatmap::fromJson(mapJsonBytes, &jsonErr);
    if (!map.isValid) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: %1").arg(jsonErr);
        if (error) *error = err;
        map.errorMessage = err;
        return map;
    }

    // Восстанавливаем путь к bkey
    map.bkeyFilePath = bkeyFilePath;

    // 4. Присутствие внутри архива аудиофайла и проверка формата строго .mp3
    if (!map.meta.audioFileName.endsWith(QStringLiteral(".mp3"), Qt::CaseInsensitive)) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: поддерживается только аудиоформат .mp3, получено '%1'.")
                          .arg(map.meta.audioFileName);
        if (error) *error = err;
        map.errorMessage = err;
        map.isValid = false;
        return map;
    }

    QByteArray audioBytes = reader.fileData(map.meta.audioFileName);
    if (audioBytes.isEmpty()) {
        QString err = QStringLiteral("Ошибка валидации карты .bkey: внутри архива отсутствует аудиофайл '%1'.")
                          .arg(map.meta.audioFileName);
        if (error) *error = err;
        map.errorMessage = err;
        map.isValid = false;
        return map;
    }

    // 5. Загрузка файла обложки (если есть)
    if (!map.meta.coverFileName.isEmpty()) {
        QByteArray coverBytes = reader.fileData(map.meta.coverFileName);
        if (!coverBytes.isEmpty()) {
            map.coverImage.loadFromData(coverBytes);
        }
    }

    // 6. Расчет Map Hash (SHA-256) по ТЗ 4.10.3
    QString calculatedHash = calculateMapHash(mapJsonBytes, audioBytes);
    if (map.id.isEmpty()) {
        map.id = calculatedHash;
    }

    // 7. Извлечение аудиофайла в кэш для воспроизведения в AudioManager
    QString songDir = m_cacheDir + QStringLiteral("/") + calculatedHash;
    QDir().mkpath(songDir);
    QString cachedAudioPath = songDir + QStringLiteral("/") + map.meta.audioFileName;

    // Записываем файл в кэш только если его там еще нет
    if (!QFile::exists(cachedAudioPath)) {
        QFile outFile(cachedAudioPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(audioBytes);
            outFile.close();
        } else {
            qWarning() << "[BeatmapManager] Не удалось записать аудиофайл в кэш:" << cachedAudioPath;
        }
    }

    map.extractedAudioPath = cachedAudioPath;
    map.isValid = true;
    return map;
}

// ============================================================================
// Export Package to .bkey
// ============================================================================

bool BeatmapManager::exportToFile(const QString &outBkeyPath,
                                  Beatmap &beatmap,
                                  const QString &audioSourceMp3Path,
                                  const QString &coverImagePath,
                                  QString *error)
{
    QFileInfo audioInfo(audioSourceMp3Path);
    if (!audioInfo.exists() || !audioInfo.fileName().endsWith(QStringLiteral(".mp3"), Qt::CaseInsensitive)) {
        QString err = QStringLiteral("Исходный аудиофайл '%1' не найден или не является .mp3.")
                          .arg(audioSourceMp3Path);
        if (error) *error = err;
        return false;
    }

    QFile audioFile(audioSourceMp3Path);
    if (!audioFile.open(QIODevice::ReadOnly)) {
        QString err = QStringLiteral("Не удалось прочитать аудиофайл '%1'.").arg(audioSourceMp3Path);
        if (error) *error = err;
        return false;
    }
    QByteArray audioBytes = audioFile.readAll();
    audioFile.close();

    beatmap.meta.audioFileName = audioInfo.fileName();

    QByteArray coverBytes;
    if (!coverImagePath.isEmpty() && QFile::exists(coverImagePath)) {
        QFile coverFile(coverImagePath);
        if (coverFile.open(QIODevice::ReadOnly)) {
            coverBytes = coverFile.readAll();
            coverFile.close();
            beatmap.meta.coverFileName = QFileInfo(coverImagePath).fileName();
        }
    }

    // Формируем map.json и рассчитываем контрольный хеш
    QByteArray tempJson = beatmap.toJson(true);
    beatmap.id = calculateMapHash(tempJson, audioBytes);
    QByteArray finalJson = beatmap.toJson(true);

    // Записываем архив через QZipWriter
    QZipWriter writer(outBkeyPath);
    if (writer.status() != QZipWriter::NoError) {
        QString err = QStringLiteral("Не удалось создать архив '%1'.").arg(outBkeyPath);
        if (error) *error = err;
        return false;
    }

    writer.addFile(QStringLiteral("map.json"), finalJson);
    writer.addFile(beatmap.meta.audioFileName, audioBytes);
    if (!coverBytes.isEmpty() && !beatmap.meta.coverFileName.isEmpty()) {
        writer.addFile(beatmap.meta.coverFileName, coverBytes);
    }

    writer.close();
    qDebug() << "[BeatmapManager] Успешно экспортирована карта:" << outBkeyPath;
    return true;
}

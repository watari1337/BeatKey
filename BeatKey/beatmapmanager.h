#ifndef BEATMAPMANAGER_H
#define BEATMAPMANAGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QStringList>

#include "beatmap.h"

/**
 * @brief BeatmapManager handles reading, validating, and exporting .bkey packages.
 *
 * Implements batch loading from directories, ZIP unpacking to local cache,
 * and SHA-256 integrity validation per TZ 4.10.
 */
class BeatmapManager : public QObject
{
    Q_OBJECT

public:
    static BeatmapManager& instance();

    // ========================================================================
    // Loading & Batch Directory Scan
    // ========================================================================
    /**
     * @brief Loads all valid .bkey packages from a user-specified directory.
     * @param dirPath Path to the target folder.
     * @param errors Optional list to accumulate human-readable error messages for corrupt files.
     * @return List of parsed and validated Beatmap objects.
     */
    QVector<Beatmap> loadAllFromDirectory(const QString &dirPath, QStringList *errors = nullptr);

    /**
     * @brief Loads and validates a single .bkey package.
     * @param bkeyFilePath Full path to .bkey file.
     * @param error Optional pointer to receive error description on failure.
     * @return Beatmap object (check .isValid before using).
     */
    Beatmap loadFromFile(const QString &bkeyFilePath, QString *error = nullptr);

    /**
     * @brief Imports all .bkey packages found in sourceDirPath (including subdirectories)
     * into targetDirPath.
     * @param sourceDirPath Folder selected by user.
     * @param targetDirPath Target directory (e.g. songs/ folder).
     * @param errors Optional list to accumulate human-readable error messages.
     * @return Number of imported/copied maps.
     */
    int importAllFromDirectory(const QString &sourceDirPath,
                               const QString &targetDirPath,
                               QStringList *errors = nullptr);

    // ========================================================================
    // Package Export (for Beatmap Editor)
    // ========================================================================
    /**
     * @brief Packages map.json, audio, and cover image into a .bkey container.
     */
    bool exportToFile(const QString &outBkeyPath,
                      Beatmap &beatmap,
                      const QString &audioSourcePath,
                      const QString &coverImagePath,
                      QString *error = nullptr);

    /**
     * @brief Generates a collision-free unique .bkey file path in the songs/ directory.
     * @param title Title of the map to base the sanitized file name on.
     * @return Full path to an available <title>_<salt>.bkey file.
     */
    QString generateUniqueBkeyPath(const QString &title) const;

    // ========================================================================
    // Map Hash Calculation (TZ 4.10.3)
    // ========================================================================
    static QString calculateMapHash(const QByteArray &mapJsonBytes, const QByteArray &audioBytes);

    // ========================================================================
    // Cache Management
    // ========================================================================
    QString cacheDirectory() const;
    void clearCache();

private:
    explicit BeatmapManager(QObject *parent = nullptr);
    ~BeatmapManager() override = default;
    BeatmapManager(const BeatmapManager&) = delete;
    BeatmapManager& operator=(const BeatmapManager&) = delete;

    QString m_cacheDir;
};

#endif // BEATMAPMANAGER_H

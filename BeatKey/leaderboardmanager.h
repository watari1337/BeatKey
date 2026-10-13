#ifndef LEADERBOARDMANAGER_H
#define LEADERBOARDMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "scorerecord.h"

/**
 * @brief Менеджер таблицы лидеров и сетевого взаимодействия с Firebase Realtime Database.
 * Реализует гибридную схему: сетевой обмен через HTTPS REST API с автоматическим
 * откатом на локальное файловое хранилище (JSON) при сбоях сети или отсутствии соединения.
 */
class LeaderboardManager : public QObject
{
    Q_OBJECT

public:
    explicit LeaderboardManager(QObject *parent = nullptr);
    ~LeaderboardManager() override;

    // --- Управление конфигурацией ---
    void setFirebaseUrl(const QString &url);
    QString firebaseUrl() const;

    void setLocalScoresPath(const QString &path);
    QString localScoresPath() const;

    void setTimeoutMs(int timeoutMs);
    int timeoutMs() const;

    // --- Основные операции ---

    /**
     * @brief Запросить глобальную таблицу лидеров (топ игроков) для заданной карты.
     * При отсутствии сети или ошибке запроса автоматически загружает локальные рекорды.
     * @param mapHash SHA-256 хеш карты.
     * @param limit Количество возвращаемых строк (по умолчанию 10).
     */
    void fetchLeaderboard(const QString &mapHash, int limit = 10);

    /**
     * @brief Отправить рекорд игрока по заданной карте в Firebase.
     * Перед перезаписью проверяет существующий результат: если в БД уже есть более высокий счет,
     * запись не перезаписывается. Рекорд также дублируется в локальное хранилище.
     * @param mapHash SHA-256 хеш карты.
     * @param newRecord Структура с данными попытки.
     */
    void submitScore(const QString &mapHash, const ScoreRecord &newRecord);

    // --- Локальное хранилище (офлайн-режим) ---

    /**
     * @brief Загрузить локальные рекорды по карте из local_scores.json.
     * @param mapHash SHA-256 хеш карты.
     * @return Отсортированный по убыванию очков список рекордов.
     */
    QList<ScoreRecord> loadLocalScores(const QString &mapHash);

    /**
     * @brief Сохранить рекорд в локальный файл local_scores.json.
     * Перезаписывает результат игрока только при превышении предыдущего лучшего счета.
     * @param mapHash SHA-256 хеш карты.
     * @param record Данные рекорда.
     * @return true в случае успешной записи, false при ошибке ввода-вывода.
     */
    bool saveLocalScore(const QString &mapHash, const ScoreRecord &record);

    /**
     * @brief Полная очистка локальной истории рекордов.
     */
    void clearLocalScores();

signals:
    /**
     * @brief Сигнал готовности таблицы лидеров для отображения в GUI.
     * @param mapHash Хеш карты.
     * @param records Список лучших результатов.
     * @param isGlobal true — данные из Firebase, false — данные из локального кэша.
     */
    void leaderboardLoaded(const QString &mapHash, const QList<ScoreRecord> &records, bool isGlobal);

    /**
     * @brief Сигнал завершения обработки отправки рекорда.
     * @param isNewHighScore true — установлен новый рекорд, false — рекорд не побит.
     * @param finalScore Итоговый лучший результат в базе.
     * @param isGlobal true — результат зафиксирован в Firebase, false — сохранен только локально.
     */
    void scoreSubmitted(bool isNewHighScore, int finalScore, bool isGlobal);

    /**
     * @brief Сигнал ошибки сетевого обмена.
     * @param errorMessage Текст сообщения об ошибке.
     */
    void networkError(const QString &errorMessage);

    /**
     * @brief Информационное сообщение о текущем статусе (для строки состояния в UI).
     * @param message Текст сообщения.
     */
    void statusMessage(const QString &message);

private slots:
    void handleFetchFinished(QNetworkReply *reply, const QString &mapHash, int limit);
    void handleCheckBeforeSubmitFinished(QNetworkReply *reply, const QString &mapHash, const ScoreRecord &newRecord);
    void handleSubmitPutFinished(QNetworkReply *reply, const QString &mapHash, const ScoreRecord &newRecord);

private:
    QUrl makeMapUrl(const QString &mapHash) const;
    QUrl makePlayerUrl(const QString &mapHash, const QString &nickname) const;
    QNetworkRequest makeRequest(const QUrl &url) const;

    QNetworkAccessManager *m_networkManager;
    QString m_firebaseUrl;
    QString m_localScoresPath;
    int m_timeoutMs;
};

#endif // LEADERBOARDMANAGER_H

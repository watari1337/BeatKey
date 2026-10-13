#include "leaderboardmanager.h"
#include "firebaseconfig.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QDebug>
#include <algorithm>

LeaderboardManager::LeaderboardManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_firebaseUrl(FirebaseConfig::DEFAULT_DATABASE_URL)
    , m_localScoresPath("local_scores.json")
    , m_timeoutMs(5000)
{
}

LeaderboardManager::~LeaderboardManager()
{
}

void LeaderboardManager::setFirebaseUrl(const QString &url)
{
    m_firebaseUrl = url;
    if (m_firebaseUrl.endsWith('/')) {
        m_firebaseUrl.chop(1);
    }
}

QString LeaderboardManager::firebaseUrl() const
{
    return m_firebaseUrl;
}

void LeaderboardManager::setLocalScoresPath(const QString &path)
{
    m_localScoresPath = path;
}

QString LeaderboardManager::localScoresPath() const
{
    return m_localScoresPath;
}

void LeaderboardManager::setTimeoutMs(int timeoutMs)
{
    m_timeoutMs = timeoutMs;
}

int LeaderboardManager::timeoutMs() const
{
    return m_timeoutMs;
}

QUrl LeaderboardManager::makeMapUrl(const QString &mapHash) const
{
    QString base = m_firebaseUrl;
    if (base.endsWith('/')) {
        base.chop(1);
    }
    return QUrl(QString("%1/leaderboards/%2.json").arg(base, mapHash));
}

QUrl LeaderboardManager::makePlayerUrl(const QString &mapHash, const QString &nickname) const
{
    QString base = m_firebaseUrl;
    if (base.endsWith('/')) {
        base.chop(1);
    }
    return QUrl(QString("%1/leaderboards/%2/%3.json").arg(base, mapHash, nickname));
}

QNetworkRequest LeaderboardManager::makeRequest(const QUrl &url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(m_timeoutMs);
    return request;
}

void LeaderboardManager::fetchLeaderboard(const QString &mapHash, int limit)
{
    if (m_firebaseUrl.isEmpty() || m_firebaseUrl.contains("YOUR-FIREBASE-PROJECT")) {
        emit statusMessage("Firebase URL не настроен. Отображаются локальные результаты.");
        QList<ScoreRecord> local = loadLocalScores(mapHash);
        if (local.size() > limit) {
            local = local.mid(0, limit);
        }
        emit leaderboardLoaded(mapHash, local, false);
        return;
    }

    QUrl url = makeMapUrl(mapHash);
    QUrlQuery query;
    query.addQueryItem("orderBy", "\"score\"");
    query.addQueryItem("limitToLast", QString::number(limit));
    url.setQuery(query);

    QNetworkRequest req = makeRequest(url);
    QNetworkReply *reply = m_networkManager->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, mapHash, limit]() {
        handleFetchFinished(reply, mapHash, limit);
    });
}

void LeaderboardManager::handleFetchFinished(QNetworkReply *reply, const QString &mapHash, int limit)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = QString("Ошибка сети (%1): %2").arg(reply->error()).arg(reply->errorString());
        emit networkError(err);
        emit statusMessage("Сетевое подключение недоступно. Отображаются локальные результаты.");

        // Автономный откат к локальному хранилищу
        QList<ScoreRecord> localRecords = loadLocalScores(mapHash);
        if (localRecords.size() > limit) {
            localRecords = localRecords.mid(0, limit);
        }
        emit leaderboardLoaded(mapHash, localRecords, false);
        return;
    }

    QByteArray rawData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(rawData);
    QList<ScoreRecord> records;

    if (doc.isObject()) {
        QJsonObject root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it) {
            if (it.value().isObject()) {
                QJsonObject playerObj = it.value().toObject();
                if (!playerObj.contains("nickname") || playerObj.value("nickname").toString().isEmpty()) {
                    playerObj["nickname"] = it.key();
                }
                records.append(ScoreRecord::fromJson(playerObj));
            }
        }
    }

    // Сортируем записи по убыванию очков (вверху лучший счет)
    std::sort(records.begin(), records.end(), [](const ScoreRecord &a, const ScoreRecord &b) {
        return a.score > b.score;
    });

    if (records.size() > limit) {
        records = records.mid(0, limit);
    }

    emit leaderboardLoaded(mapHash, records, true);
}

void LeaderboardManager::submitScore(const QString &mapHash, const ScoreRecord &newRecord)
{
    // Всегда фиксируем результат в локальном кэше
    saveLocalScore(mapHash, newRecord);

    if (m_firebaseUrl.isEmpty() || m_firebaseUrl.contains("YOUR-FIREBASE-PROJECT")) {
        emit statusMessage("Рекорд сохранен локально (Firebase URL не задан).");
        emit scoreSubmitted(true, newRecord.score, false);
        return;
    }

    // Шаг 1: проверяем текущую запись игрока в сети
    QUrl checkUrl = makePlayerUrl(mapHash, newRecord.nickname);
    QNetworkRequest checkReq = makeRequest(checkUrl);
    QNetworkReply *checkReply = m_networkManager->get(checkReq);

    connect(checkReply, &QNetworkReply::finished, this, [this, checkReply, mapHash, newRecord]() {
        handleCheckBeforeSubmitFinished(checkReply, mapHash, newRecord);
    });
}

void LeaderboardManager::handleCheckBeforeSubmitFinished(QNetworkReply *reply, const QString &mapHash, const ScoreRecord &newRecord)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = QString("Не удалось проверить рейтинг в сети (%1): %2").arg(reply->error()).arg(reply->errorString());
        emit networkError(err);
        emit statusMessage("Сетевое подключение недоступно. Результат сохранен только локально.");
        emit scoreSubmitted(true, newRecord.score, false);
        return;
    }

    QByteArray rawData = reply->readAll();
    QString responseStr = QString::fromUtf8(rawData).trimmed();

    // Если запись уже есть, проверяем сохраненный счет
    if (!responseStr.isEmpty() && responseStr != "null") {
        QJsonDocument doc = QJsonDocument::fromJson(rawData);
        if (doc.isObject()) {
            int existingScore = doc.object().value("score").toInt();
            if (newRecord.score <= existingScore) {
                // Новый счет не превышает рекорд — в базе оставляем старый более высокий счет
                emit statusMessage("Рекорд не побит. В рейтинге сохранен текущий лучший результат.");
                emit scoreSubmitted(false, existingScore, true);
                return;
            }
        }
    }

    // Рекорд побит либо новый игрок — отправляем перезапись через PUT
    QUrl putUrl = makePlayerUrl(mapHash, newRecord.nickname);
    QNetworkRequest putReq = makeRequest(putUrl);
    QByteArray body = QJsonDocument(newRecord.toJson()).toJson(QJsonDocument::Compact);

    QNetworkReply *putReply = m_networkManager->put(putReq, body);
    connect(putReply, &QNetworkReply::finished, this, [this, putReply, mapHash, newRecord]() {
        handleSubmitPutFinished(putReply, mapHash, newRecord);
    });
}

void LeaderboardManager::handleSubmitPutFinished(QNetworkReply *reply, const QString &mapHash, const ScoreRecord &newRecord)
{
    Q_UNUSED(mapHash);
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        QString err = QString("Ошибка сохранения в Firebase (%1): %2").arg(reply->error()).arg(reply->errorString());
        emit networkError(err);
        emit statusMessage("Сетевая ошибка при отправке. Результат зафиксирован локально.");
        emit scoreSubmitted(true, newRecord.score, false);
        return;
    }

    emit statusMessage("Рекорд успешно зафиксирован в глобальной таблице лидеров!");
    emit scoreSubmitted(true, newRecord.score, true);
}

QList<ScoreRecord> LeaderboardManager::loadLocalScores(const QString &mapHash)
{
    QList<ScoreRecord> list;
    QFile file(m_localScoresPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return list;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        return list;
    }

    QJsonObject root = doc.object();
    if (!root.contains(mapHash) || !root.value(mapHash).isArray()) {
        return list;
    }

    QJsonArray array = root.value(mapHash).toArray();
    for (const QJsonValue &val : array) {
        if (val.isObject()) {
            list.append(ScoreRecord::fromJson(val.toObject()));
        }
    }

    std::sort(list.begin(), list.end(), [](const ScoreRecord &a, const ScoreRecord &b) {
        return a.score > b.score;
    });

    return list;
}

bool LeaderboardManager::saveLocalScore(const QString &mapHash, const ScoreRecord &record)
{
    QFile file(m_localScoresPath);
    QJsonObject root;

    if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();
        if (doc.isObject()) {
            root = doc.object();
        }
    }

    QJsonArray mapArray = root.value(mapHash).toArray();
    bool playerFound = false;
    QJsonArray updatedArray;

    for (const QJsonValue &val : mapArray) {
        if (val.isObject()) {
            QJsonObject obj = val.toObject();
            if (obj.value("nickname").toString() == record.nickname) {
                playerFound = true;
                int currentScore = obj.value("score").toInt();
                if (record.score > currentScore) {
                    updatedArray.append(record.toJson());
                } else {
                    updatedArray.append(obj); // Сохраняем лучший
                }
            } else {
                updatedArray.append(obj);
            }
        }
    }

    if (!playerFound) {
        updatedArray.append(record.toJson());
    }

    // Сортировка перед записью
    QList<ScoreRecord> list;
    for (const QJsonValue &v : updatedArray) {
        list.append(ScoreRecord::fromJson(v.toObject()));
    }
    std::sort(list.begin(), list.end(), [](const ScoreRecord &a, const ScoreRecord &b) {
        return a.score > b.score;
    });

    QJsonArray sortedArray;
    for (const ScoreRecord &r : list) {
        sortedArray.append(r.toJson());
    }

    root[mapHash] = sortedArray;

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "LeaderboardManager: не удалось открыть файл для записи" << m_localScoresPath;
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void LeaderboardManager::clearLocalScores()
{
    QFile file(m_localScoresPath);
    if (file.exists()) {
        file.remove();
    }
}

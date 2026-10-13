#ifndef SCORERECORD_H
#define SCORERECORD_H

#include <QString>
#include <QJsonObject>
#include <QDateTime>

/**
 * @brief Структура данных одного игрового рекорда.
 * Соответствует спецификации пункта 4.8 ТЗ «BeatKey».
 */
struct ScoreRecord {
    QString nickname;
    int score = 0;
    double accuracy = 0.0;
    int maxCombo = 0;
    int count300 = 0;
    int count200 = 0;
    int count100 = 0;
    int countMiss = 0;
    QString date; // Формат ISO-8601 (YYYY-MM-DDTHH:MM:SSZ)

    ScoreRecord() {
        date = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    }

    ScoreRecord(const QString &nick, int sc, double acc, int combo,
                int c300, int c200, int c100, int cMiss,
                const QString &dt = QString())
        : nickname(nick)
        , score(sc)
        , accuracy(acc)
        , maxCombo(combo)
        , count300(c300)
        , count200(c200)
        , count100(c100)
        , countMiss(cMiss)
        , date(dt.isEmpty() ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate) : dt)
    {
    }

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["nickname"] = nickname;
        obj["score"] = score;
        obj["accuracy"] = accuracy;
        obj["maxCombo"] = maxCombo;
        obj["count300"] = count300;
        obj["count200"] = count200;
        obj["count100"] = count100;
        obj["countMiss"] = countMiss;
        obj["date"] = date.isEmpty() ? QDateTime::currentDateTimeUtc().toString(Qt::ISODate) : date;
        return obj;
    }

    static ScoreRecord fromJson(const QJsonObject &obj) {
        ScoreRecord rec;
        rec.nickname = obj.value("nickname").toString();
        rec.score = obj.value("score").toInt();
        rec.accuracy = obj.value("accuracy").toDouble();
        rec.maxCombo = obj.value("maxCombo").toInt();
        rec.count300 = obj.value("count300").toInt();
        rec.count200 = obj.value("count200").toInt();
        rec.count100 = obj.value("count100").toInt();
        rec.countMiss = obj.value("countMiss").toInt();
        rec.date = obj.value("date").toString();
        if (rec.date.isEmpty()) {
            rec.date = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        }
        return rec;
    }
};

#endif // SCORERECORD_H

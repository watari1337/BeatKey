#ifndef GAMERESULT_H
#define GAMERESULT_H

#include "beatmap.h"

/**
 * @brief GameResult содержит итоговые статистические показатели сыгранного раунда.
 * Полностью соответствует спецификации п. 4.7.10 и 4.7.11 ТЗ BeatKey.
 */
struct GameResult {
    Beatmap beatmap;
    int score = 0;
    double accuracy = 0.0;
    int maxCombo = 0;
    int count300 = 0; // Идеально (<= 25 мс)
    int count200 = 0; // Хорошо (25-50 мс)
    int count100 = 0; // Сойдет (50-100 мс)
    int countMiss = 0;// Промахи (> 100 мс или автопромах)
    bool isClear = true;
    QString rank = QStringLiteral("D"); // SS, S, A, B, C, D, FAIL
};

#endif // GAMERESULT_H

#ifndef RESULTSWIDGET_H
#define RESULTSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "gameresult.h"

class LeaderboardManager;

/**
 * @brief ResultsWidget отображает экран итоговых результатов и достижений после завершения карты.
 * Соответствует спецификации пунктов 4.7.10 и 4.7.11 ТЗ BeatKey.
 */
class ResultsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ResultsWidget(QWidget *parent = nullptr);
    ~ResultsWidget() override = default;

    /**
     * @brief Отобразить результаты сыгранного раунда.
     */
    void displayResult(const GameResult &result);

signals:
    void retryRequested(const Beatmap &beatmap);
    void exitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    QWidget* createStatCard(const QString &title, const QString &countText, const QColor &accentColor);

    GameResult m_result;

    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QLabel *m_rankBadgeLabel;
    QLabel *m_scoreValueLabel;
    QLabel *m_accuracyValueLabel;
    QLabel *m_maxComboValueLabel;

    QLabel *m_count300Label;
    QLabel *m_count200Label;
    QLabel *m_count100Label;
    QLabel *m_countMissLabel;

    QPushButton *m_retryBtn;
    QPushButton *m_exitBtn;

    LeaderboardManager *m_leaderboardManager;
};

#endif // RESULTSWIDGET_H

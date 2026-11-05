#include "resultswidget.h"
#include "theme.h"
#include "audiomanager.h"
#include "settingsmanager.h"
#include "leaderboardmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

ResultsWidget::ResultsWidget(QWidget *parent)
    : QWidget(parent)
    , m_titleLabel(nullptr)
    , m_subtitleLabel(nullptr)
    , m_rankBadgeLabel(nullptr)
    , m_scoreValueLabel(nullptr)
    , m_accuracyValueLabel(nullptr)
    , m_maxComboValueLabel(nullptr)
    , m_count300Label(nullptr)
    , m_count200Label(nullptr)
    , m_count100Label(nullptr)
    , m_countMissLabel(nullptr)
    , m_retryBtn(nullptr)
    , m_exitBtn(nullptr)
    , m_leaderboardManager(new LeaderboardManager(this))
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
}

void ResultsWidget::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(50, 40, 50, 40);
    rootLayout->setSpacing(24);

    // 1. Верхний блок: Название трека и метаданные
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(4);

    m_titleLabel = new QLabel(QStringLiteral("Название трека"), this);
    m_titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 26px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "background: transparent;"
        "border: none;"
    ));

    m_subtitleLabel = new QLabel(QStringLiteral("Исполнитель | Сложность | Автор карты"), this);
    m_subtitleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 14px;"
        "color: #a09cb0;"
        "background: transparent;"
        "border: none;"
    ));

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_subtitleLabel);
    rootLayout->addLayout(headerLayout);

    // 2. Центральная панель результатов (Карточка)
    auto *cardContainer = new QWidget(this);
    cardContainer->setObjectName(QStringLiteral("resultsCard"));
    cardContainer->setStyleSheet(Theme::dialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 160));
    shadow->setOffset(0, 10);
    cardContainer->setGraphicsEffect(shadow);

    auto *cardLayout = new QHBoxLayout(cardContainer);
    cardLayout->setContentsMargins(40, 32, 40, 32);
    cardLayout->setSpacing(40);

    // Левая колонка: Огромный бэйдж ранга
    auto *rankLayout = new QVBoxLayout();
    rankLayout->setAlignment(Qt::AlignCenter);

    m_rankBadgeLabel = new QLabel(QStringLiteral("S"), cardContainer);
    m_rankBadgeLabel->setAlignment(Qt::AlignCenter);
    m_rankBadgeLabel->setFixedSize(140, 140);
    m_rankBadgeLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 64px;"
        "font-weight: 900;"
        "color: #a259ff;"
        "background: rgba(30, 20, 50, 190);"
        "border: 3px solid #bb9af7;"
        "border-radius: 70px;"
    ));
    rankLayout->addWidget(m_rankBadgeLabel);

    auto *rankTitle = new QLabel(QStringLiteral("РАНГ"), cardContainer);
    rankTitle->setAlignment(Qt::AlignCenter);
    rankTitle->setStyleSheet(Theme::secondaryLabelStyle());
    rankLayout->addWidget(rankTitle);

    cardLayout->addLayout(rankLayout);

    // Средняя колонка: Счет, Точность, Макс комбо
    auto *mainStatsLayout = new QVBoxLayout();
    mainStatsLayout->setAlignment(Qt::AlignVCenter);
    mainStatsLayout->setSpacing(14);

    // Счет
    auto *scoreBlock = new QVBoxLayout();
    scoreBlock->setSpacing(2);
    auto *scoreTitle = new QLabel(QStringLiteral("ИТОГОВЫЙ СЧЕТ"), cardContainer);
    scoreTitle->setStyleSheet(Theme::secondaryLabelStyle());
    m_scoreValueLabel = new QLabel(QStringLiteral("0"), cardContainer);
    m_scoreValueLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 32px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "background: transparent;"
        "border: none;"
    ));
    scoreBlock->addWidget(scoreTitle);
    scoreBlock->addWidget(m_scoreValueLabel);
    mainStatsLayout->addLayout(scoreBlock);

    // Точность и комбо в один ряд
    auto *rowStats = new QHBoxLayout();
    rowStats->setSpacing(30);

    auto *accBlock = new QVBoxLayout();
    accBlock->setSpacing(2);
    auto *accTitle = new QLabel(QStringLiteral("ТОЧНОСТЬ"), cardContainer);
    accTitle->setStyleSheet(Theme::secondaryLabelStyle());
    m_accuracyValueLabel = new QLabel(QStringLiteral("100.00%"), cardContainer);
    m_accuracyValueLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #bb9af7;"
        "background: transparent;"
        "border: none;"
    ));
    accBlock->addWidget(accTitle);
    accBlock->addWidget(m_accuracyValueLabel);
    rowStats->addLayout(accBlock);

    auto *comboBlock = new QVBoxLayout();
    comboBlock->setSpacing(2);
    auto *comboTitle = new QLabel(QStringLiteral("МАКС. КОМБО"), cardContainer);
    comboTitle->setStyleSheet(Theme::secondaryLabelStyle());
    m_maxComboValueLabel = new QLabel(QStringLiteral("0x"), cardContainer);
    m_maxComboValueLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 22px;"
        "font-weight: bold;"
        "color: #2ecc71;"
        "background: transparent;"
        "border: none;"
    ));
    comboBlock->addWidget(comboTitle);
    comboBlock->addWidget(m_maxComboValueLabel);
    rowStats->addLayout(comboBlock);

    mainStatsLayout->addLayout(rowStats);
    cardLayout->addLayout(mainStatsLayout, 1);

    // Правая колонка: 4 карточки градации ударов (300, 200, 100, Miss)
    auto *hitsGrid = new QGridLayout();
    hitsGrid->setSpacing(12);

    m_count300Label = new QLabel(QStringLiteral("0"), cardContainer);
    m_count200Label = new QLabel(QStringLiteral("0"), cardContainer);
    m_count100Label = new QLabel(QStringLiteral("0"), cardContainer);
    m_countMissLabel = new QLabel(QStringLiteral("0"), cardContainer);

    hitsGrid->addWidget(createStatCard(QStringLiteral("ИДЕАЛЬНО (300)"), QStringLiteral("0"), QColor(187, 154, 247)), 0, 0);
    hitsGrid->addWidget(createStatCard(QStringLiteral("ХОРОШО (200)"), QStringLiteral("0"), Theme::Colors::Success), 0, 1);
    hitsGrid->addWidget(createStatCard(QStringLiteral("СОЙДЕТ (100)"), QStringLiteral("0"), Theme::Colors::Warning), 1, 0);
    hitsGrid->addWidget(createStatCard(QStringLiteral("ПРОМАХИ (MISS)"), QStringLiteral("0"), Theme::Colors::Danger), 1, 1);

    cardLayout->addLayout(hitsGrid);
    rootLayout->addWidget(cardContainer, 1);

    // 3. Нижняя панель действий (Кнопки)
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(16);

    m_retryBtn = new QPushButton(QStringLiteral("Повторить карту"), this);
    m_retryBtn->setCursor(Qt::PointingHandCursor);
    m_retryBtn->setStyleSheet(Theme::primaryButtonStyle());
    m_retryBtn->setFixedHeight(44);
    m_retryBtn->setMinimumWidth(180);
    connect(m_retryBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit retryRequested(m_result.beatmap);
    });

    m_exitBtn = new QPushButton(QStringLiteral("Выход в меню"), this);
    m_exitBtn->setCursor(Qt::PointingHandCursor);
    m_exitBtn->setStyleSheet(Theme::secondaryButtonStyle());
    m_exitBtn->setFixedHeight(44);
    m_exitBtn->setMinimumWidth(160);
    connect(m_exitBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit exitRequested();
    });

    bottomLayout->addStretch(1);
    bottomLayout->addWidget(m_retryBtn);
    bottomLayout->addWidget(m_exitBtn);

    rootLayout->addLayout(bottomLayout);
}

QWidget* ResultsWidget::createStatCard(const QString &title, const QString &countText, const QColor &accentColor)
{
    auto *card = new QWidget(this);
    card->setStyleSheet(QStringLiteral(
        "background: rgba(22, 14, 38, 200);"
        "border: 1px solid rgba(125, 68, 207, 120);"
        "border-radius: 10px;"
    ));
    card->setFixedSize(145, 65);

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(2);
    layout->setAlignment(Qt::AlignCenter);

    auto *titleLbl = new QLabel(title, card);
    titleLbl->setAlignment(Qt::AlignCenter);
    titleLbl->setStyleSheet(Theme::secondaryLabelStyle());

    auto *valLbl = new QLabel(countText, card);
    valLbl->setAlignment(Qt::AlignCenter);
    valLbl->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 18px;"
        "font-weight: bold;"
        "color: %1;"
        "background: transparent;"
        "border: none;"
    ).arg(accentColor.name()));

    layout->addWidget(titleLbl);
    layout->addWidget(valLbl);

    if (title.contains(QStringLiteral("300"))) {
        m_count300Label = valLbl;
    } else if (title.contains(QStringLiteral("200"))) {
        m_count200Label = valLbl;
    } else if (title.contains(QStringLiteral("100"))) {
        m_count100Label = valLbl;
    } else if (title.contains(QStringLiteral("MISS"))) {
        m_countMissLabel = valLbl;
    }

    return card;
}

void ResultsWidget::displayResult(const GameResult &result)
{
    m_result = result;

    m_titleLabel->setText(result.beatmap.meta.title.isEmpty()
        ? QStringLiteral("Без названия")
        : result.beatmap.meta.title);

    m_subtitleLabel->setText(QStringLiteral("%1 | Сложность: %2 | Автор карты: %3")
        .arg(result.beatmap.meta.artist.isEmpty() ? QStringLiteral("Unknown Artist") : result.beatmap.meta.artist,
             result.beatmap.meta.difficultyName.isEmpty() ? QStringLiteral("Нормально") : result.beatmap.meta.difficultyName,
             result.beatmap.meta.mapper.isEmpty() ? QStringLiteral("Player") : result.beatmap.meta.mapper));

    // Настройка ранга
    m_rankBadgeLabel->setText(result.rank);
    if (result.rank == QStringLiteral("SS")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 58px; font-weight: 900;"
            "color: #ffdf00; background: rgba(45, 35, 10, 210); border: 3px solid #ffdf00; border-radius: 70px;"
        ));
    } else if (result.rank == QStringLiteral("S")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 64px; font-weight: 900;"
            "color: #2ecc71; background: rgba(15, 35, 20, 210); border: 3px solid #2ecc71; border-radius: 70px;"
        ));
    } else if (result.rank == QStringLiteral("A")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 64px; font-weight: 900;"
            "color: #a259ff; background: rgba(30, 20, 50, 210); border: 3px solid #a259ff; border-radius: 70px;"
        ));
    } else if (result.rank == QStringLiteral("B")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 64px; font-weight: 900;"
            "color: #3498db; background: rgba(15, 25, 45, 210); border: 3px solid #3498db; border-radius: 70px;"
        ));
    } else if (result.rank == QStringLiteral("C")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 64px; font-weight: 900;"
            "color: #e67e22; background: rgba(40, 25, 10, 210); border: 3px solid #e67e22; border-radius: 70px;"
        ));
    } else if (result.rank == QStringLiteral("FAIL")) {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 34px; font-weight: 900;"
            "color: #ff5577; background: rgba(45, 15, 25, 210); border: 3px solid #ff5577; border-radius: 70px;"
        ));
    } else {
        m_rankBadgeLabel->setStyleSheet(QStringLiteral(
            "font-family: 'Segoe UI', sans-serif; font-size: 64px; font-weight: 900;"
            "color: #a09cb0; background: rgba(30, 25, 40, 210); border: 3px solid #a09cb0; border-radius: 70px;"
        ));
    }

    // Счет и статистика
    m_scoreValueLabel->setText(QLocale(QLocale::Russian).toString(result.score));
    m_accuracyValueLabel->setText(QString::asprintf("%.2f%%", result.accuracy));
    m_maxComboValueLabel->setText(QStringLiteral("%1x").arg(result.maxCombo));

    if (m_count300Label) m_count300Label->setText(QString::number(result.count300));
    if (m_count200Label) m_count200Label->setText(QString::number(result.count200));
    if (m_count100Label) m_count100Label->setText(QString::number(result.count100));
    if (m_countMissLabel) m_countMissLabel->setText(QString::number(result.countMiss));

    // Автоматическая фиксация рекорда в локальной базе и отправка в таблицу лидеров (п. 4.8 ТЗ)
    if (result.isClear && !result.beatmap.id.isEmpty()) {
        ScoreRecord record(
            SettingsManager::instance().nickname(),
            result.score,
            result.accuracy,
            result.maxCombo,
            result.count300,
            result.count200,
            result.count100,
            result.countMiss
        );
        m_leaderboardManager->submitScore(result.beatmap.id, record);
    }
}

void ResultsWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    if (!m_result.beatmap.coverImage.isNull()) {
        QPixmap scaled = m_result.beatmap.coverImage.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int cropX = (scaled.width() - width()) / 2;
        int cropY = (scaled.height() - height()) / 2;
        painter.drawPixmap(0, 0, scaled, cropX, cropY, width(), height());
        painter.fillRect(rect(), QColor(16, 10, 28, 220));
    } else {
        Theme::paintWindowBackground(&painter, rect());
    }
}

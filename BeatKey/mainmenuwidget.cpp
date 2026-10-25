#include "mainmenuwidget.h"
#include "settingsmanager.h"
#include "theme.h"
#include "audiomanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QFile>

MainMenuWidget::MainMenuWidget(QWidget *parent)
    : QWidget(parent)
    , m_playerLabel(nullptr)
{
    if (QFile::exists(QStringLiteral(":/assets/images/background.jpg"))) {
        m_backgroundPixmap.load(QStringLiteral(":/assets/images/background.jpg"));
    }

    setupUi();

    connect(&SettingsManager::instance(), &SettingsManager::nicknameChanged,
            this, &MainMenuWidget::refreshProfileInfo);

    // Запускаем фоновую музыку главного меню
    AudioManager::instance().playMenuMusic();
}

void MainMenuWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_backgroundPixmap.isNull()) {
        painter.drawPixmap(rect(), m_backgroundPixmap.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        // Semi-transparent dark overlay for high contrast and readability
        painter.fillRect(rect(), QColor(14, 10, 24, 190));
    } else {
        // Fallback stylish dark-violet gradient
        QLinearGradient grad(0, 0, width(), height());
        grad.setColorAt(0.0, QColor(18, 12, 33));
        grad.setColorAt(0.5, QColor(28, 16, 52));
        grad.setColorAt(1.0, QColor(10, 7, 20));
        painter.fillRect(rect(), grad);
    }
}

QPushButton* MainMenuWidget::createMenuButton(const QString &text, const QString &iconName)
{
    Q_UNUSED(iconName);
    auto *btn = new QPushButton(text, this);
    btn->setMinimumSize(320, 56);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(Theme::menuButtonStyle());

    auto *shadow = new QGraphicsDropShadowEffect(btn);
    shadow->setBlurRadius(15);
    shadow->setColor(QColor(162, 89, 255, 120));
    shadow->setOffset(0, 4);
    btn->setGraphicsEffect(shadow);

    return btn;
}

void MainMenuWidget::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 30);

    // Top Header: Logo / Game Title
    auto *headerLayout = new QVBoxLayout();
    headerLayout->setAlignment(Qt::AlignCenter);

    auto *titleLabel = new QLabel(QStringLiteral("BEATKEY"), this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', 'Impact', sans-serif;"
        "font-size: 58px;"
        "font-weight: 900;"
        "letter-spacing: 8px;"
        "color: #ffffff;"
    ));

    auto *titleShadow = new QGraphicsDropShadowEffect(titleLabel);
    titleShadow->setBlurRadius(25);
    titleShadow->setColor(QColor(162, 89, 255, 220));
    titleShadow->setOffset(0, 0);
    titleLabel->setGraphicsEffect(titleShadow);

    auto *subTitleLabel = new QLabel(QStringLiteral("RHYTHM GAME & BEATMAP STUDIO"), this);
    subTitleLabel->setAlignment(Qt::AlignCenter);
    subTitleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 13px;"
        "font-weight: 600;"
        "letter-spacing: 4px;"
        "color: #bb9af7;"
    ));

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(subTitleLabel);

    mainLayout->addSpacing(20);
    mainLayout->addLayout(headerLayout);
    mainLayout->addSpacing(35);

    // Center Navigation Buttons
    auto *buttonCenterContainer = new QVBoxLayout();
    buttonCenterContainer->setAlignment(Qt::AlignCenter);
    buttonCenterContainer->setSpacing(18);

    auto *btnPlay = createMenuButton(QStringLiteral("ИГРАТЬ"));
    auto *btnEditor = createMenuButton(QStringLiteral("РЕДАКТОР"));
    auto *btnSettings = createMenuButton(QStringLiteral("НАСТРОЙКИ"));
    auto *btnExit = createMenuButton(QStringLiteral("ВЫХОД"));

    // Danger accent for exit button hover
    btnExit->setStyleSheet(Theme::menuExitButtonStyle());

    connect(btnPlay, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit playRequested();
    });
    connect(btnEditor, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit editorRequested();
    });
    connect(btnSettings, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit settingsRequested();
    });
    connect(btnExit, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit exitRequested();
    });

    buttonCenterContainer->addWidget(btnPlay);
    buttonCenterContainer->addWidget(btnEditor);
    buttonCenterContainer->addWidget(btnSettings);
    buttonCenterContainer->addWidget(btnExit);

    mainLayout->addLayout(buttonCenterContainer);
    mainLayout->addStretch(1);

    // Bottom Footer (Player info & build version)
    auto *footerLayout = new QHBoxLayout();

    m_playerLabel = new QLabel(this);
    m_playerLabel->setStyleSheet(QStringLiteral(
        "color: #a09cb0;"
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 13px;"
    ));
    refreshProfileInfo();

    auto *versionLabel = new QLabel(QStringLiteral("BeatKey v0.2"), this);
    versionLabel->setStyleSheet(QStringLiteral(
        "color: #706a84;"
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 12px;"
    ));

    footerLayout->addWidget(m_playerLabel);
    footerLayout->addStretch(1);
    footerLayout->addWidget(versionLabel);

    mainLayout->addLayout(footerLayout);
}

void MainMenuWidget::refreshProfileInfo()
{
    if (m_playerLabel) {
        QString nick = SettingsManager::instance().nickname();
        m_playerLabel->setText(QStringLiteral("Игрок: <b style='color:#a259ff;'>%1</b>").arg(nick));
    }
}

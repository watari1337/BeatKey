#include "mainmenuwidget.h"
#include "settingsmanager.h"

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
    btn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(88, 38, 175, 210), stop:1 rgba(142, 68, 173, 210));"
        "  color: #ffffff;"
        "  border: 2px solid #a259ff;"
        "  border-radius: 12px;"
        "  font-family: 'Segoe UI', 'Arial', sans-serif;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  letter-spacing: 2px;"
        "  text-align: center;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(120, 55, 235, 240), stop:1 rgba(175, 85, 215, 240));"
        "  border: 2px solid #d580ff;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(65, 25, 135, 250), stop:1 rgba(110, 48, 140, 250));"
        "  border: 2px solid #a259ff;"
        "}"
    ));

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
    btnExit->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(80, 25, 45, 210), stop:1 rgba(120, 35, 65, 210));"
        "  color: #ffffff;"
        "  border: 2px solid #cc4466;"
        "  border-radius: 12px;"
        "  font-family: 'Segoe UI', 'Arial', sans-serif;"
        "  font-size: 18px;"
        "  font-weight: bold;"
        "  letter-spacing: 2px;"
        "  text-align: center;"
        "  padding: 8px 16px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(140, 40, 70, 240), stop:1 rgba(190, 50, 90, 240));"
        "  border: 2px solid #ff5577;"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 rgba(70, 20, 35, 250), stop:1 rgba(100, 30, 55, 250));"
        "  border: 2px solid #cc4466;"
        "}"
    ));

    connect(btnPlay, &QPushButton::clicked, this, &MainMenuWidget::playRequested);
    connect(btnEditor, &QPushButton::clicked, this, &MainMenuWidget::editorRequested);
    connect(btnSettings, &QPushButton::clicked, this, &MainMenuWidget::settingsRequested);
    connect(btnExit, &QPushButton::clicked, this, &MainMenuWidget::exitRequested);

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

    auto *versionLabel = new QLabel(QStringLiteral("BeatKey v0.1 • Qt 6.11"), this);
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
        m_playerLabel->setText(QStringLiteral("👤 Игрок: <b style='color:#a259ff;'>%1</b>").arg(nick));
    }
}

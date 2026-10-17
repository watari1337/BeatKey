#include "mainwindow.h"
#include "mainmenuwidget.h"
#include "settingswidget.h"
#include "exitdialog.h"
#include "settingsmanager.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QCloseEvent>
#include <QIcon>
#include <QFile>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_stackedWidget(new QStackedWidget(this))
    , m_mainMenuWidget(nullptr)
    , m_settingsWidget(nullptr)
{
    setWindowTitle(QStringLiteral("BeatKey - Rhythm Game & Beatmap Studio"));

    if (QFile::exists(QStringLiteral(":/assets/images/icon_bk.png"))) {
        setWindowIcon(QIcon(QStringLiteral(":/assets/images/icon_bk.png")));
    }

    resize(1280, 720);
    setMinimumSize(960, 540);

    setupUi();

    // Check initial fullscreen setting
    updateFullscreenMode(SettingsManager::instance().isFullscreen());
    connect(&SettingsManager::instance(), &SettingsManager::fullscreenChanged,
            this, &MainWindow::updateFullscreenMode);
}

void MainWindow::setupUi()
{
    setCentralWidget(m_stackedWidget);

    m_mainMenuWidget = new MainMenuWidget(this);
    m_settingsWidget = new SettingsWidget(this);

    QWidget *playPlaceholder = createPlaceholderScreen(
        QStringLiteral("Режим игры (Song Select)"),
        QStringLiteral("Модуль выбора трека, воспроизведения карты и ритм-геймплея.\n"
                       "Архитектурно изолирован для разработки вторым разработчиком.")
    );

    QWidget *editorPlaceholder = createPlaceholderScreen(
        QStringLiteral("Редактор карт (Beatmap Editor)"),
        QStringLiteral("Модуль интерактивной расстановки нот, квантования и сохранения карт (.bkey).\n"
                       "Архитектурно изолирован для разработки вторым разработчиком.")
    );

    m_stackedWidget->addWidget(m_mainMenuWidget);       // Index 0
    m_stackedWidget->addWidget(m_settingsWidget);       // Index 1
    m_stackedWidget->addWidget(playPlaceholder);        // Index 2
    m_stackedWidget->addWidget(editorPlaceholder);      // Index 3

    // Main menu navigation
    connect(m_mainMenuWidget, &MainMenuWidget::playRequested, this, [this]() {
        switchToScreen(Screen_PlayPlaceholder);
    });
    connect(m_mainMenuWidget, &MainMenuWidget::editorRequested, this, [this]() {
        switchToScreen(Screen_EditorPlaceholder);
    });
    connect(m_mainMenuWidget, &MainMenuWidget::settingsRequested, this, [this]() {
        m_settingsWidget->loadFromSettingsManager();
        switchToScreen(Screen_Settings);
    });
    connect(m_mainMenuWidget, &MainMenuWidget::exitRequested, this, &MainWindow::handleExitRequest);

    // Settings back navigation
    connect(m_settingsWidget, &SettingsWidget::backRequested, this, [this]() {
        switchToScreen(Screen_MainMenu);
    });

    m_stackedWidget->setCurrentIndex(Screen_MainMenu);
}

QWidget* MainWindow::createPlaceholderScreen(const QString &title, const QString &description)
{
    auto *widget = new QWidget(this);
    widget->setStyleSheet(QStringLiteral(
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #120c22, stop:1 #1c1034);"
    ));

    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    auto *backBtn = new QPushButton(QStringLiteral("← Главное меню"), widget);
    backBtn->setCursor(Qt::PointingHandCursor);
    backBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #2a1f42;"
        "  color: #bb9af7;"
        "  border: 1px solid #65439c;"
        "  border-radius: 8px;"
        "  padding: 8px 18px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  max-width: 180px;"
        "}"
        "QPushButton:hover { background: #3d2a61; border: 1px solid #a259ff; color: #ffffff; }"
    ));
    connect(backBtn, &QPushButton::clicked, this, [this]() {
        switchToScreen(Screen_MainMenu);
    });

    auto *titleLabel = new QLabel(title, widget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 28px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));

    auto *descLabel = new QLabel(description, widget);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 15px;"
        "color: #a09cb0;"
        "line-height: 1.5;"
    ));

    layout->addWidget(backBtn, 0, Qt::AlignLeft);
    layout->addStretch(1);
    layout->addWidget(titleLabel);
    layout->addWidget(descLabel);
    layout->addStretch(2);

    return widget;
}

void MainWindow::switchToScreen(ScreenIndex index)
{
    m_stackedWidget->setCurrentIndex(static_cast<int>(index));
}

void MainWindow::updateFullscreenMode(bool fullscreen)
{
    if (fullscreen) {
        showFullScreen();
    } else {
        showNormal();
    }
}

void MainWindow::handleExitRequest()
{
    close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    ExitDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // Graceful shutdown: persist any pending settings to settings.ini
        SettingsManager::instance().saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_stackedWidget->currentIndex() != Screen_MainMenu) {
            switchToScreen(Screen_MainMenu);
            event->accept();
            return;
        } else {
            handleExitRequest();
            event->accept();
            return;
        }
    }

    QMainWindow::keyPressEvent(event);
}

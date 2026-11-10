#include "mainwindow.h"
#include "mainmenuwidget.h"
#include "settingswidget.h"
#include "songselectwidget.h"
#include "editorhubwidget.h"
#include "beatmapeditorwidget.h"
#include "gameplaywidget.h"
#include "resultswidget.h"
#include "exitdialog.h"
#include "settingsmanager.h"
#include "audiomanager.h"
#include "theme.h"
#include "covermanager.h"

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
    , m_songSelectWidget(nullptr)
    , m_editorHubWidget(nullptr)
    , m_editorWidget(nullptr)
    , m_gameplayWidget(nullptr)
    , m_resultsWidget(nullptr)
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
    m_songSelectWidget = new SongSelectWidget(this);
    m_editorHubWidget = new EditorHubWidget(this);
    m_editorWidget = new BeatmapEditorWidget(this);
    m_gameplayWidget = new GameplayWidget(this);
    m_resultsWidget = new ResultsWidget(this);

    m_stackedWidget->addWidget(m_mainMenuWidget);       // Index 0: Screen_MainMenu
    m_stackedWidget->addWidget(m_settingsWidget);       // Index 1: Screen_Settings
    m_stackedWidget->addWidget(m_songSelectWidget);     // Index 2: Screen_SongSelect
    m_stackedWidget->addWidget(m_editorHubWidget);      // Index 3: Screen_EditorHub
    m_stackedWidget->addWidget(m_editorWidget);         // Index 4: Screen_Editor
    m_stackedWidget->addWidget(m_gameplayWidget);       // Index 5: Screen_Gameplay
    m_stackedWidget->addWidget(m_resultsWidget);        // Index 6: Screen_Results

    // Main menu navigation
    connect(m_mainMenuWidget, &MainMenuWidget::playRequested, this, [this]() {
        switchToScreen(Screen_SongSelect);
    });
    connect(m_mainMenuWidget, &MainMenuWidget::editorRequested, this, [this]() {
        switchToScreen(Screen_EditorHub);
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

    // Song select navigation
    connect(m_songSelectWidget, &SongSelectWidget::backRequested, this, [this]() {
        switchToScreen(Screen_MainMenu);
    });
    connect(m_songSelectWidget, &SongSelectWidget::playMapRequested, this, [this](const Beatmap &map) {
        if (map.notes.isEmpty() || map.totalNotes() == 0) {
            CoverManager::showMediaErrorDialog(
                this,
                QStringLiteral("Карта пуста"),
                QStringLiteral("В этой карте нет нот для игры.\nДобавьте ноты в редакторе карт перед началом игры.")
            );
            return;
        }
        m_gameplayWidget->startMap(map);
        switchToScreen(Screen_Gameplay);
    });

    // Gameplay navigation
    connect(m_gameplayWidget, &GameplayWidget::gameplayFinished, this, [this](const GameResult &result) {
        m_resultsWidget->displayResult(result);
        switchToScreen(Screen_Results);
    });
    connect(m_gameplayWidget, &GameplayWidget::exitRequested, this, [this]() {
        switchToScreen(Screen_SongSelect);
    });

    // Results navigation
    connect(m_resultsWidget, &ResultsWidget::retryRequested, this, [this](const Beatmap &map) {
        if (map.notes.isEmpty() || map.totalNotes() == 0) {
            CoverManager::showMediaErrorDialog(
                this,
                QStringLiteral("Карта пуста"),
                QStringLiteral("В этой карте нет нот для игры.\nДобавьте ноты в редакторе карт перед началом игры.")
            );
            switchToScreen(Screen_SongSelect);
            return;
        }
        m_gameplayWidget->startMap(map);
        switchToScreen(Screen_Gameplay);
    });
    connect(m_resultsWidget, &ResultsWidget::exitRequested, this, [this]() {
        switchToScreen(Screen_SongSelect);
    });




    // Editor hub navigation
    connect(m_editorHubWidget, &EditorHubWidget::backRequested, this, [this]() {
        switchToScreen(Screen_MainMenu);
    });
    connect(m_editorHubWidget, &EditorHubWidget::createMapRequested, this, [this](const Beatmap &map, const QString &audioPath, const QString &coverPath) {
        m_editorWidget->loadBeatmap(map, audioPath, coverPath);
        switchToScreen(Screen_Editor);
    });
    connect(m_editorHubWidget, &EditorHubWidget::editMapRequested, this, [this](const Beatmap &map) {
        qDebug() << "[MainWindow] editMapRequested for:" << map.meta.title << "audioPath:" << map.extractedAudioPath;
        m_editorWidget->loadBeatmap(map, map.extractedAudioPath, QString());
        qDebug() << "[MainWindow] loadBeatmap completed, switching screen";
        switchToScreen(Screen_Editor);
        qDebug() << "[MainWindow] switchToScreen completed";
    });

    // Editor canvas navigation
    connect(m_editorWidget, &BeatmapEditorWidget::backRequested, this, [this]() {
        m_editorWidget->stopPlayback();
        switchToScreen(Screen_EditorHub);
        m_editorHubWidget->refreshMapList();
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
    backBtn->setStyleSheet(Theme::secondaryButtonStyle());
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
    if (index == Screen_Editor || index == Screen_Gameplay) {
        AudioManager::instance().stopMusic();
    } else if (m_stackedWidget->currentIndex() == Screen_Editor) {
        m_editorWidget->stopPlayback();
        if (index == Screen_MainMenu) {
            AudioManager::instance().playMenuMusic();
        }
    } else if (m_stackedWidget->currentIndex() == Screen_Gameplay) {
        m_gameplayWidget->stopGame();
        if (index == Screen_SongSelect) {
            m_songSelectWidget->refreshSongList();
        } else if (index == Screen_MainMenu) {
            AudioManager::instance().playMenuMusic();
        }
    } else if (m_stackedWidget->currentIndex() == Screen_Results) {
        if (index == Screen_SongSelect) {
            m_songSelectWidget->refreshSongList();
        } else if (index == Screen_MainMenu) {
            AudioManager::instance().playMenuMusic();
        }
    } else if (index == Screen_MainMenu) {
        AudioManager::instance().playMenuMusic();
    }

    m_stackedWidget->setCurrentIndex(static_cast<int>(index));

    if (index == Screen_Editor) {
        m_editorWidget->setFocus();
        m_editorWidget->update();
    } else if (index == Screen_Gameplay) {
        m_gameplayWidget->setFocus();
        m_gameplayWidget->update();
    } else if (index == Screen_Results) {
        m_resultsWidget->setFocus();
        m_resultsWidget->update();
    }
}




















void MainWindow::updateFullscreenMode(bool fullscreen)
{
    if (fullscreen) {
        showFullScreen();
    } else {
        showNormal();
        resize(1280, 720);
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
        SettingsManager::instance().saveSettings();
        if (m_editorWidget) {
            m_editorWidget->stopPlayback();
        }
        AudioManager::instance().cleanup();
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_stackedWidget->currentIndex() == Screen_Gameplay) {
            // GameplayWidget обрабатывает Escape самостоятельно для вызова паузы
            QMainWindow::keyPressEvent(event);
            return;
        }

        if (m_stackedWidget->currentIndex() == Screen_Results) {
            switchToScreen(Screen_SongSelect);
            event->accept();
            return;
        }
        if (m_stackedWidget->currentIndex() == Screen_Editor) {
            m_editorWidget->maybeSaveAndExit();
            event->accept();
            return;
        }

        if (m_stackedWidget->currentIndex() != Screen_MainMenu) {
            if (m_stackedWidget->currentIndex() == Screen_SongSelect ||
                m_stackedWidget->currentIndex() == Screen_EditorHub) {
                AudioManager::instance().playMenuMusic();
            }
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

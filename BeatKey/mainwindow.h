#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>

class MainMenuWidget;
class SettingsWidget;
class SongSelectWidget;

/**
 * @brief MainWindow acts as the central router and coordinator between application screens.
 *
 * Utilizes QStackedWidget to decouple individual screens, allowing independent
 * development by multiple programmers without git merge conflicts.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum ScreenIndex {
        Screen_MainMenu = 0,
        Screen_Settings = 1,
        Screen_SongSelect = 2,
        Screen_PlayPlaceholder = 2,
        Screen_EditorPlaceholder = 3
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void switchToScreen(ScreenIndex index);
    void handleExitRequest();
    void updateFullscreenMode(bool fullscreen);

private:
    void setupUi();
    QWidget* createPlaceholderScreen(const QString &title, const QString &description);

    QStackedWidget *m_stackedWidget;
    MainMenuWidget *m_mainMenuWidget;
    SettingsWidget *m_settingsWidget;
    SongSelectWidget *m_songSelectWidget;
};

#endif // MAINWINDOW_H

#ifndef MAINMENUWIDGET_H
#define MAINMENUWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>

/**
 * @brief MainMenuWidget is the primary navigation hub of BeatKey.
 *
 * Implements the 4 core menu options defined in the TZ:
 * 1. Играть (Play / Song Selection)
 * 2. Редактор (Beatmap Editor)
 * 3. Настройки (Settings)
 * 4. Выход (Exit game)
 */
class MainMenuWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuWidget(QWidget *parent = nullptr);
    ~MainMenuWidget() override = default;

public slots:
    void refreshProfileInfo();

signals:
    void playRequested();
    void editorRequested();
    void settingsRequested();
    void exitRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    QPushButton* createMenuButton(const QString &text, const QString &iconName = QString());

    QLabel *m_playerLabel;
    QPixmap m_backgroundPixmap;
};

#endif // MAINMENUWIDGET_H

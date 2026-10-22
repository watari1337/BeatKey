#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>

/**
 * @brief KeyBindButton is an interactive button that captures key presses for binding controls.
 */
class KeyBindButton : public QPushButton
{
    Q_OBJECT

public:
    explicit KeyBindButton(Qt::Key initialKey, QWidget *parent = nullptr);

    Qt::Key currentKey() const;
    void setCurrentKey(Qt::Key key);

signals:
    void keyChanged(Qt::Key newKey);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void updateButtonText();

    Qt::Key m_key;
    bool m_isListening;
};

/**
 * @brief SettingsWidget displays and configures all game settings according to TZ 4.5 & 4.11.
 *
 * Interacts directly with SettingsManager to persist options to settings.ini.
 */
class SettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsWidget(QWidget *parent = nullptr);
    ~SettingsWidget() override = default;

public slots:
    void loadFromSettingsManager();
    void resetDefaults();
    void startAutoCalibration();

signals:
    void backRequested();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();
    void saveCurrentSettings();
    QWidget* createSection(const QString &titleText, QWidget *contentWidget);
    QSlider* createVolumeSlider(int initialVal, QLabel *valLabel);

    // Form controls
    QLineEdit *m_nickEdit;

    QSlider *m_masterSlider;
    QLabel *m_masterValLabel;

    QSlider *m_musicSlider;
    QLabel *m_musicValLabel;

    QSlider *m_sfxSlider;
    QLabel *m_sfxValLabel;

    KeyBindButton *m_primaryKeyBtn;
    KeyBindButton *m_secondaryKeyBtn;

    QSpinBox *m_offsetSpinBox;
    QPushButton *m_autoCalibBtn;

    QComboBox *m_screenModeCombo;

    QPushButton *m_resetBtn;
    QPushButton *m_backBtn;
};

#endif // SETTINGSWIDGET_H

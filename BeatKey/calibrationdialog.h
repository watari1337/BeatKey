#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QAudioOutput>

class CalibrationSquareWidget;

/**
 * @brief CalibrationDialog provides interactive audio latency (Audio Offset) calibration.
 * Features an animated central square flashing in sync with a procedurally generated 8-bit rhythm track,
 * fine-tuning arrow buttons (+-5ms, +-1ms), and clean save/exit confirmation.
 */
class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(QWidget *parent = nullptr);
    ~CalibrationDialog() override;

    int calibratedOffsetMs() const { return m_currentOffsetMs; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onTick();
    void adjustOffset(int deltaMs);
    void handleSaveAndExit();
    void handleCancelOrClose();

private:
    void setupUi();
    static QString generate8BitBeatFile();

    int m_initialOffsetMs = 0;
    int m_currentOffsetMs = 0;

    // Аудиоплеер и таймер синхронизации
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QTimer *m_syncTimer = nullptr;
    QElapsedTimer m_audioClock;
    qint64 m_lastAudioPos = 0;

    // Визуальное состояние
    qreal m_flashRatio = 0.0;
    int m_currentBeatIdx = 0;

    // UI Виджеты
    CalibrationSquareWidget *m_squareWidget = nullptr;
    QLabel *m_offsetValueLabel = nullptr;
    QPushButton *m_saveBtn = nullptr;
    QPushButton *m_cancelBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;
};

#endif // CALIBRATIONDIALOG_H

#ifndef NEWBEATMAPDIALOG_H
#define NEWBEATMAPDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QPoint>

#include "beatmap.h"

/**
 * @brief NewBeatmapDialog provides a modal wizard for creating a new rhythm chart.
 *
 * Implements draggable frameless design, mandatory audio/title/mapper fields,
 * collapsible advanced settings, Russian difficulty levels, and error field highlighting.
 */
class NewBeatmapDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewBeatmapDialog(QWidget *parent = nullptr);
    ~NewBeatmapDialog() override = default;

    Beatmap createdBeatmap() const;
    QString sourceAudioPath() const;
    QString sourceCoverPath() const;

    void accept() override;

protected:
    void showEvent(QShowEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void handleChooseAudio();
    void handleChooseCover();
    void toggleAdvancedSettings();
    void validateAndAccept();

private:
    void setupUi();
    void centerOnParent();
    bool validateForm();
    QLabel *createFieldLabel(const QString &text, QWidget *parent);

    // Draggable window state
    bool m_isDragging;
    QPoint m_dragPosition;

    // Mandatory fields
    QPushButton *m_chooseAudioBtn;
    QLabel *m_audioPathLabel;
    QLineEdit *m_titleEdit;
    QLineEdit *m_mapperEdit;

    // Advanced collapsible fields
    QPushButton *m_toggleAdvancedBtn;
    QWidget *m_advancedContainer;
    QLineEdit *m_artistEdit;
    QComboBox *m_difficultyCombo;
    QDoubleSpinBox *m_bpmSpin;
    QSpinBox *m_offsetSpin;
    QPushButton *m_chooseCoverBtn;
    QLabel *m_coverPathLabel;

    // Dialog action button
    QPushButton *m_createBtn;

    QString m_selectedAudioPath;
    QString m_selectedCoverPath;
    bool m_isAdvancedVisible;
};

#endif // NEWBEATMAPDIALOG_H

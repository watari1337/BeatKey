#ifndef EDITORHELPDIALOG_H
#define EDITORHELPDIALOG_H

#include <QDialog>
#include <QPushButton>

/**
 * @brief EditorHelpDialog displays complete documentation on hotkeys, tools,
 * mouse gestures, tap recording, and quantization grid usage in the BeatKey editor.
 */
class EditorHelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditorHelpDialog(QWidget *parent = nullptr);
    ~EditorHelpDialog() override = default;

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();
};

#endif // EDITORHELPDIALOG_H

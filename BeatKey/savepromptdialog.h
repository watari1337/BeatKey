#ifndef SAVEPROMPTDIALOG_H
#define SAVEPROMPTDIALOG_H

#include <QDialog>

/**
 * @brief SavePromptDialog asks the user whether to save unsaved beatmap changes before exiting.
 */
class SavePromptDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Result {
        Save,
        Discard,
        Cancel
    };

    explicit SavePromptDialog(QWidget *parent = nullptr);
    ~SavePromptDialog() override = default;

    Result promptResult() const { return m_result; }

protected:
    void showEvent(QShowEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();

    Result m_result;
};

#endif // SAVEPROMPTDIALOG_H

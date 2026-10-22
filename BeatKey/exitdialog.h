#ifndef EXITDIALOG_H
#define EXITDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QShowEvent>

/**
 * @brief ExitDialog provides a styled confirmation before quitting the game.
 */
class ExitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExitDialog(QWidget *parent = nullptr);
    ~ExitDialog() override = default;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupUi();
};

#endif // EXITDIALOG_H
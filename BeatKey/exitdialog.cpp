#include "exitdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

ExitDialog::ExitDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setupUi();
}

void ExitDialog::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(QStringLiteral(
        "QWidget#dialogContainer {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1a162b, stop:1 #271a3d);"
        "  border: 2px solid #7d44cf;"
        "  border-radius: 16px;"
        "}"
        "QLabel {"
        "  color: #ffffff;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5c2bb8, stop:1 #8e44ad);"
        "  color: #ffffff;"
        "  border: 1px solid #a259ff;"
        "  border-radius: 8px;"
        "  padding: 10px 24px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7539e6, stop:1 #a259ff);"
        "}"
        "QPushButton#cancelButton {"
        "  background: #2f2545;"
        "  border: 1px solid #554477;"
        "}"
        "QPushButton#cancelButton:hover {"
        "  background: #3e315b;"
        "  border: 1px solid #776699;"
        "}"
    ));

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(32, 28, 32, 28);
    innerLayout->setSpacing(16);

    auto *titleLabel = new QLabel(QStringLiteral("Выход из игры"), container);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #ff5577;"));
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *messageLabel = new QLabel(QStringLiteral("Вы действительно хотите выйти из BeatKey?"), container);
    messageLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #dddddd;"));
    messageLabel->setAlignment(Qt::AlignCenter);

    auto *subLabel = new QLabel(QStringLiteral("Все настройки будут сохранены автоматически."), container);
    subLabel->setStyleSheet(QStringLiteral("font-size: 12px; color: #8888aa;"));
    subLabel->setAlignment(Qt::AlignCenter);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(16);

    auto *cancelBtn = new QPushButton(QStringLiteral("Отмена"), container);
    cancelBtn->setObjectName(QStringLiteral("cancelButton"));
    cancelBtn->setCursor(Qt::PointingHandCursor);

    auto *exitBtn = new QPushButton(QStringLiteral("Выйти"), container);
    exitBtn->setCursor(Qt::PointingHandCursor);
    exitBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #b82b4a, stop:1 #e03a60);"
        "  border: 1px solid #ff5577;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #d9385c, stop:1 #ff5577);"
        "}"
    ));

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(exitBtn, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(exitBtn);

    innerLayout->addWidget(titleLabel);
    innerLayout->addWidget(messageLabel);
    innerLayout->addWidget(subLabel);
    innerLayout->addSpacing(8);
    innerLayout->addLayout(buttonLayout);

    outerLayout->addWidget(container);
}

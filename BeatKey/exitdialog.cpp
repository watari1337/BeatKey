#include "exitdialog.h"
#include "theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QLibrary>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ExitDialog::ExitDialog(QWidget *parent)
    : QDialog(parent)
{
    // Qt::NoDropShadowWindowHint отключает нативную тень и рамку окна Windows DWM
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setupUi();
}

void ExitDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    // Центрируем диалог относительно родительского окна
    if (parentWidget()) {
        QRect parentGeo = parentWidget()->geometry();
        move(parentGeo.center() - rect().center());
    }

#ifdef Q_OS_WIN
    // В Windows 11 DWM принудительно рисует системный 1-пиксельный ободок вокруг окон.
    // Подавляем его программно через DwmSetWindowAttribute без жесткой привязки к библиотекам.
    HWND hwnd = reinterpret_cast<HWND>(winId());
    typedef HRESULT (WINAPI *DwmSetWindowAttributeFunc)(HWND, DWORD, LPCVOID, DWORD);
    auto pDwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
        QLibrary::resolve(QStringLiteral("dwmapi"), "DwmSetWindowAttribute")
    );
    if (pDwmSetWindowAttribute) {
        // DWMWA_BORDER_COLOR = 34, DWMWA_COLOR_NONE = 0xFFFFFFFE
        COLORREF borderColor = 0xFFFFFFFE;
        pDwmSetWindowAttribute(hwnd, 34, &borderColor, sizeof(borderColor));

        // DWMWA_WINDOW_CORNER_PREFERENCE = 33, DWMWCP_DONOTROUND = 1
        DWORD cornerPref = 1;
        pDwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref));
    }
#endif
}

void ExitDialog::setupUi()
{
    auto *outerLayout = new QVBoxLayout(this);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(this);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(32, 28, 32, 28);
    innerLayout->setSpacing(20);

    auto *titleLabel = new QLabel(QStringLiteral("Выход из игры"), container);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 20px; font-weight: bold; color: #ff5577;"));
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *messageLabel = new QLabel(QStringLiteral("Вы действительно хотите выйти из BeatKey?"), container);
    messageLabel->setStyleSheet(QStringLiteral("font-size: 15px; color: #dddddd;"));
    messageLabel->setAlignment(Qt::AlignCenter);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(16);

    auto *cancelBtn = new QPushButton(QStringLiteral("Отмена"), container);
    cancelBtn->setObjectName(QStringLiteral("cancelButton"));
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(Theme::secondaryButtonStyle());

    auto *exitBtn = new QPushButton(QStringLiteral("Выйти"), container);
    exitBtn->setCursor(Qt::PointingHandCursor);
    exitBtn->setStyleSheet(Theme::dangerButtonStyle());

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(exitBtn, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(cancelBtn);
    buttonLayout->addWidget(exitBtn);

    innerLayout->addWidget(titleLabel);
    innerLayout->addWidget(messageLabel);
    innerLayout->addSpacing(4);
    innerLayout->addLayout(buttonLayout);

    outerLayout->addWidget(container);
}

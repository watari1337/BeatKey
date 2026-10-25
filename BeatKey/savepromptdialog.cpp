#include "savepromptdialog.h"
#include "theme.h"
#include "audiomanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QLibrary>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SavePromptDialog::SavePromptDialog(QWidget *parent)
    : QDialog(parent)
    , m_result(Result::Cancel)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
    setupUi();
}

void SavePromptDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (parentWidget()) {
        QRect parentGeo = parentWidget()->geometry();
        move(parentGeo.center() - rect().center());
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());
    typedef HRESULT (WINAPI *DwmSetWindowAttributeFunc)(HWND, DWORD, LPCVOID, DWORD);
    auto pDwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
        QLibrary::resolve(QStringLiteral("dwmapi"), "DwmSetWindowAttribute")
    );
    if (pDwmSetWindowAttribute) {
        COLORREF borderColor = 0xFFFFFFFE;
        pDwmSetWindowAttribute(hwnd, 34, &borderColor, sizeof(borderColor));

        DWORD cornerPref = 1;
        pDwmSetWindowAttribute(hwnd, 33, &cornerPref, sizeof(cornerPref));
    }
#endif
}

void SavePromptDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        AudioManager::instance().playClickSound();
        m_result = Result::Cancel;
        reject();
        return;
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        AudioManager::instance().playClickSound();
        m_result = Result::Save;
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void SavePromptDialog::setupUi()
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

    auto *titleLabel = new QLabel(QStringLiteral("Несохраненные изменения"), container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 20px;"
        "font-weight: bold;"
        "color: #ffffff;"
        "background: transparent;"
        "border: none;"
    ));
    innerLayout->addWidget(titleLabel);

    auto *descLabel = new QLabel(
        QStringLiteral("В карте есть несохраненные изменения.\nСохранить перед выходом в хаб редактора?"),
        container
    );
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 14px;"
        "color: #a09cb0;"
        "line-height: 1.4;"
        "background: transparent;"
        "border: none;"
    ));
    innerLayout->addWidget(descLabel);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setSpacing(16);

    auto *saveBtn = new QPushButton(QStringLiteral("Сохранить"), container);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(Theme::primaryButtonStyle());
    saveBtn->setFixedHeight(40);
    saveBtn->setMinimumWidth(120);
    connect(saveBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        m_result = Result::Save;
        accept();
    });
    buttonsLayout->addWidget(saveBtn);

    auto *discardBtn = new QPushButton(QStringLiteral("Не сохранять"), container);
    discardBtn->setCursor(Qt::PointingHandCursor);
    discardBtn->setStyleSheet(Theme::dangerButtonStyle());
    discardBtn->setFixedHeight(40);
    discardBtn->setMinimumWidth(120);
    connect(discardBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        m_result = Result::Discard;
        accept();
    });
    buttonsLayout->addWidget(discardBtn);

    innerLayout->addLayout(buttonsLayout);
    outerLayout->addWidget(container);
}

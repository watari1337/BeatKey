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
    innerLayout->setContentsMargins(24, 20, 24, 24);
    innerLayout->setSpacing(18);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *spacerLeft = new QWidget(container);
    spacerLeft->setFixedSize(28, 28);
    headerLayout->addWidget(spacerLeft);

    auto *titleLabel = new QLabel(QStringLiteral("Несохраненные изменения"), container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        m_result = Result::Cancel;
        reject();
    });
    headerLayout->addWidget(closeBtn);
    innerLayout->addLayout(headerLayout);

    auto *descLabel = new QLabel(
        QStringLiteral("В карте есть несохраненные изменения.\nСохранить перед выходом в хаб редактора?"),
        container
    );
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(descLabel);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->setAlignment(Qt::AlignCenter);
    buttonsLayout->setSpacing(16);

    auto *saveBtn = new QPushButton(QStringLiteral("Сохранить"), container);
    saveBtn->setCursor(Qt::PointingHandCursor);
    saveBtn->setStyleSheet(Theme::primaryButtonStyle());
    saveBtn->setFixedHeight(40);
    saveBtn->setMinimumWidth(130);
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
    discardBtn->setMinimumWidth(130);
    connect(discardBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        m_result = Result::Discard;
        accept();
    });
    buttonsLayout->addWidget(discardBtn);

    innerLayout->addLayout(buttonsLayout);
    outerLayout->addWidget(container);
}

#include "settingswidget.h"
#include "settingsmanager.h"
#include "theme.h"
#include "audiomanager.h"
#include "calibrationdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QPainter>

// ============================================================================
// KeyBindButton Implementation
// ============================================================================

KeyBindButton::KeyBindButton(Qt::Key initialKey, QWidget *parent)
    : QPushButton(parent)
    , m_key(initialKey)
    , m_isListening(false)
{
    setMinimumSize(140, 40);
    setCursor(Qt::PointingHandCursor);
    updateButtonText();

    connect(this, &QPushButton::clicked, this, [this]() {
        m_isListening = true;
        setText(QStringLiteral("[ Нажмите клавишу... ]"));
        setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: #ffaa00;"
            "  color: #111111;"
            "  font-weight: bold;"
            "  border: 2px solid #ffffff;"
            "  border-radius: 8px;"
            "  font-size: 14px;"
            "}"
        ));
    });
}

Qt::Key KeyBindButton::currentKey() const
{
    return m_key;
}

void KeyBindButton::setCurrentKey(Qt::Key key)
{
    m_key = key;
    m_isListening = false;
    updateButtonText();
}

void KeyBindButton::keyPressEvent(QKeyEvent *event)
{
    if (!m_isListening) {
        QPushButton::keyPressEvent(event);
        return;
    }

    int k = event->key();
    if (k == Qt::Key_Escape) {
        // Cancel binding on Escape
        m_isListening = false;
        updateButtonText();
        return;
    }

    if (k == Qt::Key_Shift || k == Qt::Key_Control || k == Qt::Key_Alt || k == Qt::Key_Meta) {
        // Ignore single modifier keys
        return;
    }

    m_key = SettingsManager::normalizeKey(event);
    m_isListening = false;
    updateButtonText();
    emit keyChanged(m_key);
}

void KeyBindButton::focusOutEvent(QFocusEvent *event)
{
    if (m_isListening) {
        m_isListening = false;
        updateButtonText();
    }
    QPushButton::focusOutEvent(event);
}

void KeyBindButton::updateButtonText()
{
    setText(QStringLiteral("Клавиша: %1").arg(SettingsManager::keyToString(m_key)));
    setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #3a2b56;"
        "  color: #ffffff;"
        "  font-weight: bold;"
        "  border: 1px solid #7d44cf;"
        "  border-radius: 8px;"
        "  font-size: 14px;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton:hover {"
        "  background: #503975;"
        "  border: 1px solid #a259ff;"
        "}"
    ));
}

// ============================================================================
// SettingsWidget Implementation
// ============================================================================

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
    , m_nickEdit(nullptr)
    , m_masterSlider(nullptr)
    , m_masterValLabel(nullptr)
    , m_musicSlider(nullptr)
    , m_musicValLabel(nullptr)
    , m_sfxSlider(nullptr)
    , m_sfxValLabel(nullptr)
    , m_hitsoundSlider(nullptr)
    , m_hitsoundValLabel(nullptr)
    , m_primaryKeyBtn(nullptr)
    , m_secondaryKeyBtn(nullptr)
    , m_offsetSpinBox(nullptr)
    , m_autoCalibBtn(nullptr)
    , m_screenModeCombo(nullptr)
    , m_bgDimSlider(nullptr)
    , m_bgDimValLabel(nullptr)
    , m_bgBlurSlider(nullptr)
    , m_bgBlurValLabel(nullptr)
    , m_resetBtn(nullptr)
    , m_resetRecordsBtn(nullptr)
    , m_backBtn(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setupUi();
    loadFromSettingsManager();
}

void SettingsWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    Theme::paintWindowBackground(&painter, rect());
}

QWidget* SettingsWidget::createSection(const QString &titleText, QWidget *contentWidget)
{
    auto *box = new QWidget(this);
    box->setStyleSheet(Theme::sectionBoxStyle());

    auto *layout = new QVBoxLayout(box);
    layout->setContentsMargins(20, 16, 20, 16);
    layout->setSpacing(12);

    auto *title = new QLabel(titleText, box);
    title->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 16px;"
        "font-weight: bold;"
        "color: #bb9af7;"
        "border: none;"
        "background: transparent;"
    ));

    layout->addWidget(title);
    layout->addWidget(contentWidget);

    return box;
}

QSlider* SettingsWidget::createVolumeSlider(int initialVal, QLabel *valLabel)
{
    auto *slider = new ClickableSlider(Qt::Horizontal, this);
    slider->setRange(0, 100);
    slider->setValue(initialVal);
    slider->setStyleSheet(Theme::sliderStyle());

    valLabel->setText(QString::number(initialVal) + QStringLiteral("%"));
    valLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-weight: bold; min-width: 45px; border:none; background:transparent;"));

    connect(slider, &QSlider::valueChanged, this, [valLabel](int v) {
        valLabel->setText(QString::number(v) + QStringLiteral("%"));
    });

    return slider;
}

void SettingsWidget::setupUi()
{
    // Global dark theme background styling
    setStyleSheet(QStringLiteral(
        "SettingsWidget {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #120c22, stop:1 #1c1034);"
        "}"
        "QLabel {"
        "  color: #dddddd;"
        "  font-family: 'Segoe UI', sans-serif;"
        "}"
    ));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(20);

    // Title & Back Button Header
    auto *headerLayout = new QHBoxLayout();

    m_backBtn = new QPushButton(QStringLiteral("← Главное меню"), this);
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->setStyleSheet(Theme::secondaryButtonStyle());
    connect(m_backBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        emit backRequested();
    });

    auto *titleLabel = new QLabel(QStringLiteral("НАСТРОЙКИ"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 28px;"
        "font-weight: bold;"
        "letter-spacing: 3px;"
        "color: #ffffff;"
    ));

    headerLayout->addWidget(m_backBtn);
    headerLayout->addSpacing(20);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    mainLayout->addLayout(headerLayout);

    // Scrollable area for settings cards
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->viewport()->setAutoFillBackground(false);
    scrollArea->viewport()->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    scrollArea->setStyleSheet(Theme::scrollAreaStyle());

    auto *contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(18);

    // 1. Player Profile Section
    {
        auto *playerContent = new QWidget(contentWidget);
        playerContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *layout = new QHBoxLayout(playerContent);
        layout->setContentsMargins(0, 0, 0, 0);

        auto *label = new QLabel(QStringLiteral("Имя игрока (никнейм):"), playerContent);
        label->setStyleSheet(QStringLiteral("font-size: 14px;"));

        m_nickEdit = new QLineEdit(playerContent);
        m_nickEdit->setMaxLength(20);
        m_nickEdit->setPlaceholderText(QStringLiteral("Введите ник (3-20 символов)"));
        m_nickEdit->setStyleSheet(Theme::lineEditStyle());
        connect(m_nickEdit, &QLineEdit::editingFinished, this, [this]() {
            QString nick = m_nickEdit->text().trimmed();
            if (nick.length() >= 3) {
                SettingsManager::instance().setNickname(nick);
                SettingsManager::instance().saveSettings();
            }
        });

        layout->addWidget(label);
        layout->addWidget(m_nickEdit, 1);
        contentLayout->addWidget(createSection(QStringLiteral("Профиль игрока"), playerContent));
    }

    // 2. Audio Section
    {
        auto *audioContent = new QWidget(contentWidget);
        audioContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *grid = new QGridLayout(audioContent);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(12);

        const auto &mgr = SettingsManager::instance();

        m_masterValLabel = new QLabel(audioContent);
        m_masterSlider = createVolumeSlider(mgr.masterVolume(), m_masterValLabel);
        connect(m_masterSlider, &QSlider::valueChanged, this, [](int v) {
            SettingsManager::instance().setMasterVolume(v);
            SettingsManager::instance().saveSettings();
        });

        m_musicValLabel = new QLabel(audioContent);
        m_musicSlider = createVolumeSlider(mgr.musicVolume(), m_musicValLabel);
        connect(m_musicSlider, &QSlider::valueChanged, this, [](int v) {
            SettingsManager::instance().setMusicVolume(v);
            SettingsManager::instance().saveSettings();
        });

        m_sfxValLabel = new QLabel(audioContent);
        m_sfxSlider = createVolumeSlider(mgr.sfxVolume(), m_sfxValLabel);
        connect(m_sfxSlider, &QSlider::valueChanged, this, [](int v) {
            SettingsManager::instance().setSfxVolume(v);
            SettingsManager::instance().saveSettings();
        });

        grid->addWidget(new QLabel(QStringLiteral("Общая громкость:"), audioContent), 0, 0);
        grid->addWidget(m_masterSlider, 0, 1);
        grid->addWidget(m_masterValLabel, 0, 2);

        grid->addWidget(new QLabel(QStringLiteral("Громкость музыки:"), audioContent), 1, 0);
        grid->addWidget(m_musicSlider, 1, 1);
        grid->addWidget(m_musicValLabel, 1, 2);

        grid->addWidget(new QLabel(QStringLiteral("Эффекты (Hit / Miss):"), audioContent), 2, 0);
        grid->addWidget(m_sfxSlider, 2, 1);
        grid->addWidget(m_sfxValLabel, 2, 2);

        contentLayout->addWidget(createSection(QStringLiteral("Звуковые параметры"), audioContent));
    }

    // 3. Controls Section
    {
        auto *controlsContent = new QWidget(contentWidget);
        controlsContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *layout = new QHBoxLayout(controlsContent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(25);

        auto *primBox = new QVBoxLayout();
        primBox->addWidget(new QLabel(QStringLiteral("Основная клавиша удара:"), controlsContent));
        m_primaryKeyBtn = new KeyBindButton(SettingsManager::DEFAULT_PRIMARY_KEY, controlsContent);
        connect(m_primaryKeyBtn, &KeyBindButton::keyChanged, this, [](Qt::Key k) {
            SettingsManager::instance().setPrimaryKey(k);
            SettingsManager::instance().saveSettings();
        });
        primBox->addWidget(m_primaryKeyBtn);

        auto *secBox = new QVBoxLayout();
        secBox->addWidget(new QLabel(QStringLiteral("Дополнительная клавиша:"), controlsContent));
        m_secondaryKeyBtn = new KeyBindButton(SettingsManager::DEFAULT_SECONDARY_KEY, controlsContent);
        connect(m_secondaryKeyBtn, &KeyBindButton::keyChanged, this, [](Qt::Key k) {
            SettingsManager::instance().setSecondaryKey(k);
            SettingsManager::instance().saveSettings();
        });
        secBox->addWidget(m_secondaryKeyBtn);

        layout->addLayout(primBox);
        layout->addLayout(secBox);
        layout->addStretch(1);

        contentLayout->addWidget(createSection(QStringLiteral("Управление ритм-игрой"), controlsContent));
    }

    // 4. Calibration Section
    {
        auto *calibContent = new QWidget(contentWidget);
        calibContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *vbox = new QVBoxLayout(calibContent);
        vbox->setContentsMargins(0, 0, 0, 0);
        vbox->setSpacing(12);

        auto *hbox = new QHBoxLayout();
        auto *desc = new QLabel(QStringLiteral("Смещение задержки (Audio Offset):"), calibContent);

        m_offsetSpinBox = new QSpinBox(calibContent);
        m_offsetSpinBox->setRange(SettingsManager::MIN_AUDIO_OFFSET_MS, SettingsManager::MAX_AUDIO_OFFSET_MS);
        m_offsetSpinBox->setSuffix(QStringLiteral(" мс"));
        m_offsetSpinBox->setSingleStep(5);
        m_offsetSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
        m_offsetSpinBox->setAlignment(Qt::AlignCenter);
        m_offsetSpinBox->setStyleSheet(Theme::spinBoxStyle());
        connect(m_offsetSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [](int v) {
            SettingsManager::instance().setAudioOffsetMs(v);
            SettingsManager::instance().saveSettings();
        });

        auto *minus10Btn = new QPushButton(QStringLiteral("-10 мс"), calibContent);
        auto *plus10Btn = new QPushButton(QStringLiteral("+10 мс"), calibContent);
        QString stepBtnStyle = QStringLiteral(
            "QPushButton {"
            "  background: #332350;"
            "  color: #ffffff;"
            "  border: 1px solid #6b40b0;"
            "  border-radius: 6px;"
            "  padding: 6px 12px;"
            "}"
            "QPushButton:hover { background: #4d2f80; }"
        );
        minus10Btn->setStyleSheet(stepBtnStyle);
        plus10Btn->setStyleSheet(stepBtnStyle);

        connect(minus10Btn, &QPushButton::clicked, this, [this]() {
            AudioManager::instance().playClickSound();
            m_offsetSpinBox->setValue(m_offsetSpinBox->value() - 10);
        });
        connect(plus10Btn, &QPushButton::clicked, this, [this]() {
            AudioManager::instance().playClickSound();
            m_offsetSpinBox->setValue(m_offsetSpinBox->value() + 10);
        });

        hbox->addWidget(desc);
        hbox->addWidget(minus10Btn);
        hbox->addWidget(m_offsetSpinBox);
        hbox->addWidget(plus10Btn);
        hbox->addStretch(1);

        m_autoCalibBtn = new QPushButton(QStringLiteral("Запустить автокалибровку задержки (10 ударов)"), calibContent);
        m_autoCalibBtn->setCursor(Qt::PointingHandCursor);
        m_autoCalibBtn->setStyleSheet(Theme::primaryButtonStyle());
        connect(m_autoCalibBtn, &QPushButton::clicked, this, &SettingsWidget::startAutoCalibration);

        vbox->addLayout(hbox);
        vbox->addWidget(m_autoCalibBtn);

        contentLayout->addWidget(createSection(QStringLiteral("Синхронизация и калибровка"), calibContent));
    }

    // 5. Video Section (2 режима: оконный и полноэкранный)
    {
        auto *videoContent = new QWidget(contentWidget);
        videoContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *layout = new QHBoxLayout(videoContent);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(20);

        auto *label = new QLabel(QStringLiteral("Режим отображения:"), videoContent);
        label->setStyleSheet(QStringLiteral("font-size: 14px;"));

        m_screenModeCombo = new QComboBox(videoContent);
        m_screenModeCombo->setStyleSheet(Theme::comboBoxStyle());
        m_screenModeCombo->addItem(QStringLiteral("Оконный режим"), false);
        m_screenModeCombo->addItem(QStringLiteral("Полноэкранный режим"), true);

        // При выборе режима сразу переключаем экран и сохраняем настройку
        connect(m_screenModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int index) {
            bool isFull = m_screenModeCombo->itemData(index).toBool();
            SettingsManager::instance().setFullscreen(isFull);
            SettingsManager::instance().saveSettings();
        });

        layout->addWidget(label);
        layout->addWidget(m_screenModeCombo);
        layout->addStretch(1);

        contentLayout->addWidget(createSection(QStringLiteral("Отображение экрана"), videoContent));
    }

    // 6. Background Display Section
    {
        auto *bgContent = new QWidget(contentWidget);
        bgContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *grid = new QGridLayout(bgContent);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(20);
        grid->setVerticalSpacing(14);
        grid->setColumnStretch(0, 0);
        grid->setColumnStretch(1, 1);
        grid->setColumnStretch(2, 0);

        // Row 0: Background Dim
        auto *dimLabel = new QLabel(QStringLiteral("Затемнение фона:"), bgContent);
        dimLabel->setStyleSheet(QStringLiteral("font-size: 14px;"));

        m_bgDimValLabel = new QLabel(bgContent);
        m_bgDimValLabel->setStyleSheet(Theme::secondaryLabelStyle());
        m_bgDimValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_bgDimValLabel->setMinimumWidth(55);

        m_bgDimSlider = new QSlider(Qt::Horizontal, bgContent);
        m_bgDimSlider->setRange(0, 100);
        m_bgDimSlider->setStyleSheet(Theme::sliderStyle());
        m_bgDimSlider->setToolTip(QStringLiteral("0% — оригинальная яркость обложки, 100% — полностью черный фон"));

        connect(m_bgDimSlider, &QSlider::valueChanged, this, [this](int v) {
            m_bgDimValLabel->setText(QString::number(v) + QStringLiteral("%"));
            SettingsManager::instance().setBgDimPercent(v);
            SettingsManager::instance().saveSettings();
        });

        grid->addWidget(dimLabel, 0, 0);
        grid->addWidget(m_bgDimSlider, 0, 1);
        grid->addWidget(m_bgDimValLabel, 0, 2);

        // Row 1: Background Blur
        auto *blurLabel = new QLabel(QStringLiteral("Размытие фона:"), bgContent);
        blurLabel->setStyleSheet(QStringLiteral("font-size: 14px;"));

        m_bgBlurValLabel = new QLabel(bgContent);
        m_bgBlurValLabel->setStyleSheet(Theme::secondaryLabelStyle());
        m_bgBlurValLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_bgBlurValLabel->setMinimumWidth(55);

        m_bgBlurSlider = new QSlider(Qt::Horizontal, bgContent);
        m_bgBlurSlider->setRange(0, 20);
        m_bgBlurSlider->setStyleSheet(Theme::sliderStyle());
        m_bgBlurSlider->setToolTip(QStringLiteral("Размывает мелкие детали обложки во время игры для читаемости нот"));

        connect(m_bgBlurSlider, &QSlider::valueChanged, this, [this](int v) {
            m_bgBlurValLabel->setText(QString::number(v) + QStringLiteral(" px"));
            SettingsManager::instance().setBgBlurRadius(v);
            SettingsManager::instance().saveSettings();
        });

        grid->addWidget(blurLabel, 1, 0);
        grid->addWidget(m_bgBlurSlider, 1, 1);
        grid->addWidget(m_bgBlurValLabel, 1, 2);

        contentLayout->addWidget(createSection(QStringLiteral("Отображение фона в игре"), bgContent));
    }

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);

    // Bottom Action Bar: Reset to defaults & Reset local records
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(16);

    m_resetBtn = new QPushButton(QStringLiteral("Сбросить по умолчанию"), this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(Theme::secondaryButtonStyle());
    connect(m_resetBtn, &QPushButton::clicked, this, [this]() {
        AudioManager::instance().playClickSound();
        resetDefaults();
    });

    m_resetRecordsBtn = new QPushButton(QStringLiteral("Сбросить локальные рекорды"), this);
    m_resetRecordsBtn->setCursor(Qt::PointingHandCursor);
    m_resetRecordsBtn->setStyleSheet(Theme::dangerButtonStyle());
    connect(m_resetRecordsBtn, &QPushButton::clicked, this, &SettingsWidget::handleResetRecords);

    bottomLayout->addWidget(m_resetBtn);
    bottomLayout->addWidget(m_resetRecordsBtn);
    bottomLayout->addStretch(1);

    mainLayout->addLayout(bottomLayout);
}

void SettingsWidget::loadFromSettingsManager()
{
    const auto &mgr = SettingsManager::instance();

    if (m_nickEdit) {
        const QSignalBlocker blocker(m_nickEdit);
        m_nickEdit->setText(mgr.nickname());
    }
    if (m_masterSlider) {
        const QSignalBlocker blocker(m_masterSlider);
        m_masterSlider->setValue(mgr.masterVolume());
        if (m_masterValLabel) {
            m_masterValLabel->setText(QString::number(mgr.masterVolume()) + QStringLiteral("%"));
        }
    }
    if (m_musicSlider) {
        const QSignalBlocker blocker(m_musicSlider);
        m_musicSlider->setValue(mgr.musicVolume());
        if (m_musicValLabel) {
            m_musicValLabel->setText(QString::number(mgr.musicVolume()) + QStringLiteral("%"));
        }
    }
    if (m_sfxSlider) {
        const QSignalBlocker blocker(m_sfxSlider);
        m_sfxSlider->setValue(mgr.sfxVolume());
        if (m_sfxValLabel) {
            m_sfxValLabel->setText(QString::number(mgr.sfxVolume()) + QStringLiteral("%"));
        }
    }
    if (m_primaryKeyBtn) {
        const QSignalBlocker blocker(m_primaryKeyBtn);
        m_primaryKeyBtn->setCurrentKey(mgr.primaryKey());
    }
    if (m_secondaryKeyBtn) {
        const QSignalBlocker blocker(m_secondaryKeyBtn);
        m_secondaryKeyBtn->setCurrentKey(mgr.secondaryKey());
    }
    if (m_offsetSpinBox) {
        const QSignalBlocker blocker(m_offsetSpinBox);
        m_offsetSpinBox->setValue(mgr.audioOffsetMs());
    }
    if (m_screenModeCombo) {
        const QSignalBlocker blocker(m_screenModeCombo);
        m_screenModeCombo->setCurrentIndex(mgr.isFullscreen() ? 1 : 0);
    }
    if (m_bgDimSlider) {
        const QSignalBlocker blocker(m_bgDimSlider);
        m_bgDimSlider->setValue(mgr.bgDimPercent());
        if (m_bgDimValLabel) {
            m_bgDimValLabel->setText(QString::number(mgr.bgDimPercent()) + QStringLiteral("%"));
        }
    }
    if (m_bgBlurSlider) {
        const QSignalBlocker blocker(m_bgBlurSlider);
        m_bgBlurSlider->setValue(mgr.bgBlurRadius());
        if (m_bgBlurValLabel) {
            m_bgBlurValLabel->setText(QString::number(mgr.bgBlurRadius()) + QStringLiteral(" px"));
        }
    }
}

void SettingsWidget::resetDefaults()
{
    auto reply = QMessageBox::question(this, QStringLiteral("Сброс настроек"),
                                       QStringLiteral("Вы действительно хотите сбросить все параметры к значениям по умолчанию?"),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        SettingsManager::instance().resetToDefaults();
        loadFromSettingsManager();
    }
}

void SettingsWidget::startAutoCalibration()
{
    // Воспроизводим тестовый щелчок метронома
    CalibrationDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        if (m_offsetSpinBox) {
            const QSignalBlocker blocker(m_offsetSpinBox);
            m_offsetSpinBox->setValue(SettingsManager::instance().audioOffsetMs());
        }
    }
    return;

    // Stub per TZ for interactive 10-tap calibration
    /*
    QMessageBox::information(this, QStringLiteral("Автокалибровка задержки"),
                             QStringLiteral("Режим автокалибровки:\n\n"
                                            "При запуске проигрывается 10 ритмичных кликов метронома.\n"
                                            "Игрок нажимает основную клавишу в такт каждому клику.\n"
                                            "Система рассчитывает среднее отклонение (в миллисекундах) "
                                            "и автоматически выставляет компенсирующее значение Audio Offset.")); */
}

// ============================================================================
// Easter-egg 8-step Local Records Reset
// ============================================================================

bool SettingsWidget::showConfirmStep(const QString &title, const QString &message, const QString &confirmText, int step)
{
    AudioManager::instance().playClickSound();

    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setModal(true);

    auto *outerLayout = new QVBoxLayout(&dialog);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(&dialog);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(&dialog);
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

    auto *titleLabel = new QLabel(title, container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogErrorTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.reject();
    });
    headerLayout->addWidget(closeBtn);
    innerLayout->addLayout(headerLayout);

    auto *msgLabel = new QLabel(message, container);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(msgLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(14);

    int confirmWidth = qMax(85, 160 - (step - 1) * 11);
    int confirmHeight = qMax(28, 42 - (step - 1) * 2);

    int cancelWidth = qMin(220, 110 + (step - 1) * 15);
    int cancelHeight = qMin(52, 40 + (step - 1) * 2);

    auto *confirmBtn = new QPushButton(confirmText, container);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet(Theme::easterEggDangerButtonStyle(step));
    confirmBtn->setFixedSize(confirmWidth, confirmHeight);
    QObject::connect(confirmBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.accept();
    });
    buttonLayout->addWidget(confirmBtn);

    auto *cancelBtn = new QPushButton(QStringLiteral("Отмена"), container);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(Theme::easterEggCancelButtonStyle(step));
    cancelBtn->setFixedSize(cancelWidth, cancelHeight);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.reject();
    });
    buttonLayout->addWidget(cancelBtn);

    innerLayout->addLayout(buttonLayout);
    outerLayout->addWidget(container);

    return dialog.exec() == QDialog::Accepted;
}

bool SettingsWidget::showInputStep(const QString &title, const QString &message, const QString &expectedText, bool caseSensitive, int step)
{
    AudioManager::instance().playClickSound();

    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setModal(true);

    auto *outerLayout = new QVBoxLayout(&dialog);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(&dialog);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(&dialog);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 180));
    shadow->setOffset(0, 8);
    container->setGraphicsEffect(shadow);

    auto *innerLayout = new QVBoxLayout(container);
    innerLayout->setContentsMargins(24, 20, 24, 24);
    innerLayout->setSpacing(16);

    auto *headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(0, 0, 0, 0);

    auto *spacerLeft = new QWidget(container);
    spacerLeft->setFixedSize(28, 28);
    headerLayout->addWidget(spacerLeft);

    auto *titleLabel = new QLabel(title, container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogErrorTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.reject();
    });
    headerLayout->addWidget(closeBtn);
    innerLayout->addLayout(headerLayout);

    auto *msgLabel = new QLabel(message, container);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(msgLabel);

    auto *lineEdit = new QLineEdit(container);
    lineEdit->setStyleSheet(Theme::lineEditStyle());
    lineEdit->setFixedHeight(36);
    innerLayout->addWidget(lineEdit);

    auto *errorLabel = new QLabel(container);
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setStyleSheet(QStringLiteral("color: #e57388; font-size: 12px; background: transparent; border: none;"));
    errorLabel->setVisible(false);
    innerLayout->addWidget(errorLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(14);

    int confirmWidth = qMax(85, 160 - (step - 1) * 11);
    int confirmHeight = qMax(28, 42 - (step - 1) * 2);

    int cancelWidth = qMin(220, 110 + (step - 1) * 15);
    int cancelHeight = qMin(52, 40 + (step - 1) * 2);

    auto *confirmBtn = new QPushButton(QStringLiteral("Подтвердить"), container);
    confirmBtn->setCursor(Qt::PointingHandCursor);
    confirmBtn->setStyleSheet(Theme::easterEggDangerButtonStyle(step));
    confirmBtn->setFixedSize(confirmWidth, confirmHeight);

    auto checkAndAccept = [&]() {
        QString entered = lineEdit->text().trimmed();
        bool match = caseSensitive
            ? (entered == expectedText.trimmed())
            : (entered.compare(expectedText.trimmed(), Qt::CaseInsensitive) == 0);

        if (match) {
            AudioManager::instance().playClickSound();
            dialog.accept();
        } else {
            AudioManager::instance().playMissSound();
            lineEdit->setStyleSheet(Theme::errorLineEditStyle());
            errorLabel->setText(QStringLiteral("Введено неверное значение!"));
            errorLabel->setVisible(true);
        }
    };

    QObject::connect(confirmBtn, &QPushButton::clicked, &dialog, checkAndAccept);
    QObject::connect(lineEdit, &QLineEdit::returnPressed, &dialog, checkAndAccept);
    buttonLayout->addWidget(confirmBtn);

    auto *cancelBtn = new QPushButton(QStringLiteral("Отмена"), container);
    cancelBtn->setCursor(Qt::PointingHandCursor);
    cancelBtn->setStyleSheet(Theme::easterEggCancelButtonStyle(step));
    cancelBtn->setFixedSize(cancelWidth, cancelHeight);
    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.reject();
    });
    buttonLayout->addWidget(cancelBtn);

    innerLayout->addLayout(buttonLayout);
    outerLayout->addWidget(container);

    lineEdit->setFocus();
    return dialog.exec() == QDialog::Accepted;
}

void SettingsWidget::showFinalInfo(const QString &title, const QString &message)
{
    AudioManager::instance().playClearSound();

    QDialog dialog(this);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);
    dialog.setModal(true);

    auto *outerLayout = new QVBoxLayout(&dialog);
    outerLayout->setContentsMargins(20, 20, 20, 20);

    auto *container = new QWidget(&dialog);
    container->setObjectName(QStringLiteral("dialogContainer"));
    container->setStyleSheet(Theme::exitDialogContainerStyle());

    auto *shadow = new QGraphicsDropShadowEffect(&dialog);
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

    auto *titleLabel = new QLabel(title, container);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(Theme::dialogTitleStyle());
    headerLayout->addWidget(titleLabel, 1);

    auto *closeBtn = new QPushButton(QStringLiteral("✕"), container);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setFixedSize(28, 28);
    closeBtn->setStyleSheet(Theme::dialogCloseButtonStyle());
    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.accept();
    });
    headerLayout->addWidget(closeBtn);
    innerLayout->addLayout(headerLayout);

    auto *msgLabel = new QLabel(message, container);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(msgLabel);

    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    auto *okBtn = new QPushButton(QStringLiteral("ОК"), container);
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setStyleSheet(Theme::primaryButtonStyle());
    okBtn->setFixedHeight(40);
    okBtn->setMinimumWidth(130);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, [&dialog]() {
        AudioManager::instance().playClickSound();
        dialog.accept();
    });
    buttonLayout->addWidget(okBtn);
    innerLayout->addLayout(buttonLayout);

    outerLayout->addWidget(container);
    dialog.exec();
}

void SettingsWidget::handleResetRecords()
{
    const QString warnTitle = QStringLiteral("Предупреждение");

    // 1. точно ли вы уверены?
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Точно ли вы уверены?"),
                         QStringLiteral("Да, уверен"),
                         1)) {
        return;
    }

    // 2. это безвозвратно удалит ваши рекорды!
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Это безвозвратно удалит ваши рекорды!"),
                         QStringLiteral("Я понимаю риск"),
                         2)) {
        return;
    }

    // 3. вы понимаете, что делаете?
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Вы понимаете, что делаете?"),
                         QStringLiteral("Да, абсолютно"),
                         3)) {
        return;
    }

    // 4. введите капсом слово удалить, чтобы подтвердить свои действия
    if (!showInputStep(warnTitle,
                       QStringLiteral("Введите капсом слово УДАЛИТЬ, чтобы подтвердить свои действия:"),
                       QStringLiteral("УДАЛИТЬ"),
                       true,
                       4)) {
        return;
    }

    // 5. вы никогда не вернете свои рекорды!!
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Вы никогда не вернете свои рекорды!!"),
                         QStringLiteral("Все равно удалить"),
                         5)) {
        return;
    }

    // 6. введите свой ник чтобы подтвердить свои действия (первые 3 буквы и всегда 5 звёздочек)
    QString playerNick = SettingsManager::instance().nickname().trimmed();
    if (playerNick.isEmpty()) {
        playerNick = QStringLiteral("Player");
    }
    QString maskedNick = playerNick.left(3) + QStringLiteral("*****");
    if (!showInputStep(warnTitle,
                       QStringLiteral("Введите свой ник (%1), чтобы подтвердить свои действия:").arg(maskedNick),
                       playerNick,
                       false,
                       6)) {
        return;
    }

    // 7. вы сумасшедший?! зачем это делать, просто оставьте их
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Вы сумасшедший?! Зачем это делать, просто оставьте их"),
                         QStringLiteral("Нет, удаляй!"),
                         7)) {
        return;
    }

    // 8. ладно, это ваше решение.....
    if (!showConfirmStep(warnTitle,
                         QStringLiteral("Ладно, это ваше решение....."),
                         QStringLiteral("Очистить все"),
                         8)) {
        return;
    }

    // Финальное сообщение об очистке
    showFinalInfo(QStringLiteral("Сброс рекордов"), QStringLiteral("Все локальные рекорды очищены!"));
}

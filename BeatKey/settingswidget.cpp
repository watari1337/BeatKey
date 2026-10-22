#include "settingswidget.h"
#include "settingsmanager.h"
#include "theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QKeyEvent>
#include <QMessageBox>
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

    m_key = static_cast<Qt::Key>(k);
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
    , m_primaryKeyBtn(nullptr)
    , m_secondaryKeyBtn(nullptr)
    , m_offsetSpinBox(nullptr)
    , m_autoCalibBtn(nullptr)
    , m_screenModeCombo(nullptr)
    , m_resetBtn(nullptr)
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
    auto *slider = new QSlider(Qt::Horizontal, this);
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
    connect(m_backBtn, &QPushButton::clicked, this, &SettingsWidget::backRequested);

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
        contentLayout->addWidget(createSection(QStringLiteral("👤 Профиль игрока"), playerContent));
    }

    // 2. Audio Section
    {
        auto *audioContent = new QWidget(contentWidget);
        audioContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *grid = new QGridLayout(audioContent);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setSpacing(12);

        m_masterValLabel = new QLabel(audioContent);
        m_masterSlider = createVolumeSlider(SettingsManager::DEFAULT_MASTER_VOLUME, m_masterValLabel);
        connect(m_masterSlider, &QSlider::valueChanged, this, [](int v) {
            SettingsManager::instance().setMasterVolume(v);
            SettingsManager::instance().saveSettings();
        });

        m_musicValLabel = new QLabel(audioContent);
        m_musicSlider = createVolumeSlider(SettingsManager::DEFAULT_MUSIC_VOLUME, m_musicValLabel);
        connect(m_musicSlider, &QSlider::valueChanged, this, [](int v) {
            SettingsManager::instance().setMusicVolume(v);
            SettingsManager::instance().saveSettings();
        });

        m_sfxValLabel = new QLabel(audioContent);
        m_sfxSlider = createVolumeSlider(SettingsManager::DEFAULT_SFX_VOLUME, m_sfxValLabel);
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

        contentLayout->addWidget(createSection(QStringLiteral("🔊 Звуковые параметры"), audioContent));
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

        contentLayout->addWidget(createSection(QStringLiteral("⌨ Управление ритм-игрой"), controlsContent));
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
            m_offsetSpinBox->setValue(m_offsetSpinBox->value() - 10);
        });
        connect(plus10Btn, &QPushButton::clicked, this, [this]() {
            m_offsetSpinBox->setValue(m_offsetSpinBox->value() + 10);
        });

        hbox->addWidget(desc);
        hbox->addWidget(minus10Btn);
        hbox->addWidget(m_offsetSpinBox);
        hbox->addWidget(plus10Btn);
        hbox->addStretch(1);

        m_autoCalibBtn = new QPushButton(QStringLiteral("⚡ Запустить автокалибровку задержки (10 ударов)"), calibContent);
        m_autoCalibBtn->setCursor(Qt::PointingHandCursor);
        m_autoCalibBtn->setStyleSheet(Theme::primaryButtonStyle());
        connect(m_autoCalibBtn, &QPushButton::clicked, this, &SettingsWidget::startAutoCalibration);

        vbox->addLayout(hbox);
        vbox->addWidget(m_autoCalibBtn);

        contentLayout->addWidget(createSection(QStringLiteral("⏱ Синхронизация и калибровка"), calibContent));
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
        m_screenModeCombo->addItem(QStringLiteral("Оконный режим (1280×720)"), false);
        m_screenModeCombo->addItem(QStringLiteral("Полноэкранный режим (Fullscreen)"), true);

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

        contentLayout->addWidget(createSection(QStringLiteral("🖥 Отображение экрана"), videoContent));
    }

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);

    // Bottom Action Bar: Reset to defaults
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(16);

    m_resetBtn = new QPushButton(QStringLiteral("Сбросить по умолчанию"), this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(Theme::secondaryButtonStyle());
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsWidget::resetDefaults);

    bottomLayout->addWidget(m_resetBtn);
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
    }
    if (m_musicSlider) {
        const QSignalBlocker blocker(m_musicSlider);
        m_musicSlider->setValue(mgr.musicVolume());
    }
    if (m_sfxSlider) {
        const QSignalBlocker blocker(m_sfxSlider);
        m_sfxSlider->setValue(mgr.sfxVolume());
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
    // Stub per TZ for interactive 10-tap calibration
    QMessageBox::information(this, QStringLiteral("Автокалибровка задержки"),
                             QStringLiteral("Режим автокалибровки:\n\n"
                                            "При запуске проигрывается 10 ритмичных кликов метронома.\n"
                                            "Игрок нажимает основную клавишу в такт каждому клику.\n"
                                            "Система рассчитывает среднее отклонение (в миллисекундах) "
                                            "и автоматически выставляет компенсирующее значение Audio Offset."));
}

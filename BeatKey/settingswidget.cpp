#include "settingswidget.h"
#include "settingsmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScrollArea>
#include <QKeyEvent>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

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
    , m_fullscreenCheck(nullptr)
    , m_saveBtn(nullptr)
    , m_resetBtn(nullptr)
    , m_backBtn(nullptr)
{
    setupUi();
    loadFromSettingsManager();
}

QWidget* SettingsWidget::createSection(const QString &titleText, QWidget *contentWidget)
{
    auto *box = new QWidget(this);
    box->setStyleSheet(QStringLiteral(
        "background-color: rgba(30, 22, 50, 180);"
        "border: 1px solid #5a388f;"
        "border-radius: 12px;"
    ));

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
    slider->setStyleSheet(QStringLiteral(
        "QSlider::groove:horizontal {"
        "  height: 8px;"
        "  background: #251b3d;"
        "  border-radius: 4px;"
        "}"
        "QSlider::sub-page:horizontal {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #7d44cf, stop:1 #a259ff);"
        "  border-radius: 4px;"
        "}"
        "QSlider::handle:horizontal {"
        "  background: #ffffff;"
        "  border: 2px solid #a259ff;"
        "  width: 18px;"
        "  margin-top: -5px;"
        "  margin-bottom: -5px;"
        "  border-radius: 9px;"
        "}"
    ));

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
    m_backBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #2a1f42;"
        "  color: #bb9af7;"
        "  border: 1px solid #65439c;"
        "  border-radius: 8px;"
        "  padding: 8px 18px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #3d2a61;"
        "  border: 1px solid #a259ff;"
        "  color: #ffffff;"
        "}"
    ));
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
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: none; background: transparent; }"
        "QScrollBar:vertical {"
        "  border: none;"
        "  background: #18112c;"
        "  width: 10px;"
        "  margin: 0px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #553388;"
        "  min-height: 20px;"
        "  border-radius: 5px;"
        "}"
    ));

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
        m_nickEdit->setStyleSheet(QStringLiteral(
            "QLineEdit {"
            "  background: #1c1430;"
            "  color: #ffffff;"
            "  border: 1px solid #7d44cf;"
            "  border-radius: 8px;"
            "  padding: 8px 12px;"
            "  font-size: 14px;"
            "}"
            "QLineEdit:focus {"
            "  border: 1px solid #a259ff;"
            "}"
        ));

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

        m_musicValLabel = new QLabel(audioContent);
        m_musicSlider = createVolumeSlider(SettingsManager::DEFAULT_MUSIC_VOLUME, m_musicValLabel);

        m_sfxValLabel = new QLabel(audioContent);
        m_sfxSlider = createVolumeSlider(SettingsManager::DEFAULT_SFX_VOLUME, m_sfxValLabel);

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
        primBox->addWidget(m_primaryKeyBtn);

        auto *secBox = new QVBoxLayout();
        secBox->addWidget(new QLabel(QStringLiteral("Дополнительная клавиша:"), controlsContent));
        m_secondaryKeyBtn = new KeyBindButton(SettingsManager::DEFAULT_SECONDARY_KEY, controlsContent);
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
        m_offsetSpinBox->setStyleSheet(QStringLiteral(
            "QSpinBox {"
            "  background: #1c1430;"
            "  color: #ffffff;"
            "  border: 1px solid #7d44cf;"
            "  border-radius: 6px;"
            "  padding: 6px;"
            "  font-size: 14px;"
            "  font-weight: bold;"
            "  min-width: 90px;"
            "}"
        ));

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
        m_autoCalibBtn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3c1e70, stop:1 #5a289b);"
            "  color: #ffffff;"
            "  border: 1px solid #9154f3;"
            "  border-radius: 8px;"
            "  padding: 10px 18px;"
            "  font-size: 13px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4f2794, stop:1 #7234c2);"
            "}"
        ));
        connect(m_autoCalibBtn, &QPushButton::clicked, this, &SettingsWidget::startAutoCalibration);

        vbox->addLayout(hbox);
        vbox->addWidget(m_autoCalibBtn);

        contentLayout->addWidget(createSection(QStringLiteral("⏱ Синхронизация и калибровка"), calibContent));
    }

    // 5. Video Section
    {
        auto *videoContent = new QWidget(contentWidget);
        videoContent->setStyleSheet(QStringLiteral("border:none; background:transparent;"));
        auto *layout = new QHBoxLayout(videoContent);
        layout->setContentsMargins(0, 0, 0, 0);

        m_fullscreenCheck = new QCheckBox(QStringLiteral("Полноэкранный режим (Fullscreen)"), videoContent);
        m_fullscreenCheck->setStyleSheet(QStringLiteral(
            "QCheckBox { color: #ffffff; font-size: 14px; }"
            "QCheckBox::indicator {"
            "  width: 18px;"
            "  height: 18px;"
            "  border: 2px solid #7d44cf;"
            "  border-radius: 4px;"
            "  background: #1c1430;"
            "}"
            "QCheckBox::indicator:checked {"
            "  background: #a259ff;"
            "  border: 2px solid #ffffff;"
            "}"
        ));

        layout->addWidget(m_fullscreenCheck);
        layout->addStretch(1);

        contentLayout->addWidget(createSection(QStringLiteral("🖥 Отображение"), videoContent));
    }

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea, 1);

    // Bottom Action Bar: Reset, Save
    auto *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(16);

    m_resetBtn = new QPushButton(QStringLiteral("Сбросить по умолчанию"), this);
    m_resetBtn->setCursor(Qt::PointingHandCursor);
    m_resetBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: #2a1f42;"
        "  color: #dd8888;"
        "  border: 1px solid #774455;"
        "  border-radius: 8px;"
        "  padding: 10px 20px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #442535;"
        "  border: 1px solid #aa5566;"
        "  color: #ffffff;"
        "}"
    ));
    connect(m_resetBtn, &QPushButton::clicked, this, &SettingsWidget::resetDefaults);

    m_saveBtn = new QPushButton(QStringLiteral("✓ Сохранить настройки"), this);
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    m_saveBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1b8a5a, stop:1 #27ae60);"
        "  color: #ffffff;"
        "  border: 1px solid #2ecc71;"
        "  border-radius: 8px;"
        "  padding: 10px 26px;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #23a86f, stop:1 #2ecc71);"
        "}"
    ));
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsWidget::applyAndSave);

    bottomLayout->addWidget(m_resetBtn);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(bottomLayout);
}

void SettingsWidget::loadFromSettingsManager()
{
    const auto &mgr = SettingsManager::instance();

    if (m_nickEdit) m_nickEdit->setText(mgr.nickname());
    if (m_masterSlider) m_masterSlider->setValue(mgr.masterVolume());
    if (m_musicSlider) m_musicSlider->setValue(mgr.musicVolume());
    if (m_sfxSlider) m_sfxSlider->setValue(mgr.sfxVolume());
    if (m_primaryKeyBtn) m_primaryKeyBtn->setCurrentKey(mgr.primaryKey());
    if (m_secondaryKeyBtn) m_secondaryKeyBtn->setCurrentKey(mgr.secondaryKey());
    if (m_offsetSpinBox) m_offsetSpinBox->setValue(mgr.audioOffsetMs());
    if (m_fullscreenCheck) m_fullscreenCheck->setChecked(mgr.isFullscreen());
}

void SettingsWidget::applyAndSave()
{
    auto &mgr = SettingsManager::instance();

    QString nick = m_nickEdit->text().trimmed();
    if (nick.length() < 3) {
        QMessageBox::warning(this, QStringLiteral("Ошибка ввода"),
                             QStringLiteral("Никнейм должен содержать не менее 3 символов."));
        return;
    }

    mgr.setNickname(nick);
    mgr.setMasterVolume(m_masterSlider->value());
    mgr.setMusicVolume(m_musicSlider->value());
    mgr.setSfxVolume(m_sfxSlider->value());
    mgr.setPrimaryKey(m_primaryKeyBtn->currentKey());
    mgr.setSecondaryKey(m_secondaryKeyBtn->currentKey());
    mgr.setAudioOffsetMs(m_offsetSpinBox->value());
    mgr.setFullscreen(m_fullscreenCheck->isChecked());

    mgr.saveSettings();

    QMessageBox::information(this, QStringLiteral("Настройки сохранены"),
                             QStringLiteral("Конфигурация успешно сохранена в файл settings.ini."));
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

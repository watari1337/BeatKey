#include "editorhelpdialog.h"
#include "theme.h"
#include "audiomanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QFrame>
#include <QKeyEvent>

EditorHelpDialog::EditorHelpDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("Справка редактора карт BeatKey"));
    setModal(true);
    resize(640, 560);
    setStyleSheet(QStringLiteral("QDialog { background: #1c1034; color: #ffffff; }"));

    setupUi();
}

void EditorHelpDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_F1) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void EditorHelpDialog::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 20, 24, 20);
    rootLayout->setSpacing(16);

    // Header
    auto *headerLayout = new QHBoxLayout();
    auto *titleLabel = new QLabel(QStringLiteral("Руководство пользователя редактора карт"), this);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-family: 'Segoe UI', sans-serif;"
        "font-size: 18px;"
        "font-weight: bold;"
        "color: #ffffff;"
    ));

    auto *closeBtn = new QPushButton(QStringLiteral("Закрыть"), this);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(Theme::secondaryButtonStyle());
    closeBtn->setFixedSize(90, 32);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);

    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);
    headerLayout->addWidget(closeBtn);
    rootLayout->addLayout(headerLayout);

    // Scrollable Content
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(QStringLiteral(
        "QScrollArea { border: 1px solid #5a388f; border-radius: 8px; background: #160d2b; }"
    ));

    auto *contentWidget = new QWidget(scrollArea);
    contentWidget->setStyleSheet(QStringLiteral("background: transparent;"));
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 16, 20, 16);
    contentLayout->setSpacing(16);

    auto addSection = [&](const QString &secTitle, const QString &htmlContent) {
        auto *secCard = new QFrame(contentWidget);
        secCard->setStyleSheet(QStringLiteral(
            "QFrame { background: rgba(36, 22, 60, 220); border-radius: 8px; border: 1px solid #4a287a; padding: 12px; }"
        ));
        auto *cardLayout = new QVBoxLayout(secCard);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(8);

        auto *title = new QLabel(secTitle, secCard);
        title->setStyleSheet(QStringLiteral(
            "color: #bb9af7; font-size: 14px; font-weight: bold; border: none; background: transparent;"
        ));
        cardLayout->addWidget(title);

        auto *body = new QLabel(htmlContent, secCard);
        body->setStyleSheet(QStringLiteral(
            "color: #dddddd; font-size: 12px; line-height: 1.5; border: none; background: transparent;"
        ));
        body->setWordWrap(true);
        cardLayout->addWidget(body);

        contentLayout->addWidget(secCard);
    };

    addSection(
        QStringLiteral("1. Горячие клавиши управления"),
        QStringLiteral(
            "<b>Пробел</b> — Воспроизведение / Пауза трека.<br>"
            "<b>1</b> — Переключение на инструмент выделения (Select).<br>"
            "<b>2</b> — Переключение на инструмент размещения нот (Place).<br>"
            "<b>R</b> — Включение / выключение записи на лету (Tap Record). При включении автоматически запускается музыка, при выключении — ставится на паузу.<br>"
            "<b>Z / X / Пробел</b> — Такт/удар ноты во время активной записи Tap Record.<br>"
            "<b>Стрелки Влево / Вправо</b> — Пошаговая перемотка курсора на один шаг сетки.<br>"
            "<b>Ctrl + Колесико мыши</b> — Плавное масштабирование таймлайна (зум).<br>"
            "<b>Колесико мыши</b> — Горизонтальная прокрутка дорожки.<br>"
            "<b>Ctrl + Z</b> — Отмена последнего действия (Undo).<br>"
            "<b>Ctrl + Y</b> или <b>Ctrl + Shift + Z</b> — Повтор действия (Redo).<br>"
            "<b>Ctrl + S</b> — Быстрое сохранение карты.<br>"
            "<b>Delete / Backspace</b> — Удаление выделенных нот.<br>"
            "<b>Ctrl + A</b> — Выделить все ноты на карте."
        )
    );

    addSection(
        QStringLiteral("2. Размещение и удаление нот мышей"),
        QStringLiteral(
            "<b>Одиночная ставка:</b> Включите инструмент размещения (клавиша <b>2</b>) и кликните ЛКМ по дорожке — нота появится на ближайшей риске сетки квантования.<br>"
            "<b>Рисование протяжкой (Brush):</b> Зажмите ЛКМ и плавно ведите мышь по таймлайну — ноты будут автоматически проставляться по всем рискам сетки.<br>"
            "<b>Одиночное удаление:</b> Кликните ПКМ по существующей ноте.<br>"
            "<b>Непрерывный ластик (Eraser):</b> Зажмите ПКМ и проведите курсором по нотам — все задетые ноты будут быстро удалены.<br>"
            "<i>Примечание: Все серии рисования и стирания объединяются в одну операцию отмены (Ctrl+Z).</i>"
        )
    );

    addSection(
        QStringLiteral("3. Выделение и перемещение нот"),
        QStringLiteral(
            "<b>Выделение одной ноты:</b> Кликните по ней левой кнопкой мыши.<br>"
            "<b>Рамочное выделение (Marquee Box):</b> Зажмите ЛКМ на пустом месте таймлайна и растяните прямоугольную рамку вокруг группы нот.<br>"
            "<b>Добавление к выделению:</b> Удерживайте <b>Ctrl</b> при клике или рамке для добавления/снятия выделения с конкретных нот.<br>"
            "<b>Перемещение с привязкой:</b> Зажмите ЛКМ на выделенной ноте и тяните вбок. Ноты плавно перемещаются и примагничиваются (Snap to Grid) к линиям квантования."
        )
    );

    addSection(
        QStringLiteral("4. Сетка квантования и собственный шаг"),
        QStringLiteral(
            "На панели справа можно выбрать стандартные музыкальные доли такта 4/4:<br>"
            "• <b>1/1</b> — целый такт;<br>"
            "• <b>1/2</b> — половина такта (2 четверти);<br>"
            "• <b>1/3</b> — триоли целого такта;<br>"
            "• <b>1/4</b> — четвертная доля (один бит);<br>"
            "• <b>1/8</b> — восьмая доля;<br>"
            "• <b>1/16</b> — шестнадцатая доля.<br>"
            "Также доступно поле <b>'Свой шаг (1/N)'</b>: введите любое целое число от 1 до 64 для тонкой разметки нестандартных ритмических рисунков."
        )
    );

    contentLayout->addStretch(1);
    scrollArea->setWidget(contentWidget);
    rootLayout->addWidget(scrollArea, 1);
}

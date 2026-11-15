#include "covermanager.h"
#include <QPainter>
#include <QPainterPath>
#include "theme.h"
#include "audiomanager.h"

#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QImage>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsDropShadowEffect>

QString CoverManager::standardCoverPath(StandardCover cover)
{
    return (cover == StandardCover::Standard1) ? STANDARD_COVER_1 : STANDARD_COVER_2;
}

QPixmap CoverManager::standardCoverPixmap(StandardCover cover)
{
    static QPixmap s_pixmap1(STANDARD_COVER_1);
    static QPixmap s_pixmap2(STANDARD_COVER_2);
    return (cover == StandardCover::Standard1) ? s_pixmap1 : s_pixmap2;
}

bool CoverManager::isStandardCover(const QString &path)
{
    return path == STANDARD_COVER_1 || path == STANDARD_COVER_2;
}

QString CoverManager::coverFilter()
{
    return QStringLiteral("Изображения (*.png *.jpg *.jpeg);;Все файлы (*.*)");
}

QString CoverManager::audioFilter()
{
    return QStringLiteral("Музыкальные файлы (*.mp3 *.wav *.ogg);;Все файлы (*.*)");
}

QStringList CoverManager::supportedCoverExtensions()
{
    return { QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg") };
}

QStringList CoverManager::supportedAudioExtensions()
{
    return { QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("ogg") };
}

bool CoverManager::validateAndLoadCover(const QString &filePath,
                                        QPixmap *outPixmap,
                                        QString *errorMessage)
{
    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Путь к файлу обложки не указан.");
        }
        return false;
    }

    if (isStandardCover(filePath)) {
        if (outPixmap) {
            *outPixmap = (filePath == STANDARD_COVER_1)
                ? standardCoverPixmap(StandardCover::Standard1)
                : standardCoverPixmap(StandardCover::Standard2);
        }
        return true;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл обложки не найден по указанному пути.");
        }
        return false;
    }

    if (!fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Указанный путь не является файлом.");
        }
        return false;
    }

    if (fileInfo.size() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл обложки пуст (0 байт) или поврежден.");
        }
        return false;
    }

    QString ext = fileInfo.suffix().toLower();
    if (!supportedCoverExtensions().contains(ext)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Неподдерживаемый формат изображения '%1'.\n"
                                           "Поддерживаются только форматы: .png, .jpg, .jpeg.")
                                .arg(fileInfo.suffix());
        }
        return false;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    if (!reader.canRead()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Файл изображения поврежден или имеет неверный внутренний формат.");
        }
        return false;
    }

    QImage img = reader.read();
    if (img.isNull() || img.width() <= 0 || img.height() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Не удалось открыть изображение: данные файла повреждены.");
        }
        return false;
    }

    if (outPixmap) {
        *outPixmap = QPixmap::fromImage(img);
    }
    return true;
}

bool CoverManager::validateAudioFile(const QString &filePath, QString *errorMessage)
{
    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Путь к аудиофайлу не указан.");
        }
        return false;
    }

    if (filePath.startsWith(QStringLiteral(":/")) || filePath.startsWith(QStringLiteral("qrc:/"))) {
        return true;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Аудиофайл не найден по указанному пути.");
        }
        return false;
    }

    if (!fileInfo.isFile()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Указанный путь не является файлом.");
        }
        return false;
    }

    if (fileInfo.size() <= 0) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Аудиофайл пуст (0 байт) или поврежден.");
        }
        return false;
    }

    QString ext = fileInfo.suffix().toLower();
    if (!supportedAudioExtensions().contains(ext)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Неподдерживаемый аудиоформат '%1'.\n"
                                           "Поддерживаются только форматы: .mp3, .wav, .ogg.")
                                .arg(fileInfo.suffix());
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Не удалось открыть аудиофайл для чтения.");
        }
        return false;
    }

    // Читаем первые 64 КБ для глубокого структурного анализа заголовков и фреймов
    QByteArray sampleData = file.read(65536);
    file.close();

    return validateAudioData(sampleData, ext, errorMessage);
}

bool CoverManager::validateAudioData(const QByteArray &data, const QString &extensionHint, QString *errorMessage)
{
    if (data.isEmpty() || data.size() < 16) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Аудиоданные повреждены или имеют слишком малый размер.");
        }
        return false;
    }

    // 1. Проверка на сигнатуры распространенных не-аудио форматов
    // JPEG
    if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0xFF &&
        static_cast<unsigned char>(data[1]) == 0xD8 && static_cast<unsigned char>(data[2]) == 0xFF) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является изображением JPEG, а не аудиофайлом.");
        }
        return false;
    }
    // PNG
    if (data.startsWith("\x89PNG\x0D\x0A\x1A\x0A")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является изображением PNG, а не аудиофайлом.");
        }
        return false;
    }
    // GIF
    if (data.startsWith("GIF87a") || data.startsWith("GIF89a")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является изображением GIF, а не аудиофайлом.");
        }
        return false;
    }
    // PDF
    if (data.startsWith("%PDF")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является документом PDF, а не аудиофайлом.");
        }
        return false;
    }
    // ZIP / PK
    if (data.startsWith("PK\x03\x04")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является архивом ZIP, а не аудиофайлом.");
        }
        return false;
    }
    // Executable
    if (data.startsWith("MZ")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Выбранный файл является исполняемым файлом, а не аудиофайлом.");
        }
        return false;
    }

    QString ext = extensionHint.toLower();

    // 2. Валидация формата WAV
    if (ext == QStringLiteral("wav") || data.startsWith("RIFF") || data.startsWith("RIFX")) {
        if (data.size() < 44) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл WAV поврежден: размер файла слишком мал для заголовка.");
            }
            return false;
        }
        if ((!data.startsWith("RIFF") && !data.startsWith("RIFX")) || data.mid(8, 4) != "WAVE") {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл не содержит корректного заголовка RIFF WAVE.");
            }
            return false;
        }
        int fmtIdx = data.indexOf("fmt ");
        if (fmtIdx < 0 || fmtIdx + 16 > data.size()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл WAV не содержит информации о формате аудиопотока (чанк fmt).");
            }
            return false;
        }
        quint16 channels = *reinterpret_cast<const quint16*>(data.constData() + fmtIdx + 10);
        quint32 sampleRate = *reinterpret_cast<const quint32*>(data.constData() + fmtIdx + 12);
        if (channels == 0 || channels > 8 || sampleRate < 8000 || sampleRate > 192000) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл WAV содержит недопустимые параметры аудио (каналы или частота дискретизации).");
            }
            return false;
        }
        if (data.indexOf("data", fmtIdx + 16) < 0) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл WAV не содержит блока аудиоданных (чанк data).");
            }
            return false;
        }
        return true;
    }

    // 3. Валидация формата OGG
    if (ext == QStringLiteral("ogg") || data.startsWith("OggS")) {
        if (data.size() < 28) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл Ogg поврежден: размер файла слишком мал.");
            }
            return false;
        }
        if (!data.startsWith("OggS") || static_cast<unsigned char>(data[4]) != 0) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл не содержит корректного контейнера Ogg.");
            }
            return false;
        }
        QByteArray intro = data.left(250);
        bool hasAudioCodec = intro.contains("\x01vorbis") || intro.contains("OpusHead") || intro.contains("\x7f" "FLAC");
        if (!hasAudioCodec) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл Ogg не содержит поддерживаемого аудиокодека (Vorbis/Opus).");
            }
            return false;
        }
        return true;
    }

    // 4. Валидация формата MP3
    if (ext == QStringLiteral("mp3") || data.startsWith("ID3") ||
        (static_cast<unsigned char>(data[0]) == 0xFF && (static_cast<unsigned char>(data[1]) & 0xE0) == 0xE0)) {

        int searchOffset = 0;
        if (data.startsWith("ID3")) {
            if (data.size() < 10) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Файл MP3 содержит поврежденный заголовок ID3.");
                }
                return false;
            }
            unsigned char vMajor = static_cast<unsigned char>(data[3]);
            if (vMajor == 0xFF) {
                if (errorMessage) {
                    *errorMessage = QStringLiteral("Некорректная версия ID3 тега.");
                }
                return false;
            }
            int tagSize = 10 + (((static_cast<unsigned char>(data[6]) & 0x7F) << 21)
                             | ((static_cast<unsigned char>(data[7]) & 0x7F) << 14)
                             | ((static_cast<unsigned char>(data[8]) & 0x7F) << 7)
                             | (static_cast<unsigned char>(data[9]) & 0x7F));
            searchOffset = tagSize;
        }

        int maxSearch = qMin(searchOffset + 32768, data.size() - 4);
        bool foundValidMpegFrame = false;

        for (int i = searchOffset; i < maxSearch; ++i) {
            unsigned char b1 = static_cast<unsigned char>(data[i]);
            unsigned char b2 = static_cast<unsigned char>(data[i + 1]);
            if (b1 == 0xFF && (b2 & 0xE0) == 0xE0) {
                int version = (b2 >> 3) & 0x03; // 00=2.5, 01=reserved, 10=2, 11=1
                int layer   = (b2 >> 1) & 0x03; // 00=reserved, 01=III, 10=II, 11=I
                if (version == 1 || layer == 0) {
                    continue;
                }

                unsigned char b3 = static_cast<unsigned char>(data[i + 2]);
                int bitrateIdx = (b3 >> 4) & 0x0F;
                int srateIdx   = (b3 >> 2) & 0x03;
                if (bitrateIdx == 0 || bitrateIdx == 0x0F || srateIdx == 3) {
                    continue;
                }

                static const int bitratesMpeg1L3[16] = {
                    0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0
                };
                static const int bitratesMpeg2L3[16] = {
                    0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0
                };
                static const int sampleRatesMpeg1[4] = { 44100, 48000, 32000, 0 };
                static const int sampleRatesMpeg2[4] = { 22050, 24000, 16000, 0 };

                int bitrate = (version == 3) ? bitratesMpeg1L3[bitrateIdx] : bitratesMpeg2L3[bitrateIdx];
                int srate = (version == 3) ? sampleRatesMpeg1[srateIdx] : sampleRatesMpeg2[srateIdx];
                if (bitrate <= 0 || srate <= 0) {
                    continue;
                }

                int padding = (b3 >> 1) & 1;
                int frameLen = (version == 3 ? 144 : 72) * bitrate * 1000 / srate + padding;
                if (frameLen < 24) continue;

                int nextFramePos = i + frameLen;
                if (nextFramePos + 2 < data.size()) {
                    unsigned char nb1 = static_cast<unsigned char>(data[nextFramePos]);
                    unsigned char nb2 = static_cast<unsigned char>(data[nextFramePos + 1]);
                    if (nb1 == 0xFF && (nb2 & 0xE0) == 0xE0) {
                        foundValidMpegFrame = true;
                        break;
                    }
                } else {
                    foundValidMpegFrame = true;
                    break;
                }
            }
        }

        if (!foundValidMpegFrame) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Файл имеет расширение .mp3, но не содержит корректных аудиофреймов MP3.");
            }
            return false;
        }
        return true;
    }

    if (errorMessage) {
        *errorMessage = QStringLiteral("Формат файла не распознан как поддерживаемое аудио (.mp3, .wav, .ogg).");
    }
    return false;
}

void CoverManager::showMediaErrorDialog(QWidget *parent,
                                        const QString &title,
                                        const QString &message)
{
    AudioManager::instance().playClickSound();

    QDialog dialog(parent);
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

    // Header with title and close cross
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

    // Error message
    auto *msgLabel = new QLabel(message, container);
    msgLabel->setAlignment(Qt::AlignCenter);
    msgLabel->setWordWrap(true);
    msgLabel->setStyleSheet(Theme::dialogMessageStyle());
    innerLayout->addWidget(msgLabel);

    // Centered OK button
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

QPixmap CoverManager::createRoundedThumbnail(const QPixmap &src, int size, int radius)
{
    QPixmap base = src.isNull() ? standardCoverPixmap(StandardCover::Standard2) : src;
    if (base.isNull()) {
        QPixmap fallback(size, size);
        fallback.fill(Theme::Colors::BackgroundDeep);
        return fallback;
    }

    QPixmap scaled = base.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    int cropX = qMax(0, (scaled.width() - size) / 2);
    int cropY = qMax(0, (scaled.height() - size) / 2);
    QPixmap cropped = scaled.copy(cropX, cropY, size, size);

    QPixmap result(size, size);
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QPainterPath path;
    path.addRoundedRect(QRectF(0, 0, size, size), radius, radius);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, cropped);

    painter.setClipping(false);
    painter.setPen(QPen(Theme::Colors::BorderSubtle, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(0.5, 0.5, size - 1, size - 1), radius, radius);

    return result;
}

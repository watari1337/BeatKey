#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QWidget>

/**
 * @brief CoverManager централизует валидацию, загрузку и управление обложками и аудиофайлами ритм-карт.
 */
class CoverManager
{
public:
    // Константы стандартных встроенных обложек приложения
    inline static const QString STANDARD_COVER_1 = QStringLiteral(":/assets/images/standard_bg.jpg");
    inline static const QString STANDARD_COVER_2 = QStringLiteral(":/assets/images/standard_bg2.jpg");

    enum class StandardCover {
        Standard1,
        Standard2
    };

    // Получение пути к стандартной встроенной обложке
    static QString standardCoverPath(StandardCover cover = StandardCover::Standard2);

    // Получение QPixmap стандартной встроенной обложки
    static QPixmap standardCoverPixmap(StandardCover cover = StandardCover::Standard2);

    // Проверка, является ли путь стандартной встроенной обложкой
    static bool isStandardCover(const QString &path);

    // Фильтры форматов для диалогов выбора файлов
    static QString coverFilter();
    static QString audioFilter();

    // Поддерживаемые расширения
    static QStringList supportedCoverExtensions();
    static QStringList supportedAudioExtensions();

    /**
     * @brief Проверяет формат изображения и пытается загрузить его.
     * @param filePath Путь к файлу обложки на диске.
     * @param outPixmap Опциональный указатель для сохранения загруженного изображения.
     * @param errorMessage Опциональный указатель для возврата читаемого текста ошибки.
     * @return true, если файл существует, имеет верный формат и успешно открылся как изображение.
     */
    static bool validateAndLoadCover(const QString &filePath,
                                     QPixmap *outPixmap = nullptr,
                                     QString *errorMessage = nullptr);

    /**
     * @brief Создает квадратную скругленную миниатюру обложки заданного размера.
     * @param src Исходное изображение.
     * @param size Размер стороны квадрата (в пикселях).
     * @param radius Радиус скругления углов.
     */
    static QPixmap createRoundedThumbnail(const QPixmap &src, int size = 48, int radius = 8);

    /**
     * @brief Проверяет формат и целостность аудиофайла трека (.mp3, .wav, .ogg).
     * @param filePath Путь к аудиофайлу на диске.
     * @param errorMessage Опциональный указатель для возврата читаемого текста ошибки.
     * @return true, если файл существует, имеет поддерживаемый формат и валидную структуру.
     */
    static bool validateAudioFile(const QString &filePath,
                                  QString *errorMessage = nullptr);

    /**
     * @brief Отображает модальное окно уведомления об ошибке файла с кнопкой «ОК» в стиле игры.
     * @param parent Родительский виджет.
     * @param title Заголовок окна.
     * @param message Текст сообщения об ошибке.
     */
    static void showMediaErrorDialog(QWidget *parent,
                                     const QString &title,
                                     const QString &message);

private:
    CoverManager() = default;
};

#endif // COVERMANAGER_H

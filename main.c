// ./image_craft input.bmp output.bmp -crop 800 600 -gs -blur 0.5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "bmp.h"
#include "image.h"
#include "pipeline.h"
#include "utils.h"

// Константы и глобальные переменные

#define VERSION "1.0.0"
#define MAX_FILTERS 20
#define MAX_ARG_LENGTH 256

// Функция вывода справки

void print_help(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║                   ImageCraft v%s                         ║\n", VERSION);
    printf("║        Обработчик BMP изображений с фильтрами            ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("📋 Использование:\n");
    printf("  image_craft <входной_файл> <выходной_файл> [фильтры...]\n");
    printf("\n");
    printf("🎯 Примеры:\n");
    printf("  image_craft input.bmp output.bmp -crop 800 600 -gs -blur 0.5\n");
    printf("  image_craft photo.bmp result.bmp -neg -sharp -edge 0.1\n");
    printf("  image_craft in.bmp out.bmp -crystallize 15 -glass 3.0\n");
    printf("  image_craft image.bmp mosaic.bmp -mosaic 32 tiles.bmp\n");
    printf("\n");
    printf("🛠️  Базовые фильтры:\n");
    printf("  -crop W H          Обрезка до WxH пикселей (верхний левый угол)\n");
    printf("  -gs                Преобразование в оттенки серого\n");
    printf("  -neg               Негатив изображения\n");
    printf("  -sharp             Повышение резкости\n");
    printf("  -edge THRESH       Выделение границ с порогом THRESH (0.0-1.0)\n");
    printf("  -med WINDOW        Медианный фильтр (WINDOW - нечетное число)\n");
    printf("  -blur SIGMA        Гауссово размытие с сигмой SIGMA\n");
    printf("\n");
    printf("🌟 Дополнительные фильтры:\n");
    printf("  -crystallize SIZE  Эффект кристаллизации (размер ячейки)\n");
    printf("  -glass SCALE       Стеклянная деформация (масштаб эффекта)\n");
    printf("\n");
    printf("🏆 Бонусный фильтр:\n");
    printf("  -mosaic SIZE FILE  Мозаика с плитками из FILE (размер SIZE)\n");
    printf("\n");
    printf("📝 Примечания:\n");
    printf("  • Фильтры применяются в порядке указания\n");
    printf("  • Изображения должны быть в 24-битном BMP формате\n");
    printf("  • Поддерживаются файлы с заголовком BITMAPINFOHEADER\n");
    printf("  • Все компоненты цвета представляются числами [0.0, 1.0]\n");
    printf("\n");
    printf("🔗 Ссылки:\n");
    printf("  • Формат BMP: https://en.wikipedia.org/wiki/BMP_file_format\n");
    printf("  • Пример файла: https://en.wikipedia.org/wiki/BMP_file_format#Example_1\n");
    printf("  • Свертка: https://en.wikipedia.org/wiki/Kernel_(image_processing)\n");
    printf("  • Гауссово размытие: https://ru.wikipedia.org/wiki/Размытие_по_Гауссу\n");
    printf("\n");
}

// Функция проверки расширения файла

bool has_bmp_extension(const char* filename) {
    if (!filename) return false;
    
    size_t len = strlen(filename);
    if (len < 4) return false;
    
    const char* ext = filename + len - 4;
    return (strcasecmp(ext, ".bmp") == 0);
}

// Функция обработки аргументов командной строки

bool parse_arguments(int argc, char** argv, 
                     char** input_file, 
                     char** output_file,
                     FilterPipeline** pipeline) {
    
    if (argc < 3) {
        print_help();
        return false;
    }
    
    // Получаем имена файлов
    *input_file = argv[1];
    *output_file = argv[2];
    
    // Проверяем расширения файлов
    if (!has_bmp_extension(*input_file)) {
        fprintf(stderr, "⚠️  Предупреждение: входной файл '%s' не имеет расширения .bmp\n", 
                *input_file);
    }
    
    if (!has_bmp_extension(*output_file)) {
        fprintf(stderr, "⚠️  Предупреждение: выходной файл '%s' не имеет расширения .bmp\n", 
                *output_file);
    }
    
    // Создаем конвейер фильтров
    *pipeline = pipeline_create();
    if (!*pipeline) {
        fprintf(stderr, "❌ Ошибка создания конвейера фильтров\n");
        return false;
    }
    
    // Обрабатываем фильтры (начиная с 3-го аргумента)
    int i = 3;
    while (i < argc) {
        if (argv[i][0] == '-') {
            // Нашли фильтр
            char* filter_name = argv[i] + 1;  // Пропускаем '-'
            FilterType filter_type = filter_name_to_type(filter_name);
            
            if (filter_type == FILTER_COUNT) {
                fprintf(stderr, "❌ Неизвестный фильтр: -%s\n", filter_name);
                pipeline_destroy(*pipeline);
                return false;
            }
            
            // Определяем количество аргументов для этого фильтра
            int arg_count = 0;
            char** filter_args = NULL;
            
            // Для каждого типа фильтра определяем необходимое количество аргументов
            switch (filter_type) {
                case FILTER_CROP:
                    arg_count = 2;
                    break;
                case FILTER_EDGE:
                case FILTER_MEDIAN:
                case FILTER_BLUR:
                case FILTER_CRYSTALLIZE:
                case FILTER_GLASS:
                    arg_count = 1;
                    break;
                case FILTER_MOSAIC:
                    arg_count = 2;
                    break;
                case FILTER_GRAYSCALE:
                case FILTER_NEGATIVE:
                case FILTER_SHARPEN:
                    arg_count = 0;
                    break;
                default:
                    arg_count = 0;
                    break;
            }
            
            // Проверяем, достаточно ли аргументов
            if (i + arg_count >= argc) {
                fprintf(stderr, "❌ Недостаточно аргументов для фильтра -%s\n", filter_name);
                fprintf(stderr, "   Требуется %d аргумент(ов)\n", arg_count);
                pipeline_destroy(*pipeline);
                return false;
            }
            
            // Собираем аргументы фильтра
            if (arg_count > 0) {
                filter_args = &argv[i + 1];
            }
            
            // Добавляем фильтр в конвейер
            if (!pipeline_add_filter(*pipeline, filter_type, filter_args, arg_count)) {
                fprintf(stderr, "❌ Ошибка добавления фильтра -%s\n", filter_name);
                pipeline_destroy(*pipeline);
                return false;
            }
            
            // Пропускаем обработанные аргументы
            i += arg_count + 1;
        } else {
            // Неожиданный аргумент (не начинается с '-')
            fprintf(stderr, "Неожиданный аргумент: %s (ожидается фильтр с префиксом '-')\n", 
                    argv[i]);
            pipeline_destroy(*pipeline);
            return false;
        }
    }
    
    return true;
}

// ============================================
// Основная функция
// ============================================
int main(int argc, char** argv) {
    printf("\n");
    printf("ImageCraft v%s - Запуск обработки изображений\n", VERSION);
    printf("==============================================\n");
    
    // Переменные для хранения параметров
    char* input_file = NULL;
    char* output_file = NULL;
    FilterPipeline* pipeline = NULL;
    Image* image = NULL;
    
    // 1. Парсинг аргументов командной строки
    if (!parse_arguments(argc, argv, &input_file, &output_file, &pipeline)) {
        return 1;
    }
    
    // 2. Проверка файлов
    if (!file_exists(input_file)) {
        fprintf(stderr, "Ошибка: входной файл не существует: %s\n", input_file);
        pipeline_destroy(pipeline);
        return 1;
    }
    
    // 3. Загрузка изображения
    printf("\n📥 Загрузка изображения: %s\n", input_file);
    image = bmp_load(input_file);
    
    if (!image) {
        fprintf(stderr, "Ошибка загрузки BMP изображения: %s\n", input_file);
        fprintf(stderr, "Проверьте, что файл существует и имеет правильный формат\n");
        fprintf(stderr, "Требуется: 24-битный BMP без сжатия (BITMAPINFOHEADER)\n");
        pipeline_destroy(pipeline);
        return 1;
    }
    
    printf("✅ Изображение загружено: %u x %u пикселей\n", 
           image->width, image->height);
    
    // 4. Вывод информации о конвейере фильтров
    if (pipeline->count > 0) {
        pipeline_print(pipeline);
    } else {
        printf("Фильтры не указаны, изображение будет сохранено без изменений\n");
    }
    
    // 5. Применение фильтров
    if (pipeline->count > 0) {
        printf("\nПрименение фильтров...\n");
        
        if (!pipeline_apply(pipeline, image)) {
            fprintf(stderr, "Ошибка применения фильтров\n");
            image_free(image);
            pipeline_destroy(pipeline);
            return 1;
        }
    }
    
    // 6. Сохранение результата
    printf("\nСохранение результата: %s\n", output_file);
    
    if (!bmp_save(output_file, image)) {
        fprintf(stderr, "Ошибка сохранения изображения: %s\n", output_file);
        
        // Попробуем сохранить с другим именем
        char backup_name[256];
        snprintf(backup_name, sizeof(backup_name), "backup_%s", output_file);
        
        fprintf(stderr, "Попытка сохранения как: %s\n", backup_name);
        
        if (!bmp_save(backup_name, image)) {
            fprintf(stderr, "Критическая ошибка: не удалось сохранить изображение\n");
            image_free(image);
            pipeline_destroy(pipeline);
            return 1;
        }
    }
    
    // 7. Освобождение ресурсов
    image_free(image);
    pipeline_destroy(pipeline);
    
    // 8. Завершение работы
    printf("\nОбработка завершена успешно!\n");
    printf("Входной файл:  %s\n", input_file);
    printf("Выходной файл: %s\n", output_file);
    printf("\n");
    
    return 0;
}

// Обработка ошибок и завершение программы

void cleanup(Image* image, FilterPipeline* pipeline) {
    if (image) {
        image_free(image);
    }
    if (pipeline) {
        pipeline_destroy(pipeline);
    }
}
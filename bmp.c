#include "bmp.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Константы для работы с BMP

#define BMP_SIGNATURE 0x4D42        // "BM" в little-endian
#define BMP_HEADER_SIZE 54          // 14 + 40 байт
#define BMP_BITS_PER_PIXEL 24       // 24-битный формат
#define BMP_COMPRESSION_BI_RGB 0    // Без сжатия

// Загрузка BMP изображения

Image* bmp_load(const char* filename) {
    if (!filename) {
        fprintf(stderr, "Ошибка: имя файла не указано\n");
        return NULL;
    }
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла '%s': %s\n", filename, strerror(errno));
        return NULL;
    }
    
    // Чтение заголовков
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    
    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1) {
        fprintf(stderr, "Ошибка чтения BMPFileHeader из '%s'\n", filename);
        fclose(file);
        return NULL;
    }
    
    if (fread(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        fprintf(stderr, "Ошибка чтения BMPInfoHeader из '%s'\n", filename);
        fclose(file);
        return NULL;
    }
    
    // Проверка сигнатуры
    if (file_header.bfType != BMP_SIGNATURE) {
        fprintf(stderr, "Ошибка: файл '%s' не является BMP (сигнатура: 0x%04X)\n", 
                filename, file_header.bfType);
        fclose(file);
        return NULL;
    }
    
    // Проверка формата (должен быть 24-битный без сжатия)
    if (info_header.biBitCount != BMP_BITS_PER_PIXEL) {
        fprintf(stderr, "Ошибка: неподдерживаемый формат BMP (%u бит на пиксель)\n", 
                info_header.biBitCount);
        fprintf(stderr, "Требуется: 24-битный BMP\n");
        fclose(file);
        return NULL;
    }
    
    if (info_header.biCompression != BMP_COMPRESSION_BI_RGB) {
        fprintf(stderr, "Ошибка: BMP файл сжат (сжатие: %u)\n", info_header.biCompression);
        fprintf(stderr, "Требуется: несжатый BMP (BI_RGB)\n");
        fclose(file);
        return NULL;
    }
    
    // Проверка размеров
    if (info_header.biWidth <= 0 || info_header.biHeight == 0) {
        fprintf(stderr, "Ошибка: некорректные размеры BMP: %dx%d\n", 
                info_header.biWidth, info_header.biHeight);
        fclose(file);
        return NULL;
    }
    
    // Высота может быть отрицательной (пиксели сверху вниз)
    uint32_t width = (uint32_t)abs(info_header.biWidth);
    uint32_t height = (uint32_t)abs(info_header.biHeight);
    bool top_down = (info_header.biHeight < 0);  // Отрицательная высота = сверху вниз
    
    // Создание изображения
    Image* image = image_create(width, height);
    if (!image) {
        fclose(file);
        return NULL;
    }
    
    // Вычисление размера строки с учетом выравнивания
    uint32_t row_stride = bmp_row_stride(width);
    uint32_t padding = row_stride - (width * 3);
    
    // Переход к данным пикселей
    if (fseek(file, file_header.bfOffBits, SEEK_SET) != 0) {
        fprintf(stderr, "Ошибка: не удалось перейти к данным пикселей\n");
        image_free(image);
        fclose(file);
        return NULL;
    }
    
    // Выделение буфера для чтения строки
    uint8_t* row_buffer = (uint8_t*)malloc(row_stride);
    if (!row_buffer) {
        fprintf(stderr, "Ошибка выделения памяти для буфера строки\n");
        image_free(image);
        fclose(file);
        return NULL;
    }
    
    // Чтение данных пикселей
    for (uint32_t y = 0; y < height; y++) {
        if (fread(row_buffer, 1, row_stride, file) != row_stride) {
            fprintf(stderr, "Ошибка чтения строки %u из BMP\n", y);
            free(row_buffer);
            image_free(image);
            fclose(file);
            return NULL;
        }
        
        // Определение координаты Y в зависимости от порядка строк
        uint32_t image_y = top_down ? y : (height - 1 - y);
        
        // Преобразование BGR в Color
        for (uint32_t x = 0; x < width; x++) {
            uint8_t b = row_buffer[x * 3 + 0];
            uint8_t g = row_buffer[x * 3 + 1];
            uint8_t r = row_buffer[x * 3 + 2];
            
            Color color = {
                .r = (float)r / 255.0f,
                .g = (float)g / 255.0f,
                .b = (float)b / 255.0f
            };
            
            image_set_pixel(image, x, image_y, color);
        }
    }
    
    free(row_buffer);
    fclose(file);
    
    printf("✅ Загружено BMP: %s (%ux%u, 24-бит)\n", filename, width, height);
    return image;
}

// Сохранение изображения в BMP

bool bmp_save(const char* filename, const Image* image) {
    if (!filename || !image || !image->data) {
        fprintf(stderr, "Ошибка: некорректные параметры для сохранения\n");
        return false;
    }
    
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Ошибка создания файла '%s': %s\n", filename, strerror(errno));
        return false;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Вычисление размера строки с учетом выравнивания
    uint32_t row_stride = bmp_row_stride(width);
    uint32_t padding = row_stride - (width * 3);
    uint32_t image_size = row_stride * height;
    uint32_t file_size = BMP_HEADER_SIZE + image_size;
    
    // Заполнение заголовков
    BMPFileHeader file_header = {
        .bfType = BMP_SIGNATURE,
        .bfSize = file_size,
        .bfReserved1 = 0,
        .bfReserved2 = 0,
        .bfOffBits = BMP_HEADER_SIZE
    };
    
    BMPInfoHeader info_header = {
        .biSize = sizeof(BMPInfoHeader),
        .biWidth = (int32_t)width,
        .biHeight = (int32_t)height,  // Положительное = снизу вверх
        .biPlanes = 1,
        .biBitCount = BMP_BITS_PER_PIXEL,
        .biCompression = BMP_COMPRESSION_BI_RGB,
        .biSizeImage = image_size,
        .biXPelsPerMeter = 0,
        .biYPelsPerMeter = 0,
        .biClrUsed = 0,
        .biClrImportant = 0
    };
    
    // Запись заголовков
    if (fwrite(&file_header, sizeof(BMPFileHeader), 1, file) != 1 ||
        fwrite(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        fprintf(stderr, "Ошибка записи заголовков BMP\n");
        fclose(file);
        return false;
    }
    
    // Выделение буфера для записи строки
    uint8_t* row_buffer = (uint8_t*)malloc(row_stride);
    if (!row_buffer) {
        fprintf(stderr, "Ошибка выделения памяти для буфера строки\n");
        fclose(file);
        return false;
    }
    
    // Заполнение буфера нулями (для padding)
    memset(row_buffer, 0, row_stride);
    
    // Запись данных пикселей (снизу вверх)
    for (uint32_t y = 0; y < height; y++) {
        // Координата Y для BMP (снизу вверх)
        uint32_t image_y = height - 1 - y;
        
        // Заполнение буфера BGR данными
        for (uint32_t x = 0; x < width; x++) {
            const Color* color = image_get_pixel_const(image, x, image_y);
            if (!color) {
                free(row_buffer);
                fclose(file);
                return false;
            }
            
            // Преобразование Color в BGR
            BMPixel pixel = color_to_bmpixel(*color);
            row_buffer[x * 3 + 0] = pixel.b;  // Blue
            row_buffer[x * 3 + 1] = pixel.g;  // Green
            row_buffer[x * 3 + 2] = pixel.r;  // Red
        }
        
        // Запись строки с padding
        if (fwrite(row_buffer, 1, row_stride, file) != row_stride) {
            fprintf(stderr, "Ошибка записи строки %u в BMP\n", y);
            free(row_buffer);
            fclose(file);
            return false;
        }
    }
    
    free(row_buffer);
    fclose(file);
    
    printf("✅ Сохранено BMP: %s (%ux%u, %u байт)\n", filename, width, height, file_size);
    return true;
}

// Проверка формата BMP файла

bool bmp_validate(const char* filename) {
    if (!filename) return false;
    
    FILE* file = fopen(filename, "rb");
    if (!file) return false;
    
    BMPFileHeader file_header;
    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1) {
        fclose(file);
        return false;
    }
    
    fclose(file);
    return (file_header.bfType == BMP_SIGNATURE);
}

// Печать информации о BMP файле

void bmp_print_info(const char* filename) {
    if (!filename) return;
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("Не удалось открыть файл: %s\n", filename);
        return;
    }
    
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    
    if (fread(&file_header, sizeof(BMPFileHeader), 1, file) != 1 ||
        fread(&info_header, sizeof(BMPInfoHeader), 1, file) != 1) {
        printf("Ошибка чтения заголовков BMP\n");
        fclose(file);
        return;
    }
    
    printf("\n📊 Информация о BMP файле: %s\n", filename);
    printf("========================================\n");
    printf("Сигнатура:          0x%04X (%s)\n", 
           file_header.bfType, 
           file_header.bfType == BMP_SIGNATURE ? "корректная" : "некорректная");
    printf("Размер файла:       %u байт\n", file_header.bfSize);
    printf("Смещение данных:    %u байт\n", file_header.bfOffBits);
    printf("Размер заголовка:   %u байт\n", info_header.biSize);
    printf("Размеры:            %dx%d пикселей\n", 
           info_header.biWidth, info_header.biHeight);
    printf("Бит на пиксель:     %u\n", info_header.biBitCount);
    printf("Сжатие:             %s\n", 
           info_header.biCompression == 0 ? "BI_RGB (нет)" : "есть");
    printf("Размер изображения: %u байт\n", info_header.biSizeImage);
    
    if (info_header.biBitCount == 24) {
        uint32_t width = abs(info_header.biWidth);
        uint32_t row_stride = bmp_row_stride(width);
        printf("Строка с padding:   %u байт\n", row_stride);
        printf("Padding на строку:  %u байт\n", row_stride - (width * 3));
    }
    
    printf("========================================\n");
    
    fclose(file);
}
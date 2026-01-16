//BITMAPINFO

#ifndef BMP_H
#define BMP_H

#include "image.h"
#include <stdint.h>
#include <stdbool.h>

// Структуры BMP файла

#pragma pack(push, 1)  // Отключение выравнивания для точного соответствия формату

// BITMAPFILEHEADER (14 байт)
typedef struct {
    uint16_t bfType;          // Сигнатура "BM" (0x4D42)
    uint32_t bfSize;          // Размер файла в байтах
    uint16_t bfReserved1;     // Зарезервировано (0)
    uint16_t bfReserved2;     // Зарезервировано (0)
    uint32_t bfOffBits;       // Смещение до данных пикселей
} BMPFileHeader;

// BITMAPINFOHEADER (40 байт)
typedef struct {
    uint32_t biSize;          // Размер этой структуры (40)
    int32_t  biWidth;         // Ширина изображения в пикселях
    int32_t  biHeight;        // Высота изображения в пикселях (положительное - снизу вверх)
    uint16_t biPlanes;        // Количество плоскостей (1)
    uint16_t biBitCount;      // Бит на пиксель (24)
    uint32_t biCompression;   // Тип сжатия (0 = BI_RGB)
    uint32_t biSizeImage;     // Размер данных изображения в байтах (0 для BI_RGB)
    int32_t  biXPelsPerMeter; // Горизонтальное разрешение (пикселей на метр)
    int32_t  biYPelsPerMeter; // Вертикальное разрешение
    uint32_t biClrUsed;       // Количество используемых цветов в палитре (0)
    uint32_t biClrImportant;  // Количество важных цветов (0)
} BMPInfoHeader;

#pragma pack(pop)

// Функции для работы с BMP

// Загрузка изображения из BMP файла
Image* bmp_load(const char* filename);

// Сохранение изображения в BMP файл
bool bmp_save(const char* filename, const Image* image);

// Проверка формата BMP файла
bool bmp_validate(const char* filename);

// Печать информации о BMP файле (для отладки)
void bmp_print_info(const char* filename);

// Вычисление размера строки с учетом выравнивания
static inline uint32_t bmp_row_stride(uint32_t width) {
    // Выравнивание по 4 байта: (width * 3 + 3) & ~3
    return ((width * 3 + 3) / 4) * 4;
}

#endif 
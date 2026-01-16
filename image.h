#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include <stdbool.h>

// Структура для представления цвета пикселя

#pragma pack(push, 1)
typedef struct {
    float r;  // Красная компонента [0.0, 1.0]
    float g;  // Зеленая компонента [0.0, 1.0]  
    float b;  // Синяя компонента [0.0, 1.0]
} Color;

// Структура для 24-битного BMP пикселя (BGR порядок)
typedef struct {
    uint8_t b;  // Blue (0-255)
    uint8_t g;  // Green (0-255)
    uint8_t r;  // Red (0-255)
} BMPixel;
#pragma pack(pop)

// Структура для представления изображения

typedef struct {
    Color* data;        // Массив пикселей в формате row-major
    uint32_t width;     // Ширина изображения в пикселях
    uint32_t height;    // Высота изображения в пикселях
} Image;

// Вспомогательные функции для работы с цветом

// Создание цвета из компонент
static inline Color color_create(float r, float g, float b) {
    Color c = {r, g, b};
    return c;
}

// Ограничение значения цвета в диапазон [0, 1]
static inline Color color_clamp(Color c) {
    if (c.r < 0.0f) c.r = 0.0f;
    if (c.g < 0.0f) c.g = 0.0f;
    if (c.b < 0.0f) c.b = 0.0f;
    if (c.r > 1.0f) c.r = 1.0f;
    if (c.g > 1.0f) c.g = 1.0f;
    if (c.b > 1.0f) c.b = 1.0f;
    return c;
}

// Преобразование Color в BMPixel (float[0,1] -> uint8_t[0,255])
static inline BMPixel color_to_bmpixel(Color c) {
    c = color_clamp(c);
    BMPixel p;
    p.r = (uint8_t)(c.r * 255.0f);
    p.g = (uint8_t)(c.g * 255.0f);
    p.b = (uint8_t)(c.b * 255.0f);
    return p;
}

// Преобразование BMPixel в Color
static inline Color bmpixel_to_color(BMPixel p) {
    Color c;
    c.r = (float)p.r / 255.0f;
    c.g = (float)p.g / 255.0f;
    c.b = (float)p.b / 255.0f;
    return c;
}

// Сложение цветов (используется в фильтрах)
static inline Color color_add(Color a, Color b) {
    return color_create(a.r + b.r, a.g + b.g, a.b + b.b);
}

// Умножение цвета на скаляр (используется в матричных фильтрах)
static inline Color color_mul(Color c, float scalar) {
    return color_create(c.r * scalar, c.g * scalar, c.b * scalar);
}

// Линейная интерполяция между цветами
static inline Color color_lerp(Color a, Color b, float t) {
    return color_create(
        a.r + (b.r - a.r) * t,
        a.g + (b.g - a.g) * t,
        a.b + (b.b - a.b) * t
    );
}

// Основные функции работы с изображениями

// Создание нового изображения заданного размера
Image* image_create(uint32_t width, uint32_t height);

// Освобождение памяти изображения
void image_free(Image* img);

// Создание глубокой копии изображения
Image* image_copy(const Image* src);

// Получение пикселя по координатам с проверкой границ
Color* image_get_pixel(Image* img, uint32_t x, uint32_t y);
const Color* image_get_pixel_const(const Image* img, uint32_t x, uint32_t y);

// Установка значения пикселя
void image_set_pixel(Image* img, uint32_t x, uint32_t y, Color color);

// Заполнение изображения одним цветом
void image_fill(Image* img, Color color);

// Проверка корректности координат
bool image_coord_valid(const Image* img, uint32_t x, uint32_t y);

// Получение размера изображения
uint32_t image_get_width(const Image* img);
uint32_t image_get_height(const Image* img);

// Создание подизображения (используется в crop)
Image* image_create_subimage(const Image* src, uint32_t x, uint32_t y, 
                            uint32_t width, uint32_t height);

#endif
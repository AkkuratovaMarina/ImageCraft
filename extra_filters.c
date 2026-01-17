#include "extra_filters.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// Вспомогательные функции

// Псевдослучайный генератор на основе координат
float random_in_range(int x, int y, float min, float max) {
    // Используем синус от комбинации координат для псевдослучайности
    float val = sinf(x * 12.9898f + y * 78.233f) * 43758.5453f;
    val = val - floorf(val);  // Дробная часть
    
    return min + val * (max - min);
}

// Билинейная интерполяция
Color bilinear_interpolation(const Image* image, float x, float y) {
    if (!image || !image->data) {
        return color_create(0, 0, 0);
    }
    
    // Проверка границ
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x >= (float)(image->width - 1)) x = (float)(image->width - 1);
    if (y >= (float)(image->height - 1)) y = (float)(image->height - 1);
    
    // Целочисленные координаты
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    // Дробные части
    float dx = x - (float)x0;
    float dy = y - (float)y0;
    
    // Получаем цвета четырех соседних пикселей
    const Color* pc00 = image_get_pixel_const(image, x0, y0);
    const Color* pc10 = image_get_pixel_const(image, x1, y0);
    const Color* pc01 = image_get_pixel_const(image, x0, y1);
    const Color* pc11 = image_get_pixel_const(image, x1, y1);

    // Если пиксель не найден, используем черный цвет
    Color c00 = pc00 ? *pc00 : color_create(0, 0, 0);
    Color c10 = pc10 ? *pc10 : color_create(0, 0, 0);
    Color c01 = pc01 ? *pc01 : color_create(0, 0, 0);
    Color c11 = pc11 ? *pc11 : color_create(0, 0, 0);
    
    // Интерполяция по X
    Color c0, c1;
    c0.r = c00.r + dx * (c10.r - c00.r);
    c0.g = c00.g + dx * (c10.g - c00.g);
    c0.b = c00.b + dx * (c10.b - c00.b);
    
    c1.r = c01.r + dx * (c11.r - c01.r);
    c1.g = c01.g + dx * (c11.g - c01.g);
    c1.b = c01.b + dx * (c11.b - c01.b);
    
    // Интерполяция по Y
    Color result;
    result.r = c0.r + dy * (c1.r - c0.r);
    result.g = c0.g + dy * (c1.g - c0.g);
    result.b = c0.b + dy * (c1.b - c0.b);
    
    return color_clamp(result);
}

// 1. Crystallize фильтр

bool filter_crystallize(Image* image, int cell_size) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка размера ячейки
    if (cell_size < 2) {
        fprintf(stderr, "Ошибка: размер ячейки должен быть не менее 2 пикселей\n");
        return false;
    }
    
    if (cell_size > 100) {
        fprintf(stderr, "Предупреждение: очень большой размер ячейки (%d)\n", cell_size);
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Создаем копию для чтения
    Image* copy = image_copy(image);
    if (!copy) {
        fprintf(stderr, "Ошибка создания копии изображения\n");
        return false;
    }
    
    // Инициализация случайного генератора
    srand((unsigned int)time(NULL));
    
    // Проходим по всем ячейкам
    for (uint32_t cell_y = 0; cell_y < height; cell_y += cell_size) {
        for (uint32_t cell_x = 0; cell_x < width; cell_x += cell_size) {
            // Определяем границы текущей ячейки
            uint32_t cell_end_x = cell_x + cell_size;
            uint32_t cell_end_y = cell_y + cell_size;
            
            if (cell_end_x > width) cell_end_x = width;
            if (cell_end_y > height) cell_end_y = height;
            
            // Выбираем случайную точку внутри ячейки
            uint32_t sample_x = cell_x + (rand() % (cell_end_x - cell_x));
            uint32_t sample_y = cell_y + (rand() % (cell_end_y - cell_y));
            
            // Получаем цвет выбранного пикселя
            const Color* sample_color = image_get_pixel_const(copy, sample_x, sample_y);
            if (!sample_color) {
                sample_color = &(Color){0, 0, 0};
            }
            
            Color cell_color = *sample_color;
            
            // Добавляем небольшой случайный оттенок для разнообразия
            float hue_shift = random_in_range(cell_x, cell_y, -0.05f, 0.05f);
            cell_color.r = fminf(1.0f, fmaxf(0.0f, cell_color.r + hue_shift));
            cell_color.g = fminf(1.0f, fmaxf(0.0f, cell_color.g + hue_shift));
            cell_color.b = fminf(1.0f, fmaxf(0.0f, cell_color.b + hue_shift));
            
            // Заполняем всю ячейку выбранным цветом
            for (uint32_t y = cell_y; y < cell_end_y; y++) {
                for (uint32_t x = cell_x; x < cell_end_x; x++) {
                    // Для пикселей по краям ячейки делаем плавный переход
                    if (x == cell_x || x == cell_end_x - 1 || 
                        y == cell_y || y == cell_end_y - 1) {
                        
                        // Получаем оригинальный цвет
                        const Color* original = image_get_pixel_const(copy, x, y);
                        if (original) {
                            // Смешиваем с цветом ячейки (50/50)
                            Color blended;
                            blended.r = (original->r + cell_color.r) * 0.5f;
                            blended.g = (original->g + cell_color.g) * 0.5f;
                            blended.b = (original->b + cell_color.b) * 0.5f;
                            image_set_pixel(image, x, y, blended);
                        }
                    } else {
                        // Внутренние пиксели - чистый цвет ячейки
                        image_set_pixel(image, x, y, cell_color);
                    }
                }
            }
        }
    }
    
    image_free(copy);
    
    printf("Crystallize: размер ячейки %dpx, изображение %ux%u\n", 
           cell_size, width, height);
    return true;
}

// 2. Glass Distortion фильтр

bool filter_glass_distortion(Image* image, float scale) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка масштаба
    if (scale <= 0.0f) {
        fprintf(stderr, "Ошибка: масштаб должен быть положительным (%.2f)\n", scale);
        return false;
    }
    
    if (scale > 50.0f) {
        fprintf(stderr, "Предупреждение: очень большой масштаб деформации (%.2f)\n", scale);
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Создаем копию для чтения
    Image* copy = image_copy(image);
    if (!copy) {
        fprintf(stderr, "Ошибка создания копии изображения\n");
        return false;
    }
    
    // Параметры деформации
    float frequency = 0.05f;  // Частота синусоидальных волн
    float amplitude = scale;  // Амплитуда деформации
    
    // Применяем деформацию ко всем пикселям
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            // Вычисляем смещение с помощью синусоидальных функций
            // Используем разные частоты для X и Y для более естественного эффекта
            
            float dx = sinf((float)x * frequency) * 
                      cosf((float)y * frequency * 0.7f) * amplitude;
            
            float dy = cosf((float)x * frequency * 0.8f) * 
                      sinf((float)y * frequency * 1.2f) * amplitude;
            
            // Добавляем немного шума для текстуры стекла
            float noise_x = random_in_range(x, y, -amplitude * 0.3f, amplitude * 0.3f);
            float noise_y = random_in_range(y, x, -amplitude * 0.3f, amplitude * 0.3f);
            
            dx += noise_x;
            dy += noise_y;
            
            // Новые координаты с учетом деформации
            float new_x = (float)x + dx;
            float new_y = (float)y + dy;
            
            // Проверяем границы
            if (new_x < 0.0f) new_x = 0.0f;
            if (new_y < 0.0f) new_y = 0.0f;
            if (new_x >= (float)width) new_x = (float)(width - 1);
            if (new_y >= (float)height) new_y = (float)(height - 1);
            
            // Получаем цвет с билинейной интерполяцией для плавности
            Color distorted_color = bilinear_interpolation(copy, new_x, new_y);
            
            // Сохраняем результат
            image_set_pixel(image, x, y, distorted_color);
        }
    }
    
    image_free(copy);
    
    printf("Glass Distortion: масштаб %.2f, размер %ux%u\n", scale, width, height);
    return true;
}
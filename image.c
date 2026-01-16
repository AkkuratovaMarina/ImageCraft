#include "image.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Создание нового изображения

Image* image_create(uint32_t width, uint32_t height) {
    // Проверка корректности размеров
    if (width == 0 || height == 0) {
        fprintf(stderr, "Ошибка: неверные размеры изображения %ux%u\n", width, height);
        return NULL;
    }
    
    // Выделение памяти для структуры изображения
    Image* img = (Image*)malloc(sizeof(Image));
    if (!img) {
        fprintf(stderr, "Ошибка выделения памяти для структуры Image\n");
        return NULL;
    }
    
    // Инициализация размеров
    img->width = width;
    img->height = height;
    
    // Выделение памяти для данных пикселей
    size_t pixel_count = (size_t)width * (size_t)height;
    img->data = (Color*)malloc(pixel_count * sizeof(Color));
    
    if (!img->data) {
        fprintf(stderr, "Ошибка выделения памяти для данных изображения (%zu пикселей)\n", 
                pixel_count);
        free(img);
        return NULL;
    }
    
    // Инициализация всех пикселей черным цветом
    memset(img->data, 0, pixel_count * sizeof(Color));
    
    return img;
}

// Освобождение памяти изображения

void image_free(Image* img) {
    if (img) {
        if (img->data) {
            free(img->data);
        }
        free(img);
    }
}

// Глубокое копирование

Image* image_copy(const Image* src) {
    if (!src || !src->data) {
        fprintf(stderr, "Ошибка: исходное изображение не инициализировано\n");
        return NULL;
    }
    
    // Создание нового изображения такого же размера
    Image* copy = image_create(src->width, src->height);
    if (!copy) {
        return NULL;
    }
    
    // Копирование данных пикселей
    size_t pixel_count = (size_t)src->width * (size_t)src->height;
    memcpy(copy->data, src->data, pixel_count * sizeof(Color));
    
    return copy;
}

// Получение пикселя по координатам

Color* image_get_pixel(Image* img, uint32_t x, uint32_t y) {
    if (!img || !img->data) {
        return NULL;
    }
    
    // Проверка границ изображения
    if (x >= img->width || y >= img->height) {
        return NULL;
    }
    
    // Вычисление индекса пикселя в массиве (row-major)
    size_t index = (size_t)y * (size_t)img->width + (size_t)x;
    return &img->data[index];
}

const Color* image_get_pixel_const(const Image* img, uint32_t x, uint32_t y) {
    // Константная версия функции
    return image_get_pixel((Image*)img, x, y);
}

// Установка значения пикселя

void image_set_pixel(Image* img, uint32_t x, uint32_t y, Color color) {
    Color* pixel = image_get_pixel(img, x, y);
    if (pixel) {
        *pixel = color_clamp(color);
    }
}

// Заполнение изображения одним цветом

void image_fill(Image* img, Color color) {
    if (!img || !img->data) return;
    
    color = color_clamp(color);
    size_t pixel_count = (size_t)img->width * (size_t)img->height;
    
    for (size_t i = 0; i < pixel_count; i++) {
        img->data[i] = color;
    }
}

// Проверка корректности координат

bool image_coord_valid(const Image* img, uint32_t x, uint32_t y) {
    return img && x < img->width && y < img->height;
}

// Получение размеров изображения

uint32_t image_get_width(const Image* img) {
    return img ? img->width : 0;
}

uint32_t image_get_height(const Image* img) {
    return img ? img->height : 0;
}

// Создание подизображения (для crop)

Image* image_create_subimage(const Image* src, uint32_t x, uint32_t y, 
                            uint32_t width, uint32_t height) {
    if (!src || !src->data) {
        fprintf(stderr, "Ошибка: исходное изображение не инициализировано\n");
        return NULL;
    }
    
    // Проверка границ исходного изображения
    if (x >= src->width || y >= src->height) {
        fprintf(stderr, "Ошибка: начальные координаты за пределами изображения\n");
        return NULL;
    }
    
    // Ограничение размеров подизображения размерами исходного
    uint32_t actual_width = width;
    uint32_t actual_height = height;
    
    if (x + width > src->width) {
        actual_width = src->width - x;
        fprintf(stderr, "Предупреждение: ширина crop ограничена до %u\n", actual_width);
    }
    
    if (y + height > src->height) {
        actual_height = src->height - y;
        fprintf(stderr, "Предупреждение: высота crop ограничена до %u\n", actual_height);
    }
    
    // Создание нового изображения
    Image* subimg = image_create(actual_width, actual_height);
    if (!subimg) {
        return NULL;
    }
    
    // Копирование данных из исходного изображения
    for (uint32_t row = 0; row < actual_height; row++) {
        for (uint32_t col = 0; col < actual_width; col++) {
            const Color* src_pixel = image_get_pixel_const(src, x + col, y + row);
            if (src_pixel) {
                image_set_pixel(subimg, col, row, *src_pixel);
            }
        }
    }
    
    return subimg;
}
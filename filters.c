#include "filters.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Вспомогательные функции

Color get_pixel_with_border(const Image* image, int x, int y) {
    // Обработка границ: используем ближайший пиксель
    if (!image || !image->data) {
        return color_create(0, 0, 0);
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Коррекция координат
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= (int)width) x = width - 1;
    if (y >= (int)height) y = height - 1;
    
    const Color* pixel = image_get_pixel_const(image, (uint32_t)x, (uint32_t)y);
    if (pixel) {
        return *pixel;
    }
    
    return color_create(0, 0, 0);
}

int compare_floats(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    
    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

// 1. Crop фильтр

bool filter_crop(Image* image, uint32_t width, uint32_t height) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка размеров
    if (width == 0 || height == 0) {
        fprintf(stderr, "Ошибка: неверные размеры для crop: %ux%u\n", width, height);
        return false;
    }
    
    uint32_t orig_width = image->width;
    uint32_t orig_height = image->height;
    
    // Ограничение размеров до доступных
    uint32_t crop_width = (width > orig_width) ? orig_width : width;
    uint32_t crop_height = (height > orig_height) ? orig_height : height;
    
    if (crop_width == orig_width && crop_height == orig_height) {
        printf("Crop: размеры совпадают, обрезка не требуется\n");
        return true;
    }
    
    // Создание временного изображения для crop
    Image* cropped = image_create_subimage(image, 0, 0, crop_width, crop_height);
    if (!cropped) {
        fprintf(stderr, "Ошибка создания обрезанного изображения\n");
        return false;
    }
    
    // Освобождаем старые данные
    free(image->data);
    
    // Копируем новые данные
    image->data = cropped->data;
    image->width = cropped->width;
    image->height = cropped->height;
    
    // Освобождаем структуру (но данные остаются)
    free(cropped);
    
    printf("Crop: %ux%u -> %ux%u\n", orig_width, orig_height, crop_width, crop_height);
    return true;
}

// 2. Grayscale фильтр

bool filter_grayscale(Image* image) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Коэффициенты для преобразования в оттенки серого
    // Формула: 0.299*R + 0.587*G + 0.114*B
    const float R_COEFF = 0.299f;
    const float G_COEFF = 0.587f;
    const float B_COEFF = 0.114f;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color* pixel = image_get_pixel(image, x, y);
            if (!pixel) continue;
            
            // Вычисление яркости по формуле
            float luminance = 
                pixel->r * R_COEFF + 
                pixel->g * G_COEFF + 
                pixel->b * B_COEFF;
            
            // Присваивание одинакового значения всем каналам
            pixel->r = luminance;
            pixel->g = luminance;
            pixel->b = luminance;
        }
    }
    
    printf("Grayscale: применено к %ux%u пикселей\n", width, height);
    return true;
}

// 3. Negative фильтр

bool filter_negative(Image* image) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color* pixel = image_get_pixel(image, x, y);
            if (!pixel) continue;
            
            // Формула негатива: R' = 1 - R, G' = 1 - G, B' = 1 - B
            pixel->r = 1.0f - pixel->r;
            pixel->g = 1.0f - pixel->g;
            pixel->b = 1.0f - pixel->b;
        }
    }
    
    printf("Negative: применено к %ux%u пикселей\n", width, height);
    return true;
}

// 4. Sharpening фильтр

bool filter_sharpen(Image* image) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Ядро для повышения резкости
    // [ 0 -1  0]
    // [-1  5 -1]
    // [ 0 -1  0]
    const float sharpen_kernel[9] = {
        0.0f, -1.0f,  0.0f,
       -1.0f,  5.0f, -1.0f,
        0.0f, -1.0f,  0.0f
    };
    
    // Применяем свертку
    Image* result = apply_convolution(image, sharpen_kernel, 3);
    if (!result) {
        fprintf(stderr, "Ошибка применения фильтра резкости\n");
        return false;
    }
    
    // Заменяем данные изображения
    free(image->data);
    image->data = result->data;
    image->width = result->width;
    image->height = result->height;
    
    free(result);
    
    printf("Sharpening: применен фильтр повышения резкости\n");
    return true;
}

// 5. Edge Detection фильтр

bool filter_edge_detection(Image* image, float threshold) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка порога
    if (threshold < 0.0f || threshold > 1.0f) {
        fprintf(stderr, "Ошибка: некорректный порог %.2f (должен быть 0.0-1.0)\n", threshold);
        return false;
    }
    
    // 1. Создаем копию для преобразования в grayscale
    Image* grayscale = image_copy(image);
    if (!grayscale) {
        fprintf(stderr, "Ошибка создания копии изображения\n");
        return false;
    }
    
    // 2. Преобразуем в оттенки серого
    filter_grayscale(grayscale);
    
    // 3. Ядро Лапласиана для выделения границ
    // [ 0 -1  0]
    // [-1  4 -1]
    // [ 0 -1  0]
    const float edge_kernel[9] = {
        0.0f, -1.0f,  0.0f,
       -1.0f,  4.0f, -1.0f,
        0.0f, -1.0f,  0.0f
    };
    
    // 4. Применяем свертку
    Image* edges = apply_convolution(grayscale, edge_kernel, 3);
    if (!edges) {
        fprintf(stderr, "Ошибка применения фильтра границ\n");
        image_free(grayscale);
        return false;
    }
    
    // 5. Бинаризация по порогу
    uint32_t width = edges->width;
    uint32_t height = edges->height;
    
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color* pixel = image_get_pixel(edges, x, y);
            if (!pixel) continue;
            
            // Используем только красный канал (все одинаковы после grayscale)
            float value = pixel->r;
            
            // Бинаризация: выше порога -> белый, иначе черный
            if (value > threshold) {
                pixel->r = pixel->g = pixel->b = 1.0f;  // Белый
            } else {
                pixel->r = pixel->g = pixel->b = 0.0f;  // Черный
            }
        }
    }
    
    // 6. Заменяем оригинальное изображение
    free(image->data);
    image->data = edges->data;
    image->width = edges->width;
    image->height = edges->height;
    
    // 7. Освобождаем временные изображения
    image_free(grayscale);
    free(edges);
    
    printf("Edge Detection: порог %.2f, размер %ux%u\n", threshold, width, height);
    return true;
}

// 6. Median Filter

bool filter_median(Image* image, int window) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка размера окна
    if (window <= 0 || window % 2 == 0) {
        fprintf(stderr, "Ошибка: размер окна должен быть положительным нечетным числом\n");
        return false;
    }
    
    if (window == 1) {
        printf("Median Filter: окно размером 1, фильтрация не требуется\n");
        return true;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Создаем копию для чтения
    Image* copy = image_copy(image);
    if (!copy) {
        fprintf(stderr, "Ошибка создания копии изображения\n");
        return false;
    }
    
    int half = window / 2;
    int window_size = window * window;
    
    // Буферы для значений каналов
    float* r_vals = (float*)malloc(window_size * sizeof(float));
    float* g_vals = (float*)malloc(window_size * sizeof(float));
    float* b_vals = (float*)malloc(window_size * sizeof(float));
    
    if (!r_vals || !g_vals || !b_vals) {
        fprintf(stderr, "Ошибка выделения памяти для медианного фильтра\n");
        free(r_vals);
        free(g_vals);
        free(b_vals);
        image_free(copy);
        return false;
    }
    
    // Проходим по всем пикселям изображения
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            int count = 0;
            
            // Собираем значения из окна
            for (int dy = -half; dy <= half; dy++) {
                for (int dx = -half; dx <= half; dx++) {
                    int px = (int)x + dx;
                    int py = (int)y + dy;
                    
                    // Используем обработку границ
                    Color pixel = get_pixel_with_border(copy, px, py);
                    
                    r_vals[count] = pixel.r;
                    g_vals[count] = pixel.g;
                    b_vals[count] = pixel.b;
                    count++;
                }
            }
            
            // Сортируем массивы
            qsort(r_vals, count, sizeof(float), compare_floats);
            qsort(g_vals, count, sizeof(float), compare_floats);
            qsort(b_vals, count, sizeof(float), compare_floats);
            
            // Берем медиану (центральный элемент)
            int median_index = count / 2;
            Color median_color = {
                .r = r_vals[median_index],
                .g = g_vals[median_index],
                .b = b_vals[median_index]
            };
            
            // Устанавливаем медианное значение
            image_set_pixel(image, x, y, median_color);
        }
    }
    
    // Освобождаем память
    free(r_vals);
    free(g_vals);
    free(b_vals);
    image_free(copy);
    
    printf("Median Filter: окно %dx%d, размер %ux%u\n", window, window, width, height);
    return true;
}

// 7. Gaussian Blur фильтр

bool filter_gaussian_blur(Image* image, float sigma) {
    if (!image || !image->data) {
        fprintf(stderr, "Ошибка: изображение не инициализировано\n");
        return false;
    }
    
    // Проверка параметра sigma
    if (sigma <= 0.0f) {
        fprintf(stderr, "Ошибка: sigma должен быть положительным (%.2f)\n", sigma);
        return false;
    }
    
    // Вычисляем размер ядра (правило 3σ)
    int kernel_radius = (int)ceil(3.0f * sigma);
    int kernel_size = kernel_radius * 2 + 1;
    
    // Создаем 1D гауссово ядро
    float* kernel = (float*)malloc(kernel_size * sizeof(float));
    if (!kernel) {
        fprintf(stderr, "Ошибка выделения памяти для гауссова ядра\n");
        return false;
    }
    
    // Вычисляем коэффициенты ядра
    float sum = 0.0f;
    float sigma2 = sigma * sigma;
    float two_sigma2 = 2.0f * sigma2;
    
    for (int i = -kernel_radius; i <= kernel_radius; i++) {
        int index = i + kernel_radius;
        float x = (float)i;
        kernel[index] = exp(-(x * x) / two_sigma2);
        sum += kernel[index];
    }
    
    // Нормализуем ядро
    for (int i = 0; i < kernel_size; i++) {
        kernel[i] /= sum;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    // Создаем временное изображение для горизонтального размытия
    Image* temp = image_create(width, height);
    if (!temp) {
        fprintf(stderr, "Ошибка создания временного изображения\n");
        free(kernel);
        return false;
    }
    
    // 1. Горизонтальное размытие
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color sum_color = {0, 0, 0};
            
            for (int i = -kernel_radius; i <= kernel_radius; i++) {
                int px = (int)x + i;
                int py = (int)y;
                
                Color pixel = get_pixel_with_border(image, px, py);
                float weight = kernel[i + kernel_radius];
                
                sum_color.r += pixel.r * weight;
                sum_color.g += pixel.g * weight;
                sum_color.b += pixel.b * weight;
            }
            
            image_set_pixel(temp, x, y, sum_color);
        }
    }
    
    // 2. Вертикальное размытие (применяем к оригинальному изображению)
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color sum_color = {0, 0, 0};
            
            for (int i = -kernel_radius; i <= kernel_radius; i++) {
                int px = (int)x;
                int py = (int)y + i;
                
                Color pixel = get_pixel_with_border(temp, px, py);
                float weight = kernel[i + kernel_radius];
                
                sum_color.r += pixel.r * weight;
                sum_color.g += pixel.g * weight;
                sum_color.b += pixel.b * weight;
            }
            
            image_set_pixel(image, x, y, sum_color);
        }
    }
    
    // Освобождаем память
    free(kernel);
    image_free(temp);
    
    printf("Gaussian Blur: sigma=%.2f, ядро %dx%d, размер %ux%u\n", 
           sigma, kernel_size, kernel_size, width, height);
    return true;
}

// Функция применения свертки

Image* apply_convolution(const Image* image, const float* kernel, int size) {
    if (!image || !image->data || !kernel || size % 2 == 0) {
        fprintf(stderr, "Ошибка: некорректные параметры для свертки\n");
        return NULL;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    int half = size / 2;
    
    // Создаем новое изображение для результата
    Image* result = image_create(width, height);
    if (!result) {
        return NULL;
    }
    
    // Применяем свертку ко всем пикселям
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            Color sum_color = {0, 0, 0};
            
            // Проходим по ядру свертки
            for (int ky = -half; ky <= half; ky++) {
                for (int kx = -half; kx <= half; kx++) {
                    int px = (int)x + kx;
                    int py = (int)y + ky;
                    
                    // Получаем значение пикселя с учетом границ
                    Color pixel = get_pixel_with_border(image, px, py);
                    
                    // Индекс в ядре
                    int kernel_index = (ky + half) * size + (kx + half);
                    float weight = kernel[kernel_index];
                    
                    // Добавляем взвешенное значение
                    sum_color.r += pixel.r * weight;
                    sum_color.g += pixel.g * weight;
                    sum_color.b += pixel.b * weight;
                }
            }
            
            // Ограничиваем значения и сохраняем результат
            image_set_pixel(result, x, y, sum_color);
        }
    }
    
    return result;
}
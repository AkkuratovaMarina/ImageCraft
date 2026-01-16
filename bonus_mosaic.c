#include "bonus_mosaic.h"
#include "bmp.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Вспомогательные функции

Color compute_average_color(const Image* image, 
                           uint32_t start_x, uint32_t start_y,
                           uint32_t width, uint32_t height) {
    Color avg = {0, 0, 0};
    
    if (!image || !image->data || width == 0 || height == 0) {
        return avg;
    }
    
    // Проверка границ
    if (start_x >= image->width || start_y >= image->height) {
        return avg;
    }
    
    // Коррекция размеров области
    uint32_t end_x = start_x + width;
    uint32_t end_y = start_y + height;
    
    if (end_x > image->width) end_x = image->width;
    if (end_y > image->height) end_y = image->height;
    
    uint32_t actual_width = end_x - start_x;
    uint32_t actual_height = end_y - start_y;
    
    if (actual_width == 0 || actual_height == 0) {
        return avg;
    }
    
    // Суммируем цвета
    float total_pixels = (float)(actual_width * actual_height);
    float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
    
    for (uint32_t y = start_y; y < end_y; y++) {
        for (uint32_t x = start_x; x < end_x; x++) {
            const Color* pixel = image_get_pixel_const(image, x, y);
            if (pixel) {
                sum_r += pixel->r;
                sum_g += pixel->g;
                sum_b += pixel->b;
            }
        }
    }
    
    // Вычисляем среднее
    avg.r = sum_r / total_pixels;
    avg.g = sum_g / total_pixels;
    avg.b = sum_b / total_pixels;
    
    return avg;
}

float color_distance(Color c1, Color c2) {
    // Евклидово расстояние в RGB пространстве
    float dr = c1.r - c2.r;
    float dg = c1.g - c2.g;
    float db = c1.b - c2.b;
    
    return sqrtf(dr * dr + dg * dg + db * db);
}

// Загрузка набора плиток
TileSet* load_tile_set(const char* filename, int tile_size) {
    if (!filename || tile_size <= 0) {
        fprintf(stderr, "Ошибка: некорректные параметры для загрузки плиток\n");
        return NULL;
    }
    
    printf("🔄 Загрузка плиток из: %s (размер: %dx%d)\n", 
           filename, tile_size, tile_size);
    
    // Загружаем изображение с плитками
    Image* tile_image = bmp_load(filename);
    if (!tile_image) {
        fprintf(stderr, "Ошибка загрузки изображения с плитками: %s\n", filename);
        return NULL;
    }
    
    // Проверяем размеры
    if (tile_image->width % tile_size != 0 || tile_image->height % tile_size != 0) {
        fprintf(stderr, "Ошибка: размер изображения с плитками не кратен размеру плитки\n");
        fprintf(stderr, "Размер изображения: %ux%u, размер плитки: %d\n", 
                tile_image->width, tile_image->height, tile_size);
        image_free(tile_image);
        return NULL;
    }
    
    // Вычисляем количество плиток
    int tiles_x = tile_image->width / tile_size;
    int tiles_y = tile_image->height / tile_size;
    int total_tiles = tiles_x * tiles_y;
    
    if (total_tiles == 0) {
        fprintf(stderr, "Ошибка: в изображении нет плиток\n");
        image_free(tile_image);
        return NULL;
    }
    
    printf("Найдено плиток: %d (%d x %d)\n", total_tiles, tiles_x, tiles_y);
    
    // Создаем набор плиток
    TileSet* tile_set = (TileSet*)malloc(sizeof(TileSet));
    if (!tile_set) {
        fprintf(stderr, "Ошибка выделения памяти для набора плиток\n");
        image_free(tile_image);
        return NULL;
    }
    
    tile_set->tiles = (Image**)malloc(total_tiles * sizeof(Image*));
    if (!tile_set->tiles) {
        fprintf(stderr, "Ошибка выделения памяти для массива плиток\n");
        free(tile_set);
        image_free(tile_image);
        return NULL;
    }
    
    tile_set->count = total_tiles;
    tile_set->tile_size = tile_size;
    
    // Вырезаем отдельные плитки
    int tile_index = 0;
    
    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            uint32_t start_x = tx * tile_size;
            uint32_t start_y = ty * tile_size;
            
            // Создаем плитку
            Image* tile = image_create(tile_size, tile_size);
            if (!tile) {
                fprintf(stderr, "Ошибка создания плитки %d\n", tile_index);
                // Освобождаем уже созданные плитки
                for (int i = 0; i < tile_index; i++) {
                    image_free(tile_set->tiles[i]);
                }
                free(tile_set->tiles);
                free(tile_set);
                image_free(tile_image);
                return NULL;
            }
            
            // Копируем данные
            for (uint32_t y = 0; y < (uint32_t)tile_size; y++) {
                for (uint32_t x = 0; x < (uint32_t)tile_size; x++) {
                    const Color* src_pixel = image_get_pixel_const(
                        tile_image, start_x + x, start_y + y);
                    
                    if (src_pixel) {
                        image_set_pixel(tile, x, y, *src_pixel);
                    }
                }
            }
            
            tile_set->tiles[tile_index] = tile;
            tile_index++;
        }
    }
    
    // Освобождаем исходное изображение
    image_free(tile_image);
    
    printf("✅ Загружено %d плиток размером %dx%d\n", 
           total_tiles, tile_size, tile_size);
    
    return tile_set;
}

// Освобождение набора плиток

void free_tile_set(TileSet* tile_set) {
    if (!tile_set) return;
    
    if (tile_set->tiles) {
        for (int i = 0; i < tile_set->count; i++) {
            if (tile_set->tiles[i]) {
                image_free(tile_set->tiles[i]);
            }
        }
        free(tile_set->tiles);
    }
    
    free(tile_set);
}

// Поиск наиболее подходящей плитки

int find_best_tile(const TileSet* tile_set, Color target_color) {
    if (!tile_set || !tile_set->tiles || tile_set->count == 0) {
        return 0;
    }
    
    // Предварительно вычисляем средние цвета всех плиток
    
    int best_index = 0;
    float best_distance = INFINITY;
    
    for (int i = 0; i < tile_set->count; i++) {
        Image* tile = tile_set->tiles[i];
        if (!tile) continue;
        
        // Вычисляем средний цвет плитки
        Color tile_avg = compute_average_color(tile, 0, 0, 
                                              tile->width, tile->height);
        
        // Вычисляем расстояние до целевого цвета
        float distance = color_distance(target_color, tile_avg);
        
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    
    return best_index;
}

// Основная функция фильтра мозаики

bool filter_mosaic(Image* image, int tile_size, const char* tile_file) {
    if (!image || !image->data || !tile_file) {
        fprintf(stderr, "Ошибка: некорректные параметры для мозаики\n");
        return false;
    }
    
    // Проверка размера плитки
    if (tile_size <= 0) {
        fprintf(stderr, "Ошибка: размер плитки должен быть положительным\n");
        return false;
    }
    
    if (tile_size > 100) {
        fprintf(stderr, "Предупреждение: очень большой размер плитки (%d)\n", tile_size);
    }
    
    // Загружаем набор плиток
    TileSet* tile_set = load_tile_set(tile_file, tile_size);
    if (!tile_set) {
        fprintf(stderr, "Ошибка загрузки набора плиток\n");
        return false;
    }
    
    if (tile_set->count == 0) {
        fprintf(stderr, "Ошибка: набор плиток пуст\n");
        free_tile_set(tile_set);
        return false;
    }
    
    uint32_t width = image->width;
    uint32_t height = image->height;
    
    printf("Создание мозаики: %ux%u, плитка %dx%d, всего плиток: %d\n",
           width, height, tile_size, tile_size, tile_set->count);
    
    // Создаем временное изображение для результата
    Image* result = image_create(width, height);
    if (!result) {
        fprintf(stderr, "Ошибка создания временного изображения\n");
        free_tile_set(tile_set);
        return false;
    }
    
    // Проходим по всем плиткам исходного изображения
    int tiles_x = (width + tile_size - 1) / tile_size;  // Округление вверх
    int tiles_y = (height + tile_size - 1) / tile_size;
    
    printf("Требуется плиток: %d x %d = %d\n", tiles_x, tiles_y, tiles_x * tiles_y);
    
    for (int ty = 0; ty < tiles_y; ty++) {
        for (int tx = 0; tx < tiles_x; tx++) {
            // Координаты текущей плитки в исходном изображении
            uint32_t start_x = tx * tile_size;
            uint32_t start_y = ty * tile_size;
            
            // Вычисляем средний цвет текущей области
            Color area_avg = compute_average_color(image, start_x, start_y, 
                                                  tile_size, tile_size);
            
            // Находим наиболее подходящую плитку
            int best_tile_index = find_best_tile(tile_set, area_avg);
            Image* best_tile = tile_set->tiles[best_tile_index];
            
            if (!best_tile) {
                continue;
            }
            
            // Копируем плитку в результат
            uint32_t copy_width = tile_size;
            uint32_t copy_height = tile_size;
            
            // Коррекция для последних плиток (могут быть меньше tile_size)
            if (start_x + copy_width > width) {
                copy_width = width - start_x;
            }
            if (start_y + copy_height > height) {
                copy_height = height - start_y;
            }
            
            for (uint32_t y = 0; y < copy_height; y++) {
                for (uint32_t x = 0; x < copy_width; x++) {
                    const Color* tile_pixel = image_get_pixel_const(
                        best_tile, x % tile_size, y % tile_size);
                    
                    if (tile_pixel) {
                        uint32_t dest_x = start_x + x;
                        uint32_t dest_y = start_y + y;
                        
                        // Смешиваем с оригиналом для плавности (опционально)
                        const Color* original = image_get_pixel_const(
                            image, dest_x, dest_y);
                        
                        if (original) {
                            // Можно добавить смешивание для плавных переходов
                            Color blended;
                            float blend_factor = 0.7f;  // 70% плитка, 30% оригинал
                            
                            blended.r = tile_pixel->r * blend_factor + 
                                       original->r * (1.0f - blend_factor);
                            blended.g = tile_pixel->g * blend_factor + 
                                       original->g * (1.0f - blend_factor);
                            blended.b = tile_pixel->b * blend_factor + 
                                       original->b * (1.0f - blend_factor);
                            
                            image_set_pixel(result, dest_x, dest_y, blended);
                        } else {
                            image_set_pixel(result, dest_x, dest_y, *tile_pixel);
                        }
                    }
                }
            }
        }
        
        // Прогресс
        if (tiles_y > 10 && ty % (tiles_y / 10) == 0) {
            int progress = (ty * 100) / tiles_y;
            printf("Прогресс: %d%%\n", progress);
        }
    }
    
    // Заменяем оригинальное изображение результатом
    free(image->data);
    image->data = result->data;
    image->width = result->width;
    image->height = result->height;
    
    // Освобождаем структуру result (но не данные!)
    free(result);
    
    // Освобождаем набор плиток
    free_tile_set(tile_set);
    
    printf("Мозаика создана: %ux%u, плитка %dx%d\n", 
           width, height, tile_size, tile_size);
    
    return true;
}
//  1. Изображение разбивается на плитки заданного размера
//  2. Для каждой плитки вычисляется средний цвет
//  3. Из набора плиток-картинок выбирается наиболее подходящая по цвету
//  4. Выбранная плитка подставляется вместо исходной

#ifndef BONUS_MOSAIC_H
#define BONUS_MOSAIC_H

#include "image.h"
#include <stdbool.h>

// Структура для хранения плиток

typedef struct {
    Image** tiles;        // Массив изображений-плиток
    int count;           // Количество плиток
    int tile_size;       // Размер плитки (квадратная)
} TileSet;

// Основная функция фильтра мозаики

// image Изображение для обработки
// tile_size Размер плитки (пикселей)
// tile_file Путь к файлу с набором плиток
bool filter_mosaic(Image* image, int tile_size, const char* tile_file);

// Функции работы с наборами плиток

// /**
//  Загрузка набора плиток из BMP файла
//  Предполагается, что все плитки расположены в виде сетки

// filename Путь к файлу с плитками
// tile_size Размер одной плитки

TileSet* load_tile_set(const char* filename, int tile_size);

//Освобождение памяти набора плиток

void free_tile_set(TileSet* tile_set);

//  Поиск наиболее подходящей плитки по цвету

// tile_set Набор плиток
// target_color Целевой цвет
// Возвращает индекс наиболее подходящей плитки

int find_best_tile(const TileSet* tile_set, Color target_color);

// Вспомогательные функции

// Вычисление среднего цвета области изображения

// start_x начальная X-координата
// start_y начальная Y-координата
// width Ширина области
// height Высота области
// Возвращает Средний цвет области

Color compute_average_color(const Image* image, 
                           uint32_t start_x, uint32_t start_y,
                           uint32_t width, uint32_t height);

//Вычисление расстояния между цветами (L2 норма)

float color_distance(Color c1, Color c2);

#endif 
// 1. Crystallize (-crystallize cell_size)
// Эффект кристаллизации, вдохновлен CICrystallize из Apple Core Image

// 2. Glass Distortion (-glass dist_scale)
// Эффект стеклянной деформации, вдохновлен CIGlassDistortion

#ifndef EXTRA_FILTERS_H
#define EXTRA_FILTERS_H

#include "image.h"
#include <stdbool.h>

// 1. Crystallize фильтр

// Изображение разбивается на ячейки заданного размера
// Внутри каждой ячейки все пиксели принимают цвет одного случайно выбранного пикселя из этой ячейки
// Создает эффект мозаики из "кристаллов" разного размера
// image Изображение для обработки
// cell_size Размер ячейки кристаллизации (пикселей)
bool filter_crystallize(Image* image, int cell_size);

// 2. Glass Distortion фильтр

//  Создает иллюзию просмотра изображения через текстурированное стекло

// 1. Для каждого пикселя-источника вычисляются смещенные координаты
// 2. Смещение определяется синусоидальными функциями
// 3. Цвет берется из смещенных координат (билинейная интерполяция)
// image Изображение для обработки
// scale Масштаб деформации (сила эффекта)
bool filter_glass_distortion(Image* image, float scale);

// Вспомогательные функции

float random_in_range(int x, int y, float min, float max);

Color bilinear_interpolation(const Image* image, float x, float y);

#endif 
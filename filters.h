#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"
#include <stdbool.h>

// 1. Crop фильтр

//  Обрезает изображение до заданных размеров
//  Используется верхняя левая часть изображения
//  image Исходное изображение 
//  width Новая ширина
//  height Новая высота
//  * Если запрошенные размеры больше исходных, выдается доступная часть изображения
bool filter_crop(Image* image, uint32_t width, uint32_t height);

// 2. Grayscale фильтр

//  Преобразует изображение в оттенки серого
//  Формула: R' = G' = B' = 0.299*R + 0.587*G + 0.114*B
//  image Изображение для обработки
bool filter_grayscale(Image* image);

// 3. Negative фильтр

//  Преобразует изображение в негативное((
//  Формула: R' = 1 - R, G' = 1 - G, B' = 1 - B

//  image Изображение для обработки
bool filter_negative(Image* image);

// 4. Sharpening фильтр

//  Повышение резкости изображения
//  Использует матрицу свертки 3x3:
//    [ 0 -1  0]
//    [-1  5 -1]
//    [ 0 -1  0]
//  image Изображение для обработки
bool filter_sharpen(Image* image);

// 5. Edge Detection фильтр

//  * Выделение границ на изображении
//  Изображение переводится в оттенки серого
//  Применяется матрица свертки:
//    [ 0 -1  0]
//    [-1  4 -1]
//    [ 0 -1  0]

//  image Изображение для обработки
//  threshold Порог для бинаризации (0.0-1.0)
bool filter_edge_detection(Image* image, float threshold);

// 6. Median Filter

//  Медианный фильтр для устранения шума
//  R' = median_k(R_k), G' = median_k(G_k), B' = median_k(B_k)
//  image Изображение для обработки
//  window Размер окна (нечетное число)
bool filter_median(Image* image, int window);

// 7. Gaussian Blur фильтр

// Гауссово размытие изображения
// C[x0][y0] = ΣΣ C[x][y] * (1/(2πσ²)) * exp(-((x0-x)²+(y0-y)²)/(2σ²))
// image Изображение для обработки
// sigma Сигма гауссова ядра
bool filter_gaussian_blur(Image* image, float sigma);

// Вспомогательные функции для фильтров

//  Применение матрицы свертки к изображению
// kernel Матрица ядра свертки (квадратная, нечетного размера)
Image* apply_convolution(const Image* image, const float* kernel, int size);


// Получение пикселя с обработкой границ
// Если координаты вне границ, возвращается значение ближайшего пикселя
Color get_pixel_with_border(const Image* image, int x, int y);

#endif 
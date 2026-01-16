#ifndef PIPELINE_H
#define PIPELINE_H

#include "image.h"
#include <stdbool.h>

// Типы фильтров

// Основные фильтры 
typedef enum {
    FILTER_CROP,      // -crop width height
    FILTER_GRAYSCALE, // -gs
    FILTER_NEGATIVE,  // -neg
    FILTER_SHARPEN,   // -sharp
    FILTER_EDGE,      // -edge threshold
    FILTER_MEDIAN,    // -med window
    FILTER_BLUR,      // -blur sigma
    
    // Дополнительные фильтры
    FILTER_CRYSTALLIZE, // -crystallize cell_size
    FILTER_GLASS,       // -glass dist_scale
    
    // Мозаика
    FILTER_MOSAIC,      // -mosaic tile_size tile_file
    
    FILTER_COUNT        // Количество фильтров
} FilterType;

// Структура для параметров фильтра

typedef struct FilterParams {
    FilterType type;           // Тип фильтра
    char** args;               // Аргументы фильтра
    int arg_count;             // Количество аргументов
    struct FilterParams* next; // Следующий фильтр в цепочке
} FilterParams;

// Структура конвейера фильтров

typedef struct {
    FilterParams* first;       // Первый фильтр в цепочке
    FilterParams* last;        // Последний фильтр в цепочке
    int count;                 // Количество фильтров
} FilterPipeline;

// Функции работы с конвейером

// Создание пустого конвейера
FilterPipeline* pipeline_create(void);

// Добавление фильтра в конвейер
bool pipeline_add_filter(FilterPipeline* pipeline, 
                        FilterType type, 
                        char** args, 
                        int arg_count);

// Применение всего конвейера к изображению
bool pipeline_apply(FilterPipeline* pipeline, Image* image);

// Очистка конвейера
void pipeline_clear(FilterPipeline* pipeline);

// Уничтожение конвейера
void pipeline_destroy(FilterPipeline* pipeline);

// Печать информации о конвейере
void pipeline_print(const FilterPipeline* pipeline);

// Вспомогательные функции

// Получение имени фильтра по типу
const char* filter_type_to_name(FilterType type);

// Получение типа фильтра по имени
FilterType filter_name_to_type(const char* name);

// Проверка корректности аргументов фильтра
bool validate_filter_args(FilterType type, char** args, int arg_count);

#endif 
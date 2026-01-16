#include "pipeline.h"
#include "filters.h"
#include "extra_filters.h"
#include "bonus_mosaic.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Создание и уничтожение конвейера

FilterPipeline* pipeline_create(void) {
    FilterPipeline* pipeline = (FilterPipeline*)malloc(sizeof(FilterPipeline));
    if (!pipeline) {
        fprintf(stderr, "Ошибка выделения памяти для конвейера\n");
        return NULL;
    }
    
    pipeline->first = NULL;
    pipeline->last = NULL;
    pipeline->count = 0;
    
    return pipeline;
}

void pipeline_destroy(FilterPipeline* pipeline) {
    if (!pipeline) return;
    
    pipeline_clear(pipeline);
    free(pipeline);
}

// Очистка конвейера

void pipeline_clear(FilterPipeline* pipeline) {
    if (!pipeline) return;
    
    FilterParams* current = pipeline->first;
    while (current) {
        FilterParams* next = current->next;
        
        // Освобождение аргументов
        if (current->args) {
            for (int i = 0; i < current->arg_count; i++) {
                if (current->args[i]) {
                    free(current->args[i]);
                }
            }
            free(current->args);
        }
        
        free(current);
        current = next;
    }
    
    pipeline->first = NULL;
    pipeline->last = NULL;
    pipeline->count = 0;
}

// Добавление фильтра в конвейер

bool pipeline_add_filter(FilterPipeline* pipeline, 
                        FilterType type, 
                        char** args, 
                        int arg_count) {
    if (!pipeline) {
        fprintf(stderr, "Ошибка: конвейер не инициализирован\n");
        return false;
    }
    
    // Проверка корректности аргументов
    if (!validate_filter_args(type, args, arg_count)) {
        fprintf(stderr, "Ошибка: некорректные аргументы для фильтра %s\n", 
                filter_type_to_name(type));
        return false;
    }
    
    // Создание новой структуры параметров
    FilterParams* params = (FilterParams*)malloc(sizeof(FilterParams));
    if (!params) {
        fprintf(stderr, "Ошибка выделения памяти для параметров фильтра\n");
        return false;
    }
    
    params->type = type;
    params->arg_count = arg_count;
    params->next = NULL;
    
    // Копирование аргументов
    if (arg_count > 0 && args) {
        params->args = (char**)malloc(arg_count * sizeof(char*));
        if (!params->args) {
            fprintf(stderr, "Ошибка выделения памяти для аргументов фильтра\n");
            free(params);
            return false;
        }
        
        for (int i = 0; i < arg_count; i++) {
            params->args[i] = strdup(args[i]);
            if (!params->args[i]) {
                fprintf(stderr, "Ошибка копирования аргумента %d\n", i);
                // Освобождаем уже выделенную память
                for (int j = 0; j < i; j++) {
                    free(params->args[j]);
                }
                free(params->args);
                free(params);
                return false;
            }
        }
    } else {
        params->args = NULL;
    }
    
    // Добавление в конец цепочки
    if (!pipeline->first) {
        pipeline->first = params;
        pipeline->last = params;
    } else {
        pipeline->last->next = params;
        pipeline->last = params;
    }
    
    pipeline->count++;
    
    printf("✅ Добавлен фильтр: %s (аргументов: %d)\n", 
           filter_type_to_name(type), arg_count);
    
    return true;
}

// Применение конвейера к изображению

bool pipeline_apply(FilterPipeline* pipeline, Image* image) {
    if (!pipeline || !image) {
        fprintf(stderr, "Ошибка: конвейер или изображение не инициализированы\n");
        return false;
    }
    
    if (pipeline->count == 0) {
        printf("Конвейер пуст, изображение не изменено\n");
        return true;
    }
    
    printf("\nНачало обработки изображения (%d фильтров)\n", pipeline->count);
    printf("========================================\n");
    
    FilterParams* current = pipeline->first;
    int step = 1;
    
    while (current) {
        printf("%d. Применение %s... ", step, filter_type_to_name(current->type));
        fflush(stdout);
        
        bool result = false;
        
        // Применение соответствующего фильтра
        switch (current->type) {
            case FILTER_CROP:
                if (current->arg_count >= 2) {
                    int width = atoi(current->args[0]);
                    int height = atoi(current->args[1]);
                    result = filter_crop(image, width, height);
                }
                break;
                
            case FILTER_GRAYSCALE:
                result = filter_grayscale(image);
                break;
                
            case FILTER_NEGATIVE:
                result = filter_negative(image);
                break;
                
            case FILTER_SHARPEN:
                result = filter_sharpen(image);
                break;
                
            case FILTER_EDGE:
                if (current->arg_count >= 1) {
                    float threshold = atof(current->args[0]);
                    result = filter_edge_detection(image, threshold);
                }
                break;
                
            case FILTER_MEDIAN:
                if (current->arg_count >= 1) {
                    int window = atoi(current->args[0]);
                    result = filter_median(image, window);
                }
                break;
                
            case FILTER_BLUR:
                if (current->arg_count >= 1) {
                    float sigma = atof(current->args[0]);
                    result = filter_gaussian_blur(image, sigma);
                }
                break;
                
            case FILTER_CRYSTALLIZE:
                if (current->arg_count >= 1) {
                    int cell_size = atoi(current->args[0]);
                    result = filter_crystallize(image, cell_size);
                }
                break;
                
            case FILTER_GLASS:
                if (current->arg_count >= 1) {
                    float scale = atof(current->args[0]);
                    result = filter_glass_distortion(image, scale);
                }
                break;
                
            case FILTER_MOSAIC:
                if (current->arg_count >= 2) {
                    int tile_size = atoi(current->args[0]);
                    const char* tile_file = current->args[1];
                    result = filter_mosaic(image, tile_size, tile_file);
                }
                break;
                
            default:
                fprintf(stderr, "Ошибка: неизвестный тип фильтра\n");
                break;
        }
        
        if (result) {
            printf("✅\n");
        } else {
            printf("❌\n");
            fprintf(stderr, "Ошибка применения фильтра %s\n", 
                    filter_type_to_name(current->type));
            return false;
        }
        
        current = current->next;
        step++;
    }
    
    printf("========================================\n");
    printf("Обработка завершена успешно!\n\n");
    
    return true;
}

// Печать информации о конвейере

void pipeline_print(const FilterPipeline* pipeline) {
    if (!pipeline) {
        printf("Конвейер не инициализирован\n");
        return;
    }
    
    printf("\nКонвейер фильтров (%d элементов):\n", pipeline->count);
    printf("========================================\n");
    
    FilterParams* current = pipeline->first;
    int index = 1;
    
    while (current) {
        printf("%d. %s", index, filter_type_to_name(current->type));
        
        if (current->arg_count > 0) {
            printf(" [");
            for (int i = 0; i < current->arg_count; i++) {
                printf("%s", current->args[i]);
                if (i < current->arg_count - 1) {
                    printf(", ");
                }
            }
            printf("]");
        }
        
        printf("\n");
        current = current->next;
        index++;
    }
    
    printf("========================================\n");
}

// Преобразование имени фильтра

const char* filter_type_to_name(FilterType type) {
    switch (type) {
        case FILTER_CROP:        return "Crop";
        case FILTER_GRAYSCALE:   return "Grayscale";
        case FILTER_NEGATIVE:    return "Negative";
        case FILTER_SHARPEN:     return "Sharpening";
        case FILTER_EDGE:        return "Edge Detection";
        case FILTER_MEDIAN:      return "Median Filter";
        case FILTER_BLUR:        return "Gaussian Blur";
        case FILTER_CRYSTALLIZE: return "Crystallize";
        case FILTER_GLASS:       return "Glass Distortion";
        case FILTER_MOSAIC:      return "Mosaic";
        default:                 return "Unknown";
    }
}

FilterType filter_name_to_type(const char* name) {
    if (!name) return FILTER_COUNT;
    
    // Приведение к нижнему регистру для сравнения
    char lower_name[32];
    strncpy(lower_name, name, sizeof(lower_name) - 1);
    lower_name[sizeof(lower_name) - 1] = '\0';
    
    for (int i = 0; lower_name[i]; i++) {
        lower_name[i] = tolower(lower_name[i]);
    }
    
    // Удаление префикса "-" если есть
    if (lower_name[0] == '-') {
        memmove(lower_name, lower_name + 1, strlen(lower_name));
    }
    
    // Сопоставление имен
    if (strcmp(lower_name, "crop") == 0)        return FILTER_CROP;
    if (strcmp(lower_name, "gs") == 0)          return FILTER_GRAYSCALE;
    if (strcmp(lower_name, "neg") == 0)         return FILTER_NEGATIVE;
    if (strcmp(lower_name, "sharp") == 0)       return FILTER_SHARPEN;
    if (strcmp(lower_name, "edge") == 0)        return FILTER_EDGE;
    if (strcmp(lower_name, "med") == 0)         return FILTER_MEDIAN;
    if (strcmp(lower_name, "blur") == 0)        return FILTER_BLUR;
    if (strcmp(lower_name, "crystallize") == 0) return FILTER_CRYSTALLIZE;
    if (strcmp(lower_name, "glass") == 0)       return FILTER_GLASS;
    if (strcmp(lower_name, "mosaic") == 0)      return FILTER_MOSAIC;
    
    return FILTER_COUNT;  // Неизвестный фильтр
}

// Проверка корректности аргументов

bool validate_filter_args(FilterType type, char** args, int arg_count) {
    switch (type) {
        case FILTER_CROP:
            // -crop width height
            if (arg_count != 2) {
                fprintf(stderr, "Фильтр Crop требует 2 аргумента (width height)\n");
                return false;
            }
            return (atoi(args[0]) > 0 && atoi(args[1]) > 0);
            
        case FILTER_GRAYSCALE:
        case FILTER_NEGATIVE:
        case FILTER_SHARPEN:
            // Без аргументов
            return (arg_count == 0);
            
        case FILTER_EDGE:
            // -edge threshold
            if (arg_count != 1) {
                fprintf(stderr, "Фильтр Edge Detection требует 1 аргумент (threshold)\n");
                return false;
            }
            return (atof(args[0]) >= 0.0f);
            
        case FILTER_MEDIAN:
            // -med window (нечетное)
            if (arg_count != 1) {
                fprintf(stderr, "Фильтр Median требует 1 аргумент (window size)\n");
                return false;
            }
            {
                int window = atoi(args[0]);
                return (window > 0 && window % 2 == 1);
            }
            
        case FILTER_BLUR:
            // -blur sigma
            if (arg_count != 1) {
                fprintf(stderr, "Фильтр Gaussian Blur требует 1 аргумент (sigma)\n");
                return false;
            }
            return (atof(args[0]) > 0.0f);
            
        case FILTER_CRYSTALLIZE:
            // -crystallize cell_size
            if (arg_count != 1) {
                fprintf(stderr, "Фильтр Crystallize требует 1 аргумент (cell size)\n");
                return false;
            }
            return (atoi(args[0]) > 1);
            
        case FILTER_GLASS:
            // -glass dist_scale
            if (arg_count != 1) {
                fprintf(stderr, "Фильтр Glass Distortion требует 1 аргумент (scale)\n");
                return false;
            }
            return (atof(args[0]) > 0.0f);
        
        case FILTER_MOSAIC:
            // -mosaic tile_size tile_file
            if (arg_count != 2) {
                fprintf(stderr, "Фильтр Mosaic требует 2 аргумента (tile_size tile_file)\n");
                return false;
            }
            return (atoi(args[0]) > 0);
            
        default:
            fprintf(stderr, "Неизвестный тип фильтра\n");
            return false;
    }
}
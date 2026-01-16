#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>

// Текущий уровень логирования
static int current_log_level = LOG_INFO;

// Функции работы с файлами

bool file_exists(const char* filename) {
    if (!filename) return false;
    
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

bool file_writable(const char* filename) {
    if (!filename) return false;
    
    // Проверяем, можем ли мы открыть файл для записи
    FILE* test = fopen(filename, "wb");
    if (test) {
        fclose(test);
        
        // Если файл существовал, проверяем права
        if (file_exists(filename)) {
            // В Windows используем stat
            struct stat buffer;
            if (stat(filename, &buffer) == 0) {
                // Проверяем, можем ли мы писать 
                return true;
            }
        }
        return true;
    }
    
    return false;
}

size_t file_size(const char* filename) {
    if (!filename) return 0;
    
    struct stat buffer;
    if (stat(filename, &buffer) == 0) {
        return (size_t)buffer.st_size;
    }
    
    return 0;
}

// Функции работы со строками

char* string_duplicate(const char* src) {
    if (!src) return NULL;
    
    size_t len = strlen(src) + 1;
    char* dest = (char*)malloc(len);
    
    if (dest) {
        strcpy(dest, src);
    }
    
    return dest;
}

void string_to_lower(char* str) {
    if (!str) return;
    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

void string_trim(char* str) {
    if (!str) return;
    
    // Удаляем пробелы в конце
    char* end = str + strlen(str) - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    
    // Удаляем пробелы в начале
    char* start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    
    // Сдвигаем строку если нужно
    if (start != str) {
        size_t len = strlen(start) + 1;
        memmove(str, start, len);
    }
}

bool is_numeric(const char* str) {
    if (!str || !*str) return false;
    
    // Проверяем первый символ
    if (!isdigit((unsigned char)*str) && *str != '-' && *str != '+' && *str != '.') {
        return false;
    }
    
    // Проверяем остальные символы
    bool has_dot = false;
    const char* p = str;
    
    if (*p == '-' || *p == '+') p++;
    
    while (*p) {
        if (*p == '.') {
            if (has_dot) return false;  // Больше одной точки
            has_dot = true;
        } else if (!isdigit((unsigned char)*p)) {
            return false;
        }
        p++;
    }
    
    return true;
}

// Функции работы с памятью

void* safe_malloc(size_t size, const char* description) {
    if (size == 0) {
        fprintf(stderr, "⚠️  Попытка выделить 0 байт для: %s\n", description);
        return NULL;
    }
    
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "❌ Критическая ошибка: не удалось выделить %zu байт для %s\n", 
                size, description);
        fprintf(stderr, "   Доступная память исчерпана\n");
        exit(EXIT_FAILURE);
    }
    
    // Инициализируем нулями для безопасности
    memset(ptr, 0, size);
    
    return ptr;
}

void* safe_realloc(void* ptr, size_t size, const char* description) {
    if (size == 0) {
        fprintf(stderr, "⚠️  Попытка перевыделить 0 байт для: %s\n", description);
        free(ptr);
        return NULL;
    }
    
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        fprintf(stderr, "❌ Критическая ошибка: не удалось перевыделить %zu байт для %s\n", 
                size, description);
        fprintf(stderr, "   Доступная память исчерпана\n");
        free(ptr);
        exit(EXIT_FAILURE);
    }
    
    return new_ptr;
}

// Математические функции

float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

float lerp(float a, float b, float t) {
    t = clamp_float(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

float degrees_to_radians(float degrees) {
    return degrees * 3.14159265358979323846f / 180.0f;
}

float radians_to_degrees(float radians) {
    return radians * 180.0f / 3.14159265358979323846f;
}

// Функции логирования

void set_log_level(int level) {
    if (level >= LOG_ERROR && level <= LOG_DEBUG) {
        current_log_level = level;
    }
}

void log_message(int level, const char* format, ...) {
    if (level > current_log_level) {
        return;
    }
    
    // Префиксы для разных уровней
    const char* prefixes[] = {
        "[❌ ОШИБКА] ",
        "[⚠️  ПРЕДУПРЕЖДЕНИЕ] ",
        "[ℹ️  ИНФО] ",
        "[🐛 ОТЛАДКА] "
    };
    
    // Выводим префикс
    if (level >= 0 && level <= LOG_DEBUG) {
        printf("%s", prefixes[level]);
    }
    
    // Выводим сообщение
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    // Новая строка
    printf("\n");
}
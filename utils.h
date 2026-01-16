#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

// Функции работы с файлами

// Проверка существования файла
bool file_exists(const char* filename);

// Проверка возможности записи в файл
bool file_writable(const char* filename);

// Получение размера файла в байтах
size_t file_size(const char* filename);

// Функции работы со строками

// Копирование строки с выделением памяти
char* string_duplicate(const char* src);

// Преобразование строки в нижний регистр
void string_to_lower(char* str);

// Обрезка пробелов в начале и конце строки
void string_trim(char* str);

// Проверка, является ли строка числом
bool is_numeric(const char* str);

// Функции работы с памятью

// Безопасное выделение памяти с проверкой
void* safe_malloc(size_t size, const char* description);

// Безопасное перевыделение памяти
// ptr исходный указатель
// size новый размер
// description описание для сообщения об ошибке
// Возвращает указатель на перевыделенную память или NULL

void* safe_realloc(void* ptr, size_t size, const char* description);

// Математические функции

// Ограничение значения в заданном диапазоне
// value значение для ограничения
// min минимальное значение
// max максимальное значение
float clamp_float(float value, float min, float max);

// Линейная интерполяция между двумя значениями
// a начальное значение
// b конечное значение
// t коэффициент интерполяции [0, 1]
float lerp(float a, float b, float t);

//Преобразование градусов в радианы
float degrees_to_radians(float degrees);

//Преобразование радиан в градусы

float radians_to_degrees(float radians);

// Функции логирования и отладки

// Установка уровня логирования
void set_log_level(int level);

// Логирование сообщения
// level Уровень важности (0-3)
// format Формат сообщения (как printf)
void log_message(int level, const char* format, ...);

// Уровни логирования
#define LOG_ERROR   0
#define LOG_WARN    1
#define LOG_INFO    2
#define LOG_DEBUG   3

#endif 
# Компилятор и флаги
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -O2

# Имя исполняемого файла
TARGET = image_craft

# Все .c файлы
SOURCES = bmp.c \
          bonus_mosaic.c \
          extra_filters.c \
          filters.c \
          image.c \
          main.c \
          pipeline.c \
          utils.c

# Объектные файлы (.o)
OBJECTS = $(SOURCES:.c=.o)

# Правило по умолчанию
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

# Компиляция .c → .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Зависимости от заголовочных файлов
$(OBJECTS): bmp.h bonus_mosaic.h extra_filters.h filters.h image.h pipeline.h utils.h

# Очистка
.PHONY: clean all
clean:
	rm -f $(OBJECTS) $(TARGET)
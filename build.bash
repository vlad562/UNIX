#!/bin/sh
# Включаем режим немедленного завершения при любой неошибочной команде
set -e

# --- Функции обработки ошибок ---
exceptions() {
    echo "Ошибка: $1" >&2
    # Если код возврата передан — используем его, иначе по умолчанию 1
    exit "${2:-1}"
}

# --- Функция очистки временных файлов ---
cleanup() {
    # Проверяем, существует ли каталог, чтобы не вызывать лишних ошибок
    if [ -n "$BUILD_DIR" ] && [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}

# --- Проверка аргументов ---
# Защищает как от отсутствия аргументов, так и от передачи пустой строки
if [ -z "$1" ]; then
    exceptions "Укажите исходный файл. Использование: $0 <исход_файл>" 1
fi

SRC_FILE="$1"

# Проверяем, что файл действительно существует и это обычный файл
if [ ! -f "$SRC_FILE" ]; then
    exceptions "Файл '$SRC_FILE' не найден или не существует." 1
fi

# Получаем абсолютные пути и расширение файла
SRC_DIR=$(cd "$(dirname "$SRC_FILE")" && pwd)
SRC_BASE=$(basename "$SRC_FILE")
EXT="${SRC_BASE##*.}"

# --- Шаг 1: Поиск директивы Output: ---
OUTPUT_NAME=$(sed -n 's/.*Output:[[:space:]]*\([^[:space:]]*\).*/\1/p' "$SRC_FILE" | head -n 1)

if [ -z "$OUTPUT_NAME" ]; then
    exceptions "В файле не найдена директива 'Output: имя_файла'." 2
fi

# --- Шаг 2: Создание временного каталога ---
BUILD_DIR=$(mktemp -d 2>/dev/null || mktemp -d -t 'build')
if [ -z "$BUILD_DIR" ] || [ ! -d "$BUILD_DIR" ]; then
    exceptions "Не удалось создать временный каталог." 3
fi

# --- Шаг 3: Перехват сигналов (Trap) ---
# Сигналы: 0 (EXIT - нормальный выход), 1 (HUP), 2 (INT/Ctrl+C), 3 (QUIT), 15 (TERM)
trap 'cleanup' 0 1 2 3 15

# --- Шаг 4: Подготовка сборки ---
# Копируем исходник во временный каталог, чтобы промежуточные файлы создавались там
cp "$SRC_FILE" "$BUILD_DIR/"
cd "$BUILD_DIR"

# Переменная для фиксации ошибки компиляции
COMPILE_STATUS=0

# --- Шаг 5: Компиляция в зависимости от расширения ---
case "$EXT" in
    c)
        gcc -O2 "$SRC_BASE" -o "$OUTPUT_NAME" || COMPILE_STATUS=$?
        ;;
    cpp|cc|cxx)
        g++ -O2 "$SRC_BASE" -o "$OUTPUT_NAME" || COMPILE_STATUS=$?
        ;;
    tex)
        pdflatex -interaction=batchmode "$SRC_BASE" >/dev/null 2>&1 || true
        
        TEX_OUT="${SRC_BASE%.*}.pdf"
        if [ $COMPILE_STATUS -eq 0 ] && [ -f "$TEX_OUT" ] && [ "$TEX_OUT" != "$OUTPUT_NAME" ]; then
            mv "$TEX_OUT" "$OUTPUT_NAME" || COMPILE_STATUS=$?
        fi
        ;;
    *)
        exceptions "Поддерживаются только расширения .c, .cpp, .cc, .cxx и .tex" 4
        ;;
esac

# --- Шаг 6: Проверка результата и перенос файла ---
if [ $COMPILE_STATUS -ne 0 ] || [ ! -f "$OUTPUT_NAME" ]; then
    exceptions "Сборка завершилась неудачей (Код компилятора: $COMPILE_STATUS)." 5
fi

# Возвращаем готовый файл в директорию к исходнику
mv "$OUTPUT_NAME" "$SRC_DIR/"

echo "Успешно собрано и сохранено: $SRC_DIR/$OUTPUT_NAME"

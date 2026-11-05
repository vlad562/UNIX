#!/bin/sh
VOLUME_DIR="/data"
LOCK_FILE="$VOLUME_DIR/lockfile"

# Функция обработки исключений
exceptions(){
    echo "Ошибка: $1" >&2
    exit "${2:-1}"
}

cleanUp() {
    if [ -n "$current_file" ]; then
        rm -f "$current_file"
        echo "Удален файл $current_file."
    fi
    exit 0  
}

# Проверка на существование каталога
[ -d "$VOLUME_DIR" ] || exceptions "каталог $VOLUME_DIR не существует"

# Уникальный ID контейнера
id_container=$(cat /proc/sys/kernel/random/uuid)
[ -n "$id_container" ] || exceptions "не удалось получить UUID контейнера"

counter_files=0
current_file=""

trap cleanUp EXIT HUP INT TERM

echo "Контейнер $id_container запущен."

while true; do
    # Счётчик созданных файлов
    counter_files=$((counter_files + 1))

    # Открываем lock_file как дескриптор 8
    exec 8>"$LOCK_FILE" || exceptions "Не удается открыть файл $LOCK_FILE"
    flock -x 8

    i=1
    while true; do
        NUM=$(printf '%03d' "$i")
        FILE_PATH="$VOLUME_DIR/$NUM"

       if [ ! -e "$FILE_PATH" ]; then
            echo "$id_container $counter_files" > "$FILE_PATH"
            current_file="$FILE_PATH"
            break
        fi
        i=$((i + 1))
    done

    # Закрываем дескриптор и снимаем блокировку
    exec 8>&-

    echo "Создан: $current_file"

    sleep 1

    rm -f "$current_file"
    echo "Удалён: $current_file"
    current_file=""

    sleep 1
done
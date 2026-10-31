#!/bin/sh
set -e

VOLUME_DIR="/data"
LOCK_FILE="$VOLUME_DIR/.lock"

CONTAINER_ID=$(dd if=/dev/urandom bs=16 count=1 2>/dev/null | od -An -tx1 | tr -d ' \n' | cut -c1-8)

local_file_counter=0

echo "Контейнер запущен успешно. Уникальный ID: $CONTAINER_ID"

while true; do
    created_file_name=""

    exec 9>>"$LOCK_FILE"
    flock 9

    i=1
    while [ $i -le 999 ]; do
        formatted_name=$(printf "%03d" $i)
        target_path="$VOLUME_DIR/$formatted_name"

        if [ ! -e "$target_path" ]; then
            local_file_counter=$((local_file_counter + 1))

            echo "$CONTAINER_ID $local_file_counter" > "$target_path"

            created_file_name="$formatted_name"
            break
        fi
        i=$((i + 1))
    done

    exec 9>&-

    if [ -z "$created_file_name" ]; then
        echo "Критическая ошибка: закончились свободные имена файлов!"
        sleep 1
        continue
    fi

    sleep 1

    if [ -f "$VOLUME_DIR/$created_file_name" ]; then
        rm -f "$VOLUME_DIR/$created_file_name"
    fi

    sleep 1
done

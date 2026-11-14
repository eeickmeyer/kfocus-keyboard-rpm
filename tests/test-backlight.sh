#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "Usage: sh $0 PATH_TO_PATCHED_SOURCE" >&2
    exit 2
fi

source_file=$1/src/clevo_keyboard.h
test_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -f -- "$tmp/backlight-functions.h" "$tmp/test-backlight"; rmdir -- "$tmp"' EXIT

for function in \
    'static enum led_brightness clevo_kbd_brightness_get(' \
    'static void clevo_kbd_brightness_notify(' \
    'static int clevo_set_brightness(' \
    'static int set_brightness(' \
    'static int clevo_kbd_brightness_set(' \
    'static int set_enabled_cmd(' \
    'static int set_enabled(' \
    'static int set_zonekb_color(' \
    'void clevo_keyboard_event_callb(u32 event)$'
do
    sed -n "\|^$function|,/^}/p" "$source_file" >> "$tmp/backlight-functions.h"
done

cc -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
    -I"$tmp" "$test_dir/test-backlight.c" -o "$tmp/test-backlight"
"$tmp/test-backlight"

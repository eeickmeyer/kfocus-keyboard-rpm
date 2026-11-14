#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

typedef uint8_t u8;
typedef uint32_t u32;
enum led_brightness { LED_OFF = 0 };
struct led_classdev { int unused; };
static struct led_classdev clevo_kbd_backlight;
static int clevo_kbd_backlight_lock;
static bool clevo_kbd_backlight_registered;
static u8 clevo_backlight_type;
static u8 clevo_white_max = 2;
static struct {
	u8 brightness, enabled, has_zonekb;
	struct { u32 left, center, right, numpad, extra; } color;
} kbd_led_state;

#define KEYBOARD_BRIGHTNESS 0xF4000000
#define CLEVO_METHOD_ID_SET_KB_LEDS 0x67
#define CLEVO_METHOD_GET_WHITE_BRIGHTNESS 0x3d
#define CLEVO_METHOD_SET_WHITE_BRIGHTNESS 0x27
#define CLEVO_METHOD_GET_BIOS_FEATURES_1 0x52
#define CLEVO_METHOD_GET_BIOS_FEATURES_2 0x7a
#define CLEVO_BACKLIGHT_FIXED_COLOR 1
#define CLEVO_BACKLIGHT_3_ZONE_RGB 2
#define CLEVO_FEATURE_WHITE_KEYBOARD 0x40000000
#define CLEVO_FEATURE_3_ZONE_RGB_KEYBOARD 0x00400000
#define CLEVO_FEATURE_WHITE_FIVE_LEVELS 0x4000
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 255
#define CLEVO_EVENT_DECREASE_BACKLIGHT 0x81
#define CLEVO_EVENT_INCREASE_BACKLIGHT 0x82
#define CLEVO_EVENT_NEXT_BLINKING_PATTERN 0x83
#define CLEVO_EVENT_CYCLE_BRIGHTNESS 0x8A
#define CLEVO_EVENT_TOGGLE_STATE 0x9F
#define READ_ONCE(value) (value)
#define DIV_ROUND_CLOSEST(value, divisor) (((value) + (divisor) / 2) / (divisor))
#define TUXEDO_INFO(...) ((void)0)
#define TUXEDO_ERROR(...) ((void)0)
#define TUXEDO_DEBUG(...) ((void)0)
#define mutex_lock(lock) ((void)(lock))
#define mutex_unlock(lock) ((void)(lock))
#define msleep(ms) ((void)(ms))

static int firmware_error, notifications, last_notification, reports;
static u32 last_command, last_method, white_level, features1, features2;
static int specs_error, specs_calls;
static u8 specs_type;
static int input_device;
static struct test_driver { int *input_device; } driver = { &input_device };
static struct test_driver *current_driver = &driver;

static int clevo_evaluate_method(u8 method, u32 command, u32 *result)
{
	last_method = method;
	last_command = command;
	if (!firmware_error && method == CLEVO_METHOD_GET_WHITE_BRIGHTNESS)
		*result = white_level;
	if (!firmware_error && method == CLEVO_METHOD_SET_WHITE_BRIGHTNESS)
		white_level = command;
	if (!firmware_error && method == CLEVO_METHOD_GET_BIOS_FEATURES_1)
		*result = features1;
	if (!firmware_error && method == CLEVO_METHOD_GET_BIOS_FEATURES_2)
		*result = features2;
	return firmware_error;
}

static int get_specs(u8 *type)
{
	specs_calls++;
	if (!specs_error)
		*type = specs_type;
	return specs_error;
}

static struct test_interface { int (*get_specs)(u8 *type); } interface = { get_specs };
static struct test_interface *active_clevo_interface = &interface;
static void led_classdev_notify_brightness_hw_changed(struct led_classdev *led,
						     unsigned int brightness)
{
	notifications++;
	last_notification = brightness;
}

static int set_zonekb_color_base(u32 left, u32 center, u32 right,
				 u32 numpad, u32 lightbar, u8 brightness)
{
	return firmware_error;
}

static void set_next_color_whole_kb(void) {}
static bool sparse_keymap_report_known_event(int *input, u32 event, int value, bool autorelease)
{
	reports++;
	return true;
}

#include "backlight-functions.h"

int main(void)
{
	kbd_led_state.brightness = 100;
	kbd_led_state.enabled = 1;
	assert(clevo_kbd_brightness_get(NULL) == 100);
	assert(set_brightness(75) == 0 && notifications == 0);

	clevo_kbd_backlight_registered = true;
	assert(set_brightness(125) == 0);
	assert(last_command == (KEYBOARD_BRIGHTNESS | 125));
	assert(notifications == 1 && last_notification == 125);
	assert(set_enabled(0) == 0);
	assert(kbd_led_state.brightness == 125 && clevo_kbd_brightness_get(NULL) == 0);
	assert(last_command == KEYBOARD_BRIGHTNESS && last_notification == 0);
	assert(set_enabled(1) == 0);
	assert(last_command == (KEYBOARD_BRIGHTNESS | 125) && last_notification == 125);

	firmware_error = -5;
	assert(clevo_kbd_brightness_set(NULL, 50) == -5);
	assert(set_brightness(50) == -5 && set_enabled(0) == -5);
	assert(kbd_led_state.brightness == 125 && kbd_led_state.enabled == 1);
	assert(notifications == 3);
	assert(set_zonekb_color(1, 2, 3, 4, 5, 0) == -5);
	assert(kbd_led_state.brightness == 125);

	firmware_error = 0;
	assert(clevo_kbd_brightness_set(NULL, 0) == 0);
	assert(clevo_kbd_brightness_get(NULL) == 0 && notifications == 3);
	assert(clevo_kbd_brightness_set(NULL, 50) == 0);
	assert(clevo_kbd_brightness_get(NULL) == 50 && notifications == 3);
	assert(set_zonekb_color(1, 2, 3, 4, 5, 0) == 0);
	assert(kbd_led_state.enabled == 0 && last_notification == 0);
	assert(set_zonekb_color(1, 2, 3, 4, 5, 100) == 0);
	assert(kbd_led_state.enabled == 1 && last_notification == 100);

	clevo_keyboard_event_callb(CLEVO_EVENT_INCREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 125 && last_notification == 125 && reports == 0);
	clevo_keyboard_event_callb(CLEVO_EVENT_DECREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 100 && reports == 0);
	clevo_keyboard_event_callb(CLEVO_EVENT_TOGGLE_STATE);
	assert(clevo_kbd_brightness_get(NULL) == 0 && reports == 0);
	clevo_keyboard_event_callb(CLEVO_EVENT_CYCLE_BRIGHTNESS);
	assert(reports == 1);
	assert(set_brightness(255) == 0);
	clevo_keyboard_event_callb(CLEVO_EVENT_INCREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 255 && reports == 1);
	assert(set_brightness(0) == 0);
	clevo_keyboard_event_callb(CLEVO_EVENT_DECREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 0 && reports == 1);
	assert(set_brightness(100) == 0);
	firmware_error = -5;
	clevo_keyboard_event_callb(CLEVO_EVENT_INCREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 100 && reports == 1 && last_notification == 100);
	firmware_error = 0;
	clevo_kbd_backlight_registered = false;
	clevo_keyboard_event_callb(CLEVO_EVENT_INCREASE_BACKLIGHT);
	assert(kbd_led_state.brightness == 125 && reports == 2);

	clevo_backlight_type = 1;
	clevo_kbd_backlight_registered = true;
	white_level = 1;
	assert(clevo_white_brightness_read() == 0);
	assert(clevo_kbd_brightness_get(NULL) == 1);
	assert(clevo_kbd_brightness_set(NULL, 2) == 0);
	assert(last_method == CLEVO_METHOD_SET_WHITE_BRIGHTNESS && white_level == 2);
	assert(clevo_kbd_brightness_get(NULL) == 2);
	white_level = 0;
	clevo_keyboard_event_callb(CLEVO_EVENT_CYCLE_BRIGHTNESS);
	assert(last_notification == 0 && reports == 2);
	white_level = 1;
	clevo_keyboard_event_callb(CLEVO_EVENT_CYCLE_BRIGHTNESS);
	assert(last_notification == 1 && reports == 2);
	assert(set_brightness(255) == 0 && white_level == 2);
	clevo_white_max = 5;
	assert(clevo_kbd_brightness_set(NULL, 3) == 0 && white_level == 3);
	white_level = 6;
	assert(clevo_white_brightness_read() == -ERANGE);
	firmware_error = -5;
	assert(clevo_kbd_brightness_set(NULL, 1) == -5);

	firmware_error = 0;
	specs_type = 1;
	features2 = CLEVO_FEATURE_WHITE_FIVE_LEVELS;
	assert(clevo_detect_backlight() == 0);
	assert(clevo_backlight_type == 1 && clevo_white_max == 5);
	specs_type = 2;
	assert(clevo_detect_backlight() == 0 && clevo_backlight_type == 2);
	specs_error = -5;
	features1 = CLEVO_FEATURE_WHITE_KEYBOARD;
	features2 = 0;
	assert(clevo_detect_backlight() == 0);
	assert(clevo_backlight_type == 1 && clevo_white_max == 2);
	specs_error = 0;
	specs_type = 0;
	specs_calls = 0;
	features1 = CLEVO_FEATURE_3_ZONE_RGB_KEYBOARD;
	assert(clevo_detect_backlight() == 0);
	assert(clevo_backlight_type == 2 && specs_calls == 3);
	firmware_error = -5;
	assert(clevo_detect_backlight() == -5 && clevo_backlight_type == 0);

	puts("Keyboard backlight tests passed");
	return 0;
}

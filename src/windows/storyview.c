#include <pebble.h>
#include "storyview.h"
#include "../libs/pebble-assist.h"
#include "../common.h"

static void back_single_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static Window *window;
static ScrollLayer *scroll_layer;
static TextLayer *title_layer;
static TextLayer *summary_layer;

void storyview_init(char *title, char *summary) {
	window = window_create();

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	GRect max_text_bounds = GRect(2, 0, bounds.size.w - 4, 2000);

	scroll_layer = scroll_layer_create(bounds);
	scroll_layer_set_click_config_onto_window(scroll_layer, window);
	scroll_layer_set_callbacks(scroll_layer, (ScrollLayerCallbacks) {
		.click_config_provider = click_config_provider,
	});

	title_layer = text_layer_create(max_text_bounds);
	text_layer_set_font(title_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text(title_layer, title);

	GSize title_max_size = text_layer_get_content_size(title_layer);
	text_layer_set_size(title_layer, GSize(title_max_size.w, title_max_size.h + 14));

	summary_layer = text_layer_create(GRect(2, title_max_size.h + 14, max_text_bounds.size.w, max_text_bounds.size.h - (title_max_size.h + 14)));
	text_layer_set_font(summary_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
	text_layer_set_text(summary_layer, summary);

	GSize summary_max_size = text_layer_get_content_size(summary_layer);
	text_layer_set_size(summary_layer, GSize(summary_max_size.w, summary_max_size.h + 14));

	scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, title_max_size.h + 14 + summary_max_size.h + 8));

	scroll_layer_add_child(scroll_layer, text_layer_get_layer(title_layer));
	scroll_layer_add_child(scroll_layer, text_layer_get_layer(summary_layer));

	layer_add_child(window_layer, scroll_layer_get_layer(scroll_layer));

	window_stack_push(window, true);
}

void storyview_destroy(void) {
	text_layer_destroy_safe(title_layer);
	text_layer_destroy_safe(summary_layer);
	scroll_layer_destroy_safe(scroll_layer);
	window_destroy_safe(window);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
	storyview_destroy();
	window_stack_pop(true);
}

static void click_config_provider(void *context) {
	window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);
}


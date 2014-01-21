#include <pebble.h>
#include "storylist.h"
#include "../libs/pebble-assist.h"
#include "../common.h"
#include "storyview.h"

#define MAX_STORIES 30

static HNStory stories[MAX_STORIES];

static uint16_t endpoint;
static int num_stories;
static char summary[2048];
static char error[24];

static void refresh_list();
static void request_data();
static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context);
static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context);
static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context);
static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);
static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context);

static Window *window;
static MenuLayer *menu_layer;

void storylist_init(int end_point) {
	endpoint = end_point;

	window = window_create();

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	window_stack_push(window, true);

	refresh_list();
}

void storylist_destroy(void) {
	storyview_destroy();
	layer_remove_from_parent(menu_layer_get_layer(menu_layer));
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

void storylist_in_received_handler(DictionaryIterator *iter) {
	Tuple *index_tuple = dict_find(iter, HN_KEY_INDEX);
	Tuple *title_tuple = dict_find(iter, HN_KEY_TITLE);
	Tuple *subtitle_tuple = dict_find(iter, HN_KEY_SUBTITLE);
	Tuple *summary_tuple = dict_find(iter, HN_KEY_SUMMARY);
	Tuple *error_tuple = dict_find(iter, HN_KEY_ERROR);

	if (index_tuple && title_tuple && subtitle_tuple) {
		HNStory story;
		story.index = index_tuple->value->int16;
		strncpy(story.title, title_tuple->value->cstring, sizeof(story.title));
		strncpy(story.subtitle, subtitle_tuple->value->cstring, sizeof(story.subtitle));
		stories[story.index] = story;
		num_stories++;
		menu_layer_reload_data_and_mark_dirty(menu_layer);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received story [%d] %s - %s", story.index, story.title, story.subtitle);
	}
	else if (title_tuple && !summary_tuple) {
		strncpy(error, title_tuple->value->cstring, sizeof(error));
		menu_layer_reload_data_and_mark_dirty(menu_layer);
	}

	if (summary_tuple) {
		if (title_tuple) {
			storyview_init(title_tuple->value->cstring, summary);
		} else {
			strcat(summary, summary_tuple->value->cstring);
		}
	}
}

bool storylist_is_on_top() {
	return window == window_stack_get_top_window();
}

uint16_t storylist_current_endpoint() {
	return endpoint;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - //

static void refresh_list() {
	memset(stories, 0x0, sizeof(stories));
	num_stories = 0;
	error[0] = '\0';
	menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
	request_data();
}

static void request_data() {
	Tuplet endpoint_tuple = TupletInteger(HN_KEY_ENDPOINT, endpoint);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) {
		return;
	}

	dict_write_tuplet(iter, &endpoint_tuple);
	dict_write_end(iter);

	app_message_outbox_send();
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return (num_stories) ? num_stories : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return MENU_CELL_BASIC_CELL_HEIGHT;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	switch (endpoint) {
		case ENDPOINT_TOP:
			menu_cell_basic_header_draw(ctx, cell_layer, "Hacker News - Top Stories");
			break;
		case ENDPOINT_NEW:
			menu_cell_basic_header_draw(ctx, cell_layer, "Hacker News - New Stories");
			break;
		case ENDPOINT_BST:
			menu_cell_basic_header_draw(ctx, cell_layer, "Hacker News - Best Stories");
			break;
		case ENDPOINT_ASK:
			menu_cell_basic_header_draw(ctx, cell_layer, "Hacker News - Ask HN");
			break;
	}
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	if (strlen(error) != 0) {
		menu_cell_basic_draw(ctx, cell_layer, "Error!", error, NULL);
	} else if (num_stories == 0) {
		menu_cell_basic_draw(ctx, cell_layer, "Loading...", NULL, NULL);
	} else {
		menu_cell_basic_draw(ctx, cell_layer, stories[cell_index->row].title, stories[cell_index->row].subtitle, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_stories == 0) {
		return;
	}

	Tuplet summary_tuple = TupletInteger(HN_KEY_SUMMARY, 1);
	Tuplet endpoint_tuple = TupletInteger(HN_KEY_ENDPOINT, endpoint);
	Tuplet index_tuple = TupletInteger(HN_KEY_INDEX, cell_index->row);

	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);

	if (iter == NULL) {
		return;
	}

	dict_write_tuplet(iter, &summary_tuple);
	dict_write_tuplet(iter, &endpoint_tuple);
	dict_write_tuplet(iter, &index_tuple);
	dict_write_end(iter);

	app_message_outbox_send();

	summary[0] = '\0';
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	refresh_list();
}

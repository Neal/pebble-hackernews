#include <pebble.h>
#include "common.h"
#include "appmessage.h"
#include "libs/pebble-assist.h"
#include "windows/storylist.h"

#define MENU_NUM_SECTIONS 1

#define MENU_SECTION_HOME 0

#define MENU_SECTION_ROWS_HOME 4

#define MENU_ROW_HOME_FRONTPAGE 0
#define MENU_ROW_HOME_NEWPOSTS 1
#define MENU_ROW_HOME_BESTPOSTS 2
#define MENU_ROW_HOME_SETTINGS 3

static Window *window;

static MenuLayer *menu_layer;

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return MENU_NUM_SECTIONS;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	switch (section_index) {
		case MENU_SECTION_HOME:
			return MENU_SECTION_ROWS_HOME;
			break;
	}
	return 0;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	return 36;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	menu_cell_basic_header_draw(ctx, cell_layer, "Hacker News");
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	char label[12] = "";
	switch (cell_index->section) {
		case MENU_SECTION_HOME:
			switch (cell_index->row) {
				case MENU_ROW_HOME_FRONTPAGE:
					strcpy(label, "Front Page");
					break;
				case MENU_ROW_HOME_NEWPOSTS:
					strcpy(label, "New Posts");
					break;
				case MENU_ROW_HOME_BESTPOSTS:
					strcpy(label, "Best Posts");
					break;
				case MENU_ROW_HOME_SETTINGS:
					strcpy(label, "Settings");
					break;
			}
			break;
	}
	graphics_context_set_text_color(ctx, GColorBlack);
	graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), (GRect) { .origin = { 4, 0 }, .size = { PEBBLE_WIDTH - 8, 24 } }, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	switch (cell_index->section) {
		case MENU_SECTION_HOME:
			switch (cell_index->row) {
				case MENU_ROW_HOME_FRONTPAGE:
					storylist_show(ENDPOINT_FRONTPAGE);
					break;
				case MENU_ROW_HOME_NEWPOSTS:
					storylist_show(ENDPOINT_NEWPOSTS);
					break;
				case MENU_ROW_HOME_BESTPOSTS:
					storylist_show(ENDPOINT_BESTPOSTS);
					break;
				case MENU_ROW_HOME_SETTINGS:
					break;
			}
			break;
	}
}

static void init(void) {
	appmessage_init();

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
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	window_stack_push(window, true /* animated */);

	storylist_init();
}

static void deinit(void) {
	storylist_destroy();
	layer_remove_from_parent(menu_layer_get_layer(menu_layer));
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}

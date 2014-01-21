#pragma once

#define ENDPOINT_TOP 0
#define ENDPOINT_NEW 1
#define ENDPOINT_BST 2
#define ENDPOINT_ASK 3

typedef struct {
	int index;
	char title[24];
	char subtitle[30];
} HNStory;

enum {
	HN_KEY_ENDPOINT = 0x0,
	HN_KEY_INDEX = 0x1,
	HN_KEY_TITLE = 0x2,
	HN_KEY_SUBTITLE = 0x3,
	HN_KEY_SUMMARY = 0x4,
	HN_KEY_ERROR = 0x5,
};

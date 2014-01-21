#pragma once

void storylist_init(int end_point);
void storylist_destroy(void);
void storylist_in_received_handler(DictionaryIterator *iter);
bool storylist_is_on_top();
uint16_t storylist_current_endpoint();

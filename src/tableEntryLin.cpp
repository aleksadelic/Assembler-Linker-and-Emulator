#include "../inc/tableEntryLin.h"

int Symbol::next_id = 1;
int Section::next_id = 1;
int Section::next_id_combined = 1;

extern void increment_location_counter();

void Section::addData(char c) {
  data.push_back(c);
}

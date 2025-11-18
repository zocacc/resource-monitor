#ifndef UTILS_H
#define UTILS_H

#include "monitor.h"

void export_to_json(const ResourceData *data, int count, const char *filename);
void export_to_csv(const ResourceData *data, int count, const char *filename);

#endif // UTILS_H

#ifndef LOGGER_H
#define LOGGER_H

#include "order.h"

int log_order(const Order *order);
int log_event(const Order *order, const char *event);

#endif

/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus-print-message.h  Utility function to print out a message
 *
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2003 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef DBUS_PRINT_MESSAGE_H
#define DBUS_PRINT_MESSAGE_H

#include <stdio.h>
#include <string.h>
#include <dbus/dbus.h>
#include <pulsecore/log.h>

#if __STDC_VERSION__ >= 199901L

#define LOG_FUNC(suffix, level) \
PA_GCC_UNUSED static void fnc_pa_log_##suffix(const char *format, ...) { \
    va_list ap; \
    va_start(ap, format); \
    pa_log_levelv_meta(level, NULL, 0, NULL, format, ap); \
    va_end(ap); \
}

LOG_FUNC(debug, PA_LOG_DEBUG);
LOG_FUNC(info, PA_LOG_INFO);
LOG_FUNC(notice, PA_LOG_NOTICE);
LOG_FUNC(warn, PA_LOG_WARN);
LOG_FUNC(error, PA_LOG_ERROR);

#endif


typedef void (*printer_fnc_t)(const char *format, ...);

void print_iter (printer_fnc_t printer, DBusMessageIter *iter, dbus_bool_t literal, int depth);
PA_GCC_UNUSED void print_message (DBusMessage *message, dbus_bool_t literal);
PA_GCC_UNUSED void print_message_printer (printer_fnc_t printer, DBusMessage *message, dbus_bool_t literal);

#endif /* DBUS_PRINT_MESSAGE_H */

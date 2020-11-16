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

#include <config.h>
#include "dbus-print-message.h"

#include <stdlib.h>

static const char*
type_to_name (int message_type)
{
  switch (message_type)
    {
    case DBUS_MESSAGE_TYPE_SIGNAL:
      return "signal";
    case DBUS_MESSAGE_TYPE_METHOD_CALL:
      return "method call";
    case DBUS_MESSAGE_TYPE_METHOD_RETURN:
      return "method return";
    case DBUS_MESSAGE_TYPE_ERROR:
      return "error";
    default:
      return "(unknown message type)";
    }
}

#define INDENT 3

static void
indent (printer_fnc_t printer, int depth)
{
  while (depth-- > 0)
    printer ("   "); /* INDENT spaces. */
}

static void
print_hex (printer_fnc_t printer, unsigned char *bytes, unsigned int len, int depth)
{
  unsigned int i, columns;

  printer ("array of bytes [\n");

  indent (printer, depth + 1);

  /* Each byte takes 3 cells (two hexits, and a space), except the last one. */
  columns = (80 - ((depth + 1) * INDENT)) / 3;

  if (columns < 8)
    columns = 8;

  i = 0;

  while (i < len)
    {
      printer ("%02x", bytes[i]);
      i++;

      if (i != len)
        {
          if (i % columns == 0)
            {
              printer ("\n");
              indent (printer, depth + 1);
            }
          else
            {
              printer (" ");
            }
        }
    }

  printer ("\n");
  indent (printer, depth);
  printer ("]\n");
}

#define DEFAULT_SIZE 100

static void
print_ay (printer_fnc_t printer, DBusMessageIter *iter, int depth)
{
  /* Not using DBusString because it's not public API. It's 2009, and I'm
   * manually growing a string chunk by chunk.
   */
  unsigned char *bytes = malloc (DEFAULT_SIZE + 1);
  unsigned int len = 0;
  unsigned int max = DEFAULT_SIZE;
  dbus_bool_t all_ascii = TRUE;
  int current_type;

  while ((current_type = dbus_message_iter_get_arg_type (iter))
          != DBUS_TYPE_INVALID)
    {
      unsigned char val;

      dbus_message_iter_get_basic (iter, &val);
      bytes[len] = val;
      len++;

      if (val < 32 || val > 126)
        all_ascii = FALSE;

      if (len == max)
        {
          max *= 2;
          bytes = realloc (bytes, max + 1);
        }

      dbus_message_iter_next (iter);
    }

  if (all_ascii)
    {
      bytes[len] = '\0';
      printer ("array of bytes \"%s\"\n", bytes);
    }
  else
    {
      print_hex (printer, bytes, len, depth);
    }

  free (bytes);
}

void
print_iter (printer_fnc_t printer, DBusMessageIter *iter, dbus_bool_t literal, int depth)
{
  do
    {
      int type = dbus_message_iter_get_arg_type (iter);

      if (type == DBUS_TYPE_INVALID)
	break;

      indent(printer, depth);

      switch (type)
	{
	case DBUS_TYPE_STRING:
	  {
	    char *val;
	    dbus_message_iter_get_basic (iter, &val);
	    if (!literal)
	      printer ("string \"");
	    printer ("%s", val);
	    if (!literal)
	      printer ("\"\n");
	    break;
	  }

	case DBUS_TYPE_SIGNATURE:
	  {
	    char *val;
	    dbus_message_iter_get_basic (iter, &val);
	    if (!literal)
	      printer ("signature \"");
	    printer ("%s", val);
	    if (!literal)
	      printer ("\"\n");
	    break;
	  }

	case DBUS_TYPE_OBJECT_PATH:
	  {
	    char *val;
	    dbus_message_iter_get_basic (iter, &val);
	    if (!literal)
	      printer ("object path \"");
	    printer ("%s", val);
	    if (!literal)
	      printer ("\"\n");
	    break;
	  }

	case DBUS_TYPE_INT16:
	  {
	    dbus_int16_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("int16 %d\n", val);
	    break;
	  }

	case DBUS_TYPE_UINT16:
	  {
	    dbus_uint16_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("uint16 %u\n", val);
	    break;
	  }

	case DBUS_TYPE_INT32:
	  {
	    dbus_int32_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("int32 %d\n", val);
	    break;
	  }

	case DBUS_TYPE_UINT32:
	  {
	    dbus_uint32_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("uint32 %u\n", val);
	    break;
	  }

	case DBUS_TYPE_INT64:
	  {
	    dbus_int64_t val;
	    dbus_message_iter_get_basic (iter, &val);
#ifdef DBUS_INT64_PRINTF_MODIFIER
        printer ("int64 %" DBUS_INT64_PRINTF_MODIFIER "d\n", val);
#else
        printer ("int64 (omitted)\n");
#endif
	    break;
	  }

	case DBUS_TYPE_UINT64:
	  {
	    dbus_uint64_t val;
	    dbus_message_iter_get_basic (iter, &val);
#ifdef DBUS_INT64_PRINTF_MODIFIER
        printer ("uint64 %" DBUS_INT64_PRINTF_MODIFIER "u\n", val);
#else
        printer ("uint64 (omitted)\n");
#endif
	    break;
	  }

	case DBUS_TYPE_DOUBLE:
	  {
	    double val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("double %g\n", val);
	    break;
	  }

	case DBUS_TYPE_BYTE:
	  {
	    unsigned char val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("byte %d\n", val);
	    break;
	  }

	case DBUS_TYPE_BOOLEAN:
	  {
	    dbus_bool_t val;
	    dbus_message_iter_get_basic (iter, &val);
	    printer ("boolean %s\n", val ? "true" : "false");
	    break;
	  }

	case DBUS_TYPE_VARIANT:
	  {
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    printer ("variant ");
	    print_iter (printer, &subiter, literal, depth+1);
	    break;
	  }
	case DBUS_TYPE_ARRAY:
	  {
	    int current_type;
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    current_type = dbus_message_iter_get_arg_type (&subiter);

	    if (current_type == DBUS_TYPE_BYTE)
	      {
		print_ay (printer, &subiter, depth);
		break;
	      }

	    printer("array [\n");
	    while (current_type != DBUS_TYPE_INVALID)
	      {
		print_iter (printer, &subiter, literal, depth+1);

		dbus_message_iter_next (&subiter);
		current_type = dbus_message_iter_get_arg_type (&subiter);

		if (current_type != DBUS_TYPE_INVALID)
		  printer (",");
	      }
	    indent(printer, depth);
	    printer("]\n");
	    break;
	  }
	case DBUS_TYPE_DICT_ENTRY:
	  {
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    printer("dict entry(\n");
	    print_iter (printer, &subiter, literal, depth+1);
	    dbus_message_iter_next (&subiter);
	    print_iter (printer, &subiter, literal, depth+1);
	    indent(printer, depth);
	    printer(")\n");
	    break;
	  }

	case DBUS_TYPE_STRUCT:
	  {
	    int current_type;
	    DBusMessageIter subiter;

	    dbus_message_iter_recurse (iter, &subiter);

	    printer("struct {\n");
	    while ((current_type = dbus_message_iter_get_arg_type (&subiter)) != DBUS_TYPE_INVALID)
	      {
		print_iter (printer, &subiter, literal, depth+1);
		dbus_message_iter_next (&subiter);
		if (dbus_message_iter_get_arg_type (&subiter) != DBUS_TYPE_INVALID)
		  printer (",");
	      }
	    indent(printer, depth);
	    printer("}\n");
	    break;
	  }

	default:
	  printer (" (dbus-monitor too dumb to decipher arg type '%c')\n", type);
	  break;
	}
    } while (dbus_message_iter_next (iter));
}

void
print_message (DBusMessage *message, dbus_bool_t literal)
{
    print_message_printer(&fnc_pa_log_error, message, literal);
}

void print_message_printer (printer_fnc_t printer, DBusMessage *message, dbus_bool_t literal)
{
    DBusMessageIter iter;
    const char *sender;
    const char *destination;
    int message_type;

    message_type = dbus_message_get_type (message);
    sender = dbus_message_get_sender (message);
    destination = dbus_message_get_destination (message);

    if (!literal)
    {
        printer ("%s sender=%s -> dest=%s",
            type_to_name (message_type),
            sender ? sender : "(null sender)",
            destination ? destination : "(null destination)");

        switch (message_type)
        {
            case DBUS_MESSAGE_TYPE_METHOD_CALL:
            case DBUS_MESSAGE_TYPE_SIGNAL:
                printer (" serial=%u path=%s; interface=%s; member=%s\n",
                    dbus_message_get_serial (message),
                    dbus_message_get_path (message),
                    dbus_message_get_interface (message),
                    dbus_message_get_member (message));
                break;

            case DBUS_MESSAGE_TYPE_METHOD_RETURN:
                printer (" reply_serial=%u\n",
                    dbus_message_get_reply_serial (message));
                break;

            case DBUS_MESSAGE_TYPE_ERROR:
                printer (" error_name=%s reply_serial=%u\n",
                    dbus_message_get_error_name (message),
                    dbus_message_get_reply_serial (message));
                break;

            default:
                printf ("\n");
                break;
        }
    }

    dbus_message_iter_init (message, &iter);
    print_iter (printer, &iter, literal, 1);
}

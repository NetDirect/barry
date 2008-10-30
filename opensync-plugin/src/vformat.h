/*
 * Copyright (C) 2003 Ximian, Inc. 2005 Armin Bauer
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * Author: Chris Toshok (toshok@ximian.com)
 * Author: Armin Bauer (armin.bauer@opensync.org)
 * 
 */

#ifndef _VFORMAT_H
#define _VFORMAT_H

#include <glib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VFORMAT_CARD_21,
	VFORMAT_CARD_30,
	VFORMAT_NOTE,
	VFORMAT_EVENT_10,
	VFORMAT_EVENT_20,
	VFORMAT_TODO_10,
	VFORMAT_TODO_20
} b_VFormatType;

typedef struct b_VFormat {
	//b_VFormatType type;
	GList *attributes;
} b_VFormat;

#define CRLF "\r\n"

typedef enum {
	VF_ENCODING_RAW,    /* no encoding */
	VF_ENCODING_BASE64, /* base64 */
	VF_ENCODING_QP,     /* quoted-printable */
	VF_ENCODING_8BIT
} b_VFormatEncoding;

typedef struct b_VFormatAttribute {
	char  *block;       /* "vtimezone/standard", or "vevent", depending on
				current begin/end location... may be null */
	char  *group;
	char  *name;
	GList *params; /* b_VFormatParam */
	GList *values;
	GList *decoded_values;
	b_VFormatEncoding encoding;
	gboolean encoding_set;
} b_VFormatAttribute;

typedef struct b_VFormatParam {
	char     *name;
	GList    *values;  /* GList of char*'s*/
} b_VFormatParam;


/*b_VFormat *vcard_new(b_VFormatType type);
b_VFormat *vcard_new_from_string (const char *str, b_VFormatType type);
//char *vcard_to_string(VFormat *card, VFormatType format);

VFormat *vnote_new(void);
VFormat *vnote_new_from_string(const char *str);
//char *vnote_to_string(VFormat *note);


VFormat *vevent_new(void);
VFormat *vevent_new_from_string(const char *str);
//char *vevent_to_string(VFormat *event);

VFormat *vtodo_new(void);
VFormat *vtodo_new_from_string(const char *str);*/
//char *vtodo_to_string(VFormat *todo);

/* mostly for debugging */
b_VFormat *b_vformat_new(void);
b_VFormat *b_vformat_new_from_string(const char *str);
void b_vformat_dump_structure(b_VFormat *format);
char *b_vformat_to_string(b_VFormat *evc, b_VFormatType type);
time_t b_vformat_time_to_unix(const char *inptime);
void b_vformat_free(b_VFormat *format);

/* attributes */
b_VFormatAttribute *b_vformat_attribute_new               (const char *attr_group, const char *attr_name);
void             b_vformat_attribute_free              (b_VFormatAttribute *attr);
b_VFormatAttribute *b_vformat_attribute_copy              (b_VFormatAttribute *attr);
void             b_vformat_remove_attributes           (b_VFormat *vformat, const char *attr_group, const char *attr_name);
void             b_vformat_remove_attribute            (b_VFormat *vformat, b_VFormatAttribute *attr);
void             b_vformat_add_attribute               (b_VFormat *vformat, b_VFormatAttribute *attr);
void             b_vformat_add_attribute_with_value    (b_VFormat *vformat, b_VFormatAttribute *attr, const char *value);
void             b_vformat_add_attribute_with_values   (b_VFormat *vformat, b_VFormatAttribute *attr, ...);
void             b_vformat_attribute_add_value         (b_VFormatAttribute *attr, const char *value);
void             b_vformat_attribute_set_value         (b_VFormatAttribute *attr, int nth, const char *value);
void             b_vformat_attribute_add_value_decoded (b_VFormatAttribute *attr, const char *value, int len);
void             b_vformat_attribute_add_values        (b_VFormatAttribute *attr, ...);
void             b_vformat_attribute_remove_values     (b_VFormatAttribute *attr);
void             b_vformat_attribute_remove_params     (b_VFormatAttribute *attr);
b_VFormatAttribute *b_vformat_find_attribute             (b_VFormat *evc, const char *name, int nth, const char *block);

/* attribute parameters */
b_VFormatParam* b_vformat_attribute_param_new             (const char *param_name);
void                  b_vformat_attribute_param_free            (b_VFormatParam *param);
b_VFormatParam* b_vformat_attribute_param_copy            (b_VFormatParam *param);
void                  b_vformat_attribute_add_param             (b_VFormatAttribute *attr, b_VFormatParam *param);
b_VFormatParam *b_vformat_attribute_find_param(b_VFormatAttribute *attr, const char *name, int level);
void                  b_vformat_attribute_add_param_with_value  (b_VFormatAttribute *attr, const char *name, const char *value);
void                  b_vformat_attribute_add_param_with_values (b_VFormatAttribute *attr,
							       b_VFormatParam *param, ...);

void                  b_vformat_attribute_param_add_value       (b_VFormatParam *param,
							       const char *value);
void                  b_vformat_attribute_param_add_values      (b_VFormatParam *param,
							       ...);
void                  b_vformat_attribute_param_remove_values   (b_VFormatParam *param);
gboolean b_vformat_attribute_has_param(b_VFormatAttribute *attr, const char *name);

/* b_VFormat* accessors.  nothing returned from these functions should be
   freed by the caller. */
GList*           b_vformat_get_attributes       (b_VFormat *vformat);
const char*      b_vformat_attribute_get_group  (b_VFormatAttribute *attr);
const char*      b_vformat_attribute_get_name   (b_VFormatAttribute *attr);
const char*      b_vformat_attribute_get_block  (b_VFormatAttribute *attr);
GList*           b_vformat_attribute_get_values (b_VFormatAttribute *attr);  /* GList elements are of type char* */
GList*           b_vformat_attribute_get_values_decoded (b_VFormatAttribute *attr); /* GList elements are of type GString* */
const char *b_vformat_attribute_get_nth_value(b_VFormatAttribute *attr, int nth);

/* special accessors for single valued attributes */
gboolean              b_vformat_attribute_is_single_valued      (b_VFormatAttribute *attr);
char*                 b_vformat_attribute_get_value             (b_VFormatAttribute *attr);
GString*              b_vformat_attribute_get_value_decoded     (b_VFormatAttribute *attr);

GList*           b_vformat_attribute_get_params       (b_VFormatAttribute *attr);
const char*      b_vformat_attribute_param_get_name   (b_VFormatParam *param);
GList*           b_vformat_attribute_param_get_values (b_VFormatParam *param);
const char *b_vformat_attribute_param_get_nth_value(b_VFormatParam *param, int nth);

/* special TYPE= parameter predicate (checks for TYPE=@typestr */
gboolean         b_vformat_attribute_has_type         (b_VFormatAttribute *attr, const char *typestr);

/* Utility functions. */
char*            b_vformat_escape_string (const char *str, b_VFormatType type);
char*            b_vformat_unescape_string (const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _VFORMAT_H */

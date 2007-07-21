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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
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
} VFormatType;

typedef struct VFormat {
	//VFormatType type;
	GList *attributes;
} VFormat;

#define CRLF "\r\n"

typedef enum {
	VF_ENCODING_RAW,    /* no encoding */
	VF_ENCODING_BASE64, /* base64 */
	VF_ENCODING_QP,     /* quoted-printable */
	VF_ENCODING_8BIT
} VFormatEncoding;

typedef struct VFormatAttribute {
	char  *group;
	char  *name;
	GList *params; /* VFormatParam */
	GList *values;
	GList *decoded_values;
	VFormatEncoding encoding;
	gboolean encoding_set;
} VFormatAttribute;

typedef struct VFormatParam {
	char     *name;
	GList    *values;  /* GList of char*'s*/
} VFormatParam;


/*VFormat *vcard_new(VFormatType type);
VFormat *vcard_new_from_string (const char *str, VFormatType type);
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
VFormat *vformat_new(void);
VFormat *vformat_new_from_string(const char *str);
void vformat_dump_structure(VFormat *format);
char *vformat_to_string(VFormat *evc, VFormatType type);
time_t vformat_time_to_unix(const char *inptime);
void vformat_free(VFormat *format);

/* attributes */
VFormatAttribute *vformat_attribute_new               (const char *attr_group, const char *attr_name);
void             vformat_attribute_free              (VFormatAttribute *attr);
VFormatAttribute *vformat_attribute_copy              (VFormatAttribute *attr);
void             vformat_remove_attributes           (VFormat *vformat, const char *attr_group, const char *attr_name);
void             vformat_remove_attribute            (VFormat *vformat, VFormatAttribute *attr);
void             vformat_add_attribute               (VFormat *vformat, VFormatAttribute *attr);
void             vformat_add_attribute_with_value    (VFormat *vformat, VFormatAttribute *attr, const char *value);
void             vformat_add_attribute_with_values   (VFormat *vformat, VFormatAttribute *attr, ...);
void             vformat_attribute_add_value         (VFormatAttribute *attr, const char *value);
void             vformat_attribute_set_value         (VFormatAttribute *attr, int nth, const char *value);
void             vformat_attribute_add_value_decoded (VFormatAttribute *attr, const char *value, int len);
void             vformat_attribute_add_values        (VFormatAttribute *attr, ...);
void             vformat_attribute_remove_values     (VFormatAttribute *attr);
void             vformat_attribute_remove_params     (VFormatAttribute *attr);
VFormatAttribute *vformat_find_attribute             (VFormat *evc, const char *name, int nth);

/* attribute parameters */
VFormatParam* vformat_attribute_param_new             (const char *param_name);
void                  vformat_attribute_param_free            (VFormatParam *param);
VFormatParam* vformat_attribute_param_copy            (VFormatParam *param);
void                  vformat_attribute_add_param             (VFormatAttribute *attr, VFormatParam *param);
VFormatParam *vformat_attribute_find_param(VFormatAttribute *attr, const char *name);
void                  vformat_attribute_add_param_with_value  (VFormatAttribute *attr, const char *name, const char *value);
void                  vformat_attribute_add_param_with_values (VFormatAttribute *attr,
							       VFormatParam *param, ...);

void                  vformat_attribute_param_add_value       (VFormatParam *param,
							       const char *value);
void                  vformat_attribute_param_add_values      (VFormatParam *param,
							       ...);
void                  vformat_attribute_param_remove_values   (VFormatParam *param);
gboolean vformat_attribute_has_param(VFormatAttribute *attr, const char *name);

/* VFormat* accessors.  nothing returned from these functions should be
   freed by the caller. */
GList*           vformat_get_attributes       (VFormat *vformat);
const char*      vformat_attribute_get_group  (VFormatAttribute *attr);
const char*      vformat_attribute_get_name   (VFormatAttribute *attr);
GList*           vformat_attribute_get_values (VFormatAttribute *attr);  /* GList elements are of type char* */
GList*           vformat_attribute_get_values_decoded (VFormatAttribute *attr); /* GList elements are of type GString* */
const char *vformat_attribute_get_nth_value(VFormatAttribute *attr, int nth);

/* special accessors for single valued attributes */
gboolean              vformat_attribute_is_single_valued      (VFormatAttribute *attr);
char*                 vformat_attribute_get_value             (VFormatAttribute *attr);
GString*              vformat_attribute_get_value_decoded     (VFormatAttribute *attr);

GList*           vformat_attribute_get_params       (VFormatAttribute *attr);
const char*      vformat_attribute_param_get_name   (VFormatParam *param);
GList*           vformat_attribute_param_get_values (VFormatParam *param);
const char *vformat_attribute_param_get_nth_value(VFormatParam *param, int nth);

/* special TYPE= parameter predicate (checks for TYPE=@typestr */
gboolean         vformat_attribute_has_type         (VFormatAttribute *attr, const char *typestr);

/* Utility functions. */
char*            vformat_escape_string (const char *str, VFormatType type);
char*            vformat_unescape_string (const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _VFORMAT_H */

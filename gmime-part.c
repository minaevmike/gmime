/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@helixcode.com>
 *
 *  Copyright 2000 Helix Code, Inc. (www.helixcode.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */


#include "gmime-part.h"
#include "gmime-utils.h"
#include <config.h>
#include <string.h>
#include <ctype.h>


/**
 * g_mime_part_new: Create a new MIME Part object
 *
 * Returns an empty MIME Part object with a default content-type of
 * text/plain.
 **/
GMimePart *
g_mime_part_new ()
{
	GMimePart *mime_part;
	
	mime_part = g_new0 (GMimePart, 1);
	
	/* lets set the default content type just in case the
	 * programmer forgets to set it ;-) */
	mime_part->mime_type = g_mime_content_type_new ("text", "plain");
	
	return mime_part;
}


/**
 * g_mime_part_new_with_type: Create a new MIME Part with a sepcified type
 * @type: content-type
 * @subtype: content-subtype
 *
 * Returns an empty MIME Part object with the specified content-type.
 **/
GMimePart *
g_mime_part_new_with_type (const gchar *type, const gchar *subtype)
{
	GMimePart *mime_part;
	
	mime_part = g_new0 (GMimePart, 1);
	
	mime_part->mime_type = g_mime_content_type_new (type, subtype);
	
	return mime_part;
}


/**
 * g_mime_part_destroy: Destroy the MIME Part
 * @mime_part: Mime part to destroy
 *
 * Releases all memory used by this mime part and all child mime parts.
 **/
void
g_mime_part_destroy (GMimePart *mime_part)
{
	g_free (mime_part->description);
	g_free (mime_part->content_id);
	
	if (mime_part->mime_type)
		g_mime_content_type_destroy (mime_part->mime_type);
	
	if (mime_part->disposition) {
		g_free (mime_part->disposition->disposition);
		
		if (mime_part->disposition->param_hash)
			g_hash_table_destroy (mime_part->disposition->param_hash);
		
		if (mime_part->disposition->params) {
			GList *parameter;
			
			parameter = mime_part->disposition->params;
			while (parameter) {
				GMimeParam *param = parameter->data;
				
				g_free (param->name);
				g_free (param->value);
				g_free (param);
				
				parameter = parameter->next;
			}
			
			g_list_free (mime_part->disposition->params);
		}
	}
	
	g_free (mime_part->boundary);
	
	if (mime_part->children) {
		GList *child;
		
		child = mime_part->children;
		while (child) {
			g_mime_part_destroy (child->data);
			child = child->next;
		}
		
		g_list_free (mime_part->children);
	}
	
	g_free (mime_part->content);
	
	g_free (mime_part);
}


/**
 * g_mime_part_set_content_description: Set the content description
 * @mime_part: Mime part
 * @description: content description
 *
 * Set the content description for the specified mime part.
 **/
void
g_mime_part_set_content_description (GMimePart *mime_part, const gchar *description)
{
	if (mime_part->description)
		g_free (mime_part->description);
	
	mime_part->description = g_strdup (description);
}


/**
 * g_mime_part_get_content_description: Get the content description
 * @mime_part: Mime part
 *
 * Returns the content description for the specified mime part.
 **/
const gchar *
g_mime_part_get_content_description (GMimePart *mime_part)
{
	return mime_part->description;
}


/**
 * g_mime_part_set_content_id: Set the content id
 * @mime_part: Mime part
 * @content_id: content id
 *
 * Set the content id for the specified mime part.
 **/
void
g_mime_part_set_content_id (GMimePart *mime_part, const gchar *content_id)
{
	if (mime_part->content_id)
		g_free (mime_part->content_id);
	
	mime_part->content_id = g_strdup (content_id);
}


/**
 * g_mime_part_get_content_id: Get the content id
 * @mime_part: Mime part
 *
 * Returns the content id for the specified mime part.
 **/
const gchar *
g_mime_part_get_content_id (GMimePart *mime_part)
{
	return mime_part->content_id;
}


/**
 * g_mime_part_set_content_type: Set the content type/subtype
 * @mime_part: Mime part
 * @mime_type: Mime content-type
 *
 * Set the content type/subtype for the specified mime part.
 **/
void
g_mime_part_set_content_type (GMimePart *mime_part, GMimeContentType *mime_type)
{
	if (mime_part->mime_type)
		g_mime_content_type_destroy (mime_part->mime_type);
	
	mime_part->mime_type = mime_type;
}


/**
 * g_mime_part_get_content_type: Get the content type/subtype
 * @mime_part: Mime part
 *
 * Returns the content-type object for the specified mime part.
 **/
const GMimeContentType *
g_mime_part_get_content_type (GMimePart *mime_part)
{
	return mime_part->mime_type;
}


/**
 * g_mime_part_set_encoding: Set the content encoding
 * @mime_part: Mime part
 * @encoding: Mime encoding
 *
 * Set the content encoding for the specified mime part. Available
 * values for the encoding are: GMIME_PART_ENCODING_DEFAULT,
 * GMIME_PART_ENCODING_7BIT, GMIME_PART_ENCODING_8BIT,
 * GMIME_PART_ENCODING_BASE64 and GMIME_PART_ENCODING_QUOTEDPRINTABLE.
 **/
void
g_mime_part_set_encoding (GMimePart *mime_part, GMimePartEncodingType encoding)
{
	mime_part->encoding = encoding;
}


/**
 * g_mime_part_get_encoding: Get the content encoding
 * @mime_part: Mime part
 *
 * Returns the content encoding for the specified mime part. The
 * return value will be one of the following:
 * GMIME_PART_ENCODING_DEFAULT, GMIME_PART_ENCODING_7BIT,
 * GMIME_PART_ENCODING_8BIT, GMIME_PART_ENCODING_BASE64 or
 * GMIME_PART_ENCODING_QUOTEDPRINTABLE.
 **/
GMimePartEncodingType
g_mime_part_get_encoding (GMimePart *mime_part)
{
	return mime_part->encoding;
}


/**
 * g_mime_part_encoding_to_string:
 * @encoding: Mime encoding
 *
 * Returns the encoding type as a string. Available
 * values for the encoding are: GMIME_PART_ENCODING_DEFAULT,
 * GMIME_PART_ENCODING_7BIT, GMIME_PART_ENCODING_8BIT,
 * GMIME_PART_ENCODING_BASE64 and GMIME_PART_ENCODING_QUOTEDPRINTABLE.
 **/
const gchar *
g_mime_part_encoding_to_string (GMimePartEncodingType encoding)
{
	switch (encoding) {
        case GMIME_PART_ENCODING_7BIT:
		return "7bit";
        case GMIME_PART_ENCODING_8BIT:
		return "8bit";
        case GMIME_PART_ENCODING_BASE64:
		return "base64";
        case GMIME_PART_ENCODING_QUOTEDPRINTABLE:
		return "quoted-printable";
	default:
		/* I guess this is a good default... */
		return "8bit";
	}
}


/**
 * g_mime_part_encoding_from_string:
 * @encoding: Mime encoding in string format
 *
 * Returns the encoding string as a GMimePartEncodingType.
 * Available values for the encoding are:
 * GMIME_PART_ENCODING_DEFAULT, GMIME_PART_ENCODING_7BIT,
 * GMIME_PART_ENCODING_8BIT, GMIME_PART_ENCODING_BASE64 and
 * GMIME_PART_ENCODING_QUOTEDPRINTABLE.
 **/
GMimePartEncodingType
g_mime_part_encoding_from_string (const gchar *encoding)
{
	if (!g_strcasecmp (encoding, "7bit"))
		return GMIME_PART_ENCODING_7BIT;
	else if (!g_strcasecmp (encoding, "8bit"))
		return GMIME_PART_ENCODING_8BIT;
	else if (!g_strcasecmp (encoding, "base64"))
		return GMIME_PART_ENCODING_BASE64;
	else if (!g_strcasecmp (encoding, "quoted-printable"))
		return GMIME_PART_ENCODING_QUOTEDPRINTABLE;
	else return GMIME_PART_ENCODING_DEFAULT;
}


/**
 * g_mime_part_set_content_disposition: Set the content disposition
 * @mime_part: Mime part
 * @disposition: content disposition
 *
 * Set the content disposition for the specified mime part
 **/
void
g_mime_part_set_content_disposition (GMimePart *mime_part, const gchar *disposition)
{
	if (mime_part->disposition) {
		g_free (mime_part->disposition->disposition);
		mime_part->disposition->disposition = g_strdup (disposition);
	} else {
		mime_part->disposition = g_new0 (GMimePartDisposition, 1);
		mime_part->disposition->disposition = g_strdup (disposition);
		
		/* init the parameter lookup table */
		mime_part->disposition->param_hash = g_hash_table_new (g_str_hash, g_str_equal);
	}
}


/**
 * g_mime_part_get_content_disposition: Get the content disposition
 * @mime_part: Mime part
 *
 * Returns the content disposition for the specified mime part.
 **/
const gchar *
g_mime_part_get_content_disposition (GMimePart *mime_part)
{
	if (mime_part->disposition)
		return mime_part->disposition->disposition;
	
	return NULL;
}


/**
 * g_mime_part_add_content_disposition_parameter: Add a content-disposition parameter
 * @mime_part: Mime part
 * @name: parameter name
 * @value: parameter value
 *
 * Add a content-disposition parameter to the specified mime part.
 **/
void
g_mime_part_add_content_disposition_parameter (GMimePart *mime_part, const gchar *name, const gchar *value)
{
	GMimeParam *param;
	
	if (!mime_part->disposition)
		g_mime_part_set_content_disposition (mime_part, "");
	
	if (mime_part->disposition->params) {
		/* lets look to see if we've set "name" */
		param = g_hash_table_lookup (mime_part->disposition->param_hash, name);
		if (param) {
			/* why yes, yes we have... */
			g_hash_table_remove (mime_part->disposition->param_hash, name);
			mime_part->disposition->params = g_list_remove (mime_part->disposition->params, param);
			g_free (param->name);
			g_free (param->value);
			g_free (param);
		}
	}
	
	param = g_mime_param_new (name, value);
	mime_part->disposition->params = g_list_append (mime_part->disposition->params, param);
	g_hash_table_insert (mime_part->disposition->param_hash, param->name, param);
}


/**
 * g_mime_part_get_content_disposition_parameter: Get a content-disposition parameter
 * @mime_part: Mime part
 * @name: parameter name
 *
 * Returns the value of a previously defined content-disposition
 * parameter specified by #name.
 **/
const gchar *
g_mime_part_get_content_disposition_parameter (GMimePart *mime_part, const gchar *name)
{
	GMimeParam *param;
	
	g_return_val_if_fail (mime_part != NULL, NULL);
	g_return_val_if_fail (mime_part->disposition != NULL, NULL);
	g_return_val_if_fail (mime_part->disposition->param_hash != NULL, NULL);
	
	param = g_hash_table_lookup (mime_part->disposition->param_hash, name);
	
	return param->value;
}


/**
 * g_mime_part_set_filename: Set the "filename" content-disposition parameter
 * @mime_part: Mime part
 * @filename: the filename of the Mime Part's content
 *
 * Sets the "filename" parameter on the Content-Disposition and also sets the
 * "name" parameter on the Content-Type.
 **/
void
g_mime_part_set_filename (GMimePart *mime_part, const gchar *filename)
{
	GMimeParam *param;
	
	if (!mime_part->disposition)
		g_mime_part_set_content_disposition (mime_part, "");
	
	if (mime_part->disposition->params) {
		/* lets look to see if we've set "filename" */
		param = g_hash_table_lookup (mime_part->disposition->param_hash, "filename");
		if (param) {
			/* why yes, yes we have... */
			g_hash_table_remove (mime_part->disposition->param_hash, "filename");
			mime_part->disposition->params = g_list_remove (mime_part->disposition->params, param);
			g_free (param->name);
			g_free (param->value);
			g_free (param);
		}
	}
	
	param = g_mime_param_new ("filename", filename);
	mime_part->disposition->params = g_list_append (mime_part->disposition->params, param);
	g_hash_table_insert (mime_part->disposition->param_hash, param->name, param);
	
	g_mime_content_type_add_parameter (mime_part->mime_type, "name", filename);
}


/**
 * g_mime_part_get_filename: Get the filename of the specified mime part (if it exists)
 * @mime_part: Mime part
 *
 * Returns the filename of the specified MIME Part. It first checks to
 * see if the "filename" parameter was set on the Content-Disposition
 * and if not then checks the "name" parameter in the Content-Type.
 **/
const gchar *
g_mime_part_get_filename (GMimePart *mime_part)
{
	GMimeParam *param;
	
	g_return_val_if_fail (mime_part != NULL, NULL);
	g_return_val_if_fail (mime_part->disposition != NULL, NULL);
	g_return_val_if_fail (mime_part->disposition->param_hash != NULL, NULL);
	
	param = g_hash_table_lookup (mime_part->disposition->param_hash, "filename");
	
	if (!param || !param->value) {
		/* check the "name" param in the content-type */
		return g_mime_content_type_get_parameter (mime_part->mime_type, "name");
	}
	
	return param->value;
}


/**
 * g_mime_part_set_boundary: Set the multi-part boundary (not used on non-multiparts)
 * @mime_part: Mime part
 * @boundary: the boundary for the multi-part
 *
 * Sets the boundary on the mime part.
 **/
void
g_mime_part_set_boundary (GMimePart *mime_part, const gchar *boundary)
{
	g_return_if_fail (mime_part != NULL);
	
	g_free (mime_part->boundary);
	mime_part->boundary = g_strdup (boundary);
}


/**
 * g_mime_part_get_boundary: Get the multi-part boundary
 * @mime_part: Mime part
 *
 * Returns the boundary on the mime part.
 **/
const gchar *
g_mime_part_get_boundary (GMimePart *mime_part)
{
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	return mime_part->boundary;
}


/**
 * g_mime_part_set_content: Set the content of the mime part
 * @mime_part: Mime part
 * @content: Mime part content
 *
 * Sets the content of the Mime Part (only non-multiparts)
 **/
void
g_mime_part_set_content (GMimePart *mime_part, const char *content)
{
	g_return_if_fail (mime_part != NULL);
	
	g_free (mime_part->content);
	mime_part->content = g_strdup (content);
}


/**
 * g_mime_part_add_child: Add a child mime part to a multipart
 * @mime_part: Parent Mime part
 * @child: Child Mime part
 *
 * Adds a child mime part to the parent mime part which *must* be a
 * multipart.
 **/
void
g_mime_part_add_child (GMimePart *mime_part, GMimePart *child)
{
	g_return_if_fail (mime_part != NULL);
	g_return_if_fail (child != NULL);
	
	mime_part->children = g_list_append (mime_part->children, child);
}

static gchar *
get_content_disposition (GMimePart *mime_part)
{
	GString *string;
	GList *params;
	gchar *str;
	
	g_return_val_if_fail (mime_part->disposition != NULL, NULL);
	
	params = mime_part->disposition->params;
	
	if (mime_part->disposition->disposition && *mime_part->disposition->disposition) {
		string = g_string_new (mime_part->disposition->disposition);
		if (params)
			g_string_append (string, ";");
	} else {
		string = g_string_new ("");
	}
	
	while (TRUE) {
		GMimeParam *param;
		gchar *buf;
		
		param = params->data;
		buf = g_mime_param_to_string (param);
		g_string_append_c (string, ' ');
		g_string_append (string, buf);
		g_free (buf);
		
		params = params->next;
		if (params)
			g_string_append (string, ";");
		else
			break;
	}
	
	str = string->str;
	g_string_free (string, FALSE);
	
	return str;
}


static gchar *
get_content_type (GMimeContentType *mime_type)
{
	GString *string;
	GList *params;
	gchar *str, *type;
	
	g_return_val_if_fail (mime_type != NULL, NULL);
	
	type = g_mime_content_type_to_string (mime_type);
	
	string = g_string_new (type);
	g_free (type);
	
	params = mime_type->params;
	if (params)
		g_string_append (string, ";");
	
	while (params) {
		GMimeParam *param;
		gchar *buf;
		
		param = params->data;
		buf = g_mime_param_to_string (param);
		g_string_append_c (string, ' ');
		g_string_append (string, buf);
		g_free (buf);
		
		params = params->next;
		if (params)
			g_string_append (string, ";");
		else
			break;
	}
	
	str = string->str;
	g_string_free (string, FALSE);
	
	return str;
}

/**
 * g_mime_part_to_string: Write the MIME Part to a string
 * @mime_part: MIME Part
 * @toplevel: mime part is the root mime part
 *
 * Returns an allocated string containing the MIME Part. If toplevel
 * is set to TRUE, then the MIME Part header will contain needed MIME
 * headers for rfc822 messages.
 **/
gchar *
g_mime_part_to_string (GMimePart *mime_part, gboolean toplevel) {
	gchar *string;
	
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	if (g_mime_content_type_is_type (mime_part->mime_type, "multipart", "*")) {
		GString *contents;
		GList *child;
		gchar *content_type;
		
		content_type = g_mime_content_type_to_string (mime_part->mime_type);
		
		contents = g_string_new ("");
		
		child = mime_part->children;
		while (child) {
			gchar *part, *mime_string;
			
			mime_string = g_mime_part_to_string (child->data, FALSE);
			
			part = g_strdup_printf ("--%s\n%s\n", mime_part->boundary, mime_string);
			
			g_string_append (contents, part);
			
			g_free (mime_string);
			g_free (part);
			
			child = child->next;
		}
		
		if (toplevel) {
			/* add message header stuff first */
			string = g_strdup_printf ("MIME-Version: 1.0\n"
						  "Content-Type: %s; boundary=\"%s\"\n\n"
						  "This is a multi-part message in MIME format.\n\n"
						  "%s\n--%s--\n",
						  content_type, mime_part->boundary,
						  contents->str, mime_part->boundary);
		} else {
			string = g_strdup_printf ("Content-Type: %s; boundary=\"%s\"\n\n"
						  "%s\n--%s--\n",
						  content_type, mime_part->boundary,
						  contents->str, mime_part->boundary);
		}
		
		g_free (content_type);
		g_string_free (contents, TRUE);
	} else {
		gchar *content_type;
		gchar *disposition;
		gchar *description;
		gchar *content_id;
		gchar *extras;
		gchar *text;
		
		content_type = get_content_type (mime_part->mime_type);
		disposition = get_content_disposition (mime_part);
		
		if (mime_part->description) {
			text = g_mime_utils_8bit_header_encode (mime_part->description);
			description = g_strdup_printf ("Content-Description: %s\n", text);
			g_free (text);
		} else {
			description = g_strdup ("");
		}
		
		if (mime_part->content_id)
			content_id = g_strdup_printf ("Content-Id: %s\n", mime_part->content_id);
		else
		        content_id = g_strdup ("");
		
		if (toplevel)
			extras = "MIME-Version: 1.0\n";
		else
			extras = "";
		
		if (disposition && *disposition) {
			string = g_strdup_printf ("%sContent-Type: %s\n"
						  "Content-Transfer-Encoding: %s\n"
						  "Content-Disposition: %s\n"
						  "%s"      /* description */
						  "%s\n"    /* content id */
						  "%s\n",   /* content */
						  extras, content_type,
						  g_mime_part_encoding_to_string (mime_part->encoding),
						  disposition,
						  description,
						  content_id,
						  mime_part->content);
		} else {
			string = g_strdup_printf ("%sContent-Type: %s\n"
						  "Content-Transfer-Encoding: %s\n"
						  "%s"      /* description */
						  "%s\n"    /* content id */
						  "%s\n",   /* content */
						  extras, content_type,
						  g_mime_part_encoding_to_string (mime_part->encoding),
						  description,
						  content_id,
						  mime_part->content);
		}
		
		g_free (content_type);
		g_free (disposition);
		g_free (description);
		g_free (content_id);
	}
	
	return string;
}

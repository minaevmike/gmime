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


#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include "gmime-part.h"
#include "gmime-utils.h"
#include "md5-utils.h"


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
	g_return_if_fail (mime_part != NULL);
	
	g_free (mime_part->description);
	g_free (mime_part->content_id);
	g_free (mime_part->content_md5);
	g_free (mime_part->content_location);
	
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
	
	if (mime_part->children) {
		GList *child;
		
		child = mime_part->children;
		while (child) {
			g_mime_part_destroy (child->data);
			child = child->next;
		}
		
		g_list_free (mime_part->children);
	}
	
	if (mime_part->content)
		g_byte_array_free (mime_part->content, TRUE);
	
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
	g_return_if_fail (mime_part != NULL);
	
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
g_mime_part_get_content_description (const GMimePart *mime_part)
{
	g_return_val_if_fail (mime_part != NULL, NULL);
	
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
	g_return_if_fail (mime_part != NULL);
	
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
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	return mime_part->content_id;
}


/**
 * g_mime_part_set_content_md5: Set the content md5
 * @mime_part: Mime part
 * @content_md5: content md5 or NULL to generate the md5 digest.
 *
 * Set the content md5 for the specified mime part.
 **/
void
g_mime_part_set_content_md5 (GMimePart *mime_part, const gchar *content_md5)
{
	const GMimeContentType *type;
	
	g_return_if_fail (mime_part != NULL);
	
	/* RFC 1864 states that you cannot set a Content-MD5 for these types */
	type = g_mime_part_get_content_type (mime_part);
	if (g_mime_content_type_is_type (type, "multipart", "*") ||
	    g_mime_content_type_is_type (type, "message", "rfc822"))
		return;
	
	if (mime_part->content_md5)
		g_free (mime_part->content_md5);
	
	if (content_md5) {
		mime_part->content_md5 = g_strdup (content_md5);
	} else if (mime_part->content && mime_part->content->len) {
		char digest[16], b64digest[32];
		int len, state, save;
		
		md5_get_digest (mime_part->content->data, mime_part->content->len, digest);
		
		state = save = 0;
		len = g_mime_utils_base64_encode_close (digest, 16, b64digest, &state, &save);
		b64digest[len] = '\0';
		
		mime_part->content_md5 = g_strdup (b64digest);
	}
}


/**
 * g_mime_part_verify_content_md5: Verify the content md5
 * @mime_part: Mime part
 *
 * Verify the content md5 for the specified mime part.
 *
 * Returns TRUE if the md5 is valid or FALSE otherwise. Note: will
 * return FALSE if the mime part does not contain a Content-MD5.
 **/
gboolean
g_mime_part_verify_content_md5 (GMimePart *mime_part)
{
	char digest[16], b64digest[32];
	int len, state, save;
	
	g_return_val_if_fail (mime_part != NULL, FALSE);
	g_return_val_if_fail (mime_part->content_md5 != NULL, FALSE);
	
	md5_get_digest (mime_part->content->data, mime_part->content->len, digest);
	
	state = save = 0;
	len = g_mime_utils_base64_encode_close (digest, 16, b64digest, &state, &save);
	b64digest[len] = '\0';
	
	return !strcmp (b64digest, mime_part->content_md5);
}


/**
 * g_mime_part_get_content_md5: Get the content md5
 * @mime_part: Mime part
 *
 * Returns the content md5 for the specified mime part.
 **/
const gchar *
g_mime_part_get_content_md5 (GMimePart *mime_part)
{
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	return mime_part->content_md5;
}


/**
 * g_mime_part_set_content_location: Set the content location
 * @mime_part: Mime part
 * @content_location: content location
 *
 * Set the content location for the specified mime part.
 **/
void
g_mime_part_set_content_location (GMimePart *mime_part, const gchar *content_location)
{
	g_return_if_fail (mime_part != NULL);
	
	if (mime_part->content_location)
		g_free (mime_part->content_location);
	
	mime_part->content_location = g_strdup (content_location);
}


/**
 * g_mime_part_get_content_location: Get the content location
 * @mime_part: Mime part
 *
 * Returns the content location for the specified mime part.
 **/
const gchar *
g_mime_part_get_content_location (GMimePart *mime_part)
{
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	return mime_part->content_location;
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
	g_return_if_fail (mime_part != NULL);
	
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
	g_return_val_if_fail (mime_part != NULL, NULL);
	
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
	g_return_if_fail (mime_part != NULL);
	
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
	g_return_val_if_fail (mime_part != NULL, GMIME_PART_ENCODING_DEFAULT);
	
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
	g_return_if_fail (mime_part != NULL);
	
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
	g_return_val_if_fail (mime_part != NULL, NULL);
	
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
	
	g_return_if_fail (mime_part != NULL);
	
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
	
	g_return_if_fail (mime_part != NULL);
	
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


static void
read_random_pool (gchar *buffer, size_t bytes)
{
	int fd;
	
	fd = open ("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		fd = open ("/dev/random", O_RDONLY);
		if (fd == -1)
			return;
	}
	
	read (fd, buffer, bytes);
	close (fd);
}


/**
 * g_mime_part_set_boundary: Set the multi-part boundary (not used on non-multiparts)
 * @mime_part: Mime part
 * @boundary: the boundary for the multi-part or NULL to generate a random one.
 *
 * Sets the boundary on the mime part.
 **/
void
g_mime_part_set_boundary (GMimePart *mime_part, const gchar *boundary)
{
	gchar bbuf[27];
	
	g_return_if_fail (mime_part != NULL);
	
	if (!boundary) {
		/* Generate a fairly random boundary string. */
		char digest[16], *p;
		int state, save;
		
		read_random_pool (digest, 16);
		
		strcpy (bbuf, "=-");
		p = bbuf + 2;
		state = save = 0;
		p += g_mime_utils_base64_encode_step (digest, 16, p, &state, &save);
		*p = '\0';
		
		boundary = bbuf;
	}
	
	g_mime_content_type_add_parameter (mime_part->mime_type, "boundary", boundary);
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
	
	return g_mime_content_type_get_parameter (mime_part->mime_type, "boundary");
}


/**
 * g_mime_part_set_content: Set the content of the mime part
 * @mime_part: Mime part
 * @content: raw mime part content
 * @len: raw content length
 *
 * Sets the content of the Mime Part (only non-multiparts)
 **/
void
g_mime_part_set_content (GMimePart *mime_part, const char *content, guint len)
{
	g_return_if_fail (mime_part != NULL);
	
	if (mime_part->content)
		g_byte_array_free (mime_part->content, TRUE);
	
	mime_part->content = g_byte_array_new ();
	g_byte_array_append (mime_part->content, content, len);
}


/**
 * g_mime_part_set_pre_encoded_content: Set the pre-encoded content of the mime part
 * @mime_part: Mime part
 * @content: encoded mime part content
 * @len: length of the content
 * @encoding: content encoding
 *
 * Sets the encoding type and raw content on the mime part after decoding the content.
 **/
void
g_mime_part_set_pre_encoded_content (GMimePart *mime_part, const char *content, guint len, GMimePartEncodingType encoding)
{
	gchar *raw;
	gint save = 0, state = 0;
	
	g_return_if_fail (mime_part != NULL);
	g_return_if_fail (content != NULL);
	
	if (mime_part->content)
		g_byte_array_free (mime_part->content, TRUE);
	
	mime_part->content = g_byte_array_new ();
	g_byte_array_set_size (mime_part->content, len);
	raw = mime_part->content->data;
	switch (encoding) {
	case GMIME_PART_ENCODING_BASE64:
		len = g_mime_utils_base64_decode_step (content, len, raw, &state, &save);
		g_byte_array_set_size (mime_part->content, len);
		break;
	case GMIME_PART_ENCODING_QUOTEDPRINTABLE:
		len = g_mime_utils_quoted_decode_step (content, len, raw, &state, &save);
		g_byte_array_set_size (mime_part->content, len);
		break;
	default:
		memcpy (raw, content, len);
		
		/* do some smart 8bit detection */
		if (encoding == GMIME_PART_ENCODING_DEFAULT && g_mime_utils_text_is_8bit (raw))
			encoding = GMIME_PART_ENCODING_8BIT;
	}
	
	mime_part->encoding = encoding;
}


/**
 * g_mime_part_get_content: 
 * @mime_part: the GMimePart to be decoded.
 * @len: decoded length (to be set after processing)
 * 
 * Returns a gchar * pointer to the raw contents of the MIME Part
 * and sets %len to the length of the buffer.
 **/
const gchar *
g_mime_part_get_content (const GMimePart *mime_part, guint *len)
{
	g_return_val_if_fail (mime_part != NULL, NULL);
	g_return_val_if_fail (mime_part->content != NULL, NULL);
	
	*len = mime_part->content->len;
	
	return mime_part->content->data;
}


/**
 * g_mime_part_add_subpart: Add a subpart to a multipart
 * @mime_part: Parent Mime part
 * @subpart: Child Mime part
 *
 * Adds a subpart to the parent mime part which *must* be a
 * multipart.
 **/
void
g_mime_part_add_subpart (GMimePart *mime_part, GMimePart *subpart)
{
	const GMimeContentType *type;
	
	g_return_if_fail (mime_part != NULL);
	g_return_if_fail (subpart != NULL);
	
	type = g_mime_part_get_content_type (mime_part);
	if (g_mime_content_type_is_type (type, "multipart", "*"))
		mime_part->children = g_list_append (mime_part->children, subpart);
}

static gchar *
get_content_disposition (GMimePart *mime_part)
{
	GString *string;
	GList *params;
	gchar *str;
	
	g_return_val_if_fail (mime_part->disposition != NULL, NULL);
	
	params = mime_part->disposition->params;
	
	if (mime_part->disposition->disposition && *mime_part->disposition->disposition)
		string = g_string_new (mime_part->disposition->disposition);
	else
		string = g_string_new ("");
	
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
	
	type = g_mime_utils_header_printf ("Content-Type: %s\n", str);
	g_free (str);
	
	return type;
}

static gchar *
get_content (GMimePart *part)
{
	gchar *content;
	gint save = 0, state = 0;
	gint len;
	
	if (!part->content)
		return g_strdup ("");
	
	switch (part->encoding) {
	case GMIME_PART_ENCODING_BASE64:
		content = g_malloc (BASE64_ENCODE_LEN (part->content->len));
		len = g_mime_utils_base64_encode_close (part->content->data, part->content->len, content, &state, &save);
		content[len] = '\0';
		break;
	case GMIME_PART_ENCODING_QUOTEDPRINTABLE:
		state = -1;
		content = g_malloc (QP_ENCODE_LEN (part->content->len));
		len = g_mime_utils_quoted_encode_close (part->content->data, part->content->len, content, &state, &save);
		content[len] = '\0';
		break;
	default:
		content = g_strndup (part->content->data, part->content->len);
	}
	
	return content;
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
g_mime_part_to_string (GMimePart *mime_part, gboolean toplevel)
{
	gchar *str;
	
	g_return_val_if_fail (mime_part != NULL, NULL);
	
	if (g_mime_content_type_is_type (mime_part->mime_type, "multipart", "*")) {
		const gchar *boundary;
		GString *contents;
		GList *child;
		gchar *content_type;
		
		/* make sure there's a boundary, else force a random boundary */
		boundary = g_mime_part_get_boundary (mime_part);
		if (!boundary) {
			g_mime_part_set_boundary (mime_part, NULL);
			boundary = g_mime_part_get_boundary (mime_part);
		}
		
		content_type = get_content_type (mime_part->mime_type);
		
		contents = g_string_new ("");
		
		child = mime_part->children;
		while (child) {
			gchar *part, *mime_string;
			
			mime_string = g_mime_part_to_string (child->data, FALSE);
			
			part = g_strdup_printf ("--%s\n%s\n", boundary, mime_string);
			
			g_string_append (contents, part);
			
			g_free (mime_string);
			g_free (part);
			
			child = child->next;
		}
		
		if (toplevel) {
			/* add message header stuff first */
			str = g_strdup_printf ("MIME-Version: 1.0\n"
					       "%s\n"  /* content-type */
					       "This is a multi-part message in MIME format.\n\n"
					       "%s\n--%s--\n",
					       content_type,
					       contents->str,
					       boundary);
		} else {
			str = g_strdup_printf ("%s\n"  /* content-type */
					       "%s\n--%s--\n",
					       content_type,
					       contents->str,
					       boundary);
		}
		
		g_free (content_type);
		g_string_free (contents, TRUE);
	} else {
		gchar *content_type;
		gchar *disposition;
		gchar *description;
		gchar *content_id;
		gchar *content_md5;
		gchar *content_location;
		gchar *content;
		gchar *text;
		GString *string;
		
		string = g_string_new ("");
		
		if (toplevel)
			g_string_append (string, "MIME-Version: 1.0\n");
		
		/* Content-Type: */
		content_type = get_content_type (mime_part->mime_type);
		g_string_append (string, content_type);
		g_free (content_type);
		
		/* Content-Transfer-Encoding: */
		if (mime_part->encoding != GMIME_PART_ENCODING_DEFAULT) {
			gchar *content_encoding;
			
			content_encoding = g_strdup_printf ("Content-Transfer-Encoding: %s\n",
							    g_mime_part_encoding_to_string (mime_part->encoding));
			
			g_string_append (string, content_encoding);
			g_free (content_encoding);
		}
		
		/* Content-Disposition: */
		disposition = get_content_disposition (mime_part);
		if (disposition) {
			text = g_mime_utils_header_printf ("Content-Disposition: %s\n",
							   disposition);
			g_free (disposition);
			g_string_append (string, text);
			g_free (text);
		}
		
		/* Content-Description: */
		if (mime_part->description) {
			text = g_mime_utils_8bit_header_encode (mime_part->description);
			description = g_mime_utils_header_printf ("Content-Description: %s\n", text);
			g_free (text);
			
			g_string_append (string, description);
			g_free (description);
		}
		
		/* Content-Location: */
		if (mime_part->content_location) {
			content_md5 = g_strdup_printf ("Content-Location: %s\n", mime_part->content_location);
			g_string_append (string, content_location);
			g_free (content_location);
		}
		
		/* Content-Md5: */
		if (mime_part->content_md5) {
			content_md5 = g_strdup_printf ("Content-MD5: %s\n", mime_part->content_md5);
			g_string_append (string, content_md5);
			g_free (content_md5);
		}
		
		/* Content-Id: */
		if (mime_part->content_id) {
			content_id = g_strdup_printf ("Content-Id: %s\n", mime_part->content_id);
			g_string_append (string, content_id);
			g_free (content_id);
		}
		
		g_string_append_c (string, '\n');
		
		content = get_content (mime_part);
		
		g_string_append (string, content);
		g_string_append_c (string, '\n');
		
		g_free (content);
		
		str = string->str;
		g_string_free (string, FALSE);
	}
	
	return str;
}


/**
 * g_mime_part_foreach: 
 * @mime_part: the MIME part
 * @callback: function to call for #mime_part and all it's subparts
 * @data: extra data to pass to the callback
 * 
 * Calls #callback on #mime_part and each of it's subparts.
 **/
void
g_mime_part_foreach (GMimePart *mime_part, GMimePartFunc callback, gpointer data)
{
	g_return_if_fail (mime_part != NULL);
	g_return_if_fail (callback != NULL);
	
	callback (mime_part, data);
	
	if (mime_part->children) {
		GList *child;
		
		child = mime_part->children;
		while (child) {
			g_mime_part_foreach ((GMimePart *) child->data, callback, data);
			child = child->next;
		}
	}
}


/**
 * g_mime_part_get_subpart_from_content_id: 
 * @mime_part: the MIME part
 * @content_id: the content id of the part to look for
 *
 * Returns the GMimePart whose content-id matches the search string,
 * or NULL if a match cannot be found.
 **/
const GMimePart *
g_mime_part_get_subpart_from_content_id (GMimePart *mime_part, const gchar *content_id)
{
	GList *child;
	
	g_return_val_if_fail (mime_part != NULL, NULL);
	g_return_val_if_fail (content_id != NULL, NULL);
	
	if (mime_part->content_id && !strcmp (mime_part->content_id, content_id))
		return mime_part;
	
	child = mime_part->children;
	while (child) {
		const GMimeContentType *type;
		const GMimePart *part = NULL;
		GMimePart *subpart;
		
		subpart = (GMimePart *) child->data;
		type = g_mime_part_get_content_type (subpart);
		
		if (g_mime_content_type_is_type (type, "multipart", "*"))
			part = g_mime_part_get_subpart_from_content_id (subpart, content_id);
		else if (subpart->content_id && !strcmp (subpart->content_id, content_id))
			part = subpart;
		
		if (part)
			return part;
		
		child = child->next;
	}
	
	return NULL;
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Jeffrey Stedfast <fejj@ximian.com>
 *
 *  Copyright 2001 Ximian, Inc. (www.ximian.com)
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


#include <string.h>

#include <gmime/gmime.h>


int main (int argc, char **argv)
{
	GMimeStream *istream, *ostream, *fstream;
	GMimeFilter *html;
	int i;
	
	g_mime_init (0);
	
	fstream = g_mime_stream_file_new (stdout);
	ostream = g_mime_stream_filter_new_with_stream (fstream);
	g_mime_stream_unref (fstream);
	html = g_mime_filter_html_new (GMIME_FILTER_HTML_CONVERT_NL |
				       GMIME_FILTER_HTML_CONVERT_SPACES |
				       GMIME_FILTER_HTML_CONVERT_URLS |
				       GMIME_FILTER_HTML_MARK_CITATION |
				       GMIME_FILTER_HTML_CONVERT_ADDRESSES |
				       GMIME_FILTER_HTML_ESCAPE_8BIT |
				       GMIME_FILTER_HTML_CITE, 0);
	g_mime_stream_filter_add (GMIME_STREAM_FILTER (ostream), html);
	g_object_unref (html);
	
	for (i = 1; i < argc; i++) {
		FILE *fp;
		
		fp = fopen (argv[i], "r");
		if (!fp) {
			fprintf (stderr, "failed to open %s\n", argv[i]);
			continue;
		}
		
		istream = g_mime_stream_file_new (fp);
		g_mime_stream_write_to_stream (istream, ostream);
		g_mime_stream_unref (istream);
	}
	
	g_mime_stream_unref (ostream);
	
	return 0;
}

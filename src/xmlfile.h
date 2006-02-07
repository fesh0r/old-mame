/***************************************************************************

    xmlfile.h

    XML file parsing code.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __XMLFILE_H__
#define __XMLFILE_H__

#include "mamecore.h"
#include "fileio.h"



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct _xml_attribute_node
{
	struct _xml_attribute_node *next;		/* pointer to next attribute node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
};
typedef struct _xml_attribute_node xml_attribute_node;


struct _xml_data_node
{
	struct _xml_data_node *next;				/* pointer to next sibling node */
	struct _xml_data_node *parent;			/* pointer to parent node */
	struct _xml_data_node *child;			/* pointer to first child node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
	xml_attribute_node *attribute;			/* pointer to array of attribute nodes */
};
/* In mamecore.h: typedef struct _xml_data_node xml_data_node; */



struct XML_ParserStruct;

struct _xml_custom_parse
{
	void (*init)(struct XML_ParserStruct *parser);
	size_t (*read)(void *param, void *buffer, size_t length);
	int (*eof)(void *param);
	void *param;
	int trim_whitespace;

	const char *error_message;
	int error_line;
	int error_column;

	xml_data_node **curnode;
};
typedef struct _xml_custom_parse xml_custom_parse;



/*************************************
 *
 *  Function prototypes
 *
 *************************************/

xml_data_node *xml_file_create(void);
xml_data_node *xml_file_read(mame_file *file);
xml_data_node *xml_file_read_custom(xml_custom_parse *parse_info);
void xml_file_write(xml_data_node *node, mame_file *file);
void xml_file_free(xml_data_node *node);

int xml_count_children(xml_data_node *node);
xml_data_node *xml_get_sibling(xml_data_node *node, const char *name);
xml_data_node *xml_find_matching_sibling(xml_data_node *node, const char *name, const char *attribute, const char *matchval);
xml_attribute_node *xml_get_attribute(xml_data_node *node, const char *attribute);
const char *xml_get_attribute_string(xml_data_node *node, const char *attribute, const char *defvalue);
int xml_get_attribute_int(xml_data_node *node, const char *attribute, int defvalue);
float xml_get_attribute_float(xml_data_node *node, const char *attribute, float defvalue);

xml_data_node *xml_add_child(xml_data_node *node, const char *name, const char *value);
xml_data_node *xml_get_or_add_child(xml_data_node *node, const char *name, const char *value);
xml_attribute_node *xml_set_attribute(xml_data_node *node, const char *name, const char *value);
xml_attribute_node *xml_set_attribute_int(xml_data_node *node, const char *name, int value);
xml_attribute_node *xml_set_attribute_float(xml_data_node *node, const char *name, float value);
void xml_delete_node(xml_data_node *node);

#endif	/* __XMLFILE_H__ */

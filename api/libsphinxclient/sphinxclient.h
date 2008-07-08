//
// $Id: sphinxclient.h 27 2008-04-04 13:39:05Z shodan $
//

//
// Copyright (c) 2008, Andrew Aksyonoff. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License. You should have
// received a copy of the GPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//

#ifndef _sphinxclient_
#define _sphinxclient_

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _WIN32
# include "sphinxclient_config.h"
#endif

// for 64-bti types
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_INTTYPES_H
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/// known searchd status codes
enum
{
	SEARCHD_OK				= 0,
	SEARCHD_ERROR			= 1,
	SEARCHD_RETRY			= 2,
	SEARCHD_WARNING			= 3
};

/// known match modes
enum
{
	SPH_MATCH_ALL			= 0,
	SPH_MATCH_ANY			= 1,
	SPH_MATCH_PHRASE		= 2,
	SPH_MATCH_BOOLEAN		= 3,
	SPH_MATCH_EXTENDED		= 4,
	SPH_MATCH_FULLSCAN		= 5,
	SPH_MATCH_EXTENDED2		= 6
};

/// known ranking modes (ext2 only)
enum
{
	SPH_RANK_PROXIMITY_BM25	= 0,
	SPH_RANK_BM25			= 1,
	SPH_RANK_NONE			= 2,
	SPH_RANK_WORDCOUNT		= 3
};

/// known sort modes
enum
{
	SPH_SORT_RELEVANCE		= 0,
	SPH_SORT_ATTR_DESC		= 1,
	SPH_SORT_ATTR_ASC		= 2,
	SPH_SORT_TIME_SEGMENTS	= 3,
	SPH_SORT_EXTENDED		= 4,
	SPH_SORT_EXPR			= 5
};

/// known filter types
enum
{	SPH_FILTER_VALUES		= 0,
	SPH_FILTER_RANGE		= 1,
	SPH_FILTER_FLOATRANGE	= 2
};

/// known attribute types
enum
{
	SPH_ATTR_INTEGER		= 1,
	SPH_ATTR_TIMESTAMP		= 2,
	SPH_ATTR_ORDINAL		= 3,
	SPH_ATTR_BOOL			= 4,
	SPH_ATTR_FLOAT			= 5,
	SPH_ATTR_BIGINT			= 6,
	SPH_ATTR_MULTI			= 0x40000000UL
};

/// known grouping functions
enum
{	SPH_GROUPBY_DAY			= 0,
	SPH_GROUPBY_WEEK		= 1,
	SPH_GROUPBY_MONTH		= 2,
	SPH_GROUPBY_YEAR		= 3,
	SPH_GROUPBY_ATTR		= 4,
	SPH_GROUPBY_ATTRPAIR	= 5,
	SPH_GROUPBY_EXTENDED	= 6,
};

//////////////////////////////////////////////////////////////////////////

#if defined(U64C) || defined(I64C)
#error "Internal 64-bit integer macros already defined."
#endif

#if !HAVE_STDINT_H

#if defined(_MSC_VER)
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#define U64C(v) v ## UI64
#define I64C(v) v ## I64
#define PRIu64 "I64d"
#define PRIi64 "I64d"
#else // !defined(_MSC_VER)
typedef long long int64_t;
typedef unsigned long long uint64_t;
#endif // !defined(_MSC_VER)

#endif // no stdint.h

// if platform-specific macros were not supplied, use common defaults
#ifndef U64C
#define U64C(v) v ## ULL
#endif

#ifndef I64C
#define I64C(v) v ## LL
#endif

#ifndef PRIu64
#define PRIu64 "llu"
#endif

#ifndef PRIi64
#define PRIi64 "lld"
#endif

//////////////////////////////////////////////////////////////////////////

typedef int					sphinx_bool;
#define SPH_TRUE			1
#define SPH_FALSE			0

//////////////////////////////////////////////////////////////////////////

typedef struct st_sphinx_client	sphinx_client;

typedef struct st_sphinx_wordinfo
{
	const char *			word;
	int						docs;
	int						hits;
} sphinx_wordinfo;


typedef struct st_sphinx_result
{
	const char *			error;
	const char *			warning;
	int						status;

	int						num_fields;
	char **					fields;

	int						num_attrs;
	char **					attr_names;
	int *					attr_types;

	int						num_matches;
	void *					values_pool;

	int						total;
	int						total_found;
	int						time_msec;
	int						num_words;
	sphinx_wordinfo *		words;
} sphinx_result;

//////////////////////////////////////////////////////////////////////////

sphinx_client *				sphinx_create	( sphinx_bool copy_args );
void						sphinx_destroy	( sphinx_client * client );

const char *				sphinx_error	( sphinx_client * client );
const char *				sphinx_warning	( sphinx_client * client );

sphinx_bool					sphinx_set_server				( sphinx_client * client, const char * host, int port );
sphinx_bool					sphinx_set_limits				( sphinx_client * client, int offset, int limit, int max_matches, int cutoff );
sphinx_bool					sphinx_set_max_query_time		( sphinx_client * client, int max_query_time );
sphinx_bool					sphinx_set_match_mode			( sphinx_client * client, int mode );
sphinx_bool					sphinx_set_ranking_mode			( sphinx_client * client, int ranker );
sphinx_bool					sphinx_set_sort_mode			( sphinx_client * client, int mode, const char * sortby );
sphinx_bool					sphinx_set_field_weights		( sphinx_client * client, int num_weights, const char ** field_names, const int * field_weights );
sphinx_bool					sphinx_set_index_weights		( sphinx_client * client, int num_weights, const char ** index_names, const int * index_weights );
/*
sphinx_bool					sphinx_set_field_weights_va		( sphinx_client * client, const char * first_name, int first_weight, ... );
sphinx_bool					sphinx_set_index_weights_va		( sphinx_client * client, const char * first_name, int first_weight, ... );
*/
sphinx_bool					sphinx_set_id_range				( sphinx_client * client, uint64_t minid, uint64_t maxid );
sphinx_bool					sphinx_add_filter				( sphinx_client * client, const char * attr, int num_values, const uint64_t * values, sphinx_bool exclude );
sphinx_bool					sphinx_add_filter_range			( sphinx_client * client, const char * attr, uint64_t umin, uint64_t umax, sphinx_bool exclude );
sphinx_bool					sphinx_add_filter_float_range	( sphinx_client * client, const char * attr, float fmin, float fmax, sphinx_bool exclude );
sphinx_bool					sphinx_set_geoanchor			( sphinx_client * client, const char * attr_latitude, const char * attr_longitude, float latitude, float longitude );
sphinx_bool					sphinx_set_groupby				( sphinx_client * client, const char * attr, int groupby_func, const char * group_sort );
sphinx_bool					sphinx_set_groupby_distinct		( sphinx_client * client, const char * attr );
sphinx_bool					sphinx_set_retries				( sphinx_client * client, int count, int delay );
sphinx_bool					sphinx_set_override_int			( sphinx_client * client, const char * attr, int attr_type, int num_values, const uint64_t * docids, const uint64_t * values );
sphinx_bool					sphinx_set_override_float		( sphinx_client * client, const char * attr, int num_values, const uint64_t * docids, const float * values );

void						sphinx_reset_filters			( sphinx_client * client );
void						sphinx_reset_groupby			( sphinx_client * client );
void						sphinx_reset_overrides			( sphinx_client * client );

sphinx_result *				sphinx_query					( sphinx_client * client, const char * query, const char * index_list, const char * comment );
int							sphinx_add_query				( sphinx_client * client, const char * query, const char * index_list, const char * comment );
sphinx_result *				sphinx_run_queries				( sphinx_client * client );

uint64_t					sphinx_get_id					( sphinx_result * result, int match );
int							sphinx_get_weight				( sphinx_result * result, int match );
uint64_t					sphinx_get_int					( sphinx_result * result, int match, int attr );
float						sphinx_get_float				( sphinx_result * result, int match, int attr );
unsigned int *				sphinx_get_mva					( sphinx_result * result, int match, int attr );

/////////////////////////////////////////////////////////////////////////////

/*
function BuildExcerpts ( $docs, $index, $words, $opts=array() )
function BuildKeywords ( $query, $index, $hits )
function EscapeString ( $string )
function UpdateAttributes ( $index, $attrs, $values )
*/

#ifdef	__cplusplus
}
#endif

#endif // _sphinxclient_

//
// $Id: sphinxclient.h 27 2008-04-04 13:39:05Z shodan $
//

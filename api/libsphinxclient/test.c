//
// $Id$
//

//
// Copyright (c) 2008, Andrew Aksyonoff. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License. You should have
// received a copy of the GPL license along with this program; if you
// did not, you can find it at http://www.gnu.org/
//

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if _WIN32
#include <winsock2.h>
#endif

#include "sphinxclient.h"

void die ( const char * template, ... )
{
	va_list ap;
	va_start ( ap, template );
	printf ( "FATAL: " );
	vprintf ( template, ap );
	printf ( "\n" );
	va_end ( ap );
	exit ( 1 );
}


int main ()
{
	sphinx_client * client;
	sphinx_result * res;
	const char *query, *index;
	int i, j, k;
	unsigned int * mva;
	const char * field_names[2];
	int field_weights[2];

#if _WIN32
	// init WSA on Windows
	WSADATA wsa_data;
	int wsa_startup_err;

	wsa_startup_err = WSAStartup ( WINSOCK_VERSION, &wsa_data );
	if ( wsa_startup_err )
	{
		printf ( "failed to initialize WinSock2: error %d", wsa_startup_err );
		return 1;
	}
#endif

	query = "test";
	index = "*";

	client = sphinx_create ( SPH_TRUE );
	if ( !client )
		die ( "failed to create client" );

	field_names[0] = "title";
	field_names[1] = "content";
	field_weights[0] = 100;
	field_weights[1] = 1;
	sphinx_set_field_weights ( client, 2, field_names, field_weights );
	field_weights[0] = 1;
	field_weights[1] = 1;

	res = sphinx_query ( client, query, index, NULL );
	if ( !res )
		die ( "query failed: %s", sphinx_error(client) );

	printf ( "Query '%s' retrieved %d of %d matches in %d.%03d sec.\n",
		query, res->total, res->total_found, res->time_msec/1000, res->time_msec%1000 );
	printf ( "Query stats:\n" );
	for ( i=0; i<res->num_words; i++ )
		printf ( "\t'%s' found %d times in %d documents\n",
			res->words[i].word, res->words[i].hits, res->words[i].docs );

	printf ( "\nMatches:\n" );
	for ( i=0; i<res->num_matches; i++ )
	{
		printf ( "%d. doc_id=%d, weight=%d", 1+i,
			(int)sphinx_get_id ( res, i ), sphinx_get_weight ( res, i ) );

		for ( j=0; j<res->num_attrs; j++ )
		{
			printf ( ", %s=", res->attr_names[j] );
			switch ( res->attr_types[j] )
			{
				case SPH_ATTR_MULTI | SPH_ATTR_INTEGER:
					mva = sphinx_get_mva ( res, i, j );
					printf ( "(" );
					for ( k=0; k<(int)mva[0]; k++ )
						printf ( k ? ",%u" : "%u", mva[1+k] );
					printf ( ")" );
					break;

				case SPH_ATTR_FLOAT:	printf ( "%f", sphinx_get_float ( res, i, j ) ); break;
				default:				printf ( "%u", (unsigned int)sphinx_get_int ( res, i, j ) ); break;
			}
		}

		printf ( "\n" );
	}

	sphinx_destroy ( client );
	return 0;
}

//
// $Id$
//

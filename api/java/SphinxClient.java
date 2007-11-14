/*
 * $Id$
 *
 * Java version of Sphinx searchd client (Java API)
 *
 * Copyright (c) 2007, Andrew Aksyonoff
 * Copyright (c) 2007, Vladimir Fedorkov
 * All rights reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License. You should have
 * received a copy of the GPL license along with this program; if you
 * did not, you can find it at http://www.gnu.org/
 */

package org.sphx.api;

import java.io.*;
import java.net.*;
import java.util.*;

/** Sphinx client class */
public class SphinxClient
{
	/* matching modes */
	public final static int SPH_MATCH_ALL			= 0;
	public final static int SPH_MATCH_ANY			= 1;
	public final static int SPH_MATCH_PHRASE		= 2;
	public final static int SPH_MATCH_BOOLEAN		= 3;
	public final static int SPH_MATCH_EXTENDED		= 4;

	/* sorting modes */
	public final static int SPH_SORT_RELEVANCE		= 0;
	public final static int SPH_SORT_ATTR_DESC		= 1;
	public final static int SPH_SORT_ATTR_ASC		= 2;
	public final static int SPH_SORT_TIME_SEGMENTS	= 3;
	public final static int SPH_SORT_EXTENDED		= 4;

	/* grouping functions */
	public final static int SPH_GROUPBY_DAY			= 0;
	public final static int SPH_GROUPBY_WEEK		= 1;
	public final static int SPH_GROUPBY_MONTH		= 2;
	public final static int SPH_GROUPBY_YEAR		= 3;
	public final static int SPH_GROUPBY_ATTR		= 4;
	public final static int SPH_GROUPBY_ATTRPAIR	= 5;

	/* searchd reply status codes */
	public final static int SEARCHD_OK				= 0;
	public final static int SEARCHD_ERROR			= 1;
	public final static int SEARCHD_RETRY			= 2;
	public final static int SEARCHD_WARNING			= 3;

	/* attribute types */
	public final static int SPH_ATTR_INTEGER		= 1;
	public final static int SPH_ATTR_TIMESTAMP		= 2;
	public final static int SPH_ATTR_ORDINAL		= 3;
	public final static int SPH_ATTR_BOOL			= 4;
	public final static int SPH_ATTR_FLOAT			= 5;
	public final static int SPH_ATTR_MULTI			= 0x40000000;


	/* searchd commands */
	private final static int SEARCHD_COMMAND_SEARCH	= 0;
	private final static int SEARCHD_COMMAND_EXCERPT	= 1;
	private final static int SEARCHD_COMMAND_UPDATE	= 2;

	/* searchd command versions */
	private final static int VER_MAJOR_PROTO			= 0x1;
	private final static int VER_COMMAND_SEARCH		= 0x10F;
	private final static int VER_COMMAND_EXCERPT		= 0x100;
	private final static int VER_COMMAND_UPDATE		= 0x100;

	/* filter types */
	private final static int SPH_FILTER_VALUES		= 0;
	private final static int SPH_FILTER_RANGE			= 1;
	private final static int SPH_FILTER_FLOATRANGE	= 2;


	private String		_host;
	private int			_port;
	private int			_offset;
	private int			_limit;
	private int			_mode;
	private int[]		_weights;
	private int			_sort;
	private String		_sortby;
	private int			_minId;
	private int			_maxId;
	private ByteArrayOutputStream	_rawFilters;
	private DataOutputStream		_filters;
	private int			_filterCount;
	private String		_groupBy;
	private int			_groupFunc;
	private String		_groupSort;
	private String		_groupDistinct;
	private int			_maxMatches;
	private int			_cutoff;
	private int			_retrycount;
	private int			_retrydelay;
	private String		_latitudeAttr;
	private String		_longitudeAttr;
	private float		_latitude;
	private float		_longitude;
	private String		_error;
	private String		_warning;
	private ArrayList	_reqs;
	private Map			_indexWeights;

	private static final int SPH_CLIENT_TIMEOUT_MILLISEC	= 30000;


	/**
	 * Creates new SphinxClient instance.
	 *
	 * Default host and port that the instance will connect to are
	 * localhost:3312. That can be overriden using @see SetServer().
	 */
	public SphinxClient()
	{
		this("localhost", 3312);
	}

	/**
	 * Creates new SphinxClient instance, with host:port specification.
	 *
	 * Host and port can be later overriden using @see SetSever.
	 *
	 * @param host searchd host name (default: localhost)
	 * @param port searchd port number (default: 3312)
	 */
	public SphinxClient(String host, int port)
	{
		_host	= host;
		_port	= port;
		_offset	= 0;
		_limit	= 20;
		_mode	= SPH_MATCH_ALL;
		_sort	= SPH_SORT_RELEVANCE;
		_sortby	= "";
		_minId	= 0;
		_maxId	= 0xFFFFFFFF;

		_filterCount	= 0;
		_rawFilters		= new ByteArrayOutputStream();
		_filters		= new DataOutputStream(_rawFilters);

		_groupBy		= "";
		_groupFunc		= SPH_GROUPBY_DAY;
		_groupSort		= "@group desc";
		_groupDistinct	= "";

		_maxMatches		= 1000;
		_cutoff			= 0;
		_retrycount		= 0;
		_retrydelay		= 0;

		_latitudeAttr	= null;
		_longitudeAttr	= null;
		_latitude		= 0;
		_longitude		= 0;

		_error			= "";
		_warning		= "";

		_reqs			= new ArrayList();
		_weights		= null;
		_indexWeights = new LinkedHashMap();
	}

	/**
	 * Get last error message, if any.
	 *
	 * @return string with last error message (empty string if no errors occured)
	 */
	public String GetLastError()
	{
		return _error;
	}

	/**
	 * Get last warning message, if any.
	 *
	 * @return string with last error message (empty string if no errors occured)
	 */
	public String GetLastWarning()
	{
		return _warning;
	}

	/**
	 * Set searchd host and port to connect to.
	 *
	 * @param host searchd host name (default: localhost)
	 * @param port searchd port number (default: 3312)
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetServer(String host, int port) throws SphinxException
	{
		myAssert ( host!=null && host.length()>0, "host name must not be empty" );
		myAssert ( port>0 && port<65536, "port must be in 1..65535 range" );
		_host = host;
		_port = port;
	}

	/**
	 * Connect to searchd server.
	 * Sets internal <code>_error<code> on falure
	 *
	 * @return open socket on success, <code>null</code> on falure.
	 */
	private Socket _Connect()
	{
		Socket sock;
		try {
			sock = new Socket(_host, _port);
			sock.setSoTimeout(SPH_CLIENT_TIMEOUT_MILLISEC);
		} catch (IOException e) {
			_error = "connection to " + _host + ":" + _port + " failed:" + e.getMessage();
			return null;
		}

		DataInputStream sIn;
		DataOutputStream sOut;
		try {
			InputStream sockInput = sock.getInputStream();
			OutputStream sockOutput = sock.getOutputStream();
			sIn = new DataInputStream(sockInput);
			int version = sIn.readInt();
			if (version < 1) {
				sock.close();
				_error = "expected searchd protocol version 1+, got version '" + version + "'";
				return null;
			}
			sOut = new DataOutputStream(sockOutput);
			sOut.writeInt(VER_MAJOR_PROTO);
		} catch (IOException e) {
			_error = "Connect: Read from socket failed: " + e.getMessage();
			try {
				sock.close();
			} catch (IOException e1) {
				_error = _error + " Cannot close socket: " + e1.getMessage();
			}
			return null;
		}
		return sock;

	}

	/**
	 * Get and check response packet from searchd server.
	 *
	 * @param sock socket to read from
	 * @param client_ver searchd client version
	 * @return raw byte response
	 *
	 * @throws SphinxException on invalid parameters
	 */
	private byte[] _GetResponse(Socket sock, int client_ver) throws SphinxException
	{
		short status = 0, ver = 0;
		int len = 0;
		byte[] response = new byte[65536];

		DataInputStream sIn = null;
		InputStream SockInput = null;
		try
		{
			SockInput = sock.getInputStream();
			sIn = new DataInputStream(SockInput);
		} catch (IOException e)
		{
			myAssert ( false, "getInputStream() failed: " + e.getMessage() );
			return null;
		}

		try {
			/* read status fields */
			status = sIn.readShort();
			ver = sIn.readShort();
			len = sIn.readInt();

			/* read response if non-empty */
			myAssert(len > 0, "zero-sized searchd response reported");
			if (len > 0) {
				response = new byte[len];
				sIn.readFully(response, 0, len);
			} else
			{
				/* FIXME! no response, return null? */
			}

			/* check status */
			/* FIXME! fix error string conversion */
			if (status == SEARCHD_WARNING) {
				int wlen = sIn.readInt();
				byte[] byteWarn = new byte[wlen];
				sIn.readFully(byteWarn, 0, wlen);
				String warning = new String(byteWarn);
				_warning = warning;
				return null;
			} else if (status == SEARCHD_ERROR) {
				_error = "searchd error: " + new String(response);
				return null;
			} else if (status == SEARCHD_RETRY) {
				_error = "temporary searchd error: " + new String(response);
				return null;
			}

			if (status != SEARCHD_OK) {
				_error = "unknown status code '" + status + "'";
				return null;
			}

			/* check version */
			if (ver < client_ver) {
				_warning = "searchd command v." + (ver >> 8) + "." + (ver & 0xff) + "older than client's v." + (client_ver >> 8) + "." + (client_ver & 0xff) + ", some options might not work";
			}
		} catch (IOException e) {
			if (len != 0) {
				StringBuilder error = new StringBuilder();
				error.append("failed to read searchd response (status=").append(status);
				error.append(", ver=").append(ver);
				error.append(", len=").append(len);
				error.append(") reason ");
				error.append(e.getMessage());
				error.append(" trace:\n ");

				/* get trace here to let user know which request failed */
				StringWriter errStream = new StringWriter();
				PrintWriter errWriter = new PrintWriter(errStream);
				e.printStackTrace(errWriter);
				errWriter.flush();
				errWriter.close();

				error.append(errWriter.toString());
				_error = error.toString();
			} else {
				_error = _error + " Received zero-sized searchd response " + e.getMessage();
			}
			return null;
		} finally {
			try {
				if (sIn != null) sIn.close();
			} catch (IOException e) {
				_error = _error + " Unable to close searchd response input stream: " + e.getMessage();
			}
			try {
				if (sock != null && !sock.isConnected()) sock.close();
			} catch (IOException e) {
				_error = _error + " Unable to close searchd socket: " + e.getMessage();
			}
		}
		return response;
	}

	/**
	 * Set match offset, count, max number and cutoff
	 *
	 * @param offset result offset
	 * @param limit return limit items from result set
	 * @param max	override <code>max_matches</code> option from searchd config
	 * @param cutoff when to stop searching
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetLimits(int offset, int limit, int max, int cutoff) throws SphinxException
	{
		myAssert ( offset>=0, "offset must be greater than or equal to 0" );
		myAssert ( limit>0, "limit must be greater than 0"  );
		myAssert ( max>0, "max must be greater than 0" );
		myAssert ( cutoff>=0, "max must be greater than or equal to 0" );

		_offset = offset;
		_limit = limit;
		_maxMatches = max;
		_cutoff = cutoff;
	}

	/**
	 * Set search limits with default cutoff
	 *
	 * @param offset result offset
	 * @param limit return limit items from result set
	 * @param max	override <code>max_matches</code> option from searchd config
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetLimits(int offset, int limit, int max) throws SphinxException
	{
		SetLimits(offset, limit, max, 0);
	}

	/**
	 * Set search limits with default cutoff and max matches
	 *
	 * @param offset result offset
	 * @param limit return limit items from result set
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetLimits(int offset, int limit) throws SphinxException
	{
		SetLimits(offset, limit, 0, 0);
	}

	/**
	 * Set match mode.
	 *
	 * @param mode new match mode (MUST be one of SphinxClient.SPH_MATCH_* constants)
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetMatchMode(int mode) throws SphinxException
	{
		myAssert (
			mode==SPH_MATCH_ALL ||
			mode==SPH_MATCH_ANY ||
			mode==SPH_MATCH_PHRASE ||
			mode==SPH_MATCH_BOOLEAN ||
			mode==SPH_MATCH_EXTENDED, "unknown mode value; use one of the available SPH_MATCH_xxx constants" );
		_mode = mode;
	}

	/**
	 * Set sort mode.
	 *
	 * @param mode sort mode (MUST be one of SphinxClient.SPH_SORT_* constants)
	 * @param sortby field to use with sort
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetSortMode(int mode, String sortby) throws SphinxException
	{
		myAssert (
			mode==SPH_SORT_RELEVANCE ||
			mode==SPH_SORT_ATTR_DESC ||
			mode==SPH_SORT_ATTR_ASC ||
			mode==SPH_SORT_TIME_SEGMENTS ||
			mode==SPH_SORT_EXTENDED, "unknown mode value; use one of the available SPH_SORT_xxx constants" );
		myAssert ( mode==SPH_SORT_RELEVANCE || ( sortby!=null && sortby.length()>0 ), "sortby string must not be empty in selected mode" );

		_sort = mode;
		_sortby = ( sortby==null ) ? "" : sortby;
	}

	/**
	 * Set sort mode
	 *
	 * @param mode sort mode
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetSortMode(int mode) throws SphinxException
	{
		SetSortMode(mode, null);
	}

	/**
	 * Set per-field weights.
	 * Currently supported weights from 1 to 2^31-1. This range may change in furute.
	 *
	 * @param weights int array should contain positive weight
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetWeights(int[] weights) throws SphinxException
	{
		myAssert ( weights!=null, "weights must not be null" );
		for (int i = 0; i < weights.length; i++) {
			int weight = weights[i];
			myAssert ( weight>0, "all weights must be greater than 0" );
		}
		_weights = weights;
	}

	/**
	 * set per-index weights
	 *
	 * @param indexWeights Map <String, Integer> with IndexName => Weight pair
	 */
	public void SetIndexWeights(Map indexWeights)
	{
		_indexWeights = indexWeights;
	}

	/**
	 * set IDs range to match.
	 * Only match those records where document ID
	 * is beetwen min and max (including min and max)
	 *
	 * @param min min id to match
	 * @param max max id to match
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetIDRange(int min, int max) throws SphinxException
	{
		myAssert ( min<=max, "min must be less or equal to max" );
		_minId = min;
		_maxId = max;
	}

	/**
	 * Set values filter.
	 * only match those records where $attribute column values
	 * are in specified set
	 *
	 * @param attribute attribute name
	 * @param values	values to match
	 * @param exclude invert matches
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetFilter(String attribute, int[] values, boolean exclude) throws SphinxException
	{
		myAssert ( values!=null && values.length>0, "values array must not be null or empty" );
		myAssert ( attribute!=null && attribute.length()>0, "attribute name must not be null or empty" );

		try
		{
			writeNetUTF8(_filters, attribute);
			_filters.writeInt(SPH_FILTER_VALUES);
			_filters.writeInt(values.length);
			for (int i = 0; i < values.length; i++)
				_filters.writeInt(values[i]);
			_filters.writeInt(exclude ? 1 : 0);
		} catch (Exception e)
		{
			myAssert ( false, "IOException: " + e.getMessage() );
		}
		_filterCount++;
	}

	public void SetFilter(String attribute, int value, boolean exclude) throws SphinxException
	{
		int[] values = new int[]{value};
		SetFilter(attribute, values, exclude);
	}

	/**
	 * Set integer range filter.
	 *
	 * Only match those records where <code>attribute</code> column values
	 * is beetwen <code>min</code> and <code>max</code> (including
	 * <code>min</code> and <code>max</code>)
	 *
	 * @param attribute		attribute name to filter by
	 * @param min			min attribute value
	 * @param max			max attribute value
	 * @param exclude		exclude-filter or include-filter flag
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetFilterRange ( String attribute, int min, int max, boolean exclude ) throws SphinxException
	{
		myAssert ( min<=max, "min must be less or equal to max" );
		try
		{
			writeNetUTF8(_filters, attribute);
			_filters.writeInt(SPH_FILTER_RANGE);
			_filters.writeInt(min);
			_filters.writeInt(max);
			_filters.writeInt(exclude ? 1 : 0);
		} catch (Exception e)
		{
			myAssert ( false, "IOException: " + e.getMessage() );
		}
		_filterCount++;
	}

	/**
	 * Set float range filter.
	 *
	 * Only match those records where <code>attribute</code> column values
	 * is beetwen <code>min</code> and <code>max</code> (including
	 * <code>min</code> and <code>max</code>)
	 *
	 * @param attribute		attribute name to filter by
	 * @param min			min attribute value
	 * @param max			max attribute value
	 * @param exclude		exclude-filter or include-filter flag
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetFilterFloatRange ( String attribute, float min, float max, boolean exclude ) throws SphinxException
	{
		myAssert ( min<=max, "min must be less or equal to max" );
		try
		{
			writeNetUTF8(_filters, attribute);
			_filters.writeInt(SPH_FILTER_RANGE);
			_filters.writeFloat(min);
			_filters.writeFloat(max);
			_filters.writeInt(exclude ? 1 : 0);
		} catch (Exception e)
		{
			myAssert ( false, "IOException: " + e.getMessage() );
		}
		_filterCount++;
	}

	/**
	 * Setup geographical anchor point.
	 *
	 * Required to use @geodist in filters and sorting.
	 * Distance will be computed to this point
	 *
	 * @param latitudeAttr		the name of latitude attribute
	 * @param longitudeAttr		the name of longitude attribute
	 * @param latitude			anchor point latitude, in radians
	 * @param longitude			anchor point longitude, in radians
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetGeoAnchor ( String latitudeAttr, String longitudeAttr, float latitude, float longitude ) throws SphinxException
	{
		myAssert ( latitudeAttr!=null && latitudeAttr.length()>0, "longitudeAttr string must not be null or empty" );
		myAssert ( longitudeAttr!=null && longitudeAttr.length()>0, "longitudeAttr string must not be null or empty" );

		_latitudeAttr = latitudeAttr;
		_longitudeAttr = longitudeAttr;
		_latitude = latitude;
		_longitude = longitude;
	}

	/**
	 * Set grouping attribute and function.
	 * <p/>
	 * in grouping mode, all matches are assigned to different groups
	 * based on grouping function value.
	 * <p/>
	 * each group keeps track of the total match count, and the best match
	 * (in this group) according to current sorting function.
	 * <p/>
	 * the final result set contains one best match per group, with
	 * grouping function value and matches count attached.
	 * <p/>
	 * result set could be sorted either by 1) grouping function value
	 * in descending order (this is the default mode, when $sortbygroup
	 * is set to true); or by 2) current sorting function (when $sortbygroup
	 * is false).
	 * <p/>
	 * WARNING, when sorting by current function there might be less
	 * matching groups reported than actually present. @count might also be
	 * underestimated.
	 * <p/>
	 * for example, if sorting by relevance and grouping by "published"
	 * attribute with SPH_GROUPBY_DAY function, then the result set will
	 * contain one most relevant match per each day when there were any
	 * matches published, with day number and per-day match count attached,
	 * and sorted by day number in descending order (ie. recent days first).
	 *
	 * @param attribute grouping attribute name
	 * @param func	func type (See predefined SPH_GROUPBY_* constants for details)
	 * @param groupsort soft type ("@group desc" by default)
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetGroupBy(String attribute, int func, String groupsort) throws SphinxException
	{
		myAssert (
			func==SPH_GROUPBY_DAY ||
			func==SPH_GROUPBY_WEEK ||
			func==SPH_GROUPBY_MONTH ||
			func==SPH_GROUPBY_YEAR ||
			func==SPH_GROUPBY_ATTR ||
			func==SPH_GROUPBY_ATTRPAIR, "unknown func value; use one of the available SPH_GROUPBY_xxx constants" );

		_groupBy = attribute;
		_groupFunc = func;
		_groupSort = groupsort;
	}

	/**
	 * Set grouping attribute and function with default ("@group desc") groupsort
	 *
	 * @param attribute attribute to group by
	 * @param func	groupping type
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetGroupBy(String attribute, int func) throws SphinxException
	{
		SetGroupBy(attribute, func, "@group desc");
	}

	/**
	 * set count-distinct attribute for group-by queries
	 *
	 * @param attribute group by attribute
	 */
	public void SetGroupDistinct(String attribute)
	{
		_groupDistinct = attribute;
	}

	/**
	 * set distributed retries count and delay
	 *
	 * @param count retry count
	 * @param delay retry delay
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetRetries(int count, int delay) throws SphinxException
	{
		myAssert ( count>=0, "count must not be negative" );
		myAssert ( delay>=0, "delay must not be negative" );
		_retrycount = count;
		_retrydelay = delay;
	}

	/**
	 * set distributed retries count with zero delay
	 *
	 * @param count retry count
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public void SetRetries(int count) throws SphinxException
	{
		SetRetries(count, 0);
	}

	/**
	 * clear all filters (for multi-queries)
	 */
	public void ResetFilters()
	{
		/* should we close them first? */
		_rawFilters = new ByteArrayOutputStream();
		_filters = new DataOutputStream(_rawFilters);
		_filterCount = 0;

		/* reset GEO anchor */
		_latitudeAttr = null;
		_longitudeAttr = null;
		_latitude = 0;
		_longitude = 0;
	}

	/**
	 * connect to searchd server and run given search query against all indexes
	 *
	 * @param query string
	 * @return {@link SphinxResult} result set or null on error
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public SphinxResult Query(String query) throws SphinxException
	{
		return Query(query, "*");
	}

	/**
	 * Connect to searchd server and run given search query.
	 *
	 * @param query query string
	 * @param index index name to query, can be "*" which means to query all indexes
	 * (default for <code>Query(String query)</code>)
	 * @return {@link SphinxResult} result set object, which contains retrieved
	 * matches as well as some per-collection statistics (total found matches,
	 * query time, per-word statistics, etc)
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public SphinxResult Query(String query, String index) throws SphinxException
	{
		myAssert ( _reqs==null || _reqs.size()==0, "AddQuery() and Query() can not be combined; use RunQueries() instead" );

		AddQuery(query, index);
		SphinxResult[] results = RunQueries();
		if (results == null || results.length < 1) {
			return null; /* probably network error; error message should be already filled */
		}


		SphinxResult res = results[0];
		_warning = res.warning;
		_error = res.error;
		if (res == null || res.getStatus() == SEARCHD_ERROR) {
			return null;
		} else {
			return res;
		}
	}

	public synchronized int AddQuery(String query, String index) throws SphinxException
	{
		ByteArrayOutputStream req = new ByteArrayOutputStream();

		/* build request */
		try {
			DataOutputStream out = new DataOutputStream(req);
			out.writeInt(_offset);
			out.writeInt(_limit);
			out.writeInt(_mode);
			out.writeInt(_sort);
			writeNetUTF8(out, _sortby);
			writeNetUTF8(out, query);
			int weightLen = _weights != null ? _weights.length : 0;

			out.writeInt(weightLen);
			if (_weights != null) {
				for (int i = 0; i < _weights.length; i++)
					out.writeInt(_weights[i]);
			}

			writeNetUTF8(out, index);
			out.writeInt(0);
			out.writeInt(_minId);
			out.writeInt(_maxId);

			/* filters */
			out.writeInt(_filterCount);
			out.write(_rawFilters.toByteArray());

			/* group-by, max matches, sort-by-group flag */
			out.writeInt(_groupFunc);
			writeNetUTF8(out, _groupBy);
			out.writeInt(_maxMatches);
			writeNetUTF8(out, _groupSort);

			out.writeInt(_cutoff);
			out.writeInt(_retrycount);
			out.writeInt(_retrydelay);

			writeNetUTF8(out, _groupDistinct);

			/* anchor point */
			if (_latitudeAttr == null || _latitudeAttr.length() == 0 || _longitudeAttr == null || _longitudeAttr.length() == 0) {
				out.writeInt(0);
			} else {
				out.writeInt(1);
				writeNetUTF8(out, _latitudeAttr);
				writeNetUTF8(out, _longitudeAttr);
				out.writeFloat(_latitude);
				out.writeFloat(_longitude);

			}

			/* per-index weights */
			out.writeInt(_indexWeights.size());
			for (Iterator e = _indexWeights.keySet().iterator(); e.hasNext();) {
				String indexName = (String) e.next();
				Integer weight = (Integer) _indexWeights.get(indexName);
				writeNetUTF8(out, indexName);
				out.writeInt(weight.intValue());
			}
			out.flush();
			int qIndex = _reqs.size();
			_reqs.add(qIndex, req.toByteArray());
			return qIndex;
		} catch (Exception ex) {
			myAssert(false, "error on AddQuery: " + ex.getMessage());
		} finally {
			try {
				_filters.close();
				_rawFilters.close();
			} catch (IOException e) {
				myAssert(false, "error on AddQuery: " + e.getMessage());
			}
		}
		return -1;
	}

	public SphinxResult[] RunQueries() throws SphinxException
	{
		if (_reqs == null || _reqs.size() < 1) {
			_error = "no queries defined, issue AddQuery() first";
			return null;
		}

		Socket sock = _Connect();
		if (sock == null) return null;

		/* send query, get response */
		ByteArrayOutputStream req = new ByteArrayOutputStream();
		DataOutputStream prepareRQ = null;
		int nreqs = _reqs.size();
		try {
			prepareRQ = new DataOutputStream(req);
			prepareRQ.writeShort(SEARCHD_COMMAND_SEARCH);
			prepareRQ.writeShort(VER_COMMAND_SEARCH);
			int rqLen = 4;
			for (int i = 0; i < nreqs; i++) {
				byte[] subRq = (byte[]) _reqs.get(i);
				rqLen += subRq.length;
			}
			prepareRQ.writeInt(rqLen);
			prepareRQ.writeInt(nreqs);
			for (int i = 0; i < nreqs; i++) {
				byte[] subRq = (byte[]) _reqs.get(i);
				prepareRQ.write(subRq);
			}
			OutputStream SockOut = sock.getOutputStream();
			byte[] reqBytes = req.toByteArray();
			SockOut.write(reqBytes);
		} catch (Exception e) {
			myAssert(false, "Query: Unable to create read/write streams: " + e.getMessage());
			return null;
		}

		/* reset requests */
		_reqs = new ArrayList();

		/* get response */
		byte[] response = null;
		response = _GetResponse(sock, VER_COMMAND_SEARCH);
		if (response == null) return null;

		/* parse response */
		SphinxResult[] results = new SphinxResult[nreqs];

		DataInputStream in;
		in = new DataInputStream(new ByteArrayInputStream(response));

		/* read schema */
		int ires;
		try {
			for (ires = 0; ires < nreqs; ires++) {
				SphinxResult res = new SphinxResult();
				results[ires] = res;

				int status = in.readInt();
				res.setStatus(status);
				if (status != SEARCHD_OK) {
					String message = readNetUTF8(in);
					if (status == SEARCHD_WARNING) {
						res.warning = message;
					} else {
						res.error = message;
						continue;
					}
				}

				/* read fields */
				int nfields = in.readInt();
				res.fields = new String[nfields];
				int pos = 0;
				for (int i = 0; i < nfields; i++)
					res.fields[i] = readNetUTF8(in);

				/* read arrts */
				int nattrs = in.readInt();
				res.attrTypes = new int[nattrs];
				res.attrNames = new String[nattrs];
				for (int i = 0; i < nattrs; i++) {
					String AttrName = readNetUTF8(in);
					int AttrType = in.readInt();
					res.attrNames[i] = AttrName;
					res.attrTypes[i] = AttrType;
				}

				/* read match count */
				int count = in.readInt();
				int id64 = in.readInt();
				res.matches = new SphinxDocInfo[count];
				for (int matchesNo = 0; matchesNo < count; matchesNo++) {
					SphinxDocInfo docInfo;
					if (id64 != 0) {
						int docHi = in.readInt();
						int docLo = in.readInt();
						long doc64 = docHi;
						doc64 = doc64 << 32 + docLo;
						int weight = in.readInt();
						docInfo = new SphinxDocInfo(doc64, weight);
					} else {
						int docId = in.readInt();
						int weight = in.readInt();
						docInfo = new SphinxDocInfo(docId, weight);
					}

					/* read matches */
					for (int attrNumber = 0; attrNumber < res.attrTypes.length; attrNumber++)
					{
						String attrName = res.attrNames[attrNumber];
						int type = res.attrTypes[attrNumber];

						/* handle floats */
						if (type == SPH_ATTR_FLOAT) {
							float value = in.readFloat();
							docInfo.setAttr(attrNumber, value);
							continue;
						}

						/* handle everything else as unsigned ints */
						int val = in.readInt();
						if ((type & SPH_ATTR_MULTI) != 0) {
							int[] vals = new int[val];
							for (int k = 0; k < val; k++) {
								vals[k] = in.readInt();
							}
							docInfo.setAttr(attrName, vals);
						} else {
							docInfo.setAttr(attrNumber, val);
						}
						docInfo.setAttr(attrNumber, val);
					}
					res.matches[matchesNo] = docInfo;
				}

				res.total = in.readInt();
				res.totalFound = in.readInt();
				res.time = in.readInt() / 1000; /* FIXME! format should be %.3f */
				int wordCount = in.readInt();

				res.words = new SphinxWordInfo[wordCount];
				for (int i = 0; i < wordCount; i++) {
					String word = readNetUTF8(in);
					int docs = in.readInt();
					int hits = in.readInt();
					SphinxWordInfo winfo = new SphinxWordInfo(word, docs, hits);
					res.words[i] = winfo;
				}
			}
			in.close();
			return results;
		} catch (IOException e) {
			myAssert(false, "Query: Unable to parse response: " + e.getMessage());
		} finally {
			try {
				in.close();
			} catch (IOException e) {
				myAssert(false, "Query: Unable to close datain stream : " + e.getMessage());
			}
		}

		return null;
	}

	/**
	 * connect to searchd server and generate exceprts from given documents.
	 *
	 * @param docs is an array of strings which represent the documents' contents
	 * @param index is a string specifiying the index which settings will be used for stemming, lexing and case folding
	 * @param words is a string which contains the words to highlight
	 * @param opts opts is a hash which contains additional optional highlighting parameters:
	 * "before_match" - a string to insert before a set of matching words, default is "<b>";
	 * "after_match" - a string to insert after a set of matching words, default is "<b>";
	 * "chunk_separator" - a string to insert between excerpts chunks, default is " ... ";
	 * "limit" - max excerpt size in symbols (codepoints), default is 256;
	 * "around" - how much words to highlight around each match, default is 5
	 * @return false on failure, an array of string excerpts on success
	 *
	 * @throws SphinxException on invalid parameters
	 */
	public String[] BuildExcerpts(String[] docs, String index, String words, Map opts) throws SphinxException
	{
		myAssert(docs != null && docs.length > 0, "BuildExcerpts: Have no documents to process");
		myAssert(index != null && index.length() > 0, "BuildExcerpts: Have no index to process documents");
		myAssert(words != null && words.length() > 0, "BuildExcerpts: Have no words to highlight");
		if (opts == null) opts = new LinkedHashMap();

		Socket sock = _Connect();
		if (sock == null) return null;

		/* fixup options */
		if (!opts.containsKey("before_match")) opts.put("before_match", "<b>");
		if (!opts.containsKey("after_match")) opts.put("after_match", "</b>");
		if (!opts.containsKey("chunk_separator")) opts.put("chunk_separator", "...");
		if (!opts.containsKey("limit")) opts.put("limit", new Integer(256));
		if (!opts.containsKey("around")) opts.put("around", new Integer(5));

		/* build request */
		ByteArrayOutputStream req = new ByteArrayOutputStream();
		DataOutputStream rqData = null;
		DataOutputStream socketDS = null;
		try {
			rqData = new DataOutputStream(req);

			/* v.1.0 req */
			rqData.writeInt(0);
			rqData.writeInt(1);
			writeNetUTF8(rqData, index);
			writeNetUTF8(rqData, words);

			/* send options */
			writeNetUTF8(rqData, (String) opts.get("before_match"));
			writeNetUTF8(rqData, (String) opts.get("after_match"));
			writeNetUTF8(rqData, (String) opts.get("chunk_separator"));
			rqData.writeInt(((Integer) opts.get("limit")).intValue());
			rqData.writeInt(((Integer) opts.get("around")).intValue());

			/* send documents */
			for (int i = 0; i < docs.length; i++) {
				myAssert(docs[i] != null, "BuildExcerpts: empty document #" + i);
				writeNetUTF8(rqData, docs[i]);
			}

			rqData.flush();
			byte[] byteRq = req.toByteArray();

			/* send query, get response */
			OutputStream SockOut = sock.getOutputStream();
			socketDS = new DataOutputStream(SockOut);
			socketDS.writeShort(SEARCHD_COMMAND_EXCERPT);
			socketDS.writeShort(VER_COMMAND_EXCERPT);
			socketDS.writeInt(byteRq.length + 8);
			socketDS.write(byteRq);

		} catch (Exception ex) {
			myAssert(false, "BuildExcerpts: Unable to create read/write streams: " + ex.getMessage());
			return null;
		}

		try {
			/* get response */
			byte[] response = null;
			String[] docsXrpt = new String[docs.length];
			response = _GetResponse(sock, VER_COMMAND_SEARCH);
			if (response == null) return null;

			/* parse response */
			DataInputStream in;
			in = new DataInputStream(new ByteArrayInputStream(response));
			for (int i = 0; i < docs.length; i++) {
				docsXrpt[i] = readNetUTF8(in);
			}
			return docsXrpt;
		} catch (Exception e) {
			myAssert(false, "BuildExcerpts: incomplete response " + e.getMessage());
			return null;
		}
	}

	/**
	 * Internal sanity check syntax sugar.
	 */
	private void myAssert ( boolean condition, String err ) throws SphinxException
	{
		if ( !condition )
		{
			_error = err;
			throw new SphinxException ( err );
		}
	}

	/**
	 * java to sphinx String protocol hack
	 * Adds lead 2 zero bytes to stream before string length to make length 4 bytes
	 * as expected by search server
	 *
	 * @param ostream output stream to write <code>String</code> to
	 * @param str	 <code>String</code> to send to sphinx server
	 */
	private static void writeNetUTF8(DataOutputStream ostream, String str) throws IOException
	{
		ostream.writeShort(0);
		ostream.writeUTF(str);
	}

	/**
	 * sphinx to java String protocol hack
	 * Reads first 2 lead bytes from stream before string length to make length
	 * 2 bytes as expected by Java modified UTF8 (see also {@link DataInputStream})
	 *
	 * @param istream output stream to write <code>String</code> to
	 * @return str <code>String</code> sent from sphinx server
	 */
	private static String readNetUTF8(DataInputStream istream) throws IOException
	{
		int dummy = istream.readUnsignedShort(); /* searchd emits dwords, Java expects words */
		String value = istream.readUTF();
		return value;
	}
}

/*
 * $Id$
 */
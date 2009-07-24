//
// $Id$
//

#include "sphinx.h"
#include "sphinxint.h"
#include "sphinxrt.h"
#include "sphinxsearch.h"

//////////////////////////////////////////////////////////////////////////

#define	COMPRESSED_WORDLIST		0
#define	COMPRESSED_DOCLIST		1
#define COMPRESSED_HITLIST		1

#if USE_64BIT
#define WORDID_MAX				U64C(0xffffffffffffffff)
#else
#define	WORDID_MAX				0xffffffffUL
#endif

//////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
#define Verify(_expr) assert(_expr)
#else
#define Verify(_expr) _expr
#endif

//////////////////////////////////////////////////////////////////////////

// !COMMIT cleanup extern ref to sphinx.cpp
extern void sphSortDocinfos ( DWORD * pBuf, int iCount, int iStride );

// !COMMIT yes i am when debugging
#ifndef NDEBUG
#define PARANOID 1
#endif

//////////////////////////////////////////////////////////////////////////

template < typename T >
static inline void ZipT ( CSphVector<BYTE> & dOut, T uValue )
{
	do 
	{
		BYTE bOut = BYTE( uValue & 0x7f );
		uValue >>= 7;
		if ( uValue )
			bOut |= 0x80;
		dOut.Add ( bOut );
	} while ( uValue );
}


template < typename T >
static inline const BYTE * UnzipT ( T * pValue, const BYTE * pIn )
{
	T uValue = 0;
	BYTE bIn;
	int iOff = 0;

	do 
	{
		bIn = *pIn++;
		uValue += (T( bIn & 0x7f )) << iOff;
		iOff += 7;
	} while ( bIn & 0x80 );

	*pValue = uValue;
	return pIn;
}

#define ZipDword ZipT<DWORD>
#define ZipQword ZipT<uint64_t>
#define UnzipDword UnzipT<DWORD>
#define UnzipQword UnzipT<uint64_t>

#if USE_64BIT
#define ZipDocid ZipQword
#define ZipWordid ZipQword
#define UnzipDocid UnzipQword
#define UnzipWordid UnzipQword
#else
#define ZipDocid ZipDword
#define ZipWordid ZipDword
#define UnzipDocid UnzipDword
#define UnzipWordid UnzipDword
#endif

//////////////////////////////////////////////////////////////////////////

struct CmpHit_fn
{
	inline bool IsLess ( const CSphWordHit & a, const CSphWordHit & b )
	{
		return 	( a.m_iWordID < b.m_iWordID ) ||
			( a.m_iWordID == b.m_iWordID && a.m_iDocID < b.m_iDocID ) || 
			( a.m_iWordID == b.m_iWordID && a.m_iDocID == b.m_iDocID && a.m_iWordPos < b.m_iWordPos );
	}
};


struct RtDoc_t
{
	SphDocID_t					m_uDocID;	///< my document id
	DWORD						m_uFields;	///< fields mask
	DWORD						m_uHits;	///< hit count
	DWORD						m_uHit;		///< either index into segment hits, or the only hit itself (if hit count is 1)
};


struct RtWord_t
{
	SphWordID_t					m_uWordID;	///< my keyword id
	DWORD						m_uDocs;	///< document count (for stats and/or BM25)
	DWORD						m_uHits;	///< hit count (for stats and/or BM25)
	DWORD						m_uDoc;		///< index into segment docs
};


struct RtSegment_t
{
protected:
	static const int			KLIST_ACCUM_THRESH	= 32;

public:
	static CSphMutex			m_tSegmentSeq;
	static int					m_iSegments;	///< age tag sequence generator
	int							m_iTag;			///< segment age tag

#if COMPRESSED_WORDLIST
	CSphVector<BYTE>			m_dWords;
#else
	CSphVector<RtWord_t>		m_dWords;
#endif

#if COMPRESSED_DOCLIST
	CSphVector<BYTE>			m_dDocs;
#else
	CSphVector<RtDoc_t>			m_dDocs;
#endif

#if COMPRESSED_HITLIST
	CSphVector<BYTE>			m_dHits;
#else
	CSphVector<DWORD>			m_dHits;
#endif

	int							m_iRows;		///< number of actually allocated rows
	int							m_iAliveRows;	///< number of alive (non-killed) rows
	CSphVector<CSphRowitem>		m_dRows;		///< row data storage
	CSphVector<SphDocID_t>		m_dKlist;		///< sorted K-list
	bool						m_bTlsKlist;	///< whether to apply TLS K-list during merge (only used during Commit())

	RtSegment_t ()
	{
		m_tSegmentSeq.Lock ();
		m_iTag = m_iSegments++;
		m_tSegmentSeq.Unlock ();
		m_iRows = 0;
		m_iAliveRows = 0;
		m_bTlsKlist = false;
	}

	int GetSizeBytes () const
	{
		return
			m_dWords.GetLength()*sizeof(m_dWords[0]) +
			m_dDocs.GetLength()*sizeof(m_dDocs[0]) +
			m_dHits.GetLength()*sizeof(m_dHits[0]);
	}

	int GetMergeFactor () const
	{
		return m_iRows;
	}

	bool HasDocid ( SphDocID_t uDocid ) const;
};

int RtSegment_t::m_iSegments = 0;
CSphMutex RtSegment_t::m_tSegmentSeq;

bool RtSegment_t::HasDocid ( SphDocID_t uDocid ) const
{
	// binary search through the rows
	int iStride = m_dRows.GetLength() / m_iRows;
	SphDocID_t uL = DOCINFO2ID(&m_dRows[0]);
	SphDocID_t uR = DOCINFO2ID(&m_dRows[m_dRows.GetLength()-iStride]);
	if ( uDocid==uL || uDocid==uR )
		return true;
	if ( uDocid<uL || uDocid>uR )
		return false;

	int iL = 0;
	int iR = m_iRows-1;
	while ( iR-iL>1 )
	{
		int iM = iL + (iR-iL)/2;
		SphDocID_t uM = DOCINFO2ID(&m_dRows[iM*iStride]);

		if ( uDocid==uM )
			return true;
		else if ( uDocid>uM )
			iL = iM;
		else
			iR = iM;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

#if COMPRESSED_DOCLIST

struct RtDocWriter_t
{
	CSphVector<BYTE> *	m_pDocs;
	SphDocID_t			m_uLastDocID;

	explicit RtDocWriter_t ( RtSegment_t * pSeg )
		: m_pDocs ( &pSeg->m_dDocs )
		, m_uLastDocID ( 0 )
	{}

	void ZipDoc ( const RtDoc_t & tDoc )
	{
		CSphVector<BYTE> & dDocs = *m_pDocs;
		ZipDocid ( dDocs, tDoc.m_uDocID - m_uLastDocID );
		m_uLastDocID = tDoc.m_uDocID;
		ZipDword ( dDocs, tDoc.m_uFields );
		ZipDword ( dDocs, tDoc.m_uHits );
		if ( tDoc.m_uHits==1 )
		{
			ZipDword ( dDocs, tDoc.m_uHit & 0xffffffUL );
			ZipDword ( dDocs, tDoc.m_uHit>>24 );
		} else
			ZipDword ( dDocs, tDoc.m_uHit );
	}

	DWORD ZipDocPtr () const
	{
		return m_pDocs->GetLength();
	}

	void ZipRestart ()
	{
		m_uLastDocID = 0;
	}
};

struct RtDocReader_t
{
	const BYTE *	m_pDocs;
	int				m_iLeft;
	RtDoc_t			m_tDoc;

	explicit RtDocReader_t ( const RtSegment_t * pSeg, const RtWord_t & tWord )
	{
		m_pDocs = &pSeg->m_dDocs[0] + tWord.m_uDoc;
		m_iLeft = tWord.m_uDocs;
		m_tDoc.m_uDocID = 0;
	}

	const RtDoc_t * UnzipDoc ()
	{
		if ( !m_iLeft )
			return NULL;

		const BYTE * pIn = m_pDocs;
		SphDocID_t uDeltaID;			 
		pIn = UnzipDocid ( &uDeltaID, pIn );
		m_tDoc.m_uDocID += uDeltaID;
		pIn = UnzipDword ( &m_tDoc.m_uFields, pIn );
		pIn = UnzipDword ( &m_tDoc.m_uHits, pIn );
		if ( m_tDoc.m_uHits==1 )
		{
			DWORD a, b;
			pIn = UnzipDword ( &a, pIn );
			pIn = UnzipDword ( &b, pIn );
			m_tDoc.m_uHit = a + ( b<<24 );
		} else
			pIn = UnzipDword ( &m_tDoc.m_uHit, pIn );
		m_pDocs = pIn;

		m_iLeft--;
		return &m_tDoc;
	}
};

#else

struct RtDocWriter_t
{
	CSphVector<RtDoc_t> *	m_pDocs;

	explicit				RtDocWriter_t ( RtSegment_t * pSeg )	: m_pDocs ( &pSeg->m_dDocs ) {}
	void					ZipDoc ( const RtDoc_t & tDoc )			{ m_pDocs->Add ( tDoc ); }
	DWORD					ZipDocPtr () const						{ return m_pDocs->GetLength(); }
	void					ZipRestart ()							{}
};


struct RtDocReader_t
{
	const RtDoc_t *	m_pDocs;
	int				m_iPos;
	int				m_iMax;

	explicit RtDocReader_t ( const RtSegment_t * pSeg, const RtWord_t & tWord )
	{
		m_pDocs = &pSeg->m_dDocs[0];
		m_iPos = tWord.m_uDoc;
		m_iMax = tWord.m_uDoc + tWord.m_uDocs;
	}

	const RtDoc_t * UnzipDoc ()
	{
		return m_iPos<m_iMax ? m_pDocs + m_iPos++ : NULL;
	}
};

#endif // COMPRESSED_DOCLIST


#if COMPRESSED_WORDLIST

struct RtWordWriter_t
{
	CSphVector<BYTE> *	m_pWords;
	SphWordID_t			m_uLastWordID;
	SphDocID_t			m_uLastDoc;

	explicit RtWordWriter_t ( RtSegment_t * pSeg )
		: m_pWords ( &pSeg->m_dWords )
		, m_uLastWordID ( 0 )
		, m_uLastDoc ( 0 )
	{}

	void ZipWord ( const RtWord_t & tWord )
	{
		CSphVector<BYTE> & tWords = *m_pWords;
		ZipWordid ( tWords, tWord.m_uWordID - m_uLastWordID );
		ZipDword ( tWords, tWord.m_uDocs );
		ZipDword ( tWords, tWord.m_uHits );
		ZipDocid ( tWords, tWord.m_uDoc - m_uLastDoc );
		m_uLastWordID = tWord.m_uWordID;
		m_uLastDoc = tWord.m_uDoc;
	}
};


struct RtWordReader_t
{
	const BYTE *	m_pCur;
	const BYTE *	m_pMax;
	RtWord_t		m_tWord;	

	explicit RtWordReader_t ( const RtSegment_t * pSeg )
	{
		m_pCur = &pSeg->m_dWords[0];
		m_pMax = m_pCur + pSeg->m_dWords.GetLength();

		m_tWord.m_uWordID = 0;
		m_tWord.m_uDoc = 0;
	}

	const RtWord_t * UnzipWord ()
	{
		if ( m_pCur>=m_pMax )
			return NULL;

		const BYTE * pIn = m_pCur;
		SphWordID_t uDeltaID;
		SphDocID_t uDeltaDoc;
		pIn = UnzipWordid ( &uDeltaID, pIn );
		pIn = UnzipDword ( &m_tWord.m_uDocs, pIn );
		pIn = UnzipDword ( &m_tWord.m_uHits, pIn );
		pIn = UnzipDocid ( &uDeltaDoc, pIn );
		m_pCur = pIn;

		m_tWord.m_uWordID += uDeltaID;
		m_tWord.m_uDoc += uDeltaDoc;
		return &m_tWord;
	}
};

#else

struct RtWordWriter_t
{
	CSphVector<RtWord_t> *	m_pWords;

	explicit				RtWordWriter_t ( RtSegment_t * pSeg )	: m_pWords ( &pSeg->m_dWords ) {}
	void					ZipWord ( const RtWord_t & tWord )		{ m_pWords->Add ( tWord ); }
};


struct RtWordReader_t
{
	const RtWord_t *	m_pCur;
	const RtWord_t *	m_pMax;

	explicit RtWordReader_t ( const RtSegment_t * pSeg )
	{
		m_pCur = &pSeg->m_dWords[0];
		m_pMax = m_pCur + pSeg->m_dWords.GetLength();
	}

	const RtWord_t * UnzipWord ()
	{
		return m_pCur<m_pMax ? m_pCur++ : NULL;
	}
};

#endif // COMPRESSED_WORDLIST


#if COMPRESSED_HITLIST

struct RtHitWriter_t
{
	CSphVector<BYTE> *	m_pHits;
	DWORD				m_uLastHit;

	explicit RtHitWriter_t ( RtSegment_t * pSeg )
		: m_pHits ( &pSeg->m_dHits )
		, m_uLastHit ( 0 )
	{}

	void ZipHit ( DWORD uValue )
	{
		ZipDword ( *m_pHits, uValue - m_uLastHit );
		m_uLastHit = uValue;
	}

	void ZipRestart ()
	{
		m_uLastHit = 0;
	}

	DWORD ZipHitPtr () const
	{
		return m_pHits->GetLength();
	}
};


struct RtHitReader_t
{
	const BYTE *	m_pCur;
	DWORD			m_iLeft;
	DWORD			m_uLast;

	RtHitReader_t ()
		: m_pCur ( NULL )
		, m_iLeft ( 0 )
		, m_uLast ( 0 )
	{}

	explicit RtHitReader_t ( const RtSegment_t * pSeg, const RtDoc_t * pDoc )
	{
		m_pCur = &pSeg->m_dHits [ pDoc->m_uHit ];
		m_iLeft = pDoc->m_uHits;
		m_uLast = 0;
	}

	DWORD UnzipHit ()
	{
		if ( !m_iLeft )
			return 0;

		DWORD uValue;
		m_pCur = UnzipDword ( &uValue, m_pCur );
		m_uLast += uValue;
		m_iLeft--;
		return m_uLast;
	}
};


struct RtHitReader2_t : public RtHitReader_t
{
	const BYTE * m_pBase;

	RtHitReader2_t ()
		: m_pBase ( NULL )
	{}

	void Seek ( SphOffset_t uOff, int iHits )
	{
		m_pCur = m_pBase + uOff;
		m_iLeft = iHits;
		m_uLast = 0;
	}
};

#else

struct RtHitWriter_t
{
	CSphVector<DWORD> *	m_pHits;

	explicit			RtHitWriter_t ( RtSegment_t * pSeg )	: m_pHits ( &pSeg->m_dHits ) {}
	void				ZipHit ( DWORD uValue )					{ m_pHits->Add ( uValue ); }
	void				ZipRestart ()							{}
	DWORD				ZipHitPtr () const						{ return m_pHits->GetLength(); }
};

struct RtHitReader_t
{
	const DWORD * m_pCur;
	const DWORD * m_pMax;

	explicit RtHitReader_t ( const RtSegment_t * pSeg, const RtDoc_t * pDoc )
	{
		m_pCur = &pSeg->m_dHits [ pDoc->m_uHit ];
		m_pMax = m_pCur + pDoc->m_uHits;
	}

	DWORD UnzipHit ()
	{
		return m_pCur<m_pMax ? *m_pCur++ : 0;
	}
};

#endif // COMPRESSED_HITLIST

//////////////////////////////////////////////////////////////////////////

#if USE_WINDOWS
#define __thread __declspec(thread)
#endif

/// forward ref
struct RtIndex_t;

/// indexing accumulator
struct RtAccum_t
{
	RtIndex_t *					m_pIndex;		///< my current owner in this thread
	int							m_iAccumDocs;
	CSphVector<CSphWordHit>		m_dAccum;
	CSphVector<CSphRowitem>		m_dAccumRows;
	CSphVector<SphDocID_t>		m_dAccumKlist;

					RtAccum_t() : m_pIndex ( NULL ), m_iAccumDocs ( 0 ) {}
	void			AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphMatch & tDoc, int iRowSize );
	RtSegment_t *	CreateSegment ( int iRowSize );
};

/// TLS indexing accumulator (we disallow two uncommitted adds within one thread; and so need at most one)
__thread RtAccum_t * g_pTlsAccum = NULL;

/// RAM based index
struct RtIndex_t : public ISphRtIndex, public ISphNoncopyable
{
private:
	const int					m_iStride;
	CSphVector<RtSegment_t*>	m_pSegments;

	CSphMutex					m_tWriterMutex;
	mutable CSphRwlock			m_tRwlock;

public:
	explicit					RtIndex_t ( const CSphSchema & tSchema );
	virtual						~RtIndex_t ();

	bool						AddDocument ( const CSphVector<CSphString> & dFields, const CSphMatch & tDoc );
	bool						AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphMatch & tDoc );
	bool						DeleteDocument ( SphDocID_t uDoc );
	void						Commit ();

private:
	/// acquire thread-local indexing accumulator
	/// returns NULL if another index already uses it in an open txn
	RtAccum_t *					AcquireAccum ();

	RtSegment_t *				MergeSegments ( const RtSegment_t * pSeg1, const RtSegment_t * pSeg2 );
	const RtWord_t *			CopyWord ( RtSegment_t * pDst, RtWordWriter_t & tOutWord, const RtSegment_t * pSrc, const RtWord_t * pWord, RtWordReader_t & tInWord );
	void						MergeWord ( RtSegment_t * pDst, const RtSegment_t * pSrc1, const RtWord_t * pWord1, const RtSegment_t * pSrc2, const RtWord_t * pWord2, RtWordWriter_t & tOut );
	void						CopyDoc ( RtSegment_t * pSeg, RtDocWriter_t & tOutDoc, RtWord_t * pWord, const RtSegment_t * pSrc, const RtDoc_t * pDoc );

	void						DumpToDisk ( const char * sFilename );

public:
#if USE_WINDOWS
#pragma warning(push,1)
#pragma warning(disable:4100)
#endif
	virtual SphAttr_t *			GetKillList () const			{ return NULL; }
	virtual int					GetKillListSize () const		{ return 0; }

	virtual int					Build ( const CSphVector<CSphSource*> & dSources, int iMemoryLimit, int iWriteBuffer ) { return 0; }
	virtual bool				Merge ( CSphIndex * pSource, CSphVector<CSphFilterSettings> & dFilters, bool bMergeKillLists ) { return false; }

	virtual const CSphSchema *	Prealloc ( bool bMlock, CSphString & sWarning ) { return &m_tSchema; }
	virtual void				Dealloc () {}
	virtual bool				Preread () { return true; }
	virtual void				SetBase ( const char * sNewBase ) {}
	virtual bool				Rename ( const char * sNewBase ) { return true; }
	virtual bool				Lock () { return true; }
	virtual void				Unlock () {}
	virtual bool				Mlock () { return true; }

	virtual int					UpdateAttributes ( const CSphAttrUpdate & tUpd ) { return -1; }
	virtual bool				SaveAttributes () { return false; }

	virtual void				DebugDumpHeader ( FILE * fp, const char * sHeaderName ) {}
	virtual void				DebugDumpDocids ( FILE * fp ) {}
	virtual void				DebugDumpHitlist ( FILE * fp, const char * sKeyword ) {}
#if USE_WINDOWS
#pragma warning(pop)
#endif

public:
	virtual bool						EarlyReject ( CSphQueryContext * pCtx, CSphMatch & ) const;
	virtual const CSphSourceStats &		GetStats () const { return m_tStats; }

	virtual bool				MultiQuery ( CSphQuery * pQuery, CSphQueryResult * pResult, int iSorters, ISphMatchSorter ** ppSorters ) const;
	virtual bool				MultiQueryEx ( int iQueries, CSphQuery * ppQueries, CSphQueryResult ** ppResults, ISphMatchSorter ** ppSorters ) const;
	virtual bool				GetKeywords ( CSphVector <CSphKeywordInfo> & dKeywords, const char * szQuery, bool bGetStats );

	void						CopyDocinfo ( CSphMatch & tMatch, const DWORD * pFound ) const;
	const CSphRowitem *			FindDocinfo ( const RtSegment_t * pSeg, SphDocID_t uDocID ) const;

protected:
	CSphSourceStats				m_tStats;

	// searching-only, per-query
	int							m_iWeights;						///< search query field weights count
	int							m_dWeights [ SPH_MAX_FIELDS ];	///< search query field weights
};


RtIndex_t::RtIndex_t ( const CSphSchema & tSchema )
	: ISphRtIndex ( "rtindex" )
	, m_iStride ( DOCINFO_IDSIZE + tSchema.GetRowSize() )
{
	m_tSchema = tSchema;

#ifndef NDEBUG
	// check that index cols are static
	for ( int i=0; i<m_tSchema.GetAttrsCount(); i++ )
		assert ( !m_tSchema.GetAttr(i).m_tLocator.m_bDynamic );
#endif

	Verify ( m_tWriterMutex.Init() );
	Verify ( m_tRwlock.Init() );
}


RtIndex_t::~RtIndex_t ()
{
	Verify ( m_tWriterMutex.Done() );
	Verify ( m_tRwlock.Done() );

	ARRAY_FOREACH ( i, m_pSegments )
		SafeDelete ( m_pSegments[i] );
}


class CSphSource_StringVector : public CSphSource_Document
{
public:
	explicit			CSphSource_StringVector ( const CSphVector<CSphString> & dFields, const CSphSchema & tSchema );
	virtual				~CSphSource_StringVector () {}

	virtual bool		Connect ( CSphString & ) { return true; }
	virtual void		Disconnect () {}

	virtual bool		HasAttrsConfigured () { return false; }
	virtual bool		IterateHitsStart ( CSphString & ) { return true; }

	virtual bool		IterateMultivaluedStart ( int, CSphString & ) { return false; }
	virtual bool		IterateMultivaluedNext () { return false; }

	virtual bool		IterateFieldMVAStart ( int, CSphString & ) { return false; }
	virtual bool		IterateFieldMVANext () { return false; }

	virtual bool		IterateKillListStart ( CSphString & ) { return false; }
	virtual bool		IterateKillListNext ( SphDocID_t & ) { return false; }

	virtual BYTE **		NextDocument ( CSphString & ) { return &m_dFields[0]; }

protected:
	CSphVector<BYTE *>	m_dFields;
};


CSphSource_StringVector::CSphSource_StringVector ( const CSphVector<CSphString> & dFields, const CSphSchema & tSchema )
	: CSphSource_Document ( "$stringvector" )
{
	m_tSchema = tSchema;

	m_dFields.Resize ( 1+dFields.GetLength() );
	ARRAY_FOREACH ( i, dFields )
		m_dFields[i] = (BYTE*) dFields[i].cstr();
	m_dFields [ dFields.GetLength() ] = NULL;
}


bool RtIndex_t::AddDocument ( const CSphVector<CSphString> & dFields, const CSphMatch & tDoc )
{
	if ( !tDoc.m_iDocID )
		return true;

	CSphSource_StringVector tSrc ( dFields, m_tSchema );
	tSrc.SetTokenizer ( m_pTokenizer );
	tSrc.SetDict ( m_pDict );

	tSrc.m_tDocInfo.Clone ( tDoc, m_tSchema.GetRowSize() );
	if ( !tSrc.IterateHitsNext ( m_sLastError ) )
		return false;

	return AddDocument ( tSrc.m_dHits, tDoc );
}


void AccumCleanup ( void * pArg )
{
	RtAccum_t * pAcc = (RtAccum_t *) pArg;
	SafeDelete ( pAcc );
}


RtAccum_t * RtIndex_t::AcquireAccum ()
{
	// check that no other index is holding the acc
	if ( g_pTlsAccum && g_pTlsAccum->m_pIndex!=NULL && g_pTlsAccum->m_pIndex!=this )
		return NULL;

	if ( !g_pTlsAccum )
	{
		g_pTlsAccum = new RtAccum_t ();
		sphThreadOnExit ( AccumCleanup, g_pTlsAccum );
	}

	assert ( g_pTlsAccum->m_pIndex==NULL || g_pTlsAccum->m_pIndex==this );
	g_pTlsAccum->m_pIndex = this;
	return g_pTlsAccum;
}


bool RtIndex_t::AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphMatch & tDoc )
{
	RtAccum_t * pAcc = AcquireAccum();
	if ( pAcc )
		pAcc->AddDocument ( dHits, tDoc, m_tSchema.GetRowSize() );
	return ( pAcc!=NULL );
}


void RtAccum_t::AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphMatch & tDoc, int iRowSize )
{
	// schedule existing copies for deletion
	m_dAccumKlist.Add ( tDoc.m_iDocID );

	// no pain, no gain!
	if ( !dHits.GetLength() )
		return;

	// reserve some hit space on first use
	if ( !m_dAccum.GetLength() )
		m_dAccum.Reserve ( 128*1024 );

	// accumulate row data; expect fully dynamic rows
	assert ( !tDoc.m_pStatic );
	assert (!( !tDoc.m_pDynamic && iRowSize!=0 ));
	assert (!( tDoc.m_pDynamic && (int)tDoc.m_pDynamic[-1]!=iRowSize ));

	m_dAccumRows.Resize ( m_dAccumRows.GetLength() + DOCINFO_IDSIZE + iRowSize );
	CSphRowitem * pRow = &m_dAccumRows [ m_dAccumRows.GetLength() - DOCINFO_IDSIZE - iRowSize ];
	DOCINFOSETID ( pRow, tDoc.m_iDocID );

	CSphRowitem * pAttrs = DOCINFO2ATTRS(pRow);
	for ( int i=0; i<iRowSize; i++ )
		pAttrs[i] = tDoc.m_pDynamic[i];

	// accumulate hits
	ARRAY_FOREACH ( i, dHits )
		m_dAccum.Add ( dHits[i] );

	m_iAccumDocs++;
}


RtSegment_t * RtAccum_t::CreateSegment ( int iRowSize )
{
	if ( !m_iAccumDocs )
		return NULL;

	RtSegment_t * pSeg = new RtSegment_t ();

	CSphWordHit tClosingHit;
	tClosingHit.m_iWordID = WORDID_MAX;
	tClosingHit.m_iDocID = DOCID_MAX;
	tClosingHit.m_iWordPos = 1;
	m_dAccum.Add ( tClosingHit );
	m_dAccum.Sort ( CmpHit_fn() );

	RtDoc_t tDoc;
	tDoc.m_uDocID = 0;
	tDoc.m_uFields = 0;
	tDoc.m_uHits = 0;
	tDoc.m_uHit = 0;

	RtWord_t tWord;
	tWord.m_uWordID = 0;
	tWord.m_uDocs = 0;
	tWord.m_uHits = 0;
	tWord.m_uDoc = 0;

	RtDocWriter_t tOutDoc ( pSeg );
	RtWordWriter_t tOutWord ( pSeg );
	RtHitWriter_t tOutHit ( pSeg );

	DWORD uEmbeddedHit = 0;
	ARRAY_FOREACH ( i, m_dAccum )
	{
		const CSphWordHit & tHit = m_dAccum[i];

		// new keyword or doc; flush current doc
		if ( tHit.m_iWordID!=tWord.m_uWordID || tHit.m_iDocID!=tDoc.m_uDocID )
		{
			if ( tDoc.m_uDocID )
			{
				tWord.m_uDocs++;
				tWord.m_uHits += tDoc.m_uHits;

				if ( uEmbeddedHit )
				{
					assert ( tDoc.m_uHits==1 );
					tDoc.m_uHit = uEmbeddedHit;
				}

				tOutDoc.ZipDoc ( tDoc );
				tDoc.m_uFields = 0;
				tDoc.m_uHits = 0;
				tDoc.m_uHit = tOutHit.ZipHitPtr();
			}

			tDoc.m_uDocID = tHit.m_iDocID;
			tOutHit.ZipRestart ();
			uEmbeddedHit = 0;
		}

		// new keyword; flush current keyword
		if ( tHit.m_iWordID!=tWord.m_uWordID )
		{
			tOutDoc.ZipRestart ();
			if ( tWord.m_uWordID )
				tOutWord.ZipWord (  tWord );

			tWord.m_uWordID = tHit.m_iWordID;
			tWord.m_uDocs = 0;
			tWord.m_uHits = 0;
			tWord.m_uDoc = tOutDoc.ZipDocPtr();
		}

		// just a new hit
		if ( !tDoc.m_uHits )
		{
			uEmbeddedHit = tHit.m_iWordPos;
		} else
		{
			if ( uEmbeddedHit )
			{
				tOutHit.ZipHit ( uEmbeddedHit );
				uEmbeddedHit = 0;
			}

			tOutHit.ZipHit ( tHit.m_iWordPos );
		}

		tDoc.m_uFields |= 1UL << HIT2FIELD(tHit.m_iWordPos);
		tDoc.m_uHits++;
	}

	pSeg->m_iRows = m_iAccumDocs;
	pSeg->m_iAliveRows = m_iAccumDocs;

	// copy and sort attributes
	int iStride = DOCINFO_IDSIZE + iRowSize;
	pSeg->m_dRows.SwapData ( m_dAccumRows );
	sphSortDocinfos ( &pSeg->m_dRows[0], pSeg->m_dRows.GetLength()/iStride, iStride );

	// done
	return pSeg;
}


const RtWord_t * RtIndex_t::CopyWord ( RtSegment_t * pDst, RtWordWriter_t & tOutWord, const RtSegment_t * pSrc, const RtWord_t * pWord, RtWordReader_t & tInWord )
{
	RtDocReader_t tInDoc ( pSrc, *pWord );
	RtDocWriter_t tOutDoc ( pDst );

	RtWord_t tNewWord = *pWord;
	tNewWord.m_uDoc = tOutDoc.ZipDocPtr();

	// copy docs
	for ( ;; )
	{
		const RtDoc_t * pDoc = tInDoc.UnzipDoc();
		if ( !pDoc )
			break;

		// apply klist
		assert (!( pSrc->m_bTlsKlist && !g_pTlsAccum )); // if flag is there, acc must be there
		assert ( !g_pTlsAccum || g_pTlsAccum->m_pIndex==this ); // *must* be holding acc during merge
		if ( pSrc->m_dKlist.BinarySearch ( pDoc->m_uDocID )
			|| ( pSrc->m_bTlsKlist && g_pTlsAccum->m_dAccumKlist.BinarySearch ( pDoc->m_uDocID ) ) )
		{
			tNewWord.m_uDocs--;
			tNewWord.m_uHits -= pDoc->m_uHits;
			continue;
		}

		// short route, single embedded hit
		if ( pDoc->m_uHits==1 )
		{
			tOutDoc.ZipDoc ( *pDoc );
			continue;
		}

		// long route, copy hits
		RtHitWriter_t tOutHit ( pDst );
		RtHitReader_t tInHit ( pSrc, pDoc );

		RtDoc_t tDoc = *pDoc;
		tDoc.m_uHit = tOutHit.ZipHitPtr();

		// OPTIMIZE? decode+memcpy?
		for ( DWORD uValue=tInHit.UnzipHit(); uValue; uValue=tInHit.UnzipHit() )
			tOutHit.ZipHit ( uValue );

		// copy doc
		tOutDoc.ZipDoc ( tDoc );
	}

	// append word to the dictionary
	if ( tNewWord.m_uDocs )
		tOutWord.ZipWord ( tNewWord );

	// move forward
	return tInWord.UnzipWord ();
}


void RtIndex_t::CopyDoc ( RtSegment_t * pSeg, RtDocWriter_t & tOutDoc, RtWord_t * pWord, const RtSegment_t * pSrc, const RtDoc_t * pDoc )
{
	pWord->m_uDocs++;
	pWord->m_uHits += pDoc->m_uHits;

	if ( pDoc->m_uHits==1 )
	{
		tOutDoc.ZipDoc ( *pDoc );
		return;
	}

	RtHitWriter_t tOutHit ( pSeg );
	RtHitReader_t tInHit ( pSrc, pDoc );

	RtDoc_t tDoc = *pDoc;
	tDoc.m_uHit = tOutHit.ZipHitPtr();
	tOutDoc.ZipDoc ( tDoc );

	// OPTIMIZE? decode+memcpy?
	for ( DWORD uValue=tInHit.UnzipHit(); uValue; uValue=tInHit.UnzipHit() )
		tOutHit.ZipHit ( uValue );
}


void RtIndex_t::MergeWord ( RtSegment_t * pSeg, const RtSegment_t * pSrc1, const RtWord_t * pWord1, const RtSegment_t * pSrc2, const RtWord_t * pWord2, RtWordWriter_t & tOut )
{
	assert ( pWord1->m_uWordID==pWord2->m_uWordID );

	RtDocWriter_t tOutDoc ( pSeg );

	RtWord_t tWord;
	tWord.m_uWordID = pWord1->m_uWordID;
	tWord.m_uDocs = 0;
	tWord.m_uHits = 0;
	tWord.m_uDoc = tOutDoc.ZipDocPtr();

	RtDocReader_t tIn1 ( pSrc1, *pWord1 );
	RtDocReader_t tIn2 ( pSrc2, *pWord2 );
	const RtDoc_t * pDoc1 = tIn1.UnzipDoc();
	const RtDoc_t * pDoc2 = tIn2.UnzipDoc();

	while ( pDoc1 || pDoc2 )
	{
		if ( pDoc1 && pDoc2 && pDoc1->m_uDocID==pDoc2->m_uDocID )
		{
			// dupe, must (!) be killed in the first segment, might be in both
			assert ( pSrc1->m_dKlist.BinarySearch ( pDoc1->m_uDocID ) );
			if ( !pSrc2->m_dKlist.BinarySearch ( pDoc2->m_uDocID ) )
				CopyDoc ( pSeg, tOutDoc, &tWord, pSrc2, pDoc2 );
			pDoc2 = tIn2.UnzipDoc();

		} else if ( pDoc1 && ( !pDoc2 || pDoc1->m_uDocID < pDoc2->m_uDocID ) )
		{
			// winner from the first segment
			if ( !pSrc1->m_dKlist.BinarySearch ( pDoc1->m_uDocID ) )
				CopyDoc ( pSeg, tOutDoc, &tWord, pSrc1, pDoc1 );
			pDoc1 = tIn1.UnzipDoc();

		} else
		{
			// winner from the second segment
			assert ( pDoc2 && ( !pDoc1 || pDoc2->m_uDocID < pDoc1->m_uDocID ) );
			if ( !pSrc2->m_dKlist.BinarySearch ( pDoc2->m_uDocID ) )
				CopyDoc ( pSeg, tOutDoc, &tWord, pSrc2, pDoc2 );
			pDoc2 = tIn2.UnzipDoc();
		}
	}

	if ( tWord.m_uDocs )
		tOut.ZipWord ( tWord );
}


#if PARANOID
static void CheckSegmentRows ( const RtSegment_t * pSeg, int iStride )
{
	const CSphVector<CSphRowitem> & dRows = pSeg->m_dRows; // shortcut
	for ( int i=iStride; i<dRows.GetLength(); i+=iStride )
		assert ( DOCINFO2ID(&dRows[i]) > DOCINFO2ID(&dRows[i-iStride]) );
}
#endif


struct RtRowIterator_t : public ISphNoncopyable
{
protected:
	const CSphRowitem * m_pRow;
	const CSphRowitem * m_pRowMax;
	const SphDocID_t * m_pKlist;
	const SphDocID_t * m_pKlistMax;
	const int m_iStride;

public:
	explicit RtRowIterator_t ( const RtSegment_t * pSeg, int iStride )
		: m_pRow ( &pSeg->m_dRows[0] )
		, m_pRowMax ( &pSeg->m_dRows[0] + pSeg->m_dRows.GetLength() )
		, m_pKlist ( pSeg->m_dKlist.GetLength()==0 ? NULL : &pSeg->m_dKlist[0] )
		, m_pKlistMax ( pSeg->m_dKlist.GetLength()==0 ? NULL : &pSeg->m_dKlist[0] + pSeg->m_dKlist.GetLength() )
		, m_iStride ( iStride )
	{}

	const CSphRowitem * GetNextAliveRow ()
	{
		if ( m_pRow>=m_pRowMax )
			return NULL;

		while ( m_pKlist<m_pKlistMax )
		{
			SphDocID_t uID = DOCINFO2ID(m_pRow);
			while ( m_pKlist<m_pKlistMax && *m_pKlist<uID )
				m_pKlist++;

			assert ( m_pKlist>=m_pKlistMax || *m_pKlist>=uID );
			if (!( m_pKlist<m_pKlistMax && *m_pKlist==uID ))
				break;

			m_pKlist++;
			m_pRow += m_iStride;
			if ( m_pRow>=m_pRowMax )
				return NULL;
		}

		m_pRow += m_iStride;
		return m_pRow-m_iStride;
	}
};


RtSegment_t * RtIndex_t::MergeSegments ( const RtSegment_t * pSeg1, const RtSegment_t * pSeg2 )
{
	if ( pSeg1->m_iTag > pSeg2->m_iTag )
		Swap ( pSeg1, pSeg2 );

	RtSegment_t * pSeg = new RtSegment_t ();

	////////////////////
	// merge attributes
	////////////////////

	// check that all the IDs are in proper asc order
#if PARANOID
	CheckSegmentRows ( pSeg1, m_iStride );
	CheckSegmentRows ( pSeg2, m_iStride );
#endif

	// just a shortcut
	CSphVector<CSphRowitem> & dRows = pSeg->m_dRows;

	// we might need less because of dupes, but we can not know yet
	dRows.Reserve ( pSeg1->m_dRows.GetLength() + pSeg2->m_dRows.GetLength() );

	RtRowIterator_t tIt1 ( pSeg1, m_iStride );
	RtRowIterator_t tIt2 ( pSeg2, m_iStride );

	const CSphRowitem * pRow1 = tIt1.GetNextAliveRow();
	const CSphRowitem * pRow2 = tIt2.GetNextAliveRow();

	while ( pRow1 || pRow2 )
	{
		if ( !pRow2 || ( pRow1 && pRow2 && DOCINFO2ID(pRow1)<DOCINFO2ID(pRow2) ) )
		{
			assert ( pRow1 );
			for ( int i=0; i<m_iStride; i++ )
				dRows.Add ( *pRow1++ );
			pRow1 = tIt1.GetNextAliveRow();
		} else
		{
			assert ( pRow2 );
			assert ( !pRow1 || ( DOCINFO2ID(pRow1)!=DOCINFO2ID(pRow2) ) ); // all dupes must be killed and skipped by the iterator
			for ( int i=0; i<m_iStride; i++ )
				dRows.Add ( *pRow2++ );
			pRow2 = tIt2.GetNextAliveRow();
		}
		pSeg->m_iRows++;
		pSeg->m_iAliveRows++;
	}

	assert ( pSeg->m_iRows*m_iStride==pSeg->m_dRows.GetLength() );
#if PARANOID
	CheckSegmentRows ( pSeg, m_iStride );
#endif

	//////////////////
	// merge keywords
	//////////////////

	pSeg->m_dWords.Reserve ( pSeg1->m_dWords.GetLength() + pSeg2->m_dWords.GetLength() );
	pSeg->m_dDocs.Reserve ( pSeg1->m_dDocs.GetLength() + pSeg2->m_dDocs.GetLength() );
	pSeg->m_dHits.Reserve ( pSeg1->m_dHits.GetLength() + pSeg2->m_dHits.GetLength() );

	RtWordWriter_t tOut ( pSeg );
	RtWordReader_t tIn1 ( pSeg1 );
	RtWordReader_t tIn2 ( pSeg2 );
	const RtWord_t * pWords1 = tIn1.UnzipWord ();
	const RtWord_t * pWords2 = tIn2.UnzipWord ();

	// merge while there are common words
	for ( ;; )
	{
		while ( pWords1 && pWords2 && pWords1->m_uWordID!=pWords2->m_uWordID )
			if ( pWords1->m_uWordID < pWords2->m_uWordID )
				pWords1 = CopyWord ( pSeg, tOut, pSeg1, pWords1, tIn1 );
			else
				pWords2 = CopyWord ( pSeg, tOut, pSeg2, pWords2, tIn2 );

		if ( !pWords1 || !pWords2 )
			break;

		assert ( pWords1 && pWords2 && pWords1->m_uWordID==pWords2->m_uWordID );
		MergeWord ( pSeg, pSeg1, pWords1, pSeg2, pWords2, tOut );
		pWords1 = tIn1.UnzipWord();
		pWords2 = tIn2.UnzipWord();
	}

	// copy tails
	while ( pWords1 ) pWords1 = CopyWord ( pSeg, tOut, pSeg1, pWords1, tIn1 );
	while ( pWords2 ) pWords2 = CopyWord ( pSeg, tOut, pSeg2, pWords2, tIn2 );

	assert ( pSeg->m_dRows.GetLength() );
	assert ( pSeg->m_iRows );
	assert ( pSeg->m_iAliveRows );
	return pSeg;
}


struct CmpSegments_fn
{
	inline bool IsLess ( const RtSegment_t * a, const RtSegment_t * b )
	{
		return a->GetMergeFactor() > b->GetMergeFactor();
	}
};


void RtIndex_t::Commit ()
{
	RtAccum_t * pAcc = AcquireAccum();
	if ( !pAcc )
		return;

	// phase 0, build a new segment
	// accum and segment are thread local; so no locking needed yet
	// might be NULL if we're only killing rows this txn
	RtSegment_t * pNewSeg = pAcc->CreateSegment ( m_tSchema.GetRowSize() );
	assert ( !pNewSeg || pNewSeg->m_iRows>0 );
	assert ( !pNewSeg || pNewSeg->m_iAliveRows>0 );
	assert ( !pNewSeg || pNewSeg->m_bTlsKlist==false );

	// clean up parts we no longer need
	pAcc->m_dAccum.Resize ( 0 );
	pAcc->m_dAccumRows.Resize ( 0 );

	// sort accum klist, too
	pAcc->m_dAccumKlist.Sort ();

	// phase 1, lock out other writers (but not readers yet)
	// concurrent readers are ok during merges, as existing segments won't be modified yet
	// however, concurrent writers are not
	Verify ( m_tWriterMutex.Lock() );

	// let merger know that existing segments are subject to additional, TLS K-list filter
	// safe despite the readers, flag must only be used by writer
	if ( pAcc->m_dAccumKlist.GetLength() )
		ARRAY_FOREACH ( i, m_pSegments )
	{
		// OPTIMIZE? only need to set the flag if TLS K-list *actually* affects segment
		assert ( m_pSegments[i]->m_bTlsKlist==false );
		m_pSegments[i]->m_bTlsKlist = true;
	}

	// prepare new segments vector
	// create more new segments by merging as needed
	// do not (!) kill processed old segments just yet, as sreaders might still need them
	CSphVector<RtSegment_t*> dSegments;
	CSphVector<RtSegment_t*> dToKill;

	dSegments = m_pSegments;
	if ( pNewSeg )
		dSegments.Add ( pNewSeg );

	// skip merging if no rows were added
	const int MAX_SEGMENTS = 8;
	while ( pNewSeg )
	{
		dSegments.Sort ( CmpSegments_fn() );

		// unconditionally merge if there's too much segments now
		// conditionally merge if smallest segment has grown too large
		// otherwise, we're done
		int iLen = dSegments.GetLength();
		if (!( iLen>MAX_SEGMENTS || ( iLen>=2 && dSegments[iLen-1]->GetMergeFactor()*2 > dSegments[iLen-2]->GetMergeFactor() ) ))
			break;

		RtSegment_t * pA = dSegments.Pop();
		RtSegment_t * pB = dSegments.Pop();
		dSegments.Add ( MergeSegments ( pA, pB ) );
		dToKill.Add ( pA );
		dToKill.Add ( pB );
	}

	// phase 2, obtain exclusive writer lock
	// we now have to update K-lists in (some of) the survived segments
	// and also swap in new segment list
	m_tRwlock.WriteLock ();

	// update K-lists on survivors
	if ( pAcc->m_dAccumKlist.GetLength() )
		ARRAY_FOREACH ( iSeg, dSegments )
	{
		RtSegment_t * pSeg = dSegments[iSeg];
		if ( !pSeg->m_bTlsKlist )
			continue; // should be fresh enough

		// this segment was not created by this txn
		// so we need to merge additional K-list from current txn into it
		ARRAY_FOREACH ( j, pAcc->m_dAccumKlist )
		{
			SphDocID_t uDocid = pAcc->m_dAccumKlist[j];
			if ( pSeg->HasDocid ( uDocid ) )
			{
				pSeg->m_dKlist.Add ( uDocid );
				pSeg->m_iAliveRows--;
			}
		}

		// we did not check for existance in K-list, only in segment
		// so need to use Uniq(), not just Sort()
		pSeg->m_dKlist.Uniq ();

		// mark as good
		pSeg->m_bTlsKlist = false;
	}

	// go live!
	Swap ( m_pSegments, dSegments );

	// we can kill retired segments now
	ARRAY_FOREACH ( i, dToKill )
		SafeDelete ( dToKill[i] );

	// update stats
	m_tStats.m_iTotalDocuments += pAcc->m_iAccumDocs;

	// finish cleaning up and release accumulator
	pAcc->m_pIndex = NULL;
	pAcc->m_iAccumDocs = 0;
	pAcc->m_dAccumKlist.Reset();

	// all done, unlock
	Verify ( m_tRwlock.Unlock() );
	Verify ( m_tWriterMutex.Unlock() );
}


bool RtIndex_t::DeleteDocument ( SphDocID_t uDoc )
{
	RtAccum_t * pAcc = AcquireAccum();
	if ( pAcc )
		pAcc->m_dAccumKlist.Add ( uDoc );
	return ( pAcc!=NULL );
}


/// WARNING! static buffer, non-reenterable
static const char * FormatMicrotime ( int64_t uTime )
{
	static char sBuf[32];
	snprintf ( sBuf, sizeof(sBuf), "%d.%03d", int(uTime/1000000), int((uTime%1000000)/1000) );
	return sBuf;
}


struct Checkpoint_t
{
	uint64_t m_uWord;
	uint64_t m_uOffset;
};


void RtIndex_t::DumpToDisk ( const char * sFilename )
{
	Verify ( m_tWriterMutex.Lock() );
	Verify ( m_tRwlock.WriteLock() );

	CSphString sName, sError;

	CSphWriter wrHits, wrDocs, wrDict, wrRows;
	sName.SetSprintf ( "%s.spp", sFilename ); wrHits.OpenFile ( sName.cstr(), sError );
	sName.SetSprintf ( "%s.spd", sFilename ); wrDocs.OpenFile ( sName.cstr(), sError );
	sName.SetSprintf ( "%s.spi", sFilename ); wrDict.OpenFile ( sName.cstr(), sError );
	sName.SetSprintf ( "%s.spa", sFilename ); wrRows.OpenFile ( sName.cstr(), sError );

	BYTE bDummy = 1;
	wrDict.PutBytes ( &bDummy, 1 );
	wrDocs.PutBytes ( &bDummy, 1 );
	wrHits.PutBytes ( &bDummy, 1 );

	int64_t tmStart = sphMicroTimer();

	while ( m_pSegments.GetLength()>1 )
	{
		m_pSegments.Sort ( CmpSegments_fn() );
		RtSegment_t * pSeg1 = m_pSegments.Pop();
		RtSegment_t * pSeg2 = m_pSegments.Pop();
		m_pSegments.Add ( MergeSegments ( pSeg1, pSeg2 ) );
		SafeDelete ( pSeg1 );
		SafeDelete ( pSeg2 );
	}
	RtSegment_t * pSeg = m_pSegments[0];

	int64_t tmMerged = sphMicroTimer() ;
	printf ( "final merge done in %s sec\n", FormatMicrotime ( tmMerged-tmStart ) );

	SphWordID_t uLastWord = 0;
	SphOffset_t uLastDocpos = 0;

	static const int WORDLIST_CHECKPOINT = 1024;
	int iWords = 0;

	CSphVector<Checkpoint_t> dCheckpoints;
	RtWordReader_t tInWord ( pSeg );
	for ( ;; )
	{
		const RtWord_t * pWord = tInWord.UnzipWord();
		if ( !pWord )
			break;

		SphDocID_t uLastDoc = 0;
		SphOffset_t uLastHitpos = 0;

		if ( !iWords )
		{
			Checkpoint_t & tChk = dCheckpoints.Add ();
			tChk.m_uWord = pWord->m_uWordID;
			tChk.m_uOffset = wrDict.GetPos();
		}

		wrDict.ZipOffset ( pWord->m_uWordID - uLastWord );
		wrDict.ZipOffset ( wrDocs.GetPos() - uLastDocpos );
		wrDict.ZipInt ( pWord->m_uDocs );
		wrDict.ZipInt ( pWord->m_uHits );

		uLastDocpos = wrDocs.GetPos();
		uLastWord = pWord->m_uWordID;

		RtDocReader_t tInDoc ( pSeg, *pWord );
		for ( ;; )
		{
			const RtDoc_t * pDoc = tInDoc.UnzipDoc();
			if ( !pDoc )
				break;

			wrDocs.ZipOffset ( pDoc->m_uDocID-uLastDoc );
			wrDocs.ZipOffset ( wrHits.GetPos() - uLastHitpos );
			wrDocs.ZipInt ( pDoc->m_uFields );
			wrDocs.ZipInt ( pDoc->m_uHits );
			uLastDoc = pDoc->m_uDocID;
			uLastHitpos = wrHits.GetPos();

			if ( pDoc->m_uHits>1 )
			{
				DWORD uLastHit = 0;

				RtHitReader_t tInHit ( pSeg, pDoc );
				for ( DWORD uValue=tInHit.UnzipHit(); uValue; uValue=tInHit.UnzipHit() )
				{
					wrHits.ZipInt ( uValue - uLastHit );
					uLastHit = uValue;
				}
			} else
			{
				wrHits.ZipInt ( pDoc->m_uHit );
			}
			wrHits.ZipInt ( 0 );
		}
		wrDocs.ZipInt ( 0 );

		if ( ++iWords==WORDLIST_CHECKPOINT )
		{
			wrDict.ZipInt ( 0 );
			wrDict.ZipOffset ( wrDocs.GetPos() - uLastDocpos ); // store last hitlist length

			uLastDocpos = 0;
			uLastWord = 0;

			iWords = 0;
		}
	}
	wrDict.ZipInt ( 0 ); // indicate checkpoint
	wrDict.ZipOffset ( wrDocs.GetPos() - uLastDocpos ); // store last doclist length

	if ( dCheckpoints.GetLength() )
		wrDict.PutBytes ( &dCheckpoints[0], dCheckpoints.GetLength()*sizeof(Checkpoint_t) );

	wrRows.PutBytes ( &pSeg->m_dRows[0], pSeg->m_dRows.GetLength()*sizeof(CSphRowitem) );

	wrHits.CloseFile ();
	wrDocs.CloseFile ();
	wrDict.CloseFile ();
	wrRows.CloseFile ();

	int64_t tmDump = sphMicroTimer ();
	printf ( "dump done in %s sec\n", FormatMicrotime ( tmDump-tmMerged ) );

	Verify ( m_tRwlock.Unlock() );
	Verify ( m_tWriterMutex.Lock() );
}

//////////////////////////////////////////////////////////////////////////
// SEARCHING
//////////////////////////////////////////////////////////////////////////

struct RtQword_t : public ISphQword
{
	friend struct RtIndex_t;
	friend struct RtQwordSetup_t;

protected:
	RtDocReader_t *		m_pDocReader;
	CSphMatch			m_tMatch;

	DWORD				m_uNextHit;
	RtHitReader2_t		m_tHitReader;

	RtSegment_t *		m_pSeg;

public:
	RtQword_t ()
		: m_pDocReader ( NULL )
		, m_uNextHit ( 0 )
		, m_pSeg ( NULL )
	{
		m_tMatch.Reset ( 0 );
	}

	virtual const CSphMatch & GetNextDoc ( DWORD * )
	{
		for ( ;; )
		{
			const RtDoc_t * pDoc = m_pDocReader->UnzipDoc();
			if ( !pDoc )
			{
				m_tMatch.m_iDocID = 0;
				return m_tMatch;
			}

			if ( m_pSeg->m_dKlist.BinarySearch ( pDoc->m_uDocID ) )
				continue;

			m_tMatch.m_iDocID = pDoc->m_uDocID;
			m_uFields = pDoc->m_uFields;
			m_uMatchHits = pDoc->m_uHits;
			m_iHitlistPos = (uint64_t(pDoc->m_uHits)<<32) + pDoc->m_uHit;
			return m_tMatch;
		}
	}

	virtual void SeekHitlist ( SphOffset_t uOff )
	{
		int iHits = int(uOff>>32);
		if ( iHits==1 )
		{
			m_uNextHit = DWORD(uOff);
		} else
		{
			m_uNextHit = 0;
			m_tHitReader.Seek ( DWORD(uOff), iHits );
		}
	}

	virtual DWORD GetNextHit ()
	{
		if ( m_uNextHit==0 )
		{
			return m_tHitReader.UnzipHit();

		} else if ( m_uNextHit==0xffffffffUL )
		{
			return 0;

		} else
		{
			DWORD uRes = m_uNextHit;
			m_uNextHit = 0xffffffffUL;
			return uRes;
		}
	}
};


struct RtQwordSetup_t : ISphQwordSetup
{
	RtSegment_t *		m_pSeg;

	virtual ISphQword *	QwordSpawn ( const XQKeyword_t & ) const;
	virtual bool		QwordSetup ( ISphQword * pQword ) const;
};


ISphQword * RtQwordSetup_t::QwordSpawn ( const XQKeyword_t & ) const
{
	return new RtQword_t ();
}


bool RtQwordSetup_t::QwordSetup ( ISphQword * pQword ) const
{
	RtQword_t * pMyWord = dynamic_cast<RtQword_t*> ( pQword );
	if ( !pMyWord )
		return false;

	RtWordReader_t tReader ( m_pSeg );
	for ( ;; )
	{
		const RtWord_t * pWord = tReader.UnzipWord();
		if ( !pWord )
			break;

		if ( pWord->m_uWordID==pMyWord->m_iWordID )
		{
			pMyWord->m_iDocs = pWord->m_uDocs;
			pMyWord->m_iHits = pWord->m_uHits;

			SafeDelete ( pMyWord->m_pDocReader );
			pMyWord->m_pDocReader = new RtDocReader_t ( m_pSeg, *pWord );

			pMyWord->m_tHitReader.m_pBase = NULL;
			if ( m_pSeg->m_dHits.GetLength() )
				pMyWord->m_tHitReader.m_pBase = &m_pSeg->m_dHits[0];

			pMyWord->m_pSeg = m_pSeg;
			return true;
		}
	}

	return false;
}


bool RtIndex_t::EarlyReject ( CSphQueryContext * pCtx, CSphMatch & tMatch ) const
{
	// early calc might be needed even when we do not have a filter
	if ( pCtx->m_bEarlyLookup )
		CopyDocinfo ( tMatch, FindDocinfo ( (RtSegment_t*)pCtx->m_pIndexData, tMatch.m_iDocID ) );

	pCtx->EarlyCalc ( tMatch );
	return pCtx->m_pFilter ? !pCtx->m_pFilter->Eval ( tMatch ) : false;
}


void RtIndex_t::CopyDocinfo ( CSphMatch & tMatch, const DWORD * pFound ) const
{
	if ( !pFound )
		return;

	// setup static pointer
	assert ( DOCINFO2ID(pFound)==tMatch.m_iDocID );
	tMatch.m_pStatic = DOCINFO2ATTRS(pFound);

	// FIXME? implement overrides
}


const CSphRowitem * RtIndex_t::FindDocinfo ( const RtSegment_t * pSeg, SphDocID_t uDocID ) const
{
	// FIXME! move to CSphIndex, and implement hashing
	if ( pSeg->m_dRows.GetLength()==0 )
		return NULL;

	int iStride = m_iStride;
	int iStart = 0;
	int iEnd = pSeg->m_iRows-1;
	assert ( iStride==( DOCINFO_IDSIZE + m_tSchema.GetRowSize() ) );

	const CSphRowitem * pStorage = &pSeg->m_dRows[0];
	const CSphRowitem * pFound = NULL;

	if ( uDocID==DOCINFO2ID ( &pStorage [ iStart*iStride ] ) )
	{
		pFound = &pStorage [ iStart*iStride ];

	} else if ( uDocID==DOCINFO2ID ( &pStorage [ iEnd*iStride ] ) )
	{
		pFound = &pStorage [ iEnd*iStride ];

	} else
	{
		while ( iEnd-iStart>1 )
		{
			// check if nothing found
			if (
				uDocID < DOCINFO2ID ( &pStorage [ iStart*iStride ] ) ||
				uDocID > DOCINFO2ID ( &pStorage [ iEnd*iStride ] ) )
				break;
			assert ( uDocID > DOCINFO2ID ( &pStorage [ iStart*iStride ] ) );
			assert ( uDocID < DOCINFO2ID ( &pStorage [ iEnd*iStride ] ) );

			int iMid = iStart + (iEnd-iStart)/2;
			if ( uDocID==DOCINFO2ID ( &pStorage [ iMid*iStride ] ) )
			{
				pFound = &pStorage [ iMid*iStride ];
				break;
			}
			if ( uDocID<DOCINFO2ID ( &pStorage [ iMid*iStride ] ) )
				iEnd = iMid;
			else
				iStart = iMid;
		}
	}

	return pFound;
}


// FIXME! missing MVA, index_exact_words support
// FIXME? missing enable_star, legacy match modes support
// FIXME? any chance to factor out common backend agnostic code?
bool RtIndex_t::MultiQuery ( CSphQuery * pQuery, CSphQueryResult * pResult, int iSorters, ISphMatchSorter ** ppSorters ) const
{
	// FIXME! too early (how low can you go?)
	m_tRwlock.ReadLock ();

	assert ( pQuery );
	assert ( pResult );
	assert ( ppSorters );

	// empty index, empty result
	if ( !m_pSegments.GetLength() )
	{
		pResult->m_iQueryTime = 0;
		m_tRwlock.Unlock ();
		return true;
	}

	// start counting
	pResult->m_iQueryTime = 0;
	int64_t tmQueryStart = sphMicroTimer();

	// setup calculations and result schema
	CSphQueryContext tCtx;
	if ( !tCtx.SetupCalc ( pResult, ppSorters[0]->GetSchema(), m_tSchema, NULL ) )
		return false;

	// setup search terms
	RtQwordSetup_t tTermSetup;
	tTermSetup.m_pDict = m_pDict;
	tTermSetup.m_pIndex = this;
	tTermSetup.m_eDocinfo = m_tSettings.m_eDocinfo;
	tTermSetup.m_iDynamicRowitems = pResult->m_tSchema.GetDynamicSize();
	if ( pQuery->m_uMaxQueryMsec>0 )
		tTermSetup.m_iMaxTimer = sphMicroTimer() + pQuery->m_uMaxQueryMsec*1000; // max_query_time
	tTermSetup.m_pWarning = &pResult->m_sWarning;
	tTermSetup.m_pSeg = m_pSegments[0];
	tTermSetup.m_pCtx = &tCtx;

	// bind weights
	tCtx.BindWeights ( pQuery, m_tSchema );

	// parse query
	XQQuery_t tParsed;
	if ( !sphParseExtendedQuery ( tParsed, pQuery->m_sQuery.cstr(), GetTokenizer(), GetSchema(), m_pDict ) )
	{
		pResult->m_sError = tParsed.m_sParseError;
		m_tRwlock.Unlock ();
		return false;
	}

	// setup query
	// must happen before index-level reject, in order to build proper keyword stats
	CSphScopedPtr<ISphRanker> pRanker ( sphCreateRanker ( tParsed.m_pRoot, pQuery->m_eRanker, pResult, tTermSetup ) );
	if ( !pRanker.Ptr() )
	{
		m_tRwlock.Unlock ();
		return false;
	}

	// setup filters
	if ( !tCtx.CreateFilters ( pQuery, pResult->m_tSchema, NULL, pResult->m_sError ) )
		return false;

	// FIXME! OPTIMIZE! check if we can early reject the whole index

	// setup lookup
	// do pre-filter lookup as needed
	// do pre-sort lookup in all cases
	// post-sort lookup is complicated (because of many segments)
	// pre-sort lookup is cheap now anyway, and almost always anyway
	// (except maybe by stupid relevance-sorting-only benchmarks!!)
	tCtx.m_bEarlyLookup = ( pQuery->m_dFilters.GetLength() || tCtx.m_dEarlyCalc.GetLength() );
	tCtx.m_bLateLookup = true;

	// FIXME! setup sorters vs. MVA
	for ( int i=0; i<iSorters; i++ )
		(ppSorters[i])->SetMVAPool ( NULL );

	// FIXME! setup overrides

	// do searching
	ARRAY_FOREACH ( iSeg, m_pSegments )
	{
		if ( iSeg!=0 )
		{
			tTermSetup.m_pSeg = m_pSegments[iSeg];
			pRanker->Reset ( tTermSetup );
		}

		// for lookups to work
		tCtx.m_pIndexData = m_pSegments[iSeg];

		CSphMatch * pMatch = pRanker->GetMatchesBuffer();
		for ( ;; )
		{
			int iMatches = pRanker->GetMatches ( m_iWeights, m_dWeights );
			if ( iMatches<=0 )
				break;
			for ( int i=0; i<iMatches; i++ )
			{
				CopyDocinfo ( pMatch[i], FindDocinfo ( m_pSegments[iSeg], pMatch[i].m_iDocID ) );
				for ( int iSorter=0; iSorter<iSorters; iSorter++ )
					ppSorters[iSorter]->Push ( pMatch[i] );
			}
		}
	}

	// FIXME! mva and string pools ptrs
	pResult->m_pMva = NULL;
	pResult->m_pStrings = NULL;

	// query timer
	pResult->m_iQueryTime = int( ( sphMicroTimer()-tmQueryStart )/1000 );
	m_tRwlock.Unlock ();
	return true;
}

bool RtIndex_t::MultiQueryEx ( int iQueries, CSphQuery * ppQueries, CSphQueryResult ** ppResults, ISphMatchSorter ** ppSorters ) const
{
	// FIXME! OPTIMIZE! implement common subtree cache here
	bool bResult = true;
	for ( int i=0; i<iQueries; i++ )
		bResult &= MultiQuery ( &ppQueries[i], ppResults[i], 1, &ppSorters[i] );
	return bResult;
}

bool RtIndex_t::GetKeywords ( CSphVector <CSphKeywordInfo> &, const char *, bool )
{
	/*!COMMIT*/
	return false;
}

//////////////////////////////////////////////////////////////////////////

ISphRtIndex * sphCreateIndexRT ( const CSphSchema & tSchema )
{
	return new RtIndex_t ( tSchema );
}

void sphRTInit ()
{
	Verify ( RtSegment_t::m_tSegmentSeq.Init() );
}

void sphRTDone ()
{
	Verify ( RtSegment_t::m_tSegmentSeq.Done() );
}

//
// $Id$
//

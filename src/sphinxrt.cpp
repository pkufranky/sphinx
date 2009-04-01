//
// $Id$
//

#include "sphinx.h"
#include "sphinxint.h"
#include "sphinxrt.h"

//////////////////////////////////////////////////////////////////////////

#define	COMPRESSED_WORDLIST		0
#define	COMPRESSED_DOCLIST		1
#define COMPRESSED_HITLIST		1

#if USE_64BIT
#define WORDID_MAX				U64C(0xffffffffffffffff)
#else
#define	WORDID_MAX				0xffffffffUL
#endif

/////////////////////////////////////////////////////////////////////////////

// !COMMIT replace with actual scoped locks impl later
#define RLOCK(_arg) ;
#define WLOCK(_arg) ;

// !COMMIT cleanup extern ref to sphinx.cpp
extern void sphSortDocinfos ( DWORD * pBuf, int iCount, int iStride );

// !COMMIT yes i am when debugging
#ifndef NDEBUG
#define PARANOID 1
#endif

//////////////////////////////////////////////////////////////////////////

static inline void ZipDword ( CSphVector<BYTE> & dOut, DWORD uValue )
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


static inline const BYTE * UnzipDword ( DWORD * pValue, const BYTE * pIn )
{
	DWORD uValue = 0;
	BYTE bIn;
	int iOff = 0;

	do 
	{
		bIn = *pIn++;
		uValue += ( bIn & 0x7f )<<iOff;
		iOff += 7;
	} while ( bIn & 0x80 );

	*pValue = uValue;
	return pIn;
}

//////////////////////////////////////////////////////////////////////////

struct CmpHit_fn
{
	inline int operator () ( const CSphWordHit & a, const CSphWordHit & b )
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
	CSphVector<SphDocID_t>		m_dKlistAccum;	///< unsorted K-list accumulator

	RtSegment_t ()
	{
		WLOCK ( m_iSegments );
		m_iTag = m_iSegments++;
		m_iRows = 0;
		m_iAliveRows = 0;
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
	void DeleteDocument ( SphDocID_t uDocid );
	void OptimizeKlist ();
};

int RtSegment_t::m_iSegments = 0;


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


void RtSegment_t::DeleteDocument ( SphDocID_t uDocid )
{
	// do we have it at all?
	if ( !HasDocid ( uDocid ) )
		return;

	// do we kill it already?
	if ( m_dKlist.BinarySearch ( uDocid ) )
		return;

	ARRAY_FOREACH ( i, m_dKlistAccum )
		if ( m_dKlistAccum[i]==uDocid )
			return;

	// update the segment
	WLOCK ( this );
	m_dKlistAccum.Add ( uDocid );

	// merge and sort
	// OPTIMIZE? would sorting accum and merging be faster?
	if ( m_dKlistAccum.GetLength()==KLIST_ACCUM_THRESH )
		OptimizeKlist(); // FIXME? nested wlock

	m_iAliveRows--;
}


void RtSegment_t::OptimizeKlist ()
{
	WLOCK ( this );
	ARRAY_FOREACH ( i, m_dKlistAccum )
		m_dKlist.Add ( m_dKlistAccum[i] );
	m_dKlist.Sort ();
	m_dKlistAccum.Resize ( 0 );
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
		ZipDword ( dDocs, tDoc.m_uDocID - m_uLastDocID ); // !COMMIT might be qword
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
		pIn = UnzipDword ( &uDeltaID, pIn ); // !COMMIT might be qword
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
	DWORD				m_uLastDoc;

	explicit RtWordWriter_t ( RtSegment_t * pSeg )
		: m_pWords ( &pSeg->m_dWords )
		, m_uLastWordID ( 0 )
		, m_uLastDoc ( 0 )
	{}

	void ZipWord ( const RtWord_t & tWord )
	{
		CSphVector<BYTE> & tWords = *m_pWords;
		ZipDword ( tWords, tWord.m_uWordID - m_uLastWordID ); // !COMMIT might be qword
		ZipDword ( tWords, tWord.m_uDocs );
		ZipDword ( tWords, tWord.m_uHits );
		ZipDword ( tWords, tWord.m_uDoc - m_uLastDoc );
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
		DWORD uDeltaID, uDeltaDoc;
		pIn = UnzipDword ( &uDeltaID, pIn ); // !COMMIT might be qword
		pIn = UnzipDword ( &m_tWord.m_uDocs, pIn );
		pIn = UnzipDword ( &m_tWord.m_uHits, pIn );
		pIn = UnzipDword ( &uDeltaDoc, pIn );
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

struct RtIndex_t : public ISphRtIndex, public ISphNoncopyable
{
private:
	CSphVector<CSphWordHit>		m_dAccum;
	CSphVector<CSphRowitem>		m_dAccumRows;
	int							m_iAccumDocs;

	CSphVector<RtSegment_t*>	m_pSegments;

	const CSphSchema			m_tSchema;
	const int					m_iStride;

public:
	explicit					RtIndex_t ( const CSphSchema & tSchema );
	virtual						~RtIndex_t ();

	void						AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphDocInfo & tDoc );
	void						DeleteDocument ( SphDocID_t uDoc );
	void						Commit ();

private:
	RtSegment_t *				CreateSegment ();
	RtSegment_t *				MergeSegments ( const RtSegment_t * pSeg1, const RtSegment_t * pSeg2 );
	const RtWord_t *			CopyWord ( RtSegment_t * pDst, RtWordWriter_t & tOutWord, const RtSegment_t * pSrc, const RtWord_t * pWord, RtWordReader_t & tInWord );
	void						MergeWord ( RtSegment_t * pDst, const RtSegment_t * pSrc1, const RtWord_t * pWord1, const RtSegment_t * pSrc2, const RtWord_t * pWord2, RtWordWriter_t & tOut );
	void						CopyDoc ( RtSegment_t * pSeg, RtDocWriter_t & tOutDoc, RtWord_t * pWord, const RtSegment_t * pSrc, const RtDoc_t * pDoc );

	void						DumpToDisk ( const char * sFilename );
};


RtIndex_t::RtIndex_t ( const CSphSchema & tSchema )
	: m_iAccumDocs ( 0 )
	, m_tSchema ( tSchema )
	, m_iStride ( DOCINFO_IDSIZE + tSchema.GetRowSize() )
{
	m_dAccum.Reserve ( 2*1024*1024 );
}


RtIndex_t::~RtIndex_t ()
{
	ARRAY_FOREACH ( i, m_pSegments )
		SafeDelete ( m_pSegments[i] );
}


void RtIndex_t::AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphDocInfo & tDoc )
{
	// kill existing copies
	DeleteDocument ( tDoc.m_iDocID );

	// !COMMIT make Add/Commit local to given thread
	if ( !dHits.GetLength() )
		return;

	// accumulate row data
	assert ( DOCINFO_IDSIZE + tDoc.m_iRowitems == m_iStride );
	m_dAccumRows.Resize ( m_dAccumRows.GetLength() + m_iStride );

	CSphRowitem * pRow = &m_dAccumRows [ m_dAccumRows.GetLength() - m_iStride ];
	DOCINFOSETID ( pRow, tDoc.m_iDocID );
	for ( int i=0; i<m_iStride-DOCINFO_IDSIZE; i++ )
		DOCINFO2ATTRS(pRow)[i] = tDoc.m_pRowitems[i];

	// accumulate hits
	ARRAY_FOREACH ( i, dHits )
		m_dAccum.Add ( dHits[i] );

	m_iAccumDocs++;
}


RtSegment_t * RtIndex_t::CreateSegment ()
{
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

		tDoc.m_uFields |= 1UL << ( tHit.m_iWordPos>>24 ); // !COMMIT HIT2LCS()
		tDoc.m_uHits++;
	}

	pSeg->m_iRows = m_iAccumDocs;
	pSeg->m_iAliveRows = m_iAccumDocs;

	// copy and sort attributes
	pSeg->m_dRows.SwapData ( m_dAccumRows );
	sphSortDocinfos ( &pSeg->m_dRows[0], pSeg->m_dRows.GetLength()/m_iStride, m_iStride );

	// clean up accumulators
	m_dAccum.Resize ( 0 );
	m_dAccumRows.Resize ( 0 );
	m_iAccumDocs = 0;
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
		assert ( pSrc->m_dKlistAccum.GetLength()==0 );
		if ( pSrc->m_dKlist.BinarySearch ( pDoc->m_uDocID ) )
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
	RLOCK ( pSeg1 );
	RLOCK ( pSeg2 );

	// check that k-lists are optimized
	assert ( pSeg1->m_dKlistAccum.GetLength()==0 );
	assert ( pSeg2->m_dKlistAccum.GetLength()==0 );

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
	inline int operator () ( const RtSegment_t * a, const RtSegment_t * b )
	{
		return a->GetMergeFactor() > b->GetMergeFactor();
	}
};


void RtIndex_t::Commit ()
{
	// !COMMIT make Add/Commit local to given thread
	WLOCK ( this );

	if ( !m_dAccum.GetLength() )
		return;

	RtSegment_t * pNewSeg = CreateSegment();
	assert ( pNewSeg->m_iRows );
	assert ( pNewSeg->m_iAliveRows );

	CSphVector<RtSegment_t*> dSegments;
	dSegments = m_pSegments;
	dSegments.Add ( pNewSeg );

	CSphVector<RtSegment_t*> dToKill;

	const int MAX_SEGMENTS = 8;
	for ( ;; )
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
		pA->OptimizeKlist();
		pB->OptimizeKlist();
		dSegments.Add ( MergeSegments ( pA, pB ) );
		dToKill.Add ( pA );
		dToKill.Add ( pB );
	}

	Swap ( m_pSegments, dSegments ); // !COMMIT atomic
	ARRAY_FOREACH ( i, dToKill )
		SafeDelete ( dToKill[i] ); // unused now
}


void RtIndex_t::DeleteDocument ( SphDocID_t uDoc )
{
	RLOCK ( this );
	ARRAY_FOREACH ( i, m_pSegments )
		m_pSegments[i]->DeleteDocument ( uDoc );
}


/// WARNING! static buffer, non-reenterable
static const char * FormatMicrotime ( int64_t uTime )
{
	static char sBuf[32];
	snprintf ( sBuf, sizeof(sBuf), "%d.%03d", int(uTime/1000000), int((uTime%1000000)/1000) );
	return sBuf;
}


void RtIndex_t::DumpToDisk ( const char * sFilename )
{
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
		pSeg1->OptimizeKlist ();
		pSeg2->OptimizeKlist ();
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

	struct Checkpoint_t
	{
		uint64_t m_uWord;
		uint64_t m_uOffset;
	};
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

		wrDict.ZipInt ( pWord->m_uWordID - uLastWord );
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
}

//////////////////////////////////////////////////////////////////////////

ISphRtIndex * sphCreateIndexRT ( const CSphSchema & tSchema )
{
	return new RtIndex_t ( tSchema );
}

//
// $Id$
//

//
// $Id$
//

#ifndef _sphinxint_
#define _sphinxint_

#include "sphinx.h"
#include "sphinxfilter.h"

/// file writer with write buffering and int encoder
class CSphWriter : ISphNoncopyable
{
public:
					CSphWriter ();
					~CSphWriter ();

	void			SetBufferSize ( int iBufferSize );	///< tune write cache size; must be called before OpenFile() or SetFile()

	bool			OpenFile ( const char * sName, CSphString & sErrorBuffer );
	void			SetFile ( int iFD, SphOffset_t * pSharedOffset );
	void			CloseFile ( bool bTruncate = false );	///< note: calls Flush(), ie. IsError() might get true after this call

	void			PutByte ( int uValue );
	void			PutBytes ( const void * pData, int iSize );
	void			PutDword ( DWORD uValue ) { PutBytes ( &uValue, sizeof(DWORD) ); }
	void			PutOffset ( SphOffset_t uValue ) { PutBytes ( &uValue, sizeof(SphOffset_t) ); }
	void			PutString ( const char * szString );

	void			SeekTo ( SphOffset_t pos ); ///< seeking inside the buffer will truncate it

#if USE_64BIT
	void			PutDocid ( SphDocID_t uValue ) { PutOffset ( uValue ); }
#else
	void			PutDocid ( SphDocID_t uValue ) { PutDword ( uValue ); }
#endif

	void			ZipInt ( DWORD uValue );
	void			ZipOffset ( SphOffset_t uValue );
	void			ZipOffsets ( CSphVector<SphOffset_t> * pData );

	bool			IsError () const	{ return m_bError; }
	SphOffset_t		GetPos () const		{ return m_iPos; }

private:
	CSphString		m_sName;
	SphOffset_t		m_iPos;
	SphOffset_t		m_iWritten;

	int				m_iFD;
	int				m_iPoolUsed;
	BYTE *			m_pBuffer;
	BYTE *			m_pPool;
	bool			m_bOwnFile;
	SphOffset_t	*	m_pSharedOffset;
	int				m_iBufferSize;

	bool			m_bError;
	CSphString *	m_pError;

	void			Flush ();
};

/// per-query search context
/// everything that index needs to compute/create to process the query
class CSphQueryContext
{
public:
	// searching-only, per-query
	int							m_iWeights;						///< search query field weights count
	int							m_dWeights [ SPH_MAX_FIELDS ];	///< search query field weights

	bool						m_bEarlyLookup;			///< whether early attr value lookup is needed
	bool						m_bLateLookup;			///< whether late attr value lookup is needed

	ISphFilter *				m_pFilter;
	ISphFilter *				m_pWeightFilter;

	struct CalcItem_t
	{
		CSphAttrLocator			m_tLoc;					///< result locator
		DWORD					m_uType;				///< result type
		ISphExpr *				m_pExpr;				///< evaluator (non-owned)
	};
	CSphVector<CalcItem_t>		m_dEarlyCalc;			///< early-calc evaluators
	CSphVector<CalcItem_t>		m_dLateCalc;			///< late-calc evaluators

	CSphVector<CSphAttrOverride> *	m_pOverrides;		///< overridden attribute values

public:
	CSphQueryContext ();
	~CSphQueryContext ();

	void						BindWeights ( const CSphQuery * pQuery, const CSphSchema & tSchema );
	bool						SetupCalc ( CSphQueryResult * pResult, const CSphSchema & tInSchema, const CSphSchema & tSchema, const DWORD * pMvaPool );
	bool						CreateFilters ( CSphQuery * pQuery, const CSphSchema & tSchema, const DWORD * pMvaPool, CSphString & sError );

	void						EarlyCalc ( CSphMatch & tMatch ) const;
	void						LateCalc ( CSphMatch & tMatch ) const;
};

#endif // _sphinxint_

//
// $Id$
//

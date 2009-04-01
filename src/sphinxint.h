//
// $Id$
//

#ifndef _sphinxint_
#define _sphinxint_

#include "sphinx.h"

/// file writer with write buffering and int encoder
class CSphWriter : ISphNoncopyable
{
public:
					CSphWriter ();
					~CSphWriter ();

	void			SetBufferSize ( int iBufferSize );	///< tune write cache size; must be called before OpenFile() or SetFile()

	bool			OpenFile ( const char * sName, CSphString & sErrorBuffer );
	void			SetFile ( int iFD, SphOffset_t * pSharedOffset );
	void			CloseFile (); ///< note: calls Flush(), ie. IsError() might get true after this call

	void			PutByte ( int uValue );
	void			PutBytes ( const void * pData, int iSize );
	void			PutDword ( DWORD uValue ) { PutBytes ( &uValue, sizeof(DWORD) ); }
	void			PutOffset ( SphOffset_t uValue ) { PutBytes ( &uValue, sizeof(SphOffset_t) ); }
	void			PutString ( const char * szString );

	void			SeekTo ( SphOffset_t pos );

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

#endif // _sphinxint_

//
// $Id$
//

//
// $Id$
//

#ifndef _sphinxrt_
#define _sphinxrt_

#include "sphinx.h"

/// RAM based updateable backend interface
class ISphRtIndex : public CSphIndex
{
public:
	explicit ISphRtIndex ( const char * sName ) : CSphIndex ( sName) {}

	/// insert/update document in current txn
	/// fails in case of two open txns to different indexes
	virtual bool AddDocument ( int iFields, const char ** ppFields, const CSphMatch & tDoc, bool bReplace ) = 0;

	/// insert/update document in current txn
	/// fails in case of two open txns to different indexes
	virtual bool AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphMatch & tDoc ) = 0;

	/// delete document in current txn
	/// fails in case of two open txns to different indexes
	virtual bool DeleteDocument ( SphDocID_t uDoc ) = 0;

	/// commit pending changes
	virtual void Commit () = 0;

	/// dump index data to disk
	virtual void DumpToDisk ( const char * sFilename ) = 0;
};

/// initialize subsystem
void sphRTInit ();

/// deinitialize subsystem
void sphRTDone ();

/// RT index factory
ISphRtIndex * sphCreateIndexRT ( const CSphSchema & tSchema, DWORD uRamSize, const char * sPath );

#endif // _sphinxrt_

//
// $Id$
//

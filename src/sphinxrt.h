//
// $Id$
//

#ifndef _sphinxrt_
#define _sphinxrt_

#include "sphinx.h"

class ISphRtIndex
{
public:
	virtual void	AddDocument ( const CSphVector<CSphWordHit> & dHits, const CSphDocInfo & tDoc ) = 0;
	virtual void	Commit () = 0;
	virtual void	DumpToDisk ( const char * sFilename ) = 0;
};

ISphRtIndex * sphCreateIndexRT ( const CSphSchema & tSchema );

#endif // _sphinxrt_

//
// $Id$
//

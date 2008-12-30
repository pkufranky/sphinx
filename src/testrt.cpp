//
// $Id$
//

#include "sphinx.h"
#include "sphinxrt.h"

void main ()
{
	int COMMIT_STEP = 1;

	CSphSourceParams_MySQL tParams;
	tParams.m_sHost = "localhost";
	tParams.m_sUser = "root";
	tParams.m_sDB = "lj";
	tParams.m_dQueryPre.Add ( "SET NAMES utf8" );
	tParams.m_sQuery = "SELECT id, title, UNCOMPRESS(content) content FROM posting WHERE id<=10000";

	CSphSource_MySQL * pSrc ( new CSphSource_MySQL ( "test" ) );
	if ( !pSrc->Setup ( tParams ) )
		sphDie ( "setup failed" );

	ISphTokenizer * pTok = sphCreateUTF8Tokenizer();

	CSphDictSettings tDictSettings;
	CSphString sError;
	CSphDict * pDict = sphCreateDictionaryCRC ( tDictSettings, pTok, sError );

	ISphRtIndex * pIndex = sphCreateIndexRT ();

	pSrc->SetTokenizer ( pTok );
	pSrc->SetDict ( pDict );

	if ( !pSrc->Connect ( sError ) )
		sphDie ( "connect failed: %s", sError.cstr() );
	if ( !pSrc->IterateHitsStart ( sError ) )
		sphDie ( "iterate-start failed: %s", sError.cstr() );

	float tmStart = sphLongTimer ();
	for ( ;; )
	{
		if ( !pSrc->IterateHitsNext ( sError ) )
			sphDie ( "iterate-next failed: %s", sError.cstr() );

		if ( !pSrc->m_tDocInfo.m_iDocID )
			break;

		pIndex->AddDocument ( pSrc->m_dHits );
		if (!( pSrc->GetStats().m_iTotalDocuments % COMMIT_STEP ))
			pIndex->Commit ();

		if (!( pSrc->GetStats().m_iTotalDocuments % 1000 ))
		{
			printf ( "%d docs\r", (int)pSrc->GetStats().m_iTotalDocuments );
		}
	}
	pIndex->Commit ();

	float tmEnd = sphLongTimer ();
	float fTotalMB = (float)pSrc->GetStats().m_iTotalBytes/1000000.0f;
	printf ( "commit-step %d, %d docs, %d bytes, %.2f sec, %.2f MB/sec\n",
		COMMIT_STEP,
		(int)pSrc->GetStats().m_iTotalDocuments,
		(int)pSrc->GetStats().m_iTotalBytes, tmEnd-tmStart, fTotalMB/(tmEnd-tmStart) );

	pIndex->DumpToDisk ( "dump" );

	tmEnd = sphLongTimer();
	printf ( "total with dump %.2f sec, %.2f MB/sec\n", tmEnd-tmStart, fTotalMB/(tmEnd-tmStart) );

	SafeDelete ( pIndex );
	SafeDelete ( pDict );
	SafeDelete ( pTok );
}

//
// $Id$
//

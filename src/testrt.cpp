//
// $Id$
//

#include "sphinx.h"
#include "sphinxrt.h"

#if USE_WINDOWS
#include "psapi.h"
#pragma comment(linker, "/defaultlib:psapi.lib")
#pragma message("Automatically linking with psapi.lib")
#endif

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
	float tmAvgCommit = 0.0f;
	float tmMaxCommit = 0.0f;
	int iCommits = 0;
	for ( ;; )
	{
		if ( !pSrc->IterateHitsNext ( sError ) )
			sphDie ( "iterate-next failed: %s", sError.cstr() );

		if ( pSrc->m_tDocInfo.m_iDocID )
			pIndex->AddDocument ( pSrc->m_dHits );

		if ( ( pSrc->GetStats().m_iTotalDocuments % COMMIT_STEP )==0 || !pSrc->m_tDocInfo.m_iDocID )
		{
			float tmCommit = sphLongTimer();
			pIndex->Commit ();
			tmCommit = sphLongTimer()-tmCommit;

			iCommits++;
			tmAvgCommit += tmCommit;
			tmMaxCommit = Max ( tmMaxCommit, tmCommit );

			if ( !pSrc->m_tDocInfo.m_iDocID )
			{
				tmAvgCommit /= iCommits;
				break;
			}
		}

		if (!( pSrc->GetStats().m_iTotalDocuments % 1000 ))
			printf ( "%d docs\r", (int)pSrc->GetStats().m_iTotalDocuments );
	}

	float tmEnd = sphLongTimer ();
	float fTotalMB = (float)pSrc->GetStats().m_iTotalBytes/1000000.0f;
	printf ( "commit-step %d, %d docs, %d bytes, %.2f sec, %.2f MB/sec\n",
		COMMIT_STEP,
		(int)pSrc->GetStats().m_iTotalDocuments,
		(int)pSrc->GetStats().m_iTotalBytes, tmEnd-tmStart, fTotalMB/(tmEnd-tmStart) );
	printf ( "commit-docs %d, avg %.2f msec, max %.2f msec\n", COMMIT_STEP, tmAvgCommit*1000, tmMaxCommit*1000 );

	pIndex->DumpToDisk ( "dump" );

	tmEnd = sphLongTimer();
	printf ( "total with dump %.2f sec, %.2f MB/sec\n", tmEnd-tmStart, fTotalMB/(tmEnd-tmStart) );

#if SPH_ALLOCS_PROFILER
	sphAllocsStats();
#endif
#if USE_WINDOWS
    PROCESS_MEMORY_COUNTERS pmc;
	HANDLE hProcess = OpenProcess ( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId() );
	if ( hProcess && GetProcessMemoryInfo ( hProcess, &pmc, sizeof(pmc)) )
	{
		printf ( "--- peak-wss=%d, peak-pagefile=%d\n", (int)pmc.PeakWorkingSetSize, (int)pmc.PeakPagefileUsage  );
	}
#endif

	SafeDelete ( pIndex );
	SafeDelete ( pDict );
	SafeDelete ( pTok );
}

//
// $Id$
//

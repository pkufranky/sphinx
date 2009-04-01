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

const int	COMMIT_STEP = 1;
float		g_fTotalMB = 0.0f;

void SetupIndexing ( CSphSource_MySQL * pSrc, const CSphSourceParams_MySQL & tParams )
{
	CSphString sError;
	if ( !pSrc->Setup ( tParams ) )
		sphDie ( "setup failed" );
	if ( !pSrc->Connect ( sError ) )
		sphDie ( "connect failed: %s", sError.cstr() );
	if ( !pSrc->IterateHitsStart ( sError ) )
		sphDie ( "iterate-start failed: %s", sError.cstr() );
}


void DoIndexing ( CSphSource * pSrc, ISphRtIndex * pIndex )
{
	CSphString sError;

	int64_t tmStart = sphMicroTimer ();
	int64_t tmAvgCommit = 0;
	int64_t tmMaxCommit = 0;
	int iCommits = 0;
	for ( ;; )
	{
		if ( !pSrc->IterateHitsNext ( sError ) )
			sphDie ( "iterate-next failed: %s", sError.cstr() );

		if ( pSrc->m_tDocInfo.m_iDocID )
			pIndex->AddDocument ( pSrc->m_dHits, pSrc->m_tDocInfo );

		if ( ( pSrc->GetStats().m_iTotalDocuments % COMMIT_STEP )==0 || !pSrc->m_tDocInfo.m_iDocID )
		{
			int64_t tmCommit = sphMicroTimer();
			pIndex->Commit ();
			tmCommit = sphMicroTimer()-tmCommit;

			iCommits++;
			tmAvgCommit += tmCommit;
			tmMaxCommit = Max ( tmMaxCommit, tmCommit );

			if ( !pSrc->m_tDocInfo.m_iDocID )
			{
				tmAvgCommit /= iCommits;
				break;
			}
		}

		if (!( pSrc->GetStats().m_iTotalDocuments % 100 ))
			printf ( "%d docs\r", (int)pSrc->GetStats().m_iTotalDocuments );
	}

	pSrc->Disconnect();

	int64_t tmEnd = sphMicroTimer ();
	float fTotalMB = (float)pSrc->GetStats().m_iTotalBytes/1000000.0f;
	printf ( "commit-step %d, %d docs, %d bytes, %d.%03d sec, %.2f MB/sec\n",
		COMMIT_STEP,
		(int)pSrc->GetStats().m_iTotalDocuments,
		(int)pSrc->GetStats().m_iTotalBytes,
		int((tmEnd-tmStart)/1000000), int(((tmEnd-tmStart)%1000000)/1000),
		fTotalMB*1000000.0f/(tmEnd-tmStart) );
	printf ( "commit-docs %d, avg %d.%3d msec, max %d.%3d msec\n", COMMIT_STEP,
		int(tmAvgCommit/1000), int(tmAvgCommit%1000),
		int(tmMaxCommit/1000), int(tmMaxCommit/1000) );
	g_fTotalMB += fTotalMB;
}

void main ()
{
	CSphString sError;

	CSphSource_MySQL * pSrc = new CSphSource_MySQL ( "test" );
	CSphDictSettings tDictSettings;
	ISphTokenizer * pTok = sphCreateUTF8Tokenizer();
	CSphDict * pDict = sphCreateDictionaryCRC ( tDictSettings, pTok, sError );
	pSrc->SetTokenizer ( pTok );
	pSrc->SetDict ( pDict );

	CSphSourceParams_MySQL tParams;
	tParams.m_sHost = "localhost";
	tParams.m_sUser = "root";
	tParams.m_sDB = "lj";
	tParams.m_dQueryPre.Add ( "SET NAMES utf8" );
	tParams.m_sQuery = "SELECT id, channel_id, UNIX_TIMESTAMP(published) published, title, UNCOMPRESS(content) content FROM rt1 WHERE id<=10000";

	CSphColumnInfo tCol;
	tCol.m_eAttrType = SPH_ATTR_INTEGER;
	tCol.m_sName = "channel_id";
	tParams.m_dAttrs.Add ( tCol );
	tCol.m_eAttrType = SPH_ATTR_TIMESTAMP;
	tCol.m_sName = "published";
	tParams.m_dAttrs.Add ( tCol );

	// initial indexing
	SetupIndexing ( pSrc, tParams );

	CSphSchema tSchema;
	if ( !pSrc->UpdateSchema ( &tSchema, sError ) )
		sphDie ( "update-schema failed: %s", sError.cstr() );
	ISphRtIndex * pIndex = sphCreateIndexRT ( tSchema );

	int64_t tmStart = sphMicroTimer();
	DoIndexing ( pSrc, pIndex );

	// update
	tParams.m_sQuery = "SELECT id, channel_id, UNIX_TIMESTAMP(published) published, title, UNCOMPRESS(content) content FROM rt2 WHERE id<=10000";
	SetupIndexing ( pSrc, tParams );
	DoIndexing ( pSrc, pIndex );

	// dump
	printf ( "pre-dump allocs=%d, bytes=%d\n", sphAllocsCount(), sphAllocBytes() );
	pIndex->DumpToDisk ( "dump" );
	printf ( "post-dump allocs=%d, bytes=%d\n", sphAllocsCount(), sphAllocBytes() );

	int64_t tmEnd = sphMicroTimer();
	printf ( "total with dump %d.%03d sec, %.2f MB/sec\n",
		int((tmEnd-tmStart)/1000000), int(((tmEnd-tmStart)%1000000)/1000),
		g_fTotalMB*1000000.0f/(tmEnd-tmStart) );

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

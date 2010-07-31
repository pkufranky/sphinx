#include <iostream>
#include "sphinx.h"

using namespace std;

void show_usage(const char *cmd) {
	cerr << "Usage: " << cmd << "<dictfile> <word>" << endl;
	cerr << "Word segmentation" << endl;
}

int main(int argc, char **argv) {
	if (argc != 3) {
		show_usage(argv[0]);
		return 1;
	}
	char *szDictFile = argv[1];
	char *szQuery = argv[2];
	cerr << "Dict: " << szDictFile << endl;
	cerr << "Query: " << szQuery << endl;
	CSphString sError;
	ISphTokenizer *pTokenizer = sphCreateUTF8ChineseTokenizer ();
	if (!pTokenizer->SetChineseDictionary (szDictFile, sError )) {
		cerr << "Error: " << sError.cstr() << endl;
		return 1;
	}


	CSphString sQbuf ( szQuery );
	pTokenizer->SetBuffer ( (BYTE*)sQbuf.cstr(), strlen(szQuery) );
	BYTE * sWord;
	while ( ( sWord = pTokenizer->GetToken() )!=NULL ) {
		cerr << "Word: " << sWord << endl;
	}
	return 0;
}

// 初期設定管理クラス

#include "stdafx.h"
#include <windows.h>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>

#include "fwatch.h"
#include "misc.h"

using namespace std;

// クリティカルセッションの定義
//extern CRITICAL_SECTION xCriticalSection;

TIniInfo::TIniInfo()
{
}

TIniInfo::~TIniInfo()
{
}

// キーワードから文字列パラメータを返す。
string TIniInfo::GetParam( string keyword, int index )
{
	string search_keyword = keyword;
	tolowers( &search_keyword );

	map<string, string>::iterator iniparam_it;
	// インデックスが１以上ならば、キーワードに"_"とインデックスの数字を加える。
	if ( index > 0 ) {
		search_keyword += "_" + itos( index );
	}
	iniparam_it = iniparam.find( search_keyword );

	// 指定されたキーワードが登録されていなければ空文字列を返す。
	if ( iniparam_it == iniparam.end() ) return "";

	return iniparam_it->second;
}

// キーワードから数値パラメータを返す。キーワードが存在しないならば-1を返す。
int TIniInfo::GetParamInt( string keyword, int index )
{
	string number = GetParam( keyword, index );
	if ( number.empty() ) return -1;
	return atoi( number.c_str() );
}

// 指定した「キーワード+"_"」と前方一致するキーワードリストを返す。戻り値はキーワードリストの数。
int TIniInfo::GetParamEx( string keyword, map<int,string>* keywordlist )
{
	string search_keyword = keyword;
	tolowers( &search_keyword );

	search_keyword += "_";

	// キーワードリストの初期化
	keywordlist->clear();

	// 指定されたキーワードと前方一致する読み込み済キーワードをキーワードリストへ積む。
	map<string, string>::iterator iniparam_it;
	for ( iniparam_it = iniparam.begin(); iniparam_it != iniparam.end(); iniparam_it++ ) {
		string rkeyword = iniparam_it->first;
		if ( rkeyword.compare( 0, search_keyword.size(), search_keyword ) == 0 ) {
			int number = atoi( rkeyword.substr( search_keyword.size(), rkeyword.size()-search_keyword.size() ).c_str() ); 
			(*keywordlist)[number] = rkeyword;
		}
	}

	return keywordlist->size();
}

// 初期設定ファイルを読み込む（戻り値：成功=true, 失敗=false）
bool TIniInfo::Load( string filename )
{
	// ファイル・ストリームの生成
	ifstream* iniparamifs;
	iniparamifs = new ifstream;

	// ファイルを開く
	iniparamifs->open( filename.c_str() );

	// ファイルが開けた場合
	if ( iniparamifs->is_open() ) {
		string buffer;
		// １行ずつ読み込む
		while ( getline( *iniparamifs, buffer ) ) {
				vector<string> keyword;
				keyword.clear();
				// キーワードとパラメータを取得する
				GetIniTokens( &keyword, &buffer );
				// 必要な情報がない場合、次の行へ進む
				if ( keyword.size() < 2 ) continue;
				// 初期設定パラメータを登録
				tolowers( &keyword[0] );
				cout << "[" << INI_FILENAME << "] " << keyword[0] << " = " << keyword[1] << endl;
				iniparam[keyword[0]] = keyword[1];
		}
		// ファイルを閉じる
		iniparamifs->close();
	}
	// ファイルが開けなかった場合、ログを出力して終了する
	else {
		return false;
	}
	// ファイル・ストリームの削除
	if ( iniparamifs ) delete iniparamifs;

	return true;
}


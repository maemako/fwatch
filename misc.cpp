// 雑用

#include "stdafx.h"
#include <sstream>
#include "misc.h"

// 整数からstringへ変換
string itos( int n, int width )
{
	stringstream buffer;
	buffer.fill( '0' );

	buffer.width( width );
	buffer << n;

	return buffer.str();
}

// 文字列を小文字に変換する
void tolowers( string *buffer )
{
	for ( unsigned int i = 0; i < buffer->length(); i++ ) buffer->at( i ) = tolower( buffer->at( i ) );
}

// 初期設定ファイルからキーワードとパラメータを抽出
void GetIniTokens( vector<string>* keyword, string* buffer )
{
	string delim = " \t=";

	string rkeyword;

	string::size_type spos;
	string::size_type epos;

	string::size_type data_start;
	string::size_type data_end;

	data_start = data_end = spos = 0;
	epos = buffer->size();

	// キーワードの抽出
	data_end = data_start = buffer->find_first_of( delim, data_start );
	// "=" を探したら行末になった＝キーワード無し
	if ( data_start >= epos ) return;
	rkeyword = buffer->substr( spos, data_end );
	// コメント行＝キーワード無し
	if ( rkeyword[0] == '#' ) return;
	if ( rkeyword.size() > 1 ) if ( rkeyword[0] == '/' && rkeyword[1] == '/' ) return;
	spos = data_end;

	// "="の位置がキーワードの直後にあるか確認
	data_start = buffer->find_first_not_of( delim, spos );
	data_end = buffer->find_first_of( "=", spos );
	// キーワードの途中に空白文字が入っている＝パラメータ無し
	if ( data_end > data_start ) return;
	spos = data_end = data_start;

	// キーワードの登録
	keyword->push_back( rkeyword );
	
	// パラメータの抽出
	if ( buffer->at( data_start ) != '"' ) {
		rkeyword = GetToken( buffer, &spos, &epos, " \t" );
	}
	else {
		data_end = buffer->find_last_of( "\"", epos );
		if ( data_end <= data_start ) {
			rkeyword = GetToken( buffer, &spos, &epos, " \t\"" );
		}
		else {
			data_start++;
			rkeyword = buffer->substr( data_start, data_end-data_start );
		}
	}

	keyword->push_back( rkeyword );
}

// トークンを抽出
string GetToken( string* buffer, string::size_type* start, string::size_type* end, string delim )
{
	string::size_type data_start, data_end;

	data_start = buffer->find_first_not_of( delim, *start );
	if ( data_start >= *end ) {
		*start = *end;
		return string( "" );
	}
	data_end = buffer->find_first_of( delim, data_start );

	if ( data_end != string::npos || data_end < *end ) {
		*start = data_end;
	}
	else {
		// string::npos を代入してしまわないための保険
		*start = *end;
	}

	return buffer->substr( data_start, data_end - data_start );
}

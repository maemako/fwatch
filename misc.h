// 雑用ヘッダー

#ifndef __MISC_H__
#define __MISC_H__

#include <vector>

using namespace std;

// 整数からstringへ変換
string itos( int n, int width = 0 );

// 文字列を小文字に変換する
void tolowers( string *buffer );

// 初期設定ファイルからキーワードとパラメータを抽出
void GetIniTokens( vector<string>* keyword, std::string* buffer );

// トークンを抽出
std::string GetToken( std::string* buffer, string::size_type* start, string::size_type* end, std::string delim );

#endif __MISC_H__

// fwatch.cpp : コンソール アプリケーション用のエントリ ポイントの定義
// Makoto Maekawa - IGS.IBM Japan.

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "fwatch.h"
#include "misc.h"

using namespace std;

// スレッド構造体
typedef struct{
	// スレッドのハンドル
    HANDLE	hThread;
	// スレッドの状態（true=開始可能）
	bool	bStatus;
	// スレッドへの指示（true=開始）
    bool	bValid;
	// 再起動回数
	int		RestartCounter;
	// 監視カウンター
	int		IntervalCounter;
	// 監視間隔
	int		IntervalTime;
	// ファイル数の閾値
	int		ThresholdFileCount;
	// 滞留時間
	int		ThresholdStaytime;
	// 滞留時間を過ぎたファイル数の閾値
	int		ThresholdStayCount;
	// 監視ディレクトリ名
	string dirname;
} THREAD;

// 初期設定管理
TIniInfo* IniInfo;

// サーバー名
string ServerName;

// ログファイル名
string LogFileName;

// スレッドの管理域
THREAD xThread[MAX_FOLDERS];

// クリティカルセッションの定義
CRITICAL_SECTION xCriticalSection;

// メイン
int main( int argc, char* argv[] )
{
	// バージョン表示
	cout << APP_NAME << " version " << APP_VERSION << endl;

	// スレッドＩＤの格納域
	DWORD ThreadId[MAX_FOLDERS];

	// ファイル数の閾値
	int ThresholdFileCount;

	// 滞留時間
	int ThresholdStaytime;

	// 滞留時間を過ぎたファイル数の閾値
	int ThresholdStayCount;

	// 再起動回数の閾値
	int ThresholdRestartCount;

	// 初期設定管理のオブジェクト生成
	IniInfo = new TIniInfo;

	// 初期設定ファイルの読み込み
	if ( ! IniInfo->Load( INI_FILENAME ) ) {
		// 初期ファイルの読み込みに失敗したら終了
		cout << CreateLogMessage( MES_INIFILE_OPENERROR ); cout.flush();
		return -1;
	}

	// ログファイル名取得
	LogFileName = IniInfo->GetParam( LOGFILENAME );
	// 初期設定ファイルから取得できなければ、カレントディレクトリに生成する
	if ( LogFileName.empty() ) {
		LogFileName = ".\\";
		LogFileName += APP_NAME;
		LogFileName += ".log";
		cout << LOGFILENAME << "=" << LogFileName << " に設定しました。" << endl;
	}

	// 二重起動の防止
	CreateMutex(NULL, TRUE, APP_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		cout << CreateLogMessage( MES_DOUBLE_STARTINGERROR ); cout.flush();
		return -1;
	}

	// サーバー名
	ServerName = IniInfo->GetParam( SERVERNAME );
	if ( ServerName.empty() ) { // 初期設定がない場合はアプリケーション名を設定
		ServerName = APP_NAME;
		cout << SERVERNAME << "=" << ServerName << " に設定しました。" << endl;
	}

	// クリティカルセクションの初期化
	InitializeCriticalSection( &xCriticalSection );

	// 開始ログを出力
	WriteLogMessage( CreateLogMessage( MES_APP_START ) );

	// 監視フォルダを取得
	map<int,string> keywordlist;
	int FolderCount;
	if ( ( FolderCount = IniInfo->GetParamEx( DIRPATH_PREFIX, &keywordlist ) ) <= 0 ) {
		// 初期設定に存在しない場合は終了（メインからは非同期でログ出力すること）
		WriteLogMessage( CreateLogMessage( MES_DIRPATH_NOTEXIST ) );
		WriteLogMessage( CreateLogMessage( MES_APP_STOP ) );
		return -1;
	}

	// ここからフォルダ別対応可能パラメータ

	// 監視間隔の取得
	int IntervalTime;
	if ( ( IntervalTime = IniInfo->GetParamInt( INTERVALTIME ) ) < 0 ) {
		IntervalTime = 60; // 初期設定がない場合の値
		cout << INTERVALTIME << "=" << IntervalTime << " に設定しました。" << endl;
	}

	// ファイル数の閾値の取得
	if ( ( ThresholdFileCount = IniInfo->GetParamInt( THRESHOLD_FILECOUNT ) ) < 0 ) {
		ThresholdFileCount = 50; // 初期設定がない場合の値
		cout << THRESHOLD_FILECOUNT << "=" << ThresholdFileCount << " に設定しました。" << endl;
	}

	// 滞留時間の取得
	if ( ( ThresholdStaytime = IniInfo->GetParamInt( THRESHOLD_STAYTIME ) ) < 0 ) {
		ThresholdStaytime = 300; // 初期設定がない場合の値
		cout << THRESHOLD_STAYTIME << "=" << ThresholdStaytime << " に設定しました。" << endl;
	}

	// 滞留時間を過ぎたファイル数の閾値の取得
	if ( ( ThresholdStayCount = IniInfo->GetParamInt( THRESHOLD_STAYCOUNT ) ) < 0 ) {
		ThresholdStayCount = 5; // 初期設定がない場合の値
		cout << THRESHOLD_STAYCOUNT << "=" << ThresholdStayCount << " に設定しました。" << endl;
	}

	// 再起動回数の閾値の取得
	if ( ( ThresholdRestartCount = IniInfo->GetParamInt( THRESHOLD_RESTARTCOUNT ) ) < 0 ) {
		ThresholdStayCount = 3; // 初期設定がない場合の値
		cout << THRESHOLD_RESTARTCOUNT << "=" << ThresholdRestartCount << " に設定しました。" << endl;
	}

	// 有効なスレッド数
	int exist = 0;

	// フォルダ情報のイタレータ
	map<int,string>::const_iterator keywordlist_it;

	// スレッド管理域の設定
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		// 識別子が０は初期設定ファイルの設定エラーなのでスキップ
		if ( keywordlist_it->first <= 0 ) continue;

		// フォルダの管理番号を１から０のオフセットに変更
		int counter = keywordlist_it->first-1;
	
		// スレッド管理域をクリア
		ZeroMemory( &xThread[counter], sizeof( THREAD ) );
	
		// スレッドの実行許可（無効）
		xThread[counter].bValid = false;
	
		// スレッド内でファイルチェックが完了していないステータスを設定（完了していない）
		xThread[counter].bStatus = false;

		// 監視間隔の初期設定
		if ( ( xThread[counter].IntervalTime = IniInfo->GetParamInt( INTERVALTIME, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].IntervalTime = IntervalTime;
		}
		// 監視カウンターの初期化
		xThread[counter].IntervalCounter = xThread[counter].IntervalTime;

		// ファイル数の閾値の初期値指定
		if ( ( xThread[counter].ThresholdFileCount = IniInfo->GetParamInt( THRESHOLD_FILECOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdFileCount = ThresholdFileCount;
		}

		// 滞留時間の初期値指定
		if ( ( xThread[counter].ThresholdStaytime = IniInfo->GetParamInt( THRESHOLD_STAYTIME, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdStaytime = ThresholdStaytime;
		}

		// 滞留時間を過ぎたファイル数の閾値の初期値指定
		if ( ( xThread[counter].ThresholdStayCount = IniInfo->GetParamInt( THRESHOLD_STAYCOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdStayCount = ThresholdStayCount;
		}

		// 再起動回数の閾値を設定
		// 滞留時間を過ぎたファイル数の閾値の初期値指定
		if ( ( xThread[counter].RestartCounter = IniInfo->GetParamInt( THRESHOLD_RESTARTCOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].RestartCounter = ThresholdRestartCount;
		}

		// ディレクトリ名を設定
		xThread[counter].dirname = IniInfo->GetParam( DIRNAME_PREFIX, keywordlist_it->first );
		// 取得できない場合、ディレクトリーパスを設定
		if ( xThread[counter].dirname.empty() ) {
			xThread[counter].dirname = IniInfo->GetParam( DIRPATH_PREFIX, keywordlist_it->first );
		}

		// ファイルチェック用のスレッドを作成＆実行
		xThread[counter].hThread = CreateThread(
			NULL,
			0, 
			( LPTHREAD_START_ROUTINE )FileWatch,
			( LPVOID )&keywordlist_it->first,
			0,
			&ThreadId[counter] );

		// スレッドの生成に失敗した
		if ( xThread[counter].hThread == NULL ) {
			WriteLogMessage( CreateLogMessage( MES_CREATETHREAD_ERROR ) + "(" + xThread[counter].dirname + ")" );
		}
		else {
			// 成功したらスレッドを有効にする
			xThread[counter].bValid = true;
			// 有効なスレッド数を＋１
			exist++;
		}
	}

	while ( true ) {

		// 監視最小単位は１秒
		Sleep( 1000L );

		// 監視間隔カウンターを減らして０になっていたらチェック処理、カウンターのリセットをする
		for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
			// スレッドが有効な場合
			if ( xThread[keywordlist_it->first-1].bValid ) {
				// カウンターが０以下の場合
				if ( --xThread[keywordlist_it->first-1].IntervalCounter <= 0 ) {
					// 処理が完了していない場合
					if ( xThread[keywordlist_it->first-1].bStatus != true ) {
						// スレッドの戻り値の一時格納用
						DWORD ThreadRc;
						// スレッドを強制停止する。
						// スレッドの状態を確認する。
						GetExitCodeThread( xThread[keywordlist_it->first-1].hThread, &ThreadRc );
						// スレッドが動作中ならば停止する
						if ( ThreadRc == STILL_ACTIVE ) {
							TerminateThread( xThread[keywordlist_it->first-1].hThread, TERMINATED_THREAD );
							CloseHandle( xThread[keywordlist_it->first-1].hThread );

							// 停止したスレッドを無効にする
							xThread[keywordlist_it->first-1].bValid = false;
							exist--;

							// クリティカルセッションを取得する（約１秒間取得を試みる）
							// 取得できない場合は、応答のないスレッドが持っている可能性がある
							int getcs;
							for ( getcs = 0; getcs < 1000; getcs++ ) {
								if ( TryEnterCriticalSection( &xCriticalSection ) ) break;
								Sleep( 1 );
							}

							// 有効なスレッドをサスペンドする
							map<int,string>::const_iterator list_it;
							for ( list_it = keywordlist.begin(); list_it != keywordlist.end(); list_it++ ) {
								if ( xThread[list_it->first-1].bValid ) {
									SuspendThread( xThread[list_it->first-1].hThread );
								}
							}

							// クリティカルセッションの再初期化
							if ( getcs < 1000 ) LeaveCriticalSection( &xCriticalSection ); // クリティカルセクションを取得できた場合
							DeleteCriticalSection( &xCriticalSection );
							InitializeCriticalSection( &xCriticalSection );

							// 有効なスレッドをリジュームする
							for ( list_it = keywordlist.begin(); list_it != keywordlist.end(); list_it++ ) {
								if ( xThread[list_it->first-1].bValid ) {
									ResumeThread( xThread[list_it->first-1].hThread );
								}
							}
						}

						// 再起動回数を減らして閾値と比較
						if ( --xThread[keywordlist_it->first-1].RestartCounter >= 0 ) {
							// 閾値以内の場合、再起動する
							WriteLogMessage( CreateLogMessage( MES_THREAD_TIMEOUT ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
							// ファイルチェック用のスレッドを作成＆実行
							xThread[keywordlist_it->first-1].hThread = CreateThread(
								NULL,
								0, 
								( LPTHREAD_START_ROUTINE )FileWatch,
								( LPVOID )&keywordlist_it->first,
								0,
								&ThreadId[keywordlist_it->first-1] );
							// スレッドの生成に失敗した
							if ( xThread[keywordlist_it->first-1].hThread == NULL ) {
								WriteLogMessage( CreateLogMessage( MES_CREATETHREAD_ERROR ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
							}
							else {
								// スレッドを有効にする
								xThread[keywordlist_it->first-1].bValid = true;
								exist++;
							}
						}
						else {
							// 閾値を越えた場合、再起動しない
							// 処理対象から外す
							WriteLogMessage( CreateLogMessage( MES_RESTARTCOUNT_OVER ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
						}
					} // 処理が完了していない
					// スレッドが有効ならば、処理開始
					if ( xThread[keywordlist_it->first-1].bValid ) {
						// 監視間隔カウンターをリセット
						xThread[keywordlist_it->first-1].IntervalCounter = xThread[keywordlist_it->first-1].IntervalTime;
						// ファイルチェックの開始を指示
						xThread[keywordlist_it->first-1].bStatus = false;
					}
				} // カウンターが０以下
			} // if
		} // for
		// 有効なスレッドが存在しない場合、アプリケーションを停止する。
		if ( exist <= 0 ) break;
	} // while

	// アプリケーションの終了
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		xThread[keywordlist_it->first-1].bValid = false;
	}
	Sleep( 1000L ); // すべてのスレッドが終わるまで待つのが正解。
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		if ( xThread[keywordlist_it->first-1].hThread != NULL )
			CloseHandle( xThread[keywordlist_it->first-1].hThread );
	}

	// 終了ログを出力
	WriteLogMessage( CreateLogMessage( MES_APP_STOP ) );

	return 0;
}

// ファイル監視のスレッド
void FileWatch( void *id )
{
	// フォルダーID
	int *FolderId = (int*)id;
	
	// 監視するフォルダ
	string FolderPath;
	FolderPath = IniInfo->GetParam( DIRPATH_PREFIX, (int)*FolderId );
	FolderPath += "\\*.*";

	// 検索ハンドル
	HANDLE hFind;

	// ファイル・ディスクリプタ
	WIN32_FIND_DATA fd;

	// ファイルタイム形式の現在の時刻
	FILETIME	nFileTime;

	while ( true ) {
		// ファイルタイム形式の現在の時刻を取得
		GetSystemTimeAsFileTime( &nFileTime );

		// 検索ハンドルの取得
		hFind = FindFirstFile( FolderPath.c_str(), &fd );
		
		// 検索ハンドルが取得できた場合
		if ( hFind != INVALID_HANDLE_VALUE ) {
			// ファイル数をカウントする
			int filecount = 0;
			// 滞留時間を過ぎたファイル数をカウントする
			int staycount = 0;

			do {
				// ディレクトリでない場合
				if ( ! ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
					// ファイル数＋１
					filecount++;
					// ファイル名の取得
					string wfile = string( fd.cFileName );

					// ファイルの最終保存日時が現在の日時よりも古い場合
					if ( CompareFileTime( &fd.ftLastWriteTime, &nFileTime ) < 0 ) {
						// ULONGLONG→FILETIMEキャスト用（ファイル側）
						union {
							FILETIME	fft;
							ULONGLONG	flong;
						};
						// ULONGLONG→FILETIMEキャスト用（現在の時刻用）
						union {
							FILETIME	nft;
							ULONGLONG	nlong;
						};

						// 上記を使用してキャスト
						fft = fd.ftLastWriteTime;
						nft = nFileTime;

						// ファイルと現在の時刻の差を取得
						nlong -= flong;
						// 単位：100ナノ秒 → 秒変換
						nlong /= 10*1000*1000;

						// 64bitINT → 32bitINT のキャスト
						unsigned long difftime = (unsigned long)nlong;

						// 滞留時間の閾値を超えていたらメッセージ出力
						if ( difftime > (unsigned long)xThread[*FolderId-1].ThresholdStaytime ) {
							staycount++;
							// 滞留時間を超えたファイル数が閾値以下ならばメッセージを出力
							if ( staycount <= xThread[*FolderId-1].ThresholdStayCount ) {
								WriteLogMessage( CreateLogMessage( MES_STAYTIME_OVER ) + "(" + wfile + "," + IniInfo->GetParam( DIRNAME_PREFIX, *FolderId ) + ")" );
							}
						}

					}
				}
			} while ( FindNextFile( hFind, &fd ) ); // 次のファイルを取得

			// 滞留時間を超えたファイル数の閾値を超えていたらメッセージ出力
			if ( staycount > xThread[*FolderId-1].ThresholdStayCount ) {
				WriteLogMessage( CreateLogMessage( MES_STAYCOUNT_OVER ) + "(" + itos( staycount ) + "ファイル," + xThread[*FolderId-1].dirname + ")" );
			}

			// ファイル数の閾値を超えていたらメッセージ出力
			if ( filecount > xThread[*FolderId-1].ThresholdFileCount ) {
				WriteLogMessage( CreateLogMessage( MES_FILECOUNT_OVER ) + "(" + itos( filecount ) + "ファイル," + xThread[*FolderId-1].dirname + ")" );
			}

			// 検索ハンドルのクローズ
			FindClose( hFind );
		}
		else {
			// ハンドルの取得エラー
			WriteLogMessage( CreateLogMessage( MES_FINDHANDLE_ERROR ) + "(" + IniInfo->GetParam( DIRNAME_PREFIX, *FolderId ) + ")" );
		}

		// テスト用
		//Sleep( 12000L );

		// ファイルチェックが完了したらステータスを変更する
		xThread[*FolderId-1].bStatus = true;

		// ファイルチェックの開始がＯＫになるまで待つ
		while ( xThread[*FolderId-1].bStatus == true ) {
			// スレッドの終了指示があったら戻る
			if ( xThread[*FolderId-1].bValid == false ) return;
			// 開始がＯＫになるまで待つ間隔
			Sleep( 1000L );
		}
	}
}

// エラーコードからログメッセージの生成
string CreateLogMessage( int id )
{
	// ログメッセージの格納域
	string mes;

	// エラーコードによってメッセージとその付加情報を変える
	switch ( id ) {
		case MES_INIFILE_OPENERROR:
			mes = "初期設定ファイル\"";
			mes += INI_FILENAME;
			mes += "\"の読み込みに失敗しました。\n";
			break;
		case MES_APP_START:
		case MES_APP_STOP:
		case MES_DIRPATH_NOTEXIST:
		case MES_CREATETHREAD_ERROR:
		case MES_THREAD_TIMEOUT:
		case MES_FINDHANDLE_ERROR:
		case MES_STAYTIME_OVER:
		case MES_STAYCOUNT_OVER:
		case MES_FILECOUNT_OVER:
		case MES_RESTARTCOUNT_OVER:
		case MES_DOUBLE_STARTINGERROR:
			mes = IniInfo->GetParam( MESSAGE_PREFIX, id );
			break;
		default:
			mes = "一般的なエラーが発生しました\n";
			break;
	}

	// メッセージが空でなければフォーマットに従って生成する
	if ( ! mes.empty() ) {
		// 現在の時刻
		SYSTEMTIME nLocalTime;
		// 現在の時間を取得
		GetLocalTime( &nLocalTime );
		// 時刻を整形して追加
		mes = itos( nLocalTime.wYear, 4 ) +
			"/" + 
			itos( nLocalTime.wMonth, 2 ) +
			"/" +
			itos( nLocalTime.wDay, 2 ) +
			" " +
			itos( nLocalTime.wHour, 2 ) +
			":" +
			itos( nLocalTime.wMinute, 2 ) +
			":" +
			itos( nLocalTime.wSecond, 2 ) +
			"." +
			itos( nLocalTime.wMilliseconds, 3 ) +
			" " + ServerName + " " + mes;
	}

	return mes;
}

// ログファイルへの書き込み
bool WriteLogMessage( string mes )
{
	// メッセージが空でなければログを書き込む
	if ( ! mes.empty() ) {
		// クリティカルセクションに入る
		EnterCriticalSection(&xCriticalSection);

		// 改行の追加
		mes += "\n";
		// 標準出力にも表示
		cout << mes; cout.flush();

		// ファイルのハンドル
		HANDLE hFile;
		// ログファイルを開く（無ければ作成、エラーなら標準出力をして戻る）
		// 負荷軽減のため、fstream は使用しない
		hFile = CreateFile( LogFileName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		if ( hFile == INVALID_HANDLE_VALUE ) {
			// ログファイルのオープンエラー（標準出力）
			cout << CreateLogMessage( MES_LOGFILE_OPENERROR ) << endl;
			// クリティカルセクションから抜ける
			LeaveCriticalSection(&xCriticalSection);

			return false;
		}
		// ファイルのポインターは最後に移動する
		SetFilePointer( hFile, 0, NULL, FILE_END );

		// ファイルへ書き込み（エラーなら標準出力をして戻る）
		DWORD length; // ダミー
		if ( WriteFile( hFile,
			( LPCVOID )mes.c_str(),
			( DWORD )mes.size(),
			( LPDWORD )&length,
			NULL ) == 0 ) {
			// ログファイルへの書き込みエラー（標準出力）
			cout << CreateLogMessage( MES_LOGFILE_WRITEERROR ) << endl;
			// ファイルのハンドルを開放
			CloseHandle( hFile );
			// クリティカルセクションから抜ける
			LeaveCriticalSection(&xCriticalSection);

			return false;
		}
		// ファイルのハンドルを開放
		CloseHandle( hFile );

		// クリティカルセクションから抜ける
		LeaveCriticalSection(&xCriticalSection);
	}

	return true;
}

// fwatch 用ヘッダー

#ifndef __FWATCH_H__
#define __FWATCH_H__

// disable warning C4786: symbol greater than 255 character, okay to ignore
#pragma warning( disable : 4786 )
#pragma setlocale( "japanese" )

// アプリケーション名
#define APP_NAME				"fwatch"
// アプリケーションのバージョン
#define APP_VERSION				"1.0"
// 初期情報ファイル
#define INI_FILENAME			"fwatch.ini"
// ファイル監視できる最大フォルダ数
#define MAX_FOLDERS				50

// 初期設定ファイルのキーワード定義（大文字、小文字の区別はしません）
#define SERVERNAME				"servername"
#define LOGFILENAME				"logfile"
// 以下はサフィックスとして"_(0を含まない正数)"が使用でき、設定をオーバーライトする
#define INTERVALTIME			"interval"
#define THRESHOLD_FILECOUNT		"filecount"
#define THRESHOLD_STAYTIME		"staytime"
#define THRESHOLD_STAYCOUNT		"filecountbystaytime"
#define THRESHOLD_RESTARTCOUNT	"restartcount"
#define DIRNAME_PREFIX			"directoryname"
#define DIRPATH_PREFIX			"directorypath"
#define MESSAGE_PREFIX			"message"

#include <map>
using namespace std;

// スレッドを強制終了したときの戻り値
#define TERMINATED_THREAD		-1

// メッセージ定義
enum
{
	MES_INIFILE_OPENERROR = 0,
	MES_LOGFILE_OPENERROR,
	MES_LOGFILE_WRITEERROR,
	MES_APP_START,
	MES_APP_STOP,
	MES_DIRPATH_NOTEXIST,
	MES_CREATETHREAD_ERROR,
	MES_THREAD_TIMEOUT,
	MES_FINDHANDLE_ERROR,
	MES_STAYTIME_OVER,
	MES_STAYCOUNT_OVER,
	MES_FILECOUNT_OVER,
	MES_RESTARTCOUNT_OVER,
	MES_DOUBLE_STARTINGERROR
};
// データ種別
enum
{
	TYPE_STRING = 0,
	TYPE_INT
};

// ファイル監視スレッド
void FileWatch( void *param );
// エラーコードからログメッセージの生成
std::string CreateLogMessage( int id );
// ログファイルへの書き込み
bool WriteLogMessage( std::string mes );

// 初期設定管理
class TIniInfo
{
private:
	// ini情報
	map<string, string> iniparam;

public:
	TIniInfo();
	~TIniInfo();
	// 初期設定ファイルを読み込む（戻り値：成功=true, 失敗=false）
	bool Load( std::string filename );
	// キーワードから文字列パラメータを返す。
	string GetParam( string keyword, int index = 0);
	// キーワードから数値パラメータを返す。
	int GetParamInt( string keyword, int index = 0);
	// 指定したキーワードと前方一致するキーワードリストを返す。戻り値はキーワードリストの数。
	int GetParamEx( string keyword, map<int,string>* keywordlist );
};

#endif __FWATCH_H__

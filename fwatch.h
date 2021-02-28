// fwatch �p�w�b�_�[

#ifndef __FWATCH_H__
#define __FWATCH_H__

// disable warning C4786: symbol greater than 255 character, okay to ignore
#pragma warning( disable : 4786 )
#pragma setlocale( "japanese" )

// �A�v���P�[�V������
#define APP_NAME				"fwatch"
// �A�v���P�[�V�����̃o�[�W����
#define APP_VERSION				"1.0"
// �������t�@�C��
#define INI_FILENAME			"fwatch.ini"
// �t�@�C���Ď��ł���ő�t�H���_��
#define MAX_FOLDERS				50

// �����ݒ�t�@�C���̃L�[���[�h��`�i�啶���A�������̋�ʂ͂��܂���j
#define SERVERNAME				"servername"
#define LOGFILENAME				"logfile"
// �ȉ��̓T�t�B�b�N�X�Ƃ���"_(0���܂܂Ȃ�����)"���g�p�ł��A�ݒ���I�[�o�[���C�g����
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

// �X���b�h�������I�������Ƃ��̖߂�l
#define TERMINATED_THREAD		-1

// ���b�Z�[�W��`
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
// �f�[�^���
enum
{
	TYPE_STRING = 0,
	TYPE_INT
};

// �t�@�C���Ď��X���b�h
void FileWatch( void *param );
// �G���[�R�[�h���烍�O���b�Z�[�W�̐���
std::string CreateLogMessage( int id );
// ���O�t�@�C���ւ̏�������
bool WriteLogMessage( std::string mes );

// �����ݒ�Ǘ�
class TIniInfo
{
private:
	// ini���
	map<string, string> iniparam;

public:
	TIniInfo();
	~TIniInfo();
	// �����ݒ�t�@�C����ǂݍ��ށi�߂�l�F����=true, ���s=false�j
	bool Load( std::string filename );
	// �L�[���[�h���當����p�����[�^��Ԃ��B
	string GetParam( string keyword, int index = 0);
	// �L�[���[�h���琔�l�p�����[�^��Ԃ��B
	int GetParamInt( string keyword, int index = 0);
	// �w�肵���L�[���[�h�ƑO����v����L�[���[�h���X�g��Ԃ��B�߂�l�̓L�[���[�h���X�g�̐��B
	int GetParamEx( string keyword, map<int,string>* keywordlist );
};

#endif __FWATCH_H__

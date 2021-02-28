// fwatch.cpp : �R���\�[�� �A�v���P�[�V�����p�̃G���g�� �|�C���g�̒�`
// Makoto Maekawa - IGS.IBM Japan.

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

#include "fwatch.h"
#include "misc.h"

using namespace std;

// �X���b�h�\����
typedef struct{
	// �X���b�h�̃n���h��
    HANDLE	hThread;
	// �X���b�h�̏�ԁitrue=�J�n�\�j
	bool	bStatus;
	// �X���b�h�ւ̎w���itrue=�J�n�j
    bool	bValid;
	// �ċN����
	int		RestartCounter;
	// �Ď��J�E���^�[
	int		IntervalCounter;
	// �Ď��Ԋu
	int		IntervalTime;
	// �t�@�C������臒l
	int		ThresholdFileCount;
	// �ؗ�����
	int		ThresholdStaytime;
	// �ؗ����Ԃ��߂����t�@�C������臒l
	int		ThresholdStayCount;
	// �Ď��f�B���N�g����
	string dirname;
} THREAD;

// �����ݒ�Ǘ�
TIniInfo* IniInfo;

// �T�[�o�[��
string ServerName;

// ���O�t�@�C����
string LogFileName;

// �X���b�h�̊Ǘ���
THREAD xThread[MAX_FOLDERS];

// �N���e�B�J���Z�b�V�����̒�`
CRITICAL_SECTION xCriticalSection;

// ���C��
int main( int argc, char* argv[] )
{
	// �o�[�W�����\��
	cout << APP_NAME << " version " << APP_VERSION << endl;

	// �X���b�h�h�c�̊i�[��
	DWORD ThreadId[MAX_FOLDERS];

	// �t�@�C������臒l
	int ThresholdFileCount;

	// �ؗ�����
	int ThresholdStaytime;

	// �ؗ����Ԃ��߂����t�@�C������臒l
	int ThresholdStayCount;

	// �ċN���񐔂�臒l
	int ThresholdRestartCount;

	// �����ݒ�Ǘ��̃I�u�W�F�N�g����
	IniInfo = new TIniInfo;

	// �����ݒ�t�@�C���̓ǂݍ���
	if ( ! IniInfo->Load( INI_FILENAME ) ) {
		// �����t�@�C���̓ǂݍ��݂Ɏ��s������I��
		cout << CreateLogMessage( MES_INIFILE_OPENERROR ); cout.flush();
		return -1;
	}

	// ���O�t�@�C�����擾
	LogFileName = IniInfo->GetParam( LOGFILENAME );
	// �����ݒ�t�@�C������擾�ł��Ȃ���΁A�J�����g�f�B���N�g���ɐ�������
	if ( LogFileName.empty() ) {
		LogFileName = ".\\";
		LogFileName += APP_NAME;
		LogFileName += ".log";
		cout << LOGFILENAME << "=" << LogFileName << " �ɐݒ肵�܂����B" << endl;
	}

	// ��d�N���̖h�~
	CreateMutex(NULL, TRUE, APP_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		cout << CreateLogMessage( MES_DOUBLE_STARTINGERROR ); cout.flush();
		return -1;
	}

	// �T�[�o�[��
	ServerName = IniInfo->GetParam( SERVERNAME );
	if ( ServerName.empty() ) { // �����ݒ肪�Ȃ��ꍇ�̓A�v���P�[�V��������ݒ�
		ServerName = APP_NAME;
		cout << SERVERNAME << "=" << ServerName << " �ɐݒ肵�܂����B" << endl;
	}

	// �N���e�B�J���Z�N�V�����̏�����
	InitializeCriticalSection( &xCriticalSection );

	// �J�n���O���o��
	WriteLogMessage( CreateLogMessage( MES_APP_START ) );

	// �Ď��t�H���_���擾
	map<int,string> keywordlist;
	int FolderCount;
	if ( ( FolderCount = IniInfo->GetParamEx( DIRPATH_PREFIX, &keywordlist ) ) <= 0 ) {
		// �����ݒ�ɑ��݂��Ȃ��ꍇ�͏I���i���C������͔񓯊��Ń��O�o�͂��邱�Ɓj
		WriteLogMessage( CreateLogMessage( MES_DIRPATH_NOTEXIST ) );
		WriteLogMessage( CreateLogMessage( MES_APP_STOP ) );
		return -1;
	}

	// ��������t�H���_�ʑΉ��\�p�����[�^

	// �Ď��Ԋu�̎擾
	int IntervalTime;
	if ( ( IntervalTime = IniInfo->GetParamInt( INTERVALTIME ) ) < 0 ) {
		IntervalTime = 60; // �����ݒ肪�Ȃ��ꍇ�̒l
		cout << INTERVALTIME << "=" << IntervalTime << " �ɐݒ肵�܂����B" << endl;
	}

	// �t�@�C������臒l�̎擾
	if ( ( ThresholdFileCount = IniInfo->GetParamInt( THRESHOLD_FILECOUNT ) ) < 0 ) {
		ThresholdFileCount = 50; // �����ݒ肪�Ȃ��ꍇ�̒l
		cout << THRESHOLD_FILECOUNT << "=" << ThresholdFileCount << " �ɐݒ肵�܂����B" << endl;
	}

	// �ؗ����Ԃ̎擾
	if ( ( ThresholdStaytime = IniInfo->GetParamInt( THRESHOLD_STAYTIME ) ) < 0 ) {
		ThresholdStaytime = 300; // �����ݒ肪�Ȃ��ꍇ�̒l
		cout << THRESHOLD_STAYTIME << "=" << ThresholdStaytime << " �ɐݒ肵�܂����B" << endl;
	}

	// �ؗ����Ԃ��߂����t�@�C������臒l�̎擾
	if ( ( ThresholdStayCount = IniInfo->GetParamInt( THRESHOLD_STAYCOUNT ) ) < 0 ) {
		ThresholdStayCount = 5; // �����ݒ肪�Ȃ��ꍇ�̒l
		cout << THRESHOLD_STAYCOUNT << "=" << ThresholdStayCount << " �ɐݒ肵�܂����B" << endl;
	}

	// �ċN���񐔂�臒l�̎擾
	if ( ( ThresholdRestartCount = IniInfo->GetParamInt( THRESHOLD_RESTARTCOUNT ) ) < 0 ) {
		ThresholdStayCount = 3; // �����ݒ肪�Ȃ��ꍇ�̒l
		cout << THRESHOLD_RESTARTCOUNT << "=" << ThresholdRestartCount << " �ɐݒ肵�܂����B" << endl;
	}

	// �L���ȃX���b�h��
	int exist = 0;

	// �t�H���_���̃C�^���[�^
	map<int,string>::const_iterator keywordlist_it;

	// �X���b�h�Ǘ���̐ݒ�
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		// ���ʎq���O�͏����ݒ�t�@�C���̐ݒ�G���[�Ȃ̂ŃX�L�b�v
		if ( keywordlist_it->first <= 0 ) continue;

		// �t�H���_�̊Ǘ��ԍ����P����O�̃I�t�Z�b�g�ɕύX
		int counter = keywordlist_it->first-1;
	
		// �X���b�h�Ǘ�����N���A
		ZeroMemory( &xThread[counter], sizeof( THREAD ) );
	
		// �X���b�h�̎��s���i�����j
		xThread[counter].bValid = false;
	
		// �X���b�h���Ńt�@�C���`�F�b�N���������Ă��Ȃ��X�e�[�^�X��ݒ�i�������Ă��Ȃ��j
		xThread[counter].bStatus = false;

		// �Ď��Ԋu�̏����ݒ�
		if ( ( xThread[counter].IntervalTime = IniInfo->GetParamInt( INTERVALTIME, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].IntervalTime = IntervalTime;
		}
		// �Ď��J�E���^�[�̏�����
		xThread[counter].IntervalCounter = xThread[counter].IntervalTime;

		// �t�@�C������臒l�̏����l�w��
		if ( ( xThread[counter].ThresholdFileCount = IniInfo->GetParamInt( THRESHOLD_FILECOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdFileCount = ThresholdFileCount;
		}

		// �ؗ����Ԃ̏����l�w��
		if ( ( xThread[counter].ThresholdStaytime = IniInfo->GetParamInt( THRESHOLD_STAYTIME, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdStaytime = ThresholdStaytime;
		}

		// �ؗ����Ԃ��߂����t�@�C������臒l�̏����l�w��
		if ( ( xThread[counter].ThresholdStayCount = IniInfo->GetParamInt( THRESHOLD_STAYCOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].ThresholdStayCount = ThresholdStayCount;
		}

		// �ċN���񐔂�臒l��ݒ�
		// �ؗ����Ԃ��߂����t�@�C������臒l�̏����l�w��
		if ( ( xThread[counter].RestartCounter = IniInfo->GetParamInt( THRESHOLD_RESTARTCOUNT, keywordlist_it->first ) ) < 0 ) {
			xThread[counter].RestartCounter = ThresholdRestartCount;
		}

		// �f�B���N�g������ݒ�
		xThread[counter].dirname = IniInfo->GetParam( DIRNAME_PREFIX, keywordlist_it->first );
		// �擾�ł��Ȃ��ꍇ�A�f�B���N�g���[�p�X��ݒ�
		if ( xThread[counter].dirname.empty() ) {
			xThread[counter].dirname = IniInfo->GetParam( DIRPATH_PREFIX, keywordlist_it->first );
		}

		// �t�@�C���`�F�b�N�p�̃X���b�h���쐬�����s
		xThread[counter].hThread = CreateThread(
			NULL,
			0, 
			( LPTHREAD_START_ROUTINE )FileWatch,
			( LPVOID )&keywordlist_it->first,
			0,
			&ThreadId[counter] );

		// �X���b�h�̐����Ɏ��s����
		if ( xThread[counter].hThread == NULL ) {
			WriteLogMessage( CreateLogMessage( MES_CREATETHREAD_ERROR ) + "(" + xThread[counter].dirname + ")" );
		}
		else {
			// ����������X���b�h��L���ɂ���
			xThread[counter].bValid = true;
			// �L���ȃX���b�h�����{�P
			exist++;
		}
	}

	while ( true ) {

		// �Ď��ŏ��P�ʂ͂P�b
		Sleep( 1000L );

		// �Ď��Ԋu�J�E���^�[�����炵�ĂO�ɂȂ��Ă�����`�F�b�N�����A�J�E���^�[�̃��Z�b�g������
		for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
			// �X���b�h���L���ȏꍇ
			if ( xThread[keywordlist_it->first-1].bValid ) {
				// �J�E���^�[���O�ȉ��̏ꍇ
				if ( --xThread[keywordlist_it->first-1].IntervalCounter <= 0 ) {
					// �������������Ă��Ȃ��ꍇ
					if ( xThread[keywordlist_it->first-1].bStatus != true ) {
						// �X���b�h�̖߂�l�̈ꎞ�i�[�p
						DWORD ThreadRc;
						// �X���b�h��������~����B
						// �X���b�h�̏�Ԃ��m�F����B
						GetExitCodeThread( xThread[keywordlist_it->first-1].hThread, &ThreadRc );
						// �X���b�h�����쒆�Ȃ�Β�~����
						if ( ThreadRc == STILL_ACTIVE ) {
							TerminateThread( xThread[keywordlist_it->first-1].hThread, TERMINATED_THREAD );
							CloseHandle( xThread[keywordlist_it->first-1].hThread );

							// ��~�����X���b�h�𖳌��ɂ���
							xThread[keywordlist_it->first-1].bValid = false;
							exist--;

							// �N���e�B�J���Z�b�V�������擾����i��P�b�Ԏ擾�����݂�j
							// �擾�ł��Ȃ��ꍇ�́A�����̂Ȃ��X���b�h�������Ă���\��������
							int getcs;
							for ( getcs = 0; getcs < 1000; getcs++ ) {
								if ( TryEnterCriticalSection( &xCriticalSection ) ) break;
								Sleep( 1 );
							}

							// �L���ȃX���b�h���T�X�y���h����
							map<int,string>::const_iterator list_it;
							for ( list_it = keywordlist.begin(); list_it != keywordlist.end(); list_it++ ) {
								if ( xThread[list_it->first-1].bValid ) {
									SuspendThread( xThread[list_it->first-1].hThread );
								}
							}

							// �N���e�B�J���Z�b�V�����̍ď�����
							if ( getcs < 1000 ) LeaveCriticalSection( &xCriticalSection ); // �N���e�B�J���Z�N�V�������擾�ł����ꍇ
							DeleteCriticalSection( &xCriticalSection );
							InitializeCriticalSection( &xCriticalSection );

							// �L���ȃX���b�h�����W���[������
							for ( list_it = keywordlist.begin(); list_it != keywordlist.end(); list_it++ ) {
								if ( xThread[list_it->first-1].bValid ) {
									ResumeThread( xThread[list_it->first-1].hThread );
								}
							}
						}

						// �ċN���񐔂����炵��臒l�Ɣ�r
						if ( --xThread[keywordlist_it->first-1].RestartCounter >= 0 ) {
							// 臒l�ȓ��̏ꍇ�A�ċN������
							WriteLogMessage( CreateLogMessage( MES_THREAD_TIMEOUT ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
							// �t�@�C���`�F�b�N�p�̃X���b�h���쐬�����s
							xThread[keywordlist_it->first-1].hThread = CreateThread(
								NULL,
								0, 
								( LPTHREAD_START_ROUTINE )FileWatch,
								( LPVOID )&keywordlist_it->first,
								0,
								&ThreadId[keywordlist_it->first-1] );
							// �X���b�h�̐����Ɏ��s����
							if ( xThread[keywordlist_it->first-1].hThread == NULL ) {
								WriteLogMessage( CreateLogMessage( MES_CREATETHREAD_ERROR ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
							}
							else {
								// �X���b�h��L���ɂ���
								xThread[keywordlist_it->first-1].bValid = true;
								exist++;
							}
						}
						else {
							// 臒l���z�����ꍇ�A�ċN�����Ȃ�
							// �����Ώۂ���O��
							WriteLogMessage( CreateLogMessage( MES_RESTARTCOUNT_OVER ) + "(" + xThread[keywordlist_it->first-1].dirname + ")" );
						}
					} // �������������Ă��Ȃ�
					// �X���b�h���L���Ȃ�΁A�����J�n
					if ( xThread[keywordlist_it->first-1].bValid ) {
						// �Ď��Ԋu�J�E���^�[�����Z�b�g
						xThread[keywordlist_it->first-1].IntervalCounter = xThread[keywordlist_it->first-1].IntervalTime;
						// �t�@�C���`�F�b�N�̊J�n���w��
						xThread[keywordlist_it->first-1].bStatus = false;
					}
				} // �J�E���^�[���O�ȉ�
			} // if
		} // for
		// �L���ȃX���b�h�����݂��Ȃ��ꍇ�A�A�v���P�[�V�������~����B
		if ( exist <= 0 ) break;
	} // while

	// �A�v���P�[�V�����̏I��
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		xThread[keywordlist_it->first-1].bValid = false;
	}
	Sleep( 1000L ); // ���ׂẴX���b�h���I���܂ő҂̂������B
	for ( keywordlist_it = keywordlist.begin(); keywordlist_it != keywordlist.end(); keywordlist_it++ ) {
		if ( xThread[keywordlist_it->first-1].hThread != NULL )
			CloseHandle( xThread[keywordlist_it->first-1].hThread );
	}

	// �I�����O���o��
	WriteLogMessage( CreateLogMessage( MES_APP_STOP ) );

	return 0;
}

// �t�@�C���Ď��̃X���b�h
void FileWatch( void *id )
{
	// �t�H���_�[ID
	int *FolderId = (int*)id;
	
	// �Ď�����t�H���_
	string FolderPath;
	FolderPath = IniInfo->GetParam( DIRPATH_PREFIX, (int)*FolderId );
	FolderPath += "\\*.*";

	// �����n���h��
	HANDLE hFind;

	// �t�@�C���E�f�B�X�N���v�^
	WIN32_FIND_DATA fd;

	// �t�@�C���^�C���`���̌��݂̎���
	FILETIME	nFileTime;

	while ( true ) {
		// �t�@�C���^�C���`���̌��݂̎������擾
		GetSystemTimeAsFileTime( &nFileTime );

		// �����n���h���̎擾
		hFind = FindFirstFile( FolderPath.c_str(), &fd );
		
		// �����n���h�����擾�ł����ꍇ
		if ( hFind != INVALID_HANDLE_VALUE ) {
			// �t�@�C�������J�E���g����
			int filecount = 0;
			// �ؗ����Ԃ��߂����t�@�C�������J�E���g����
			int staycount = 0;

			do {
				// �f�B���N�g���łȂ��ꍇ
				if ( ! ( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) ) {
					// �t�@�C�����{�P
					filecount++;
					// �t�@�C�����̎擾
					string wfile = string( fd.cFileName );

					// �t�@�C���̍ŏI�ۑ����������݂̓��������Â��ꍇ
					if ( CompareFileTime( &fd.ftLastWriteTime, &nFileTime ) < 0 ) {
						// ULONGLONG��FILETIME�L���X�g�p�i�t�@�C�����j
						union {
							FILETIME	fft;
							ULONGLONG	flong;
						};
						// ULONGLONG��FILETIME�L���X�g�p�i���݂̎����p�j
						union {
							FILETIME	nft;
							ULONGLONG	nlong;
						};

						// ��L���g�p���ăL���X�g
						fft = fd.ftLastWriteTime;
						nft = nFileTime;

						// �t�@�C���ƌ��݂̎����̍����擾
						nlong -= flong;
						// �P�ʁF100�i�m�b �� �b�ϊ�
						nlong /= 10*1000*1000;

						// 64bitINT �� 32bitINT �̃L���X�g
						unsigned long difftime = (unsigned long)nlong;

						// �ؗ����Ԃ�臒l�𒴂��Ă����烁�b�Z�[�W�o��
						if ( difftime > (unsigned long)xThread[*FolderId-1].ThresholdStaytime ) {
							staycount++;
							// �ؗ����Ԃ𒴂����t�@�C������臒l�ȉ��Ȃ�΃��b�Z�[�W���o��
							if ( staycount <= xThread[*FolderId-1].ThresholdStayCount ) {
								WriteLogMessage( CreateLogMessage( MES_STAYTIME_OVER ) + "(" + wfile + "," + IniInfo->GetParam( DIRNAME_PREFIX, *FolderId ) + ")" );
							}
						}

					}
				}
			} while ( FindNextFile( hFind, &fd ) ); // ���̃t�@�C�����擾

			// �ؗ����Ԃ𒴂����t�@�C������臒l�𒴂��Ă����烁�b�Z�[�W�o��
			if ( staycount > xThread[*FolderId-1].ThresholdStayCount ) {
				WriteLogMessage( CreateLogMessage( MES_STAYCOUNT_OVER ) + "(" + itos( staycount ) + "�t�@�C��," + xThread[*FolderId-1].dirname + ")" );
			}

			// �t�@�C������臒l�𒴂��Ă����烁�b�Z�[�W�o��
			if ( filecount > xThread[*FolderId-1].ThresholdFileCount ) {
				WriteLogMessage( CreateLogMessage( MES_FILECOUNT_OVER ) + "(" + itos( filecount ) + "�t�@�C��," + xThread[*FolderId-1].dirname + ")" );
			}

			// �����n���h���̃N���[�Y
			FindClose( hFind );
		}
		else {
			// �n���h���̎擾�G���[
			WriteLogMessage( CreateLogMessage( MES_FINDHANDLE_ERROR ) + "(" + IniInfo->GetParam( DIRNAME_PREFIX, *FolderId ) + ")" );
		}

		// �e�X�g�p
		//Sleep( 12000L );

		// �t�@�C���`�F�b�N������������X�e�[�^�X��ύX����
		xThread[*FolderId-1].bStatus = true;

		// �t�@�C���`�F�b�N�̊J�n���n�j�ɂȂ�܂ő҂�
		while ( xThread[*FolderId-1].bStatus == true ) {
			// �X���b�h�̏I���w������������߂�
			if ( xThread[*FolderId-1].bValid == false ) return;
			// �J�n���n�j�ɂȂ�܂ő҂Ԋu
			Sleep( 1000L );
		}
	}
}

// �G���[�R�[�h���烍�O���b�Z�[�W�̐���
string CreateLogMessage( int id )
{
	// ���O���b�Z�[�W�̊i�[��
	string mes;

	// �G���[�R�[�h�ɂ���ă��b�Z�[�W�Ƃ��̕t������ς���
	switch ( id ) {
		case MES_INIFILE_OPENERROR:
			mes = "�����ݒ�t�@�C��\"";
			mes += INI_FILENAME;
			mes += "\"�̓ǂݍ��݂Ɏ��s���܂����B\n";
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
			mes = "��ʓI�ȃG���[���������܂���\n";
			break;
	}

	// ���b�Z�[�W����łȂ���΃t�H�[�}�b�g�ɏ]���Đ�������
	if ( ! mes.empty() ) {
		// ���݂̎���
		SYSTEMTIME nLocalTime;
		// ���݂̎��Ԃ��擾
		GetLocalTime( &nLocalTime );
		// �����𐮌`���Ēǉ�
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

// ���O�t�@�C���ւ̏�������
bool WriteLogMessage( string mes )
{
	// ���b�Z�[�W����łȂ���΃��O����������
	if ( ! mes.empty() ) {
		// �N���e�B�J���Z�N�V�����ɓ���
		EnterCriticalSection(&xCriticalSection);

		// ���s�̒ǉ�
		mes += "\n";
		// �W���o�͂ɂ��\��
		cout << mes; cout.flush();

		// �t�@�C���̃n���h��
		HANDLE hFile;
		// ���O�t�@�C�����J���i������΍쐬�A�G���[�Ȃ�W���o�͂����Ė߂�j
		// ���׌y���̂��߁Afstream �͎g�p���Ȃ�
		hFile = CreateFile( LogFileName.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
		if ( hFile == INVALID_HANDLE_VALUE ) {
			// ���O�t�@�C���̃I�[�v���G���[�i�W���o�́j
			cout << CreateLogMessage( MES_LOGFILE_OPENERROR ) << endl;
			// �N���e�B�J���Z�N�V�������甲����
			LeaveCriticalSection(&xCriticalSection);

			return false;
		}
		// �t�@�C���̃|�C���^�[�͍Ō�Ɉړ�����
		SetFilePointer( hFile, 0, NULL, FILE_END );

		// �t�@�C���֏������݁i�G���[�Ȃ�W���o�͂����Ė߂�j
		DWORD length; // �_�~�[
		if ( WriteFile( hFile,
			( LPCVOID )mes.c_str(),
			( DWORD )mes.size(),
			( LPDWORD )&length,
			NULL ) == 0 ) {
			// ���O�t�@�C���ւ̏������݃G���[�i�W���o�́j
			cout << CreateLogMessage( MES_LOGFILE_WRITEERROR ) << endl;
			// �t�@�C���̃n���h�����J��
			CloseHandle( hFile );
			// �N���e�B�J���Z�N�V�������甲����
			LeaveCriticalSection(&xCriticalSection);

			return false;
		}
		// �t�@�C���̃n���h�����J��
		CloseHandle( hFile );

		// �N���e�B�J���Z�N�V�������甲����
		LeaveCriticalSection(&xCriticalSection);
	}

	return true;
}

// �����ݒ�Ǘ��N���X

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

// �N���e�B�J���Z�b�V�����̒�`
//extern CRITICAL_SECTION xCriticalSection;

TIniInfo::TIniInfo()
{
}

TIniInfo::~TIniInfo()
{
}

// �L�[���[�h���當����p�����[�^��Ԃ��B
string TIniInfo::GetParam( string keyword, int index )
{
	string search_keyword = keyword;
	tolowers( &search_keyword );

	map<string, string>::iterator iniparam_it;
	// �C���f�b�N�X���P�ȏ�Ȃ�΁A�L�[���[�h��"_"�ƃC���f�b�N�X�̐�����������B
	if ( index > 0 ) {
		search_keyword += "_" + itos( index );
	}
	iniparam_it = iniparam.find( search_keyword );

	// �w�肳�ꂽ�L�[���[�h���o�^����Ă��Ȃ���΋󕶎����Ԃ��B
	if ( iniparam_it == iniparam.end() ) return "";

	return iniparam_it->second;
}

// �L�[���[�h���琔�l�p�����[�^��Ԃ��B�L�[���[�h�����݂��Ȃ��Ȃ��-1��Ԃ��B
int TIniInfo::GetParamInt( string keyword, int index )
{
	string number = GetParam( keyword, index );
	if ( number.empty() ) return -1;
	return atoi( number.c_str() );
}

// �w�肵���u�L�[���[�h+"_"�v�ƑO����v����L�[���[�h���X�g��Ԃ��B�߂�l�̓L�[���[�h���X�g�̐��B
int TIniInfo::GetParamEx( string keyword, map<int,string>* keywordlist )
{
	string search_keyword = keyword;
	tolowers( &search_keyword );

	search_keyword += "_";

	// �L�[���[�h���X�g�̏�����
	keywordlist->clear();

	// �w�肳�ꂽ�L�[���[�h�ƑO����v����ǂݍ��ݍσL�[���[�h���L�[���[�h���X�g�֐ςށB
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

// �����ݒ�t�@�C����ǂݍ��ށi�߂�l�F����=true, ���s=false�j
bool TIniInfo::Load( string filename )
{
	// �t�@�C���E�X�g���[���̐���
	ifstream* iniparamifs;
	iniparamifs = new ifstream;

	// �t�@�C�����J��
	iniparamifs->open( filename.c_str() );

	// �t�@�C�����J�����ꍇ
	if ( iniparamifs->is_open() ) {
		string buffer;
		// �P�s���ǂݍ���
		while ( getline( *iniparamifs, buffer ) ) {
				vector<string> keyword;
				keyword.clear();
				// �L�[���[�h�ƃp�����[�^���擾����
				GetIniTokens( &keyword, &buffer );
				// �K�v�ȏ�񂪂Ȃ��ꍇ�A���̍s�֐i��
				if ( keyword.size() < 2 ) continue;
				// �����ݒ�p�����[�^��o�^
				tolowers( &keyword[0] );
				cout << "[" << INI_FILENAME << "] " << keyword[0] << " = " << keyword[1] << endl;
				iniparam[keyword[0]] = keyword[1];
		}
		// �t�@�C�������
		iniparamifs->close();
	}
	// �t�@�C�����J���Ȃ������ꍇ�A���O���o�͂��ďI������
	else {
		return false;
	}
	// �t�@�C���E�X�g���[���̍폜
	if ( iniparamifs ) delete iniparamifs;

	return true;
}


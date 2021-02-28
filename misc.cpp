// �G�p

#include "stdafx.h"
#include <sstream>
#include "misc.h"

// ��������string�֕ϊ�
string itos( int n, int width )
{
	stringstream buffer;
	buffer.fill( '0' );

	buffer.width( width );
	buffer << n;

	return buffer.str();
}

// ��������������ɕϊ�����
void tolowers( string *buffer )
{
	for ( unsigned int i = 0; i < buffer->length(); i++ ) buffer->at( i ) = tolower( buffer->at( i ) );
}

// �����ݒ�t�@�C������L�[���[�h�ƃp�����[�^�𒊏o
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

	// �L�[���[�h�̒��o
	data_end = data_start = buffer->find_first_of( delim, data_start );
	// "=" ��T������s���ɂȂ������L�[���[�h����
	if ( data_start >= epos ) return;
	rkeyword = buffer->substr( spos, data_end );
	// �R�����g�s���L�[���[�h����
	if ( rkeyword[0] == '#' ) return;
	if ( rkeyword.size() > 1 ) if ( rkeyword[0] == '/' && rkeyword[1] == '/' ) return;
	spos = data_end;

	// "="�̈ʒu���L�[���[�h�̒���ɂ��邩�m�F
	data_start = buffer->find_first_not_of( delim, spos );
	data_end = buffer->find_first_of( "=", spos );
	// �L�[���[�h�̓r���ɋ󔒕����������Ă��遁�p�����[�^����
	if ( data_end > data_start ) return;
	spos = data_end = data_start;

	// �L�[���[�h�̓o�^
	keyword->push_back( rkeyword );
	
	// �p�����[�^�̒��o
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

// �g�[�N���𒊏o
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
		// string::npos �������Ă��܂�Ȃ����߂̕ی�
		*start = *end;
	}

	return buffer->substr( data_start, data_end - data_start );
}

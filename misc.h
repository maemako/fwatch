// �G�p�w�b�_�[

#ifndef __MISC_H__
#define __MISC_H__

#include <vector>

using namespace std;

// ��������string�֕ϊ�
string itos( int n, int width = 0 );

// ��������������ɕϊ�����
void tolowers( string *buffer );

// �����ݒ�t�@�C������L�[���[�h�ƃp�����[�^�𒊏o
void GetIniTokens( vector<string>* keyword, std::string* buffer );

// �g�[�N���𒊏o
std::string GetToken( std::string* buffer, string::size_type* start, string::size_type* end, std::string delim );

#endif __MISC_H__

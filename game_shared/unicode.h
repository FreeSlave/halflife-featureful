#pragma once
#ifndef UNICODE_H
#define UNICODE_H

bool Q_IsValidUChar32( unsigned int uVal );
int Q_UTF8ToUChar32( const char *pUTF8_, unsigned int &uValueOut, bool &bErrorOut );
bool Q_UnicodeValidate( const char *pUTF8 );

#endif

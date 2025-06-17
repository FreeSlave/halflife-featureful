#include "unicode.h"

//-----------------------------------------------------------------------------
// Purpose: determine if a uchar32 represents a valid Unicode code point
//-----------------------------------------------------------------------------
bool Q_IsValidUChar32( unsigned int uVal )
{
	// Values > 0x10FFFF are explicitly invalid; ditto for UTF-16 surrogate halves,
	// values ending in FFFE or FFFF, or values in the 0x00FDD0-0x00FDEF reserved range
	return ( ( uVal - 0x0u ) < 0x110000u ) && ( (uVal - 0x00D800u) > 0x7FFu ) && ( (uVal & 0xFFFFu) < 0xFFFEu ) && ( ( uVal - 0x00FDD0u ) > 0x1Fu );
}

// Decode one character from a UTF-8 encoded string. Treats 6-byte CESU-8 sequences
// as a single character, as if they were a correctly-encoded 4-byte UTF-8 sequence.
int Q_UTF8ToUChar32( const char *pUTF8_, unsigned int &uValueOut, bool &bErrorOut )
{
	const unsigned char *pUTF8 = (const unsigned char*)pUTF8_;

	int nBytes = 1;
	unsigned int uValue = pUTF8[0];
	unsigned int uMinValue = 0;

	// 0....... single byte
	if( uValue < 0x80 )
		goto decodeFinishedNoCheck;

	// Expecting at least a two-byte sequence with 0xC0 <= first <= 0xF7 (110...... and 11110...)
	if( ( uValue - 0xC0u ) > 0x37u || ( pUTF8[1] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = ( uValue << 6 ) - ( 0xC0 << 6 ) + pUTF8[1] - 0x80;
	nBytes = 2;
	uMinValue = 0x80;

	// 110..... two-byte lead byte
	if( !( uValue & ( 0x20 << 6 ) ) )
		goto decodeFinished;

	// Expecting at least a three-byte sequence
	if( ( pUTF8[2] & 0xC0 ) != 0x80 )
		goto decodeError;

	uValue = ( uValue << 6 ) - ( 0x20 << 12 ) + pUTF8[2] - 0x80;
	nBytes = 3;
	uMinValue = 0x800;

// 1110.... three-byte lead byte
decodeFinished:
	if( uValue >= uMinValue && Q_IsValidUChar32( uValue ) )
	{
	decodeFinishedNoCheck:
		uValueOut = uValue;
		bErrorOut = false;
		return nBytes;
	}
decodeError:
	uValueOut = '?';
	bErrorOut = true;
	return nBytes;
#if 0
decodeFinishedMaybeCESU8:
	// Do we have a full UTF-16 surrogate pair that's been UTF-8 encoded afterwards?
	// That is, do we have 0xD800-0xDBFF followed by 0xDC00-0xDFFF? If so, decode it all.
	if( ( uValue - 0xD800u ) < 0x400u && pUTF8[3] == 0xED && (unsigned char)( pUTF8[4] - 0xB0 ) < 0x10 && ( pUTF8[5] & 0xC0 ) == 0x80 )
	{
		uValue = 0x10000 + ( ( uValue - 0xD800u ) << 10 ) + ( (unsigned char)( pUTF8[4] - 0xB0 ) << 6 ) + pUTF8[5] - 0x80;
		nBytes = 6;
		uMinValue = 0x10000;
	}
	goto decodeFinished;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if UTF-8 string contains invalid sequences.
//-----------------------------------------------------------------------------
bool Q_UnicodeValidate( const char *pUTF8 )
{
	bool bError = false;

	while( *pUTF8 )
	{
		unsigned int uVal;
		// Our UTF-8 decoder silently fixes up 6-byte CESU-8 (improperly re-encoded UTF-16) sequences.
		// However, these are technically not valid UTF-8. So if we eat 6 bytes at once, it's an error.
		int nCharSize = Q_UTF8ToUChar32( pUTF8, uVal, bError );
		if( bError || nCharSize == 6 )
			return false;
		pUTF8 += nCharSize;
	}
	return true;
}

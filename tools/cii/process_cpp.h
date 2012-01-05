/*------------------------------------------------------------------
// Copyright (c) 1997 - 2011
// Robert Umbehant
// htmapp@wheresjames.com
// http://www.wheresjames.com
//
// Redistribution and use in source and binary forms, with or
// without modification, are permitted for commercial and
// non-commercial purposes, provided that the following
// conditions are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * The names of the developers or contributors may not be used to
//   endorse or promote products derived from this software without
//   specific prior written permission.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
//   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
//   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
//   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
//   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
//   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//   EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//----------------------------------------------------------------*/

#pragma once

#include <string.h>

/// Returns non-zero if the character is a valid html character
template< typename T >
	static bool IsCppChar( T x_ch )
{
	switch( x_ch )
	{	case tcTC( T, '"' ) :
		case tcTC( T, '\\' ) :
			return false;
	} // end switch

	return ( 0 > x_ch || tcTC( T, ' ' ) <= x_ch ) ? true : false;
}

template< typename T, typename T_STR >
	static T_STR CppEncodeChar( T x_ch )
	{
		switch( x_ch )
		{
			case tcTC( T, '"' ) :
				return tcTT( T, "\\\"" );

			case tcTC( T, '\'' ) :
				return tcTT( T, "\\\\" );

			case tcTC( T, '\t' ) :
				return tcTT( T, "\\t" );

			case tcTC( T, '\r' ) :
				return tcTT( T, "\\r" );

			case tcTC( T, '\n' ) :
				return tcTT( T, "\\n" );

		} // end switch

		// Convert to two byte character
		T s[ 16 ] = { '"', ' ', '"', '\\', 'x', 0, 0, '"', ' ', '"', 0 };
		str::ntoa< char >( &s[ 5 ], (char)x_ch );
		
		return T_STR( s, 11 );
	}

template< typename T, typename T_STR >
	static T_STR CppEncode( const T *x_pStr, typename T_STR::size_type x_lSize = 0 )
	{
		if ( !x_pStr || !*x_pStr || 0 >= x_lSize )
			return T_STR();
		
		T_STR ret;
		typename T_STR::size_type nStart = 0, nPos = 0;

		while ( nPos < x_lSize )
		{
			// Must we encode this one?
			if ( !IsCppChar( x_pStr[ nPos ] ) )
			{
				// Copy data that's ok
				if ( nStart < nPos )
					ret.append( &x_pStr[ nStart ], nPos - nStart );

				// Encode this character
				ret.append( CppEncodeChar< T, T_STR >( x_pStr[ nPos ] ) );

				// Next
				nStart = ++nPos;

			} // end if

			else
				nPos++;

		} // end while

		// Copy remaining data
		if ( nStart < nPos )
			ret.append( &x_pStr[ nStart ], nPos - nStart );

		return ret;
	}

template< typename T, typename T_STR >
	static T_STR CppEncode( const T_STR &x_str )
	{
		return CppEncode< T, T_STR >( x_str.data(), x_str.length() );
	}


/// This function turns html with embedded c++ code inside out
/**

  @param[in] sName		- Namespace / function name
  @param[in] sScript	- File or script data
  @param[in] bFile		- Non-zero if sScript is a file name
  @param[in] sFn		- Function declaration, the name must
						  be _internal_run

  @code
  @endcode

*/
template< typename T, typename T_STR >
	int process_cpp( const T_STR sIn, const T_STR sOut, const T_STR sVar, const T_STR sFn )
	{
		// Attempt to read in the file data, it must fit in memory
		T_STR sSrc = disk::ReadFile< T, T_STR >( sIn ), sDst;
		const T *pSrc = sSrc.data();
		long szSrc = sSrc.length();
		if ( 0 >= szSrc )
			return -1;

		// Function prototype
		const T *pFn = sFn.data();
		if ( !sFn.length() || !pFn || !*pFn )
			pFn = tcTT( T, "static int _internal_run( const TPropertyBag< char > &in, std::basic_string< char > &out )" ); 

		// Start out with some space
		sDst.reserve( sSrc.length() * 2 );

		// Start off the file
		sDst = T_STR() + tcTT( T, "\n#include \"htmapp.h\"\n\n" ) + pFn + tcTT( T, "\n{\n" );

		// Global data
		T_STR sGlobal;

		// Tags
		const T *tsOpen = tcTT( T, "<?c" ), *tsGlobal = tcTT( T, "<?global" ), *tsClose = tcTT( T, "?>" );
		long szOpen = zstr::Length( tsOpen ), szGlobal = zstr::Length( tsGlobal ), szClose = zstr::Length( tsClose );

		// Process the data
		long nPos = 0, nOpen = 0, nClose = 0, nSkip = 0, nStart = 0, nType = 0;
		while ( ( nPos + szOpen + szClose ) < szSrc )
		{
			// Initialize
			nStart = nPos;
			nOpen = nClose = szSrc;

			// Attempt to find an open bracket
			while ( nOpen == szSrc && ( nPos + szOpen + szClose ) < szSrc )
				if ( pSrc[ nPos ] == tsGlobal[ 0 ] && !memcmp( &pSrc[ nPos ], tsGlobal, szGlobal ) )
					nOpen = nPos, nType = 0, nSkip = szGlobal;
				else if ( pSrc[ nPos ] == tsOpen[ 0 ] && !memcmp( &pSrc[ nPos ], tsOpen, szOpen ) )
					nOpen = nPos, nType = 1, nSkip = szOpen;
				else
					nPos++;

			// Find a closing bracket
			while ( nClose == szSrc && ( nPos + szClose ) < szSrc )
				if ( pSrc[ nPos ] == tsClose[ 0 ] && !memcmp( &pSrc[ nPos ], tsClose, szClose ) )
					nClose = nPos;
				else
					nPos++;

			// Did we find embedded code?
			if ( nOpen < szSrc && nClose < szSrc )
			{
				// Encode any text
				if ( nStart < nOpen )
				{	sDst += tcTT( T, "\n\tout += \"" );
					sDst += CppEncode< T, T_STR >( &pSrc[ nStart ], nOpen - nStart );
					sDst += tcTT( T, "\";\n" );
				} // end if

				// Just copy code straight over
				nOpen += nSkip;
				if ( nOpen < nClose )
				switch( nType )
				{
					// Global data
					case 0 :
						sGlobal.append( &pSrc[ nOpen ], nClose - nOpen );
						break;

					// c code
					case 1 :
						sDst.append( &pSrc[ nOpen ], nClose - nOpen ), sDst += tcTT( T, ";\n" );
						break;

				} // end switch

				// Point to data after code
				nPos += szClose;
				nStart = nPos;

			} // end if

		} // end while

		// Copy whatever is left
		if ( nStart < szSrc )
		{	sDst += tcTT( T, "\n\tout += \"" );
			sDst += CppEncode< T, T_STR >( &pSrc[ nStart ], szSrc - nStart );
			sDst += tcTT( T, "\";\n" );
		} // end if

		// Footer
		sDst += tcTT( T, "\n\treturn 0;\n}\n" );

		// Add extern pointer to function
		sDst += T_STR() + tcTT( T, "\n\nvoid * f_" ) + sVar + tcTT( T, " = (void*)&_internal_run;\n" );

		// Write out the file
		return disk::WriteFile< T >( sOut, sGlobal, sDst );
	}

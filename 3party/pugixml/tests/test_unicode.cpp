#ifndef PUGIXML_NO_STL

#include "common.hpp"

#include <string>

// letters taken from http://www.utf8-chartable.de/

TEST(as_wide_empty)
{
	CHECK(as_wide("") == L"");
}

TEST(as_wide_valid_basic)
{
	// valid 1-byte, 2-byte and 3-byte inputs
#ifdef U_LITERALS
	CHECK(as_wide("?\xd0\x80\xe2\x80\xbd") == L"?\u0400\u203D");
#else
	CHECK(as_wide("?\xd0\x80\xe2\x80\xbd") == L"?\x0400\x203D");
#endif
}

TEST(as_wide_valid_astral)
{
	// valid 4-byte input
	std::basic_string<wchar_t> b4 = as_wide("\xf2\x97\x98\xa4 \xf4\x80\x8f\xbf");

	size_t wcharsize = sizeof(wchar_t);

	if (wcharsize == 4)
	{
		CHECK(b4.size() == 3 && b4[0] == wchar_cast(0x97624) && b4[1] == L' ' && b4[2] == wchar_cast(0x1003ff));
	}
	else
	{
		CHECK(b4.size() == 5 && b4[0] == wchar_cast(0xda1d) && b4[1] == wchar_cast(0xde24) && b4[2] == L' ' && b4[3] == wchar_cast(0xdbc0) && b4[4] == wchar_cast(0xdfff));
	}
}

TEST(as_wide_invalid)
{
	// invalid 1-byte input
	CHECK(as_wide("a\xb0") == L"a");
	CHECK(as_wide("a\xb0_") == L"a_");

	// invalid 2-byte input
	CHECK(as_wide("a\xc0") == L"a");
	CHECK(as_wide("a\xd0") == L"a");
	CHECK(as_wide("a\xc0_") == L"a_");
	CHECK(as_wide("a\xd0_") == L"a_");

	// invalid 3-byte input
	CHECK(as_wide("a\xe2\x80") == L"a");
	CHECK(as_wide("a\xe2") == L"a");
	CHECK(as_wide("a\xe2\x80_") == L"a_");
	CHECK(as_wide("a\xe2_") == L"a_");

	// invalid 4-byte input
	CHECK(as_wide("a\xf2\x97\x98") == L"a");
	CHECK(as_wide("a\xf2\x97") == L"a");
	CHECK(as_wide("a\xf2") == L"a");
	CHECK(as_wide("a\xf2\x97\x98_") == L"a_");
	CHECK(as_wide("a\xf2\x97_") == L"a_");
	CHECK(as_wide("a\xf2_") == L"a_");

	// invalid 5-byte input
	std::basic_string<wchar_t> b5 = as_wide("\xf8\nbcd");
	CHECK(b5 == L"\nbcd");
}

TEST(as_wide_string)
{
    std::string s = "abcd";

    CHECK(as_wide(s) == L"abcd");
}

TEST(as_utf8_empty)
{
	CHECK(as_utf8(L"") == "");
}

TEST(as_utf8_valid_basic)
{
	// valid 1-byte, 2-byte and 3-byte outputs
#ifdef U_LITERALS
	CHECK(as_utf8(L"?\u0400\u203D") == "?\xd0\x80\xe2\x80\xbd");
#else
	CHECK(as_utf8(L"?\x0400\x203D") == "?\xd0\x80\xe2\x80\xbd");
#endif
}

TEST(as_utf8_valid_astral)
{
	// valid 4-byte output
	size_t wcharsize = sizeof(wchar_t);

	if (wcharsize == 4)
	{
		std::basic_string<wchar_t> s;
		s.resize(3);
		s[0] = wchar_cast(0x97624);
		s[1] = ' ';
		s[2] = wchar_cast(0x1003ff);

		CHECK(as_utf8(s.c_str()) == "\xf2\x97\x98\xa4 \xf4\x80\x8f\xbf");
	}
	else
	{
	#ifdef U_LITERALS
		CHECK(as_utf8(L"\uda1d\ude24 \udbc0\udfff") == "\xf2\x97\x98\xa4 \xf4\x80\x8f\xbf");
	#else
		CHECK(as_utf8(L"\xda1d\xde24 \xdbc0\xdfff") == "\xf2\x97\x98\xa4 \xf4\x80\x8f\xbf");
	#endif
	}
}

TEST(as_utf8_invalid)
{
	size_t wcharsize = sizeof(wchar_t);

	if (wcharsize == 2)
	{
		// check non-terminated degenerate handling
	#ifdef U_LITERALS
		CHECK(as_utf8(L"a\uda1d") == "a");
		CHECK(as_utf8(L"a\uda1d_") == "a_");
	#else
		CHECK(as_utf8(L"a\xda1d") == "a");
		CHECK(as_utf8(L"a\xda1d_") == "a_");
	#endif

		// check incorrect leading code
	#ifdef U_LITERALS
		CHECK(as_utf8(L"a\ude24") == "a");
		CHECK(as_utf8(L"a\ude24_") == "a_");
	#else
		CHECK(as_utf8(L"a\xde24") == "a");
		CHECK(as_utf8(L"a\xde24_") == "a_");
	#endif
	}
}

TEST(as_utf8_string)
{
    std::basic_string<wchar_t> s = L"abcd";

    CHECK(as_utf8(s) == "abcd");
}
#endif

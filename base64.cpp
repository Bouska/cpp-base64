/*
   base64.cpp and base64.h

   base64 encoding and decoding with C++.
   More information at
     https://renenyffenegger.ch/notes/development/Base64/Encoding-and-decoding-base-64-with-cpp

   Version: 2.rc.04 (release candidate)

   Copyright (C) 2004-2017, 2020 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

*/

#include "base64.h"

#include <algorithm>
#include <stdexcept>

//
// Depending on the url parameter in base64_chars, one of
// two sets of base64 characters needs to be chosen.
// They differ in their last two characters.
//
static const char* to_base64_chars[2] = {
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/",

  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "-_"};

static const unsigned char from_base64_chars[256] = {
  // clang-format off
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 62, 64, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
  64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 63,
  64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  // clang-format on
};

static unsigned int pos_of_char(const unsigned char chr) {
    //
    // Return the position of chr within base64_encode()
    //

    if (from_base64_chars[chr] != 64) return from_base64_chars[chr];

    throw std::runtime_error("Input is not valid base64-encoded data.");
}

static std::string insert_linebreaks(std::string str, size_t distance) {
    //
    // Provided by https://github.com/JomaCorpFX, adapted by me.
    //
    if (str.empty()) return str;

    size_t pos = distance;

    while (pos < str.size()) {
        str.insert(pos, "\n");
        pos += distance + 1;
    }

    return str;
}

template <typename String, unsigned int line_length>
static std::string encode_with_line_breaks(String s) {
    return insert_linebreaks(base64_encode(s, false), line_length);
}

template <typename String>
static std::string encode_pem(String s) {
    return encode_with_line_breaks<String, 64>(s);
}

template <typename String>
static std::string encode_mime(String s) {
    return encode_with_line_breaks<String, 76>(s);
}

template <typename String>
static std::string encode(String s, bool url) {
    return base64_encode(reinterpret_cast<const unsigned char*>(s.data()), s.length(), url);
}

std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len, bool url) {

    const size_t len_encoded = (in_len + 2) / 3 * 4;
    const size_t pad         = in_len % 3;
    const size_t len         = in_len - pad;

    unsigned char trailing_char = url ? '.' : '=';

    //
    // Choose set of base64 characters. They differ
    // for the last two positions, depending on the url
    // parameter.
    // A bool (as is the parameter url) is guaranteed
    // to evaluate to either 0 or 1 in C++ therfore,
    // the correct character set is chosen by subscripting
    // base64_chars with url.
    //
    const char* base64_chars_ = to_base64_chars[url];

    std::string ret;
    ret.reserve(len_encoded);

    unsigned int pos = 0;
    unsigned int chunk;

    while (pos < len) {
        chunk = unsigned(bytes_to_encode[pos + 0]) << 16 | unsigned(bytes_to_encode[pos + 1]) << 8 | unsigned(bytes_to_encode[pos + 2]);
        ret.push_back(base64_chars_[chunk >> 18]);
        ret.push_back(base64_chars_[chunk >> 12 & 0x3f]);
        ret.push_back(base64_chars_[chunk >> 6 & 0x3f]);
        ret.push_back(base64_chars_[chunk & 0x3f]);

        pos += 3;
    }

    switch (pad) {
        case 2:
            chunk = int(bytes_to_encode[pos + 0]) << 8 | int(bytes_to_encode[pos + 1]);
            ret.push_back(base64_chars_[chunk >> 10]);
            ret.push_back(base64_chars_[chunk >> 4 & 0x3f]);
            ret.push_back(base64_chars_[chunk << 2 & 0x3f]);
            ret.push_back(trailing_char);
            break;
        case 1:
            chunk = int(bytes_to_encode[pos + 0]);
            ret.push_back(base64_chars_[chunk >> 2]);
            ret.push_back(base64_chars_[chunk << 4 & 0x3f]);
            ret.push_back(trailing_char);
            ret.push_back(trailing_char);
            break;
        default:
            break;
    }

    return ret;
}

template <typename String>
static std::string decode(String encoded_string, bool remove_linebreaks) {
    //
    // decode(…) is templated so that it can be used with String = const std::string&
    // or std::string_view (requires at least C++17)
    //

    if (encoded_string.empty()) return std::string();

    if (remove_linebreaks) {

        std::string copy(encoded_string);

        copy.erase(std::remove(copy.begin(), copy.end(), '\n'), copy.end());

        return base64_decode(copy, false);
    }

    size_t in_len = encoded_string.length();
    size_t pos    = 0;

    //
    // The approximate length (bytes) of the decoded string might be one ore
    // two bytes smaller, depending on the amount of trailing equal signs
    // in the encoded string. This approximation is needed to reserve
    // enough space in the string to be returned.
    //
    size_t approx_length_of_decoded_string = in_len / 4 * 3;
    const size_t len                       = in_len - 4;
    std::string ret;
    ret.reserve(approx_length_of_decoded_string);

    while (pos < len) {
        const unsigned int chunk = pos_of_char(encoded_string[pos + 0]) << 18 | pos_of_char(encoded_string[pos + 1]) << 12 | pos_of_char(encoded_string[pos + 2]) << 6 | pos_of_char(encoded_string[pos + 3]);
        ret.push_back(static_cast<std::string::value_type>(chunk >> 16 & 0xff));
        ret.push_back(static_cast<std::string::value_type>(chunk >> 8 & 0xff));
        ret.push_back(static_cast<std::string::value_type>(chunk & 0xff));
        pos += 4;
    }

    if (encoded_string[pos + 2] == '=' || encoded_string[pos + 2] == '.') {  // accept URL-safe base 64 strings, too, so check for '.' also.
        const unsigned int chunk = pos_of_char(encoded_string[pos + 0]) << 6 | pos_of_char(encoded_string[pos + 1]);
        ret.push_back(static_cast<std::string::value_type>(chunk >> 4 & 0xff));
    } else if (encoded_string[pos + 3] == '=' || encoded_string[pos + 3] == '.') {
        const unsigned int chunk = pos_of_char(encoded_string[pos + 0]) << 12 | pos_of_char(encoded_string[pos + 1]) << 6 | pos_of_char(encoded_string[pos + 2]);
        ret.push_back(static_cast<std::string::value_type>(chunk >> 10 & 0xff));
        ret.push_back(static_cast<std::string::value_type>(chunk >> 2 & 0xff));
    } else {
        const unsigned int chunk = pos_of_char(encoded_string[pos + 0]) << 18 | pos_of_char(encoded_string[pos + 1]) << 12 | pos_of_char(encoded_string[pos + 2]) << 6 | pos_of_char(encoded_string[pos + 3]);
        ret.push_back(static_cast<std::string::value_type>(chunk >> 16 & 0xff));
        ret.push_back(static_cast<std::string::value_type>(chunk >> 8 & 0xff));
        ret.push_back(static_cast<std::string::value_type>(chunk & 0xff));
    }

    return ret;
}

std::string base64_decode(std::string const& s, bool remove_linebreaks) {
    return decode(s, remove_linebreaks);
}

std::string base64_encode(std::string const& s, bool url) {
    return encode(s, url);
}

std::string base64_encode_pem(std::string const& s) {
    return encode_pem(s);
}

std::string base64_encode_mime(std::string const& s) {
    return encode_mime(s);
}

#if __cplusplus >= 201703L
//
// Interface with std::string_view rather than const std::string&
// Requires C++17
// Provided by Yannic Bonenberger (https://github.com/Yannic)
//

std::string base64_encode(std::string_view s, bool url) {
    return encode(s, url);
}

std::string base64_encode_pem(std::string_view s) {
    return encode_pem(s);
}

std::string base64_encode_mime(std::string_view s) {
    return encode_mime(s);
}

std::string base64_decode(std::string_view s, bool remove_linebreaks) {
    return decode(s, remove_linebreaks);
}

#endif  // __cplusplus >= 201703L

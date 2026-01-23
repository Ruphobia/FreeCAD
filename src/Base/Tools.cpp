// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <unicode/unistr.h>
#include <unicode/uchar.h>
#include <vector>
#include <string>
#include <sstream>
#include <QDateTime>
#include <QTimeZone>

#include "Tools.h"

namespace
{
constexpr auto underscore = static_cast<UChar32>(U'_');

bool isValidFirstChar(UChar32 c)
{
    auto category = static_cast<UCharCategory>(u_charType(c));

    return (c == underscore)
        || (category == U_UPPERCASE_LETTER || category == U_LOWERCASE_LETTER
            || category == U_TITLECASE_LETTER || category == U_MODIFIER_LETTER
            || category == U_OTHER_LETTER || category == U_LETTER_NUMBER);
}

bool isValidSubsequentChar(UChar32 c)
{
    auto category = static_cast<UCharCategory>(u_charType(c));
    return (c == underscore)
        || (category == U_UPPERCASE_LETTER || category == U_LOWERCASE_LETTER
            || category == U_TITLECASE_LETTER || category == U_MODIFIER_LETTER
            || category == U_OTHER_LETTER || category == U_LETTER_NUMBER
            || category == U_DECIMAL_DIGIT_NUMBER || category == U_NON_SPACING_MARK
            || category == U_COMBINING_SPACING_MARK || category == U_CONNECTOR_PUNCTUATION);
}

std::string unicodeCharToStdString(UChar32 c)
{
    icu::UnicodeString uChar(c);
    std::string utf8Char;
    return uChar.toUTF8String(utf8Char);
}

};  // namespace

std::string Base::Tools::getIdentifier(const std::string& name)
{
    if (name.empty()) {
        return "_";
    }

    icu::UnicodeString uName = icu::UnicodeString::fromUTF8(name);
    std::stringstream result;

    // Handle the first character independently, prepending an underscore if it is not a valid
    // first character, but *is* a valid later character
    UChar32 firstChar = uName.char32At(0);
    const int32_t firstCharLength = U16_LENGTH(firstChar);
    if (!isValidFirstChar(firstChar)) {
        result << "_";
        if (isValidSubsequentChar(firstChar)) {
            result << unicodeCharToStdString(firstChar);
        }
    }
    else {
        result << unicodeCharToStdString(firstChar);
    }

    for (int32_t i = firstCharLength; i < uName.length(); /* will increment by char length */) {
        UChar32 c = uName.char32At(i);
        int32_t charLength = U16_LENGTH(c);
        i += charLength;

        if (isValidSubsequentChar(c)) {
            result << unicodeCharToStdString(c);
        }
        else {
            result << "_";
        }
    }

    return result.str();
}

std::wstring Base::Tools::widen(const std::string& str)
{
    std::wostringstream wstm;
    const std::ctype<wchar_t>& ctfacet = std::use_facet<std::ctype<wchar_t>>(wstm.getloc());
    for (char i : str) {
        wstm << ctfacet.widen(i);
    }
    return wstm.str();
}

std::string Base::Tools::narrow(const std::wstring& str)
{
    std::ostringstream stm;
    const std::ctype<char>& ctfacet = std::use_facet<std::ctype<char>>(stm.getloc());
    for (wchar_t i : str) {
        stm << ctfacet.narrow(i, 0);
    }
    return stm.str();
}

std::string Base::Tools::escapedUnicodeFromUtf8(const char* s)
{
    // Convert UTF-8 string to unicode escape sequences (\uXXXX / \UXXXXXXXX)
    icu::UnicodeString ustr = icu::UnicodeString::fromUTF8(s);
    std::ostringstream oss;
    for (int32_t i = 0; i < ustr.length(); ) {
        UChar32 c = ustr.char32At(i);
        i += U16_LENGTH(c);
        if (c < 0x80) {
            oss << static_cast<char>(c);
        } else if (c <= 0xFFFF) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
            oss << buf;
        } else {
            char buf[12];
            snprintf(buf, sizeof(buf), "\\U%08x", static_cast<unsigned>(c));
            oss << buf;
        }
    }
    return oss.str();
}

std::string Base::Tools::escapedUnicodeToUtf8(const std::string& s)
{
    // Parse unicode escape sequences (\uXXXX / \UXXXXXXXX) and convert to UTF-8
    icu::UnicodeString ustr;
    size_t len = s.size();
    for (size_t i = 0; i < len; ) {
        if (s[i] == '\\' && i + 1 < len) {
            if (s[i + 1] == 'u' && i + 5 < len) {
                // \uXXXX
                unsigned int cp = 0;
                bool valid = true;
                for (int j = 0; j < 4; ++j) {
                    char c = s[i + 2 + j];
                    cp <<= 4;
                    if (c >= '0' && c <= '9') cp |= (c - '0');
                    else if (c >= 'a' && c <= 'f') cp |= (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') cp |= (c - 'A' + 10);
                    else { valid = false; break; }
                }
                if (valid) {
                    ustr.append(static_cast<UChar32>(cp));
                    i += 6;
                    continue;
                }
            } else if (s[i + 1] == 'U' && i + 9 < len) {
                // \UXXXXXXXX
                unsigned int cp = 0;
                bool valid = true;
                for (int j = 0; j < 8; ++j) {
                    char c = s[i + 2 + j];
                    cp <<= 4;
                    if (c >= '0' && c <= '9') cp |= (c - '0');
                    else if (c >= 'a' && c <= 'f') cp |= (c - 'a' + 10);
                    else if (c >= 'A' && c <= 'F') cp |= (c - 'A' + 10);
                    else { valid = false; break; }
                }
                if (valid && cp <= 0x10FFFF) {
                    ustr.append(static_cast<UChar32>(cp));
                    i += 10;
                    continue;
                }
            }
        }
        ustr.append(static_cast<UChar32>(static_cast<unsigned char>(s[i])));
        ++i;
    }
    std::string result;
    ustr.toUTF8String(result);
    return result;
}

std::string Base::Tools::escapeQuotesFromString(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

QString Base::Tools::escapeEncodeString(const QString& s)
{
    QString result;
    const int len = s.length();
    result.reserve(int(len * 1.1));
    for (int i = 0; i < len; ++i) {
        if (s.at(i) == QLatin1Char('\\')) {
            result += QLatin1String("\\\\");
        }
        else if (s.at(i) == QLatin1Char('\"')) {
            result += QLatin1String("\\\"");
        }
        else if (s.at(i) == QLatin1Char('\'')) {
            result += QLatin1String("\\\'");
        }
        else {
            result += s.at(i);
        }
    }
    result.squeeze();
    return result;
}

std::string Base::Tools::escapeEncodeString(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\\':
                result += "\\\\";
                break;
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

QString Base::Tools::escapeEncodeFilename(const QString& s)
{
    QString result;
    const int len = s.length();
    result.reserve(int(len * 1.1));
    for (int i = 0; i < len; ++i) {
        if (s.at(i) == QLatin1Char('\"')) {
            result += QLatin1String("\\\"");
        }
        else if (s.at(i) == QLatin1Char('\'')) {
            result += QLatin1String("\\\'");
        }
        else {
            result += s.at(i);
        }
    }
    result.squeeze();
    return result;
}

std::string Base::Tools::escapeEncodeFilename(const std::string& s)
{
    std::string result;
    size_t len = s.size();
    for (size_t i = 0; i < len; ++i) {
        switch (s.at(i)) {
            case '\"':
                result += "\\\"";
                break;
            case '\'':
                result += "\\\'";
                break;
            default:
                result += s.at(i);
                break;
        }
    }
    return result;
}

std::string Base::Tools::quoted(const char* name)
{
    std::stringstream str;
    str << "\"" << name << "\"";
    return str.str();
}

std::string Base::Tools::quoted(const std::string& name)
{
    std::stringstream str;
    str << "\"" << name << "\"";
    return str.str();
}

std::string Base::Tools::joinList(const std::vector<std::string>& vec, const std::string& sep)
{
    std::stringstream str;
    for (const auto& it : vec) {
        str << it << sep;
    }
    return str.str();
}

std::string Base::Tools::currentDateTimeString()
{
    return QDateTime::currentDateTime()
        .toTimeZone(QTimeZone::systemTimeZone())
        .toString(Qt::ISODate)
        .toStdString();
}

std::vector<std::string> Base::Tools::splitSubName(const std::string& subname)
{
    // Turns 'Part.Part001.Body.Pad.Edge1'
    // Into ['Part', 'Part001', 'Body', 'Pad', 'Edge1']
    std::vector<std::string> subNames;
    std::string subName;
    std::istringstream subNameStream(subname);
    while (std::getline(subNameStream, subName, '.')) {
        subNames.push_back(subName);
    }

    // Check if the last character of the input string is the delimiter.
    // If so, add an empty string to the subNames vector.
    // Because the last subname is the element name and can be empty.
    if (!subname.empty() && subname.back() == '.') {
        subNames.push_back("");  // Append empty string for trailing dot.
    }

    return subNames;
}

// ------------------------------------------------------------------------------------------------

void Base::ZipTools::rewrite(const std::string& source, const std::string& target)
{
    // Strip: Python removed - zip rewrite not implemented
    (void)source;
    (void)target;
}

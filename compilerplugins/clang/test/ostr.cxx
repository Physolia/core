/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sal/config.h"

#include <string>
#include <string_view>

#include "rtl/ustring.hxx"

#define M(arg) f(arg, arg)

struct S
{
    OUString s;
};

void f(OUString const&);

void f(OUString const&, OUString const&);

void takeStdString(std::string const&);

void takeStdString(std::u16string const&);

void takeStdView(std::string_view);

void takeStdView(std::u16string_view);

void f()
{
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    OUString s1 = "foo";
    (void)s1;
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    OUString s2 = (("foo"));
    (void)s2;
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    OUString s3("foo");
    (void)s3;
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    OUString s4((("foo")));
    (void)s4;
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f(OUString("foo"));
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f(((OUString((("foo"))))));
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f(OUString("foo", rtl::libreoffice_internal::Dummy()));
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f(((OUString((("foo")), rtl::libreoffice_internal::Dummy()))));
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f("foo");
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    f((("foo")));

    // Only generate one warning here, not two, for a macro argument used twice in the macro's
    // expansion:
    // expected-error-re@+1 {{use a _ustr user-defined string literal instead of constructing an instance of '{{(rtl::)?}}OUString' from an ordinary string literal [loplugin:ostr]}}
    M("foo");

    // expected-note@+1 {{intermediary variable l1 declared here [loplugin:ostr]}}
    constexpr OStringLiteral l1("foo");
    // expected-error@+1 {{directly use a _ostr user-defined string literal instead of introducing the intermediary 'OStringLiteral' variable l1 [loplugin:ostr]}}
    (void)l1;
    // expected-error@+1 {{use 'OString', created from a _ostr user-defined string literal, instead of 'OStringLiteral' for the variable l2 [loplugin:ostr]}}
    constexpr OStringLiteral l2("foo");
    (void)l2;
    (void)l2;
    // expected-note@+1 {{intermediary variable l3 declared here [loplugin:ostr]}}
    OUStringLiteral l3(u"foo");
    // expected-error@+1 {{directly use a _ustr user-defined string literal instead of introducing the intermediary 'OUStringLiteral' variable l3 [loplugin:ostr]}}
    (void)l3;
    // expected-error@+1 {{use 'OUString', created from a _ustr user-defined string literal, instead of 'OUStringLiteral' for the variable l4 [loplugin:ostr]}}
    OUStringLiteral l4(u"foo");
    (void)l4;
    (void)l4;
}

void passLiteral()
{
    // expected-error-re@+1 {{directly use a 'std::string' (aka 'basic_string<char{{(, char_traits<char>, allocator<char>)?}}>') value instead of a _ostr user-defined string literal [loplugin:ostr]}}
    takeStdString(std::string(""_ostr));
    // expected-error-re@+1 {{directly use a 'std::u16string' (aka 'basic_string<char16_t{{(, char_traits<char16_t>, allocator<char16_t>)?}}>') value instead of a _ustr user-defined string literal [loplugin:ostr]}}
    takeStdString(std::u16string(u""_ustr));
    // expected-error@+1 {{directly use a 'std::string_view' (aka 'basic_string_view<char>') value instead of a _ostr user-defined string literal [loplugin:ostr]}}
    takeStdView(""_ostr);
    // expected-error@+1 {{directly use a 'std::u16string_view' (aka 'basic_string_view<char16_t>') value instead of a _ustr user-defined string literal [loplugin:ostr]}}
    takeStdView(u""_ustr);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */

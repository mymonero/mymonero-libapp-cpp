//
//  Currencies.cpp
//  MyMonero
//
//  Copyright (c) 2014-2019, MyMonero.com
//
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without modification, are
//  permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//	of conditions and the following disclaimer in the documentation and/or other
//	materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors may be
//	used to endorse or promote products derived from this software without specific
//	prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
//  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
//  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//

#ifndef Currencies_hpp
#define Currencies_hpp

#include <iostream>
#include <boost/optional/optional.hpp>

namespace Currencies
{
	using namespace std;
	using namespace boost;
	//
	typedef double CcyConversion_Rate;
	typedef string CurrencySymbol;
	typedef string CurrencyUID;
	//
	enum Currency
	{
		none,
		XMR,
		USD,
		AUD,
		BRL,
		CAD,
		CHF,
		CNY,
		EUR,
		GBP,
		HKD,
		INR,
		JPY,
		KRW,
		MXN,
		NOK,
		NZD,
		SEK,
		SGD,
		TRY,
		RUB,
		ZAR
	};
	inline CurrencySymbol CurrencySymbolFrom(Currency ccy)
	{
		switch (ccy) {
			case Currency::none:
				return "";
			case XMR:
				return "XMR";
			case USD:
				return "USD";
			case AUD:
				return "AUD";
			case BRL:
				return "BRL";
			case CAD:
				return "CAD";
			case CHF:
				return "CHF";
			case CNY:
				return "CNY";
			case EUR:
				return "EUR";
			case GBP:
				return "GBP";
			case HKD:
				return "HKD";
			case INR:
				return "INR";
			case JPY:
				return "JPY";
			case KRW:
				return "KRW";
			case MXN:
				return "MXN";
			case NOK:
				return "NOK";
			case NZD:
				return "NZD";
			case SEK:
				return "SEK";
			case SGD:
				return "SGD";
			case TRY:
				return "TRY";
			case RUB:
				return "RUB";
			case ZAR:
				return "ZAR";
			BOOST_THROW_EXCEPTION(logic_error("Unrecognized Currency"));
			return "";
		}
	}
	inline Currency CurrencyFrom(CurrencySymbol symbol)
	{
		if (symbol == "") {
			return Currency::none;
		} else if (symbol == CurrencySymbolFrom(XMR)) {
			return XMR;
		} else if (symbol == CurrencySymbolFrom(USD)) {
			return USD;
		} else if (symbol == CurrencySymbolFrom(AUD)) {
			return AUD;
		} else if (symbol == CurrencySymbolFrom(BRL)) {
			return BRL;
		} else if (symbol == CurrencySymbolFrom(CAD)) {
			return CAD;
		} else if (symbol == CurrencySymbolFrom(CHF)) {
			return CHF;
		} else if (symbol == CurrencySymbolFrom(CNY)) {
			return CNY;
		} else if (symbol == CurrencySymbolFrom(EUR)) {
			return EUR;
		} else if (symbol == CurrencySymbolFrom(GBP)) {
			return GBP;
		} else if (symbol == CurrencySymbolFrom(HKD)) {
			return HKD;
		} else if (symbol == CurrencySymbolFrom(INR)) {
			return INR;
		} else if (symbol == CurrencySymbolFrom(JPY)) {
			return JPY;
		} else if (symbol == CurrencySymbolFrom(KRW)) {
			return KRW;
		} else if (symbol == CurrencySymbolFrom(MXN)) {
			return MXN;
		} else if (symbol == CurrencySymbolFrom(NOK)) {
			return NOK;
		} else if (symbol == CurrencySymbolFrom(NZD)) {
			return NZD;
		} else if (symbol == CurrencySymbolFrom(SEK)) {
			return SEK;
		} else if (symbol == CurrencySymbolFrom(SGD)) {
			return SGD;
		} else if (symbol == CurrencySymbolFrom(TRY)) {
			return TRY;
		} else if (symbol == CurrencySymbolFrom(RUB)) {
			return RUB;
		} else if (symbol == CurrencySymbolFrom(ZAR)) {
			return ZAR;
		}
		BOOST_THROW_EXCEPTION(logic_error("Unrecognized CurrencySymbol"));
		return Currency::none;
	}
}

#endif /* Currencies_hpp */

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
#include <unordered_map>
#include <locale>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include <boost/algorithm/string.hpp>
#include "misc_log_ex.h"
#include "cryptonote_format_utils.h"
//
namespace MoneroConstants
{
	static size_t currency_unitPlaces = 12;
}
//
namespace Currencies
{
	typedef uint64_t TwelveDecimalMoneyAmount; // aka 12-decimal (-atomic-unit) money-amount
	typedef uint64_t TwoDecimalMoneyAmount;
}
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
	inline CurrencyUID CurrencyUIDFrom(Currency ccy)
	{
		return CurrencySymbolFrom(ccy); // same thing since they're unique
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
	inline size_t unitsForDisplay(Currency ccy)
	{
		if (ccy == Currency::XMR) {
			return MoneroConstants::currency_unitPlaces;
		}
		return 2;
	}
}
namespace Currencies
{
	class ConversionRatesController
	{
	public:
		//
		// Lifecycle
		void setup() {}
		//
		// Signals
		boost::signals2::signal<void()> didUpdateAvailabilityOfRates_signal;
		//
		// Accessors
		bool isRateReady(Currency currency) const
		{
			if (currency == Currency::none || currency == Currency::XMR)
			{
				BOOST_THROW_EXCEPTION(logic_error("Invalid 'currency' argument value"));
			}
			auto it = xmrToCurrencyRatesByCurrencyUID.find(CurrencyUIDFrom(currency));
			//
			return it != xmrToCurrencyRatesByCurrencyUID.end();
		}
		optional<CcyConversion_Rate> rateFromXMR_orNoneIfNotReady(Currency toCurrency) const
		{
			if (toCurrency == Currency::none || toCurrency == Currency::XMR) {
				BOOST_THROW_EXCEPTION(logic_error("Invalid 'currency' argument value"));
			}
			auto it = xmrToCurrencyRatesByCurrencyUID.find(CurrencyUIDFrom(toCurrency));
			if (it == xmrToCurrencyRatesByCurrencyUID.end()) {
				return none;
			}
			return it->second;
		}
		//
		// Imperatives
		bool set(
			CcyConversion_Rate XMRToCurrencyRate,
			Currency forCurrency,
			bool isPartOfBatch = false
		) {
			bool doNotNotify = isPartOfBatch;
			string ccyUID = CurrencyUIDFrom(forCurrency);
			bool wasSetValueDifferent = XMRToCurrencyRate != xmrToCurrencyRatesByCurrencyUID[ccyUID];
			xmrToCurrencyRatesByCurrencyUID[ccyUID] = XMRToCurrencyRate;
			if (doNotNotify != true) {
				_notifyOf_updateTo_XMRToCurrencyRate();
			}
			return wasSetValueDifferent;
		}
		void ifBatched_notifyOf_set_XMRToCurrencyRate()
		{
			MDEBUG("CcyConversionRates: Received updates: $xmrToCurrencyRatesByCurrencyUID");
			_notifyOf_updateTo_XMRToCurrencyRate();
		}
		void set_xmrToCcyRatesByCcy(std::unordered_map<Currency, double> xmrToCcyRatesByCcy)
		{
			bool wasAnyRateChanged = false;
			std::unordered_map<Currency, double>::iterator it = xmrToCcyRatesByCcy.begin();
			while (it != xmrToCcyRatesByCcy.end()) {
				Currency currency = it->first;
				double rate = it->second;
				bool wasSetValueDifferent = set(
					rate,
					currency,
					true // isPartOfBatch
				);
				if (wasSetValueDifferent) {
					wasAnyRateChanged = true;
				}
				it++;
			}
			if (wasAnyRateChanged) {
				ifBatched_notifyOf_set_XMRToCurrencyRate();
			}
		}
	private:
		//
		// Properties
		std::unordered_map<CurrencyUID, CcyConversion_Rate> xmrToCurrencyRatesByCurrencyUID;
		//
		// Imperatives
		void _notifyOf_updateTo_XMRToCurrencyRate()
		{
			didUpdateAvailabilityOfRates_signal();
		}
	};
}
namespace Currencies
{
	inline double doubleFrom(TwelveDecimalMoneyAmount moneroAmount)
	{
		return stod(cryptonote::print_money(moneroAmount));
	}
	//
	inline char money_decimal_punctuation_char()
	{
		std::locale mylocale;
		const std::moneypunct<char>& mp = std::use_facet<std::moneypunct<char> >(mylocale);
		//
		return mp.decimal_point();
	}
	class twodecimal_comma_moneypunct : public std::moneypunct<char>
	{
	public:
		static void imbue(std::ostream &os)
		{
			os.imbue(std::locale(os.getloc(), new twodecimal_comma_moneypunct));
		}
	protected:
		virtual char_type do_decimal_point() const
		{
			return money_decimal_punctuation_char();
		}
		std::string do_grouping() const
		{
			return "\0"; // the zero by itself means do no grouping
		}
	};
	inline string localizedTwoDecimalDoubleFormattedString(double val)
	{
		stringstream ss;
		twodecimal_comma_moneypunct::imbue(ss);
		ss << val;
		//
		return ss.str();
	}
	//
	inline vector<string> split(char *phrase, string delimiter)
	{
		vector<string> list;
		string s = string(phrase);
		size_t pos = 0;
		string token;
		while ((pos = s.find(delimiter)) != string::npos) {
			token = s.substr(0, pos);
			list.push_back(token);
			s.erase(0, pos + delimiter.length());
		}
		list.push_back(s);
		return list;
	}
	//
	inline string nonAtomicCurrency_localized_formattedString( // is nonAtomic-unit'd currency a good enough way to categorize these?
		double final_amountDouble,
		Currencies::Currency inCcy,
		optional<string> decimalSeparator_orNoneForLocaleDefault
	) {
		if (inCcy == Currencies::XMR) {
			BOOST_THROW_EXCEPTION(logic_error("nonAtomicCurrency_localized_formattedString should not be called with ccy of XMR"));
			return "";
		}
		string decimalSeparator;
		if (decimalSeparator_orNoneForLocaleDefault != boost::none) {
			decimalSeparator = decimalSeparator_orNoneForLocaleDefault.get();
		} else {
			decimalSeparator = money_decimal_punctuation_char();
		}
		if (final_amountDouble == 0) {
			return "0"; // not 0.0 / 0,0 / ...
		}
		string naiveLocalizedString = localizedTwoDecimalDoubleFormattedString(final_amountDouble);
		vector<string> components;
		boost::split(components, naiveLocalizedString, boost::is_any_of(decimalSeparator));
		size_t components_count = components.size();
		if (components_count == 0) {
			BOOST_THROW_EXCEPTION(logic_error("Unexpected 0 components while formatting nonatomic currency"));
			return "";
		}
		if (components_count == 1) { // meaning there's no '.'
			if (boost::algorithm::contains(naiveLocalizedString, decimalSeparator)) {
				BOOST_THROW_EXCEPTION(logic_error("Expected naiveLocalizedString not to contain decimalSeparator"));
				return "";
			}
			stringstream oss;
			oss << naiveLocalizedString << decimalSeparator << "00";
			//
			return oss.str();
		}
		if (components_count != 2) {
			BOOST_THROW_EXCEPTION(logic_error("Expected naiveLocalizedString components_count to be 2"));
			return "";
		}
		string component_1 = components[0];
		string component_2 = components[1];
		size_t component_2_characters_count = component_2.size();
		if (component_2_characters_count > unitsForDisplay(inCcy)) {
			BOOST_THROW_EXCEPTION(logic_error("Expected component_2_characters_count <= unitsForDisplay(inCcy)"));
			return "";
		}
		size_t requiredNumberOfZeroes = unitsForDisplay(inCcy) - component_2_characters_count;
		stringstream rightSidePaddingZeroes;
		if (requiredNumberOfZeroes > 0) {
			for (int i = 0 ; i < requiredNumberOfZeroes ; i++) {
				rightSidePaddingZeroes << "0"; // TODO: less verbose way to do this?
			}
		}
		stringstream return_ss;
		return_ss << component_1 << decimalSeparator << component_2 << rightSidePaddingZeroes.str(); // pad
		//
		return return_ss.str();
	}
	//
	//
	inline optional<double> displayUnitsRounded_amountInCurrency( // NOTE: __DISPLAY_ units
		TwelveDecimalMoneyAmount moneroAmount,
		Currency inCcy,
		const ConversionRatesController &controller
	) {
		if (inCcy == Currency::none) {
			BOOST_THROW_EXCEPTION(logic_error("Selected currency unexpectedly ::none"));
			return none;
		}
		double moneroAmountDouble = doubleFrom(moneroAmount);
		if (inCcy == Currencies::XMR) {
			return moneroAmountDouble; // no conversion necessary
		}
		optional<double> xmrToCurrencyRate = controller.rateFromXMR_orNoneIfNotReady(inCcy);
		if (xmrToCurrencyRate == boost::none) {
			return none; // ccyConversion rate unavailable - consumers will try again
		}
		double raw_ccyConversionRateApplied_amount = moneroAmountDouble * xmrToCurrencyRate.get();
		double roundingMultiplier = pow((double)10, (double)unitsForDisplay(inCcy));
		double truncated_amount = round(roundingMultiplier * raw_ccyConversionRateApplied_amount) / roundingMultiplier;
		//
		return truncated_amount;
	}
	//
	static int ccyConversionRateCalculated_moneroAmountDouble_roundingPlaces = 4; // 4 rather than, say, 2, b/c it's relatively more unlikely that fiat amts will be over 10-100 xmr - and b/c some currencies require it for xmr value not to be 0 - and 5 places is a bit excessive
	inline optional<double> rounded_ccyConversionRateCalculated_moneroAmountDouble(
		double userInputAmountDouble,
		Currency selectedCurrency,
		const ConversionRatesController &controller
	) { // may return nil if ccyConversion rate unavailable - consumers will try again on 'didUpdateAvailabilityOfRates'
		if (selectedCurrency == Currency::none) {
			BOOST_THROW_EXCEPTION(logic_error("Selected currency unexpectedly none"));
			return none;
		}
		optional<CcyConversion_Rate> xmrToCurrencyRate = controller.rateFromXMR_orNoneIfNotReady(selectedCurrency);
		if (xmrToCurrencyRate == boost::none) {
			return none; // ccyConversion rate unavailable - consumers will try again on 'didUpdateAvailabilityOfRates'
		}
		// conversion:
		// currencyAmt = xmrAmt * xmrToCurrencyRate;
		// xmrAmt = currencyAmt / xmrToCurrencyRate.
		// I figure it's better to apply the rounding here rather than only at the display level so that what is actually sent corresponds to what the user saw, even if greater ccyConversion precision /could/ be accomplished..
		double raw_ccyConversionRateApplied_amount = userInputAmountDouble * (1 / (*xmrToCurrencyRate));
		double roundingMultiplier = (double)pow(10, ccyConversionRateCalculated_moneroAmountDouble_roundingPlaces);
		double truncated_amount = (double)round(roundingMultiplier * raw_ccyConversionRateApplied_amount) / roundingMultiplier; // must be truncated for display purposes
		//
		return truncated_amount;
	}
//	inline func amountConverted_displayStringComponents(
//														from amount: MoneroAmount,
//														ccy: CcyConversionRates.Currency,
//														chopNPlaces: UInt = 0
//														) -> (
//															  formattedAmount: String,
//															  final_ccy: CcyConversionRates.Currency
//															  ) {
//		var formattedAmount: String
//		var final_input_amount: MoneroAmount!
//		if chopNPlaces != 0 {
//			let power = MoneroAmount("10")!.power(Int(chopNPlaces)) // this *should* be ok for amount, even if it has no decimal places, because those places would be filled with 0s in such a number
//			final_input_amount = (amount / power) * power
//		} else {
//			final_input_amount = amount
//		}
//			var mutable_ccy = ccy
//			if ccy == .XMR {
//				formattedAmount = final_input_amount!.localized_formattedString
//			} else {
//				let convertedAmount = ccy.displayUnitsRounded_amountInCurrency(fromMoneroAmount: final_input_amount!)
//				if convertedAmount != nil {
//					formattedAmount = MoneroAmount.shared_localized_twoDecimalPlaceDoubleFormatter().string(for: convertedAmount)!
//				} else {
//					formattedAmount = final_input_amount!.localized_formattedString
//					mutable_ccy = .XMR // display XMR until rate is ready? or maybe just show 'LOADING…'?
//				}
//			}
//			return (formattedAmount, mutable_ccy)
//			}
}

#endif /* Currencies_hpp */

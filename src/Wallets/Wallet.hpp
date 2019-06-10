//
//  Wallet.hpp
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

#ifndef Wallet_hpp
#define Wallet_hpp

#include <string>
#include <unordered_map>
#include "../Persistence/PersistableObject.hpp"
#include "monero_wallet_utils.hpp"
#include "../APIClient/HostedMonero.hpp"
#include "./Wallet_HostPollingController.hpp"
#include "./Wallet_TxCleanupController.hpp"
#include "../APIClient/parsing.hpp"
#include "../UserIdle/UserIdle.hpp"
#include "../Currencies/Currencies.hpp"
//
namespace Wallets
{
	using namespace std;
	//
	enum Currency
	{
		Monero = 0
	};
	//
	static string  _ccy_symbol_string__xmr = "xmr";
	static inline const string &symbol_string_from(Wallets::Currency currency)
	{
		switch (currency) {
			case Monero:
				return _ccy_symbol_string__xmr;
			default:
				BOOST_THROW_EXCEPTION(logic_error("Unhandled Wallets::Currency"));
				return symbol_string_from(Monero); // default … not rea
		}
	}
	static inline const string &jsonRepresentation(Wallets::Currency currency)
	{
		return symbol_string_from(currency);
	}
	static inline const Currency currencyFromJSONRepresentation(const string &str)
	{
		if (str == jsonRepresentation(Monero)) {
			return Monero;
		}
		BOOST_THROW_EXCEPTION(logic_error("currencyFromJSONRepresentation: Unhandled jsonRepresentation"));
		return Monero; // default … not rea
	}
	static inline string humanReadableCurrencySymbolString(Wallets::Currency currency)
	{
		switch (currency) {
			case Monero:
				return "XMR";
			default:
				BOOST_THROW_EXCEPTION(logic_error("Unhandled Wallets::Currency"));
				return humanReadableCurrencySymbolString(Monero); // default … not rea
		}
	}
}
namespace Wallets
{
	using namespace std;
	//
	enum SwatchColor
	{
		darkGrey = 1,
		lightGrey = 2,
		teal = 3,
		purple = 4,
		salmon = 5,
		orange = 6,
		yellow = 7,
		blue = 8
	};
	static inline string hex_color_string_from(SwatchColor swatchColor)
	{
		switch (swatchColor) {
			case darkGrey:
				return "#6B696B";
			case lightGrey:
				return "#CFCECF";
			case teal:
				return "#00F4CD";
			case purple:
				return "#D975E1";
			case salmon:
				return "#F97777";
			case orange:
				return "#EB8316";
			case yellow:
				return "#EACF12";
			case blue:
				return "#00C6FF";
		}
		BOOST_THROW_EXCEPTION(logic_error("Unhandled swatch color"));
		return hex_color_string_from(blue); // default
	}
	static inline string color_name_string_from(SwatchColor swatchColor)
	{
		switch (swatchColor) {
			case darkGrey:
				return "darkGrey";
			case lightGrey:
				return "lightGrey";
			case teal:
				return "teal";
			case purple:
				return "purple";
			case salmon:
				return "salmon";
			case orange:
				return "orange";
			case yellow:
				return "yellow";
			case blue:
				return "blue";
		}
		BOOST_THROW_EXCEPTION(logic_error("Unhandled swatch color"));
		return color_name_string_from(blue); // default
	}
	static inline string jsonRepresentation(SwatchColor swatchColor)
	{
		return hex_color_string_from(swatchColor);
	}
	static inline SwatchColor swatchColorFromJSONRepresentation(const string &str)
	{
		if (str == jsonRepresentation(darkGrey)) {
			return darkGrey;
		} else if (str == jsonRepresentation(lightGrey)) {
			return lightGrey;
		} else if (str == jsonRepresentation(teal)) {
			return teal;
		} else if (str == jsonRepresentation(purple)) {
			return purple;
		} else if (str == jsonRepresentation(salmon)) {
			return salmon;
		} else if (str == jsonRepresentation(orange)) {
			return orange;
		} else if (str == jsonRepresentation(yellow)) {
			return yellow;
		} else if (str == jsonRepresentation(blue)) {
			return blue;
		}
		BOOST_THROW_EXCEPTION(logic_error("swatchColorFromJSONRepresentation: Unhandled jsonRepresentation"));
		return blue; // default
	}
	static inline bool isADarkColor(SwatchColor swatchColor)
	{
		switch (swatchColor) {
			case darkGrey:
				return true;
			default:
				return false;
		}
	}
}
//
namespace Wallets
{
	using namespace std;
	using namespace HostedMonero;
	//
	// Constants
	static const CollectionName collectionName = "Wallets";
	// Constants - Persistence - Keys
	static const string __dictKey_login__new_address = "login__new_address";
	static const string __dictKey_login__generated_locally = "login__generated_locally";
	static const string __dictKey_local_wasAGeneratedWallet = "local_wasAGeneratedWallet";
	//
	static const string __dictKey_currency = "currency";
	static const string __dictKey_walletLabel = "walletLabel";
	static const string __dictKey_swatchColorHexString = "swatchColorHexString";
	static const string __dictKey_mnemonic_wordsetName = "mnemonic_wordsetName";
	static const string __dictKey_publicAddress = "publicAddress";
	static const string __dictKey_privateKeys = "privateKeys";
	static const string __dictKey_publicKeys = "publicKeys";
	static const string __dictKey__keyDuo__view = "view";
	static const string __dictKey__keyDuo__spend = "spend";
	static const string __dictKey_accountSeed = "accountSeed";
	// we do not save the mnemonic-encoded seed to disk, only accountSeed
	//
	static const string __dictKey_totalReceived = "totalReceived";
	static const string __dictKey_totalSent = "totalSent";
	static const string __dictKey_lockedBalance = "lockedBalance";
	//
	static const string __dictKey_account_scanned_tx_height = "account_scanned_tx_height";
	static const string __dictKey_account_scanned_height = "account_scanned_height";
	static const string __dictKey_account_scanned_block_height = "account_scanned_block_height";
	static const string __dictKey_account_scan_start_height = "account_scan_start_height";
	static const string __dictKey_transaction_height = "transaction_height";
	static const string __dictKey_blockchain_height = "blockchain_height";
	//
	static const string __dictKey_spentOutputs = "spentOutputs";
	//
	static const string __dictKey_transactions = "transactions";
	//
	static const string __dictKey_dateThatLast_fetchedAccountInfo = "dateThatLast_fetchedAccountInfo";
	static const string __dictKey_dateThatLast_fetchedAccountTransactions = "dateThatLast_fetchedAccountTransactions";
	//
	static const string __dictKey_isLoggedIn = "isLoggedIn";
	static const string __dictKey_isInViewOnlyMode = "isInViewOnlyMode";
	//
	//
	class Object: public Persistable::Object, public std::enable_shared_from_this<Object>
	{
	public:
		Object(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<const Passwords::PasswordProvider> passwordProvider,
			optional<monero_wallet_utils::WalletDescription> ifGeneratingNewWallet_walletDescription,
			const cryptonote::network_type nettype,
			std::shared_ptr<HostedMonero::APIClient> apiClient,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr,
			std::shared_ptr<UserIdle::Controller> userIdleController,
			std::shared_ptr<Currencies::ConversionRatesController> ccyConversionRatesController
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider)
		),
		_nettype(nettype),
		_apiClient(apiClient),
		_dispatch_ptr(dispatch_ptr),
		_userIdleController(userIdleController),
		_ccyConversionRatesController(ccyConversionRatesController),
		_generatedOnInit_walletDescription(ifGeneratingNewWallet_walletDescription)
		{
			_currency = Wallets::Currency::Monero;
			if (_generatedOnInit_walletDescription != none) {
				_mnemonic_wordsetName = (*_generatedOnInit_walletDescription).mnemonic_language;
			}
		}
		Object(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
			const document_persister::DocumentJSON &plaintextData,
			const cryptonote::network_type nettype,
			std::shared_ptr<HostedMonero::APIClient> apiClient,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr,
			std::shared_ptr<UserIdle::Controller> userIdleController,
			std::shared_ptr<Currencies::ConversionRatesController> ccyConversionRatesController
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider),
			plaintextData
		),
		_nettype(nettype),
		_apiClient(apiClient),
		_dispatch_ptr(dispatch_ptr),
		_userIdleController(userIdleController),
		_ccyConversionRatesController(ccyConversionRatesController)
		{
			if (this->_id == boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->_id == boost::none"));
			}
			if (this->insertedAt_sSinceEpoch == boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->insertedAt_sSinceEpoch == boost::none"));
			}
			//
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_local_wasAGeneratedWallet);
				if (itr != plaintextData.MemberEnd()) {
					_local_wasAGeneratedWallet = itr->value.GetBool();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_login__generated_locally);
				if (itr != plaintextData.MemberEnd()) {
					_login__generated_locally = itr->value.GetBool();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_login__new_address);
				if (itr != plaintextData.MemberEnd()) {
					_login__new_address = itr->value.GetBool();
				}
			}
			//
			_isLoggedIn = plaintextData[__dictKey_isLoggedIn].GetBool();
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_isInViewOnlyMode);
				if (itr != plaintextData.MemberEnd()) {
					_isInViewOnlyMode = itr->value.GetBool();
				} else {
					_isInViewOnlyMode = boost::none;
				}
			}
			
			
			
			
			
			//
			//
			//
			//
			//
			// TODO: read as actual double, read as JS date string, and read as long (time_t) in a string
			cout << "TODO: read dates as actual double, read as JS date string, and read as long (time_t) in a string" << endl;
			//
			//
			//
			//
			//
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_dateThatLast_fetchedAccountInfo);
				if (itr != plaintextData.MemberEnd()) {
					_dateThatLast_fetchedAccountInfo = strtol(itr->value.GetString(), NULL, 0); // must parse from string to 'long'
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_dateThatLast_fetchedAccountTransactions);
				if (itr != plaintextData.MemberEnd()) {
					_dateThatLast_fetchedAccountTransactions = strtol(itr->value.GetString(), NULL, 0); // must parse from string to 'long'
				}
			}
//			//
			_currency = currencyFromJSONRepresentation(string(
				plaintextData[__dictKey_currency].GetString(), plaintextData[__dictKey_currency].GetStringLength()
			));
			_walletLabel = string(plaintextData[__dictKey_walletLabel].GetString(), plaintextData[__dictKey_walletLabel].GetStringLength());
			_swatchColor = swatchColorFromJSONRepresentation(string(
				plaintextData[__dictKey_swatchColorHexString].GetString(), plaintextData[__dictKey_swatchColorHexString].GetStringLength()
			));
			//
			// Not going to check whether the acct seed is nil/'' here because if the wallet was
			// imported with public addr, view key, and spend key only rather than seed/mnemonic, we
			// cannot obtain the seed.
			//
			_public_address = string(plaintextData[__dictKey_publicAddress].GetString(), plaintextData[__dictKey_publicAddress].GetStringLength());
			auto public_keys = plaintextData[__dictKey_publicKeys].GetObject();
			_view_pub_key = string(public_keys[__dictKey__keyDuo__view].GetString(), public_keys[__dictKey__keyDuo__view].GetStringLength());
			_spend_pub_key = string(public_keys[__dictKey__keyDuo__spend].GetString(), public_keys[__dictKey__keyDuo__spend].GetStringLength());
			//
			auto private_keys = plaintextData[__dictKey_privateKeys].GetObject();
			_view_sec_key = string(private_keys[__dictKey__keyDuo__view].GetString(), private_keys[__dictKey__keyDuo__view].GetStringLength());
			_spend_sec_key = string(private_keys[__dictKey__keyDuo__spend].GetString(), private_keys[__dictKey__keyDuo__spend].GetStringLength());
			//
			_walletLabel = string(plaintextData[__dictKey_walletLabel].GetString(), plaintextData[__dictKey_walletLabel].GetStringLength());
			_account_seed = string(plaintextData[__dictKey_accountSeed].GetString(), plaintextData[__dictKey_accountSeed].GetStringLength());
			_mnemonic_wordsetName = string(plaintextData[__dictKey_mnemonic_wordsetName].GetString(), plaintextData[__dictKey_mnemonic_wordsetName].GetStringLength());
			//
			
			//
			//
			//
			//
			//
			// TODO: read totals alternatively from 'totals' object within doc, or else as root members" << endl;
			cout << "TODO: read totals alternatively from 'totals' object within doc, or else as root members" << endl;
			//
			//
			//
			//
			//
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_totalReceived);
				if (itr != plaintextData.MemberEnd()) {
					_totalReceived = strtoull(itr->value.GetString(), NULL, 0); // must parse from string to u long long
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_totalSent);
				if (itr != plaintextData.MemberEnd()) {
					_totalSent = strtoull(itr->value.GetString(), NULL, 0); // must parse from string to u long long
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_lockedBalance);
				if (itr != plaintextData.MemberEnd()) {
					_lockedBalance = strtoull(itr->value.GetString(), NULL, 0); // must parse from string to u long long
				}
			}
			
			
			//
			//
			//
			//
			// TODO: read heights alternatively from 'heights' object within doc, or else as root members" << endl;
			cout << "TODO: read heights alternatively from 'heights' object within doc, or else as root members" << endl;
			//
			//
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_account_scanned_tx_height);
				if (itr != plaintextData.MemberEnd()) {
					_account_scanned_tx_height = itr->value.GetUint64();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_account_scanned_height);
				if (itr != plaintextData.MemberEnd()) {
					_account_scanned_height = itr->value.GetUint64();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_account_scanned_block_height);
				if (itr != plaintextData.MemberEnd()) {
					_account_scanned_block_height = itr->value.GetUint64();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_account_scan_start_height);
				if (itr != plaintextData.MemberEnd()) {
					_account_scan_start_height = itr->value.GetUint64();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_transaction_height);
				if (itr != plaintextData.MemberEnd()) {
					_transaction_height = itr->value.GetUint64();
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_blockchain_height);
				if (itr != plaintextData.MemberEnd()) {
					_blockchain_height = itr->value.GetUint64();
				}
			}
			//
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_spentOutputs);
				if (itr != plaintextData.MemberEnd()) {
					_spentOutputs = SpentOutputDescription::newArray_fromJSONRepresentations(itr->value);
//					if (_spentOutputs != none) {
//						BOOST_FOREACH(const SpentOutputDescription &obj, *_spentOutputs)
//						{
//							cout << "_spentOutput recovered from disk: " << obj << endl;
//						}
//					}
				}
			}
			{
				Value::ConstMemberIterator itr = plaintextData.FindMember(__dictKey_transactions);
				if (itr != plaintextData.MemberEnd()) {
					_transactions = HistoricalTxRecord::newArray_fromJSONRepresentations(itr->value, *_blockchain_height);
//					if (_transactions != none) {
//						BOOST_FOREACH(const HistoricalTxRecord &obj, *_transactions)
//						{
//							cout << "_transaction recovered from disk: " << obj << endl;
//						}
//					}
				}
			}
			//
			// Regenerate any runtime vals that depend on persisted vals..
			regenerate_shouldDisplayImportAccountOption();
		}
		~Object()
		{
			cout << "A Wallets::Object concrete destructor" << endl;
			teardown();
		}
		//
		// Lifecycle
		void teardown() override
		{
			cout << "Wallet: Tearing down " << this << endl;
			Persistable::Object::teardown();
			tearDownRuntime();
		}
		//
		// Accessors
		virtual document_persister::DocumentJSON new_dictRepresentation() const override
		{
			auto dict = Persistable::Object::new_dictRepresentation();
			//
			// TODO: use mutex lock
			//
			if (_login__new_address != none) {
				Value k(StringRef(__dictKey_login__new_address));
				dict.AddMember(k, Value(*(_login__new_address)).Move(), dict.GetAllocator());
			}
			if (_login__generated_locally != none) {
				Value k(StringRef(__dictKey_login__generated_locally));
				dict.AddMember(k, Value(*(_login__generated_locally)).Move(), dict.GetAllocator());
			}
			if (_local_wasAGeneratedWallet != none) {
				Value k(StringRef(__dictKey_local_wasAGeneratedWallet));
				dict.AddMember(k, Value(*(_local_wasAGeneratedWallet)).Move(), dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_currency));
				Value v(jsonRepresentation(_currency), dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_walletLabel));
				Value v(_walletLabel, dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_swatchColorHexString));
				Value v(jsonRepresentation(_swatchColor), dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_publicAddress));
				Value v(_public_address, dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			}
			if (_account_seed != none && _account_seed->empty() == false) {
				Value k(StringRef(__dictKey_accountSeed));
				Value v(*_account_seed, dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			} else {
				MWARNING("Wallets: Saving w/o acct seed");
			}
			if (_mnemonic_wordsetName != none) {
				Value k(StringRef(__dictKey_mnemonic_wordsetName));
				Value v(*_mnemonic_wordsetName, dict.GetAllocator()); // copy
				dict.AddMember(k, v, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_publicKeys));
				Value o(kObjectType);
				{
					{
						Value k_inner(StringRef(__dictKey__keyDuo__view));
						Value v(_view_pub_key, dict.GetAllocator()); // copy
						o.AddMember(k_inner, v, dict.GetAllocator());
					}
					{
						Value k_inner(StringRef(__dictKey__keyDuo__spend));
						Value v(_spend_pub_key, dict.GetAllocator()); // copy
						o.AddMember(k_inner, v, dict.GetAllocator());
					}
				}
				dict.AddMember(k, o, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_privateKeys));
				Value o(kObjectType);
				{
					{
						Value k_inner(StringRef(__dictKey__keyDuo__view));
						Value v(_view_sec_key, dict.GetAllocator()); // copy
						o.AddMember(k_inner, v, dict.GetAllocator());
					}
					{
						Value k_inner(StringRef(__dictKey__keyDuo__spend));
						Value v(_spend_sec_key, dict.GetAllocator()); // copy
						o.AddMember(k_inner, v, dict.GetAllocator());
					}
				}
				dict.AddMember(k, o, dict.GetAllocator());
			}
			{
				Value k(StringRef(__dictKey_isLoggedIn));
				dict.AddMember(k, Value(_isLoggedIn).Move(), dict.GetAllocator());
			}
			if (_isInViewOnlyMode != none) {
				Value k(StringRef(__dictKey_isInViewOnlyMode));
				dict.AddMember(k, Value(*(_isInViewOnlyMode)).Move(), dict.GetAllocator());
			}
			//
			if (_dateThatLast_fetchedAccountInfo != none) {
				Value k(StringRef(__dictKey_dateThatLast_fetchedAccountInfo));
				ostringstream oss;
				oss << *_dateThatLast_fetchedAccountInfo; // assume storing as a 'long' in a string
				Value v(oss.str(), dict.GetAllocator());
				dict.AddMember(k, v.Move(), dict.GetAllocator());
			}
			if (_dateThatLast_fetchedAccountTransactions != none) {
				Value k(StringRef(__dictKey_dateThatLast_fetchedAccountTransactions));
				ostringstream oss;
				oss << *_dateThatLast_fetchedAccountTransactions; // assume storing as a 'long' in a string
				Value v(oss.str(), dict.GetAllocator());
				dict.AddMember(k, v.Move(), dict.GetAllocator());
			}
			//
			if (_totalReceived != none) {
				Value k(StringRef(__dictKey_totalReceived));
				ostringstream oss;
				oss << *_totalReceived; // assume storing as a 'unsigned long long' in a string
				Value v(oss.str(), dict.GetAllocator());
				dict.AddMember(k, v.Move(), dict.GetAllocator());
			}
			if (_totalSent != none) {
				Value k(StringRef(__dictKey_totalSent));
				ostringstream oss;
				oss << *_totalSent; // assume storing as a 'unsigned long long' in a string
				Value v(oss.str(), dict.GetAllocator());
				dict.AddMember(k, v.Move(), dict.GetAllocator());
			}
			if (_lockedBalance != none) {
				Value k(StringRef(__dictKey_lockedBalance));
				ostringstream oss;
				oss << *_lockedBalance; // assume storing as a 'unsigned long long' in a string
				Value v(oss.str(), dict.GetAllocator());
				dict.AddMember(k, v.Move(), dict.GetAllocator());
			}
			//
			if (_spentOutputs != none) {
				Value k(StringRef(__dictKey_spentOutputs));
				dict.AddMember(k, new_arrayOfSerializedDicts(*_spentOutputs, dict.GetAllocator()).Move(), dict.GetAllocator());
			}
			//
			if (_account_scanned_tx_height != none) {
				Value v;
				v.SetUint64(*_account_scanned_tx_height);
				dict.AddMember(StringRef(__dictKey_account_scanned_tx_height), v, dict.GetAllocator());
			}
			if (_account_scanned_height != none) {
				Value v;
				v.SetUint64(*_account_scanned_height);
				dict.AddMember(StringRef(__dictKey_account_scanned_height), v, dict.GetAllocator());
			}
			if (_account_scanned_block_height != none) {
				Value v;
				v.SetUint64(*_account_scanned_block_height);
				dict.AddMember(StringRef(__dictKey_account_scanned_block_height), v, dict.GetAllocator());
			}
			if (_account_scan_start_height != none) {
				Value v;
				v.SetUint64(*_account_scan_start_height);
				dict.AddMember(StringRef(__dictKey_account_scan_start_height), v, dict.GetAllocator());
			}
			if (_transaction_height != none) {
				Value v;
				v.SetUint64(*_transaction_height);
				dict.AddMember(StringRef(__dictKey_transaction_height), v, dict.GetAllocator());
			}
			if (_blockchain_height != none) {
				Value v;
				v.SetUint64(*_blockchain_height);
				dict.AddMember(StringRef(__dictKey_blockchain_height), v, dict.GetAllocator());
			}
			if (_transactions != none) {
				Value k(StringRef(__dictKey_transactions));
				dict.AddMember(k, new_arrayOfSerializedDicts(*_transactions, dict.GetAllocator()).Move(), dict.GetAllocator());
			}
			auto s = Persistable::new_plaintextJSONStringFrom_movedDocumentDict(dict);
			
			return dict;
		}
		const CollectionName &collectionName() const override { return Wallets::collectionName; }
		//
		// Accessors - Properties
		
		
		
		
		
		//
		//
		//
		// TODO: use mutex on accessing these
		//
		//
		//
		//
		
		
		
		
		
		Wallets::Currency currency() { return _currency; }
		string walletLabel() { return _walletLabel; }
		SwatchColor swatchColor() { return _swatchColor; }
		bool isLoggedIn() { return _isLoggedIn; }
		optional<string> mnemonicString() { return _mnemonicString; } // TODO: is this the correct way to return an optional?
		string public_address() { return _public_address; }
		string view_sec_key() { return _view_sec_key; }
		string spend_sec_key() { return _spend_sec_key; }
		string spend_pub_key() { return _spend_pub_key; }
		std::shared_ptr<Wallets::KeyImageCache> lazy_keyImageCache() {
			if (_keyImageCache == nullptr) {
				_keyImageCache = std::make_shared<Wallets::KeyImageCache>();
			}
			return _keyImageCache;
		}

		
		
		
		
		
		
		
		
		
		
		
		//
		// TODO
//		var isFetchingAnyUpdates: Bool {
//			if self.hostPollingController == nil {
//				return false
//			}
//			return self.hostPollingController!.isFetchingAnyUpdates
//		}
//		var hasEverFetched_accountInfo: Bool
//		{
//			return self.dateThatLast_fetchedAccountInfo != nil
//		}
//		var hasEverFetched_transactions: Bool
//		{
//			return self.dateThatLast_fetchedAccountTransactions != nil
//		}
//		var isAccountScannerCatchingUp: Bool
//		{
//			if self.didFailToInitialize_flag == true || self.didFailToBoot_flag == true {
//				assert(false, "not strictly illegal but accessing isAccountScannerCatching up before logged in")
//				return false
//			}
//			if self.blockchain_height == nil || self.blockchain_height == 0 {
//				DDLog.Warn("Wallets", ".isScannerCatchingUp called while nil/0 blockchain_height")
//				return true
//			}
//			if self.account_scanned_block_height == nil || self.account_scanned_block_height == 0  {
//				DDLog.Warn("Wallets", ".isScannerCatchingUp called while nil/0 account_scanned_block_height")
//				return true
//			}
//			let nBlocksBehind = self.blockchain_height! - self.account_scanned_block_height!
//			if nBlocksBehind >= 10 { // grace interval, i believe
//				return true
//			}
//			return false
//		}
//		var nBlocksBehind: UInt64
//		{
//			if self.blockchain_height == nil || self.blockchain_height == 0 {
//				DDLog.Warn("Wallets", ".nBlocksBehind called while nil/0 blockchain_height")
//				return 0
//			}
//			if self.account_scanned_block_height == nil || self.account_scanned_block_height == 0  {
//				DDLog.Warn("Wallets", ".nBlocksBehind called while nil/0 account_scanned_block_height")
//				return 0
//			}
//			let nBlocksBehind = self.blockchain_height! - self.account_scanned_block_height!
//			//
//			return nBlocksBehind
//		}
//			var catchingUpPercentageFloat: Double // btn 0 and 1.0
//		{
//			if self.account_scanned_height == nil || self.account_scanned_height == 0 {
//				DDLog.Warn("Wallets", ".catchingUpPercentageFloat accessed while nil/0 self.account_scanned_height. Bailing.")
//				return 0
//			}
//			if self.transaction_height == nil || self.transaction_height == 0 {
//				DDLog.Warn("Wallets", ".catchingUpPercentageFloat accessed while nil/0 self.transaction_height. Bailing.")
//				return 0
//			}
//			let pct: Double = Double(self.account_scanned_height!) / Double(self.transaction_height!)
//			DDLog.Info("Wallets", "CatchingUpPercentageFloat \(self.account_scanned_height!)/\(self.transaction_height!) = \(pct)%")
//			return pct
//		}
//		//
//		var balanceAmount: MoneroAmount {
//			let balanceAmount = (self.totalReceived ?? MoneroAmount(0)) - (self.totalSent ?? MoneroAmount(0))
//			if balanceAmount < 0 {
//				return MoneroAmount("0")!
//			}
//			return balanceAmount
//		}
//		var lockedBalanceAmount: MoneroAmount {
//			return (self.lockedBalance ?? MoneroAmount(0))
//		}
//		var hasLockedFunds: Bool {
//			if self.lockedBalance == nil {
//				return false
//			}
//			if self.lockedBalance == MoneroAmount(0) {
//				return false
//			}
//			return true
//		}
//		var unlockedBalance: MoneroAmount {
//			let lb = self.lockedBalanceAmount
//			let b = self.balanceAmount
//			if b < lb {
//				return 0
//			}
//			return b - lb
//		}
//		var new_pendingBalanceAmount: MoneroAmount {
//			var amount = MoneroAmount(0)
//			(self.transactions ?? [MoneroHistoricalTransactionRecord]()).forEach { (tx) in
//				if tx.cached__isConfirmed != true {
//					if tx.isFailed != true /* nil -> false */{ // just filtering these out
//						// now, adding both of these (positive) values to contribute to the total
//						let abs_mag = (tx.totalSent - tx.totalReceived).magnitude // throwing away the sign
//						amount += MoneroAmount(abs_mag)
//					}
//				}
//			}
//			return amount
//		}
		//
		// Properties - Signals
		boost::signals2::signal<void()> labelChanged_signal;
		boost::signals2::signal<void()> swatchColorChanged_signal;
		boost::signals2::signal<void()> balanceChanged_signal;
		//
		boost::signals2::signal<void()> spentOutputsChanged_signal;
		boost::signals2::signal<void()> heightsUpdated_signal;
		boost::signals2::signal<void()> transactionsChanged_signal;
		//
		boost::signals2::signal<void()> didChange_isFetchingAnyUpdates_signal;
		//
		// Imperatives
		void Boot_havingLoadedDecryptedExistingInitDoc(
			std::function<void(optional<string> err_str)> fn
		);
		void Boot_byLoggingIn_givenNewlyCreatedWallet(
			const string &walletLabel,
			Wallets::SwatchColor swatchColor,
			std::function<void(optional<string> err_str)> fn
		);
		void Boot_byLoggingIn_existingWallet_withMnemonic(
			const string &walletLabel,
			SwatchColor swatchColor,
			const string &mnemonic_string,
			bool persistEvenIfLoginFailed_forServerChange,
			std::function<void(optional<string> err_str)> fn
		);
		void Boot_byLoggingIn_existingWallet_withAddressAndKeys(
			const string &walletLabel,
			SwatchColor swatchColor,
			const string &address,
			const string &sec_viewKey_string,
			const string &sec_spendKey_string,
			bool persistEvenIfLoginFailed_forServerChange,
			std::function<void(optional<string> err_str)> fn
		);
		void logOutThenSaveAndLogIn();
		//
		void requestManualUserRefresh();
		//
		optional<string/*err_str*/> SetValuesAndSave(
			string walletLabel,
			SwatchColor swatchColor
		);
		//
		// HostPollingController - Delegation / Protocol
		void _HostPollingController_didFetch_addressInfo(
			const HostedMonero::ParsedResult_AddressInfo &parsedResult
		);
		void _HostPollingController_didFetch_addressTransactions(
			const HostedMonero::ParsedResult_AddressTransactions &parsedResult
		);
	private:
		//
		// Lifecycle / Initialization
		void tearDownRuntime();
		void deBoot();
		//
		// Properties
		optional<bool> _local_wasAGeneratedWallet;
		optional<bool> _login__new_address;
		optional<bool> _login__generated_locally;
		//
		cryptonote::network_type _nettype;
		std::shared_ptr<HostedMonero::APIClient> _apiClient;
		std::shared_ptr<Dispatch::Dispatch> _dispatch_ptr;
		std::shared_ptr<UserIdle::Controller> _userIdleController;
		std::shared_ptr<Currencies::ConversionRatesController> _ccyConversionRatesController;
		//
		std::shared_ptr<Wallets::KeyImageCache> _keyImageCache; // initialized lazily ... do not access directly
		//
		Wallets::Currency _currency;
		string _walletLabel;
		SwatchColor _swatchColor;
		//
		optional<string> _mnemonicString;
		optional<string> _mnemonic_wordsetName;
		optional<monero_wallet_utils::WalletDescription> _generatedOnInit_walletDescription;

		optional<string> _account_seed;
		string _view_sec_key;
		string _spend_sec_key;
		string _view_pub_key;
		string _spend_pub_key;
		string _public_address;
		//
		optional<uint64_t> _totalReceived;
		optional<uint64_t> _totalSent;
		optional<uint64_t> _lockedBalance;
		//
		optional<uint64_t> _account_scanned_tx_height;
		optional<uint64_t> _account_scanned_height; // TODO: it would be good to resolve account_scanned_height vs account_scanned_tx_height
		optional<uint64_t> _account_scanned_block_height;
		optional<uint64_t> _account_scan_start_height;
		optional<uint64_t> _transaction_height;
		optional<uint64_t> _blockchain_height;
		//
		optional<std::vector<HostedMonero::SpentOutputDescription>> _spentOutputs;
		optional<std::vector<HostedMonero::HistoricalTxRecord>> _transactions;
		//
		optional<time_t> _dateThatLast_fetchedAccountInfo;
		optional<time_t> _dateThatLast_fetchedAccountTransactions;
		//
		// Properties - Boolean State
		// persisted
		bool _isLoggedIn = false;
		optional<bool> _isInViewOnlyMode;
		// transient/not persisted
		bool _isBooted = false;
		optional<bool> _shouldDisplayImportAccountOption;
		bool _isLoggingIn = false;
		optional<bool> _wasInitializedWith_addrViewAndSpendKeysInsteadOfSeed;
		bool _isSendingFunds = false;
		//
		// Properties - Objects
		std::shared_ptr<HTTPRequests::Handle> _logIn_requestHandle;
		std::shared_ptr<Wallets::HostPollingController> _hostPollingController; // strong; can be nullptr
		std::unique_ptr<Wallets::TxCleanupController> _txCleanupController; // strong; can be nullptr
		std::unique_ptr<HTTPRequests::Handle> _current_sendFunds_request;
//		var submitter: SendFundsFormSubmissionHandle?
		//
		// Accessors
		//
		// Imperatives
		void _setStateThatFailedToBoot(optional<string> err_str);
		void __trampolineFor_failedToBootWith_fnAndErrStr(
			std::function<void(optional<string> err_str)>&& fn,
			optional<string> err_str
		);
		void _trampolineFor_successfullyBooted(
			std::function<void(optional<string> err_str)>&& fn
		);
		void ___proceed_havingActuallyBooted__trampolineFor_successfullyBooted(
			std::function<void(optional<string> err_str)>&& fn
		);
		void _atRuntime_setup_hostPollingController();
		void _boot_byLoggingIn(
			const string &address,
			const string &sec_viewKey_string,
			const string &sec_spendKey_string,
			optional<string> seed_orNone,
			bool wasAGeneratedWallet,
			bool persistEvenIfLoginFailed_forServerChange,
			std::function<void(optional<string> err_str)> fn
		);
		void _manuallyInsertTransactionRecord(
			const HostedMonero::HistoricalTxRecord &transaction
		);
		//
		void __lock_sending();
		void __unlock_sending();
		//
		void regenerate_shouldDisplayImportAccountOption();
		//
		// Delegation
		void ___didReceiveActualChangeTo_balance();
		void ___didReceiveActualChangeTo_spentOutputs();
		void ___didReceiveActualChangeTo_heights();
		void ___didReceiveActualChangeTo_transactions();
	};
}

#endif /* Wallet_hpp */



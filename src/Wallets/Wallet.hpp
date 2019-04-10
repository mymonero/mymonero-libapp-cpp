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
#include "../Persistence/PersistableObject.hpp"
#include "monero_wallet_utils.hpp"
#include "../APIClient/HostedMonero.hpp"
#include "./Wallet_HostPollingController.hpp"
#include "./Wallet_TxCleanupController.hpp"
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
	static inline string symbol_string_from(Wallets::Currency currency)
	{
		switch(currency) {
			case Monero:
				return "xmr";
			default:
				BOOST_THROW_EXCEPTION(logic_error("Unhandled Wallets::Currency"));
				return symbol_string_from(Monero); // default … not rea
		}
	}
	static inline string jsonRepresentation(Wallets::Currency currency)
	{
		return symbol_string_from(currency);
	}
	static inline string humanReadableCurrencySymbolString(Wallets::Currency currency)
	{
		switch(currency) {
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
	//
	// Constants
	static const CollectionName collectionName = "Wallets";
	//
	//
	class Object: public Persistable::Object
	{
	public:
		Object(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<const Passwords::PasswordProvider> passwordProvider,
			optional<monero_wallet_utils::WalletDescription> ifGeneratingNewWallet_walletDescription,
			const cryptonote::network_type nettype,
			std::shared_ptr<HostedMonero::APIClient> apiClient,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider)
		),
		_nettype(nettype),
		_apiClient(apiClient),
		_dispatch_ptr(dispatch_ptr),
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
			const property_tree::ptree &plaintextData,
			const cryptonote::network_type nettype,
			std::shared_ptr<HostedMonero::APIClient> apiClient,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider),
			plaintextData
		),
		_nettype(nettype),
		_apiClient(apiClient),
		_dispatch_ptr(dispatch_ptr)
		{
			if (this->_id != boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->_id == boost::none"));
			}
			if (this->insertedAt_sSinceEpoch != boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->insertedAt_sSinceEpoch == boost::none"));
			}
			//
			// TODO:
//			this->addtlVal = plaintextData.get<std::string>("addtlVal");
//			BOOST_REQUIRE(this->addtlVal != boost::none);
		}
		//
		// Lifecycle
		void teardown() override; // override
		//
		// Accessors
		virtual property_tree::ptree new_dictRepresentation() const override
		{
			property_tree::ptree dict = Persistable::Object::new_dictRepresentation();
			//
			// TODO:
			//
//			if (this->addtlVal) {
//				dict.put("addtlVal", *(this->addtlVal));
//			}
			//
			return dict;
		}
		const CollectionName &collectionName() const override { return Wallets::collectionName; }
		//
		// Accessors - Properties
		Wallets::Currency currency() { return _currency; }
		string walletLabel() { return _walletLabel; }
		SwatchColor swatchColor() { return _swatchColor; };
		//
		// Signals
		boost::signals2::signal<void()> didChange_isFetchingAnyUpdates_signal;
		//
		// Imperatives
		void Boot_havingLoadedDecryptedExistingInitDoc(
			std::function<void(optional<string> err_str)>&& fn
		);
		void Boot_byLoggingIn_givenNewlyCreatedWallet(
			const string &walletLabel,
			Wallets::SwatchColor swatchColor,
			std::function<void(optional<string> err_str)>&& fn
		);
		void Boot_byLoggingIn_existingWallet_withMnemonic(
			const string &walletLabel,
			SwatchColor swatchColor,
			const string &mnemonic_string,
			bool persistEvenIfLoginFailed_forServerChange,
			std::function<void(optional<string> err_str)>&& fn
		);
		void Boot_byLoggingIn_existingWallet_withAddressAndKeys(
			const string &walletLabel,
			SwatchColor swatchColor,
			const string &address,
			const string &sec_viewKey_string,
			const string &sec_spendKey_string,
			bool persistEvenIfLoginFailed_forServerChange,
			std::function<void(optional<string> err_str)>&& fn
		);
		void logOutThenSaveAndLogIn();
		//
		void requestManualUserRefresh();
		//
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
//		var totalReceived: MoneroAmount?
//		var totalSent: MoneroAmount?
//		var lockedBalance: MoneroAmount?
//		//
		optional<uint64_t> _account_scanned_tx_height;
		optional<uint64_t> _account_scanned_height; // TODO: it would be good to resolve account_scanned_height vs account_scanned_tx_height
		optional<uint64_t> _account_scanned_block_height;
		optional<uint64_t> _account_scan_start_height;
		optional<uint64_t> _transaction_height;
		optional<uint64_t> _blockchain_height;
//		//
//		var spentOutputs: [MoneroSpentOutputDescription]?
//		var transactions: [MoneroHistoricalTransactionRecord]?
//		//
//		var dateThatLast_fetchedAccountInfo: Date?
//		var dateThatLast_fetchedAccountTransactions: Date?
		//
		// Properties - Boolean State
		// persisted
		bool _isLoggedIn = false;
		optional<bool> _isInViewOnlyMode;
//		// transient/not persisted
		bool _isBooted = false;
		optional<bool> _shouldDisplayImportAccountOption;
		bool _isLoggingIn = false;
		optional<bool> _wasInitializedWith_addrViewAndSpendKeysInsteadOfSeed;
		bool _isSendingFunds = false;
		//
		// Properties - Objects
		// TODO: key image cache
		//
		std::unique_ptr<HTTPRequests::Handle> _logIn_requestHandle;
		std::unique_ptr<Wallets::HostPollingController> _hostPollingController; // strong; can be nullptr
		std::unique_ptr<Wallets::TxCleanupController> _txCleanupController; // strong; can be nullptr
		std::unique_ptr<HTTPRequests::Handle> _current_sendFunds_request;
//		var submitter: SendFundsFormSubmissionHandle?
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
			std::function<void(optional<string> err_str)>&& fn
		);
		//
		void regenerate_shouldDisplayImportAccountOption();
	};
}

#endif /* Wallet_hpp */



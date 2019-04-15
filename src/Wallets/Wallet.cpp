//
//  Wallet.cpp
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
#include "Wallet.hpp"
using namespace Wallets;
#include "misc_log_ex.h"
using namespace monero_wallet_utils;
//
// Accessory types
struct OptlErrStrCBFunctor
{ // when you pass this functor to its destination, do a std::move of it to the destination
	std::function<void(optional<string> err_str)> fn; // do a std::move to this property manually
	void operator()(optional<string> err_str)
	{
		fn(err_str);
	}
};
//
// Lifecycle - Deinit
void Object::teardown()
{
	Persistable::Object::teardown();
	tearDownRuntime();
}
void Object::tearDownRuntime()
{
	_hostPollingController = nullptr; // stop requests
	_txCleanupController = nullptr; // stop timer
	//
	if (_logIn_requestHandle != nullptr) {
		_logIn_requestHandle->cancel(); // in case wallet is being rebooted on API address change via settings
		_logIn_requestHandle = nullptr;
	}
	_isLoggingIn = false;
	//
	if (_isSendingFunds) { // just in case - i.e. on teardown while sending but user sends the app to the background
		
		// TODO		
		
//		userIdle->reEnable_userIdle();
//		ScreenSleep.reEnable_screenSleep();
		// ^------ TODO: for this, need to call out to app bridge layer somehow…… maybe fire signal or something
	}
	if (_current_sendFunds_request != nullptr) { // to get the network request cancel immediately
		_current_sendFunds_request->cancel();
		_current_sendFunds_request = nullptr;
	}
}
void Object::deBoot()
{
	
	// TODO
	
	
//	let old__totalReceived = self.totalReceived
//	let old__totalSent = self.totalSent
//	let old__lockedBalance = self.lockedBalance
//	let old__spentOutputs = self.spentOutputs
//	let old__transactions = self.transactions
//	{
//		tearDownRuntime() // stop any requests, etc
//	}
//	{
//		// important flags to clear:
//		self.isLoggedIn = false
//		self.didFailToBoot_flag = nil
//		self.didFailToBoot_errStr = nil
//		self.isBooted = false
//		//
//		self.totalReceived = nil
//		self.totalSent = nil
//		self.lockedBalance = nil
//		//
//		self.account_scanned_tx_height = nil
//		self.account_scanned_height = nil
//		self.account_scanned_block_height = nil
//		self.account_scan_start_height = nil
//		self.transaction_height = nil
//		self.blockchain_height = nil
//		//
//		self.spentOutputs = nil
//		self.transactions = nil
//		//
//		self.dateThatLast_fetchedAccountInfo = nil
//		self.dateThatLast_fetchedAccountTransactions = nil
//	}
//	{
//		self.___didReceiveActualChangeTo_balance(
//			old_totalReceived: old__totalReceived,
//			old_totalSent: old__totalSent,
//			old_lockedBalance: old__lockedBalance
//		)
//		self.___didReceiveActualChangeTo_spentOutputs(
//			old_spentOutputs: old__spentOutputs
//		)
//		self.___didReceiveActualChangeTo_heights()
//		self.___didReceiveActualChangeTo_transactions(
//			old_transactions: old__transactions
//		)
//		self.regenerate_shouldDisplayImportAccountOption()
//	}
//	let save_err_str = self.saveToDisk()
//	if save_err_str != nil {
//		DDLog.Error("Wallet", "Error while saving during a deBoot(): \(save_err_str!)")
//	}
}
//
// Runtime - Imperatives - Private - Booting
void Object::Boot_havingLoadedDecryptedExistingInitDoc(
	std::function<void(optional<string> err_str)>&& fn
) { // nothing to do here as we assume validation done on init
	_trampolineFor_successfullyBooted(std::move(fn));
}
void Object::_setStateThatFailedToBoot(
	optional<string> err_str
) {
	didFailToBoot_flag = true;
	didFailToBoot_errStr = err_str;
}
void Object::__trampolineFor_failedToBootWith_fnAndErrStr(
	std::function<void(optional<string> err_str)>&& fn,
	optional<string> err_str
) {
	_setStateThatFailedToBoot(err_str);
	//
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_dispatch_ptr->async([weak_this]() {
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->failedToBoot_signal();
		}
	});
	fn(err_str);
}
void Object::_trampolineFor_successfullyBooted(
	std::function<void(optional<string> err_str)>&& fn
) {
	if (_account_seed == none || _account_seed->empty()) {
		MWARNING("Wallets: Wallet initialized without an account_seed.");
		_wasInitializedWith_addrViewAndSpendKeysInsteadOfSeed = true;
		___proceed_havingActuallyBooted__trampolineFor_successfullyBooted(std::move(fn));
		return;
	}
	// re-derive mnemonic string from account seed
	SeedDecodedMnemonic_RetVals retVals = mnemonic_string_from_seed_hex_string(
		*_account_seed,
		*_mnemonic_wordsetName
	);
	boost::property_tree::ptree root;
	if (retVals.err_string != none) {
		__trampolineFor_failedToBootWith_fnAndErrStr(std::move(fn), std::move(*(retVals.err_string)));
		return;
	}
	auto seedAsMnemonic = std::string((*(retVals.mnemonic_string)).data(), (*(retVals.mnemonic_string)).size());
	if (_mnemonicString != none) {
		bool equal;
		try {
			equal = are_equal_mnemonics(*_mnemonicString, seedAsMnemonic);
		} catch (std::exception const& e) {
			__trampolineFor_failedToBootWith_fnAndErrStr(std::move(fn), string(e.what()));
			return;
		}
		if (equal == false) { // would be rather odd; NOTE: must use this comparator instead of string comparison to support partial-word mnemonic strings
			BOOST_THROW_EXCEPTION(logic_error("Different mnemonicString derived from accountSeed than was entered for login"));
			__trampolineFor_failedToBootWith_fnAndErrStr(std::move(fn), string("Mnemonic seed mismatch"));
			return;
		}
	}
	_mnemonicString = std::move(seedAsMnemonic); // set it in all cases - because we want to support converting partial-word input to full-word for display and recording
	___proceed_havingActuallyBooted__trampolineFor_successfullyBooted(std::move(fn));
}
void Object::___proceed_havingActuallyBooted__trampolineFor_successfullyBooted(
	std::function<void(optional<string> err_str)>&& fn
) {
//	DDLog.Done("Wallets", "Successfully booted \(self)")
	_isBooted = true;
	didFailToBoot_errStr = none;
	didFailToBoot_flag = none;
	//
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_dispatch_ptr->async([weak_this]()
	{
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->_atRuntime_setup_hostPollingController(); // instantiate (and kick off) polling controller
			inner_spt->_txCleanupController = std::make_unique<Wallets::TxCleanupController>(*inner_spt);
			//
			inner_spt->booted_signal();
		}
	});
	fn(none);
}
void Object::_atRuntime_setup_hostPollingController()
{
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_hostPollingController = std::make_unique<Wallets::HostPollingController>(
		*this,
		[weak_this]()
		{
			if (auto inner_spt = weak_this.lock()) {
				inner_spt->_dispatch_ptr->async([weak_this]() {
					if (auto inner_inner_spt = weak_this.lock()) {
						inner_inner_spt->didChange_isFetchingAnyUpdates_signal();
					}
				});
			}
		}
	);
}
//
//
void Object::_boot_byLoggingIn(
	const string &address,
	const string &sec_viewKey_string,
	const string &sec_spendKey_string,
	optional<string> seed_orNone,
	bool wasAGeneratedWallet,
	bool persistEvenIfLoginFailed_forServerChange,
	std::function<void(optional<string> err_str)>&& fn
) {
	_isLoggingIn = true;
	//
	WalletComponentsValidationResults retVals;
	bool r = validate_wallet_components_with( // returns !did_error
		address,
		sec_viewKey_string,
		sec_spendKey_string,
		seed_orNone,
		_nettype,
		retVals
	);
	bool did_error = retVals.did_error;
	if (!r) {
		if (persistEvenIfLoginFailed_forServerChange == true) {
			BOOST_THROW_EXCEPTION(logic_error("Only expecting already-persisted wallets to have had persistEvenIfLoginFailed_forServerChange=true")); // yet components are now invalid…?
		}
		__trampolineFor_failedToBootWith_fnAndErrStr(std::move(fn), std::move(*retVals.err_string));
		return;
	}
	if (did_error) {
		BOOST_THROW_EXCEPTION(logic_error("Illegal success flag but did_error"));
		return;
	}
	if (!retVals.isValid) {
		BOOST_THROW_EXCEPTION(logic_error("Found unexpectedly invalid wallet components without an error"));
		return;
	}
	if (seed_orNone == none) {
		if (seed_orNone->empty()) {
			BOOST_THROW_EXCEPTION(logic_error("Invalid empty string seed"));
			return;
		}
	}
	{ // record these properties regardless of whether we are about to error on login
		_public_address = address;
		_account_seed = *seed_orNone;
		_view_pub_key = retVals.pub_viewKey_string;
		_spend_pub_key = retVals.pub_spendKey_string;
		_view_sec_key = sec_viewKey_string;
		_spend_sec_key = sec_spendKey_string;
		_isInViewOnlyMode = retVals.isInViewOnlyMode;
		_local_wasAGeneratedWallet = wasAGeneratedWallet;
	}
	{ // this state must be reset or a prior failure may appear not to reset state (more of an issue in the JS app since the state was not reset on boot success)
		didFailToBoot_errStr = none;
		didFailToBoot_flag = none;
	}
	if (_public_address.size() == 0) {
		BOOST_THROW_EXCEPTION(logic_error("Expected non-empty _public_address"));
		return;
	}
	if (_view_pub_key.size() == 0) {
		BOOST_THROW_EXCEPTION(logic_error("Expected non-empty _view_pub_key"));
		return;
	}
	auto cb_functor(OptlErrStrCBFunctor{
		std::move(fn)
	});
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_logIn_requestHandle = _apiClient->logIn(
		_public_address,
		_view_pub_key,
		*_local_wasAGeneratedWallet,
		[
			weak_this,
			persistEvenIfLoginFailed_forServerChange,
			cb_functor
		] (
			optional<string> login__err_str,
			optional<HostedMonero::ParsedResult_Login> result
		) {
			if (auto inner_spt = weak_this.lock()) {
				inner_spt->_isLoggingIn = false;
				inner_spt->_isLoggedIn = login__err_str == none; // supporting shouldExitOnLoginError=false for wallet reboot
				//
				bool shouldExitOnLoginError = persistEvenIfLoginFailed_forServerChange == false;
				if (login__err_str != none) {
					if (shouldExitOnLoginError == true) {
						inner_spt->__trampolineFor_failedToBootWith_fnAndErrStr(
							std::move(cb_functor),
							std::move(login__err_str)
						);
						inner_spt->_logIn_requestHandle = nullptr; // release
						return;
					} else {
					// this allows us to continue with the above-set login info to call 'saveToDisk()' when this call to log in is coming from a wallet reboot. reason is that we expect all such wallets to be valid monero wallets if they are able to have been rebooted.
					}
				}
				if (result != none) { // i.e. on error but shouldExitOnLoginError != true
					inner_spt->_login__new_address = (*result).isANewAddressToServer;
					inner_spt->_login__generated_locally = (*result).generated_locally;
					inner_spt->_account_scan_start_height = (*result).start_height;
					//
					inner_spt->regenerate_shouldDisplayImportAccountOption(); // now this can be called
				}
				optional<string> saveToDisk__err_str = inner_spt->saveToDisk();
				if (saveToDisk__err_str != none) {
					inner_spt->__trampolineFor_failedToBootWith_fnAndErrStr(
						std::move(cb_functor),
						std::move(*saveToDisk__err_str)
					);
					inner_spt->_logIn_requestHandle = nullptr; // release
					return;
				}
				if (shouldExitOnLoginError == false && login__err_str != none) {
					// if we are attempting to re-boot the wallet, but login failed
					inner_spt->__trampolineFor_failedToBootWith_fnAndErrStr( // i.e. leave the wallet in the 'errored'/'failed to boot' state even though we saved
						std::move(cb_functor),
						std::move(*login__err_str)
					);
					inner_spt->_logIn_requestHandle = nullptr; // release
				} else { // it's actually a success
					inner_spt->_trampolineFor_successfullyBooted(std::move(cb_functor));
					inner_spt->_logIn_requestHandle = nullptr; // release
				}
			}
		}
	);
}
//
// Imperatives
void Object::requestManualUserRefresh()
{
	if (_hostPollingController != nullptr) {
		_hostPollingController->requestFromUI_manualRefresh();
	} else {
		MWARNING("Wallet: Manual refresh requested before hostPollingController set up.");
		// not booted yet.. ignoring
	}
}
void Object::regenerate_shouldDisplayImportAccountOption()
{
	bool isAPIBeforeGeneratedLocallyAPISupport = _login__generated_locally == none || _account_scan_start_height == none;
	if (isAPIBeforeGeneratedLocallyAPISupport) {
		if (_local_wasAGeneratedWallet == none) {
			_local_wasAGeneratedWallet = false; // just going to set this to false - it means the user is on a wallet which was logged in via a previous version
		}
		if (_login__new_address == none) {
			_login__new_address = false; // going to set this to false if it doesn't exist - it means the user is on a wallet which was logged in via a previous version
		}
		_shouldDisplayImportAccountOption = *_local_wasAGeneratedWallet == false && *_login__new_address == true;
	} else {
		if (_account_scan_start_height == none) {
			BOOST_THROW_EXCEPTION(logic_error("Logic error: expected latest_scan_start_height"));
			return;
		}
		_shouldDisplayImportAccountOption = (_login__generated_locally == none || *_login__generated_locally != true) && *_account_scan_start_height != 0;
	}
}
//
//
void Object::Boot_byLoggingIn_givenNewlyCreatedWallet(
	const string &walletLabel,
	SwatchColor swatchColor,
	std::function<void(optional<string> err_str)>&& fn
) {
	_walletLabel = walletLabel;
	_swatchColor = swatchColor;
	//
	if (_generatedOnInit_walletDescription == none) {
		BOOST_THROW_EXCEPTION(logic_error("nil generatedOnInit_walletDescription"));
		return;
	}
	_boot_byLoggingIn(
		std::move((*_generatedOnInit_walletDescription).address_string),
		epee::string_tools::pod_to_hex((*_generatedOnInit_walletDescription).sec_viewKey),
		epee::string_tools::pod_to_hex((*_generatedOnInit_walletDescription).sec_spendKey),
		std::move((*_generatedOnInit_walletDescription).sec_seed_string),
		true, // wasAGeneratedWallet: in this case
		false, // persistEvenIfLoginFailed_forServerChange: always, in this case
		std::move(fn)
	);
}
void Object::Boot_byLoggingIn_existingWallet_withMnemonic(
	const string &walletLabel,
	SwatchColor swatchColor,
	const string &mnemonic_string,
	bool persistEvenIfLoginFailed_forServerChange,
	std::function<void(optional<string> err_str)>&& fn
) {
	_walletLabel = walletLabel;
	_swatchColor = swatchColor;
	//
	_mnemonicString = mnemonic_string; // even though we re-derive the mnemonicString on success, this is being set here so as to prevent the bug where it gets lost when changing the API server and a reboot w/mnemonicSeed occurs
	// we'll set the wordset name in a moment
	//
	WalletDescriptionRetVals retVals;
	bool r = wallet_with(
		mnemonic_string,
		retVals,
		_nettype
	);
	if (!r) {
		__trampolineFor_failedToBootWith_fnAndErrStr(std::move(fn), std::move(*retVals.err_string));
		return;
	}
	if (retVals.did_error) {
		BOOST_THROW_EXCEPTION(logic_error("Illegal success flag but did_error"));
		return;
	}
	if ((*retVals.optl__desc).sec_seed_string.size() == 0) {
		BOOST_THROW_EXCEPTION(logic_error("Unexpectedly empty seed"));
		return;
	}
	_mnemonic_wordsetName = std::move((*retVals.optl__desc).mnemonic_language);
	_boot_byLoggingIn(
		std::move((*(retVals.optl__desc)).address_string),
		epee::string_tools::pod_to_hex((*(retVals.optl__desc)).sec_viewKey),
		epee::string_tools::pod_to_hex((*(retVals.optl__desc)).sec_spendKey),
		std::move((*retVals.optl__desc).sec_seed_string),
		false, // wasAGeneratedWallet
		persistEvenIfLoginFailed_forServerChange,
		std::move(fn)
	);
}
void Object::Boot_byLoggingIn_existingWallet_withAddressAndKeys(
	const string &walletLabel,
	SwatchColor swatchColor,
	const string &address,
	const string &sec_viewKey_string,
	const string &sec_spendKey_string,
	bool persistEvenIfLoginFailed_forServerChange,
	std::function<void(optional<string> err_str)>&& fn
) {
	_walletLabel = walletLabel;
	_swatchColor = swatchColor;
	_boot_byLoggingIn(
		address,
		sec_viewKey_string,
		sec_spendKey_string,
		none, // seed_orNil
		false, // wasAGeneratedWallet
		persistEvenIfLoginFailed_forServerChange,
		std::move(fn)
	);
}
void Object::logOutThenSaveAndLogIn()
{
	if (_isLoggedIn || (didFailToBoot_flag != none && *didFailToBoot_flag == true) || _isBooted == true) { // if we actually do need to log out ... otherwise this may be an attempt by the ListController to log in after having loaded a failed login from a previous user session upon launching the app
		deBoot();
	}
	_boot_byLoggingIn(
		_public_address,
		_view_sec_key,
		_spend_sec_key, // currently not expecting nil
		_account_seed,
		_local_wasAGeneratedWallet != none ? *_local_wasAGeneratedWallet : false,
		true, // persistEvenIfLoginFailed_forServerChange
		[](optional<string> err_str)
		{
			if (err_str != none) {
				MERROR("Wallets: Failed to log back in with error: " << *err_str);
				return;
			}
			MDEBUG("Wallets: Logged back in.");
		}
	);
}




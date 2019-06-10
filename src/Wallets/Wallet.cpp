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
struct LogInReqCBFunctor
{ // when you pass this functor to its destination, do a std::move of it to the destination
	std::function<void(optional<string> err_str, optional<HostedMonero::ParsedResult_Login> result)> fn; // do a std::move to this property manually
	~LogInReqCBFunctor() {
		cout << "LogInReqCBFunctor dtor" << endl;
	}
	void operator()(optional<string> err_str, optional<HostedMonero::ParsedResult_Login> result)
	{
		if (err_str != none) {
			fn(std::move(*err_str), none);
			return;
		}
		fn(none, std::move(*result));
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
	_keyImageCache = nullptr;
	//
	if (_logIn_requestHandle != nullptr) {
		_logIn_requestHandle->cancel(); // in case wallet is being rebooted on API address change via settings
		_logIn_requestHandle = nullptr;
	}
	_isLoggingIn = false;
	//
	if (_isSendingFunds) { // just in case - i.e. on teardown while sending but user sends the app to the background
		_userIdleController->reEnable_userIdle();
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
//	optional<uint64_t> old__totalReceived = none;
//	optional<uint64_t> old__totalSent = none;
//	optional<uint64_t> old__lockedBalance = none;
//	optional<std::vector<HostedMonero::SpentOutputDescription>> old__spentOutputs = none;
//	optional<std::vector<HostedMonero::HistoricalTxRecord>> old__transactions = none;
//	if (_totalReceived != none) {
//		old__totalReceived = *_totalReceived;
//	}
//	if (_totalSent != none) {
//		old__totalSent = *_totalSent;
//	}
//	if (_lockedBalance != none) {
//		old__lockedBalance = *_lockedBalance;
//	}
//	if (_spentOutputs != none) {
//		old__spentOutputs = std::move(*_spentOutputs);
//	}
//	if (_transactions != none) {
//		old__transactions = std::move(*_transactions);
//	}
	{
		tearDownRuntime(); // stop any requests, etc
	}
	{
		// important flags to clear:
		_isLoggedIn = false;
		didFailToBoot_flag = none;
		didFailToBoot_errStr = none;
		_isBooted = false;
		//
		_totalReceived = none;
		_totalSent = none;
		_lockedBalance = none;
		//
		_account_scanned_tx_height = none;
		_account_scanned_height = none;
		_account_scanned_block_height = none;
		_account_scan_start_height = none;
		_transaction_height = none;
		_blockchain_height = none;
		//
		_spentOutputs = none;
		_transactions = none;
		//
		_dateThatLast_fetchedAccountInfo = none;
		_dateThatLast_fetchedAccountTransactions = none;
	}
	{
		___didReceiveActualChangeTo_balance();
		___didReceiveActualChangeTo_spentOutputs();
		___didReceiveActualChangeTo_heights();
		___didReceiveActualChangeTo_transactions();
		regenerate_shouldDisplayImportAccountOption();
	}
	optional<string> save__err_str = saveToDisk();
	if (save__err_str != none) {
		MERROR("Wallet: Error while saving during a deBoot(): " << *save__err_str);
	}
}
//
// Runtime - Imperatives - Private - Booting
void Object::Boot_havingLoadedDecryptedExistingInitDoc(
	std::function<void(optional<string> err_str)> fn
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
	_hostPollingController = std::make_shared<Wallets::HostPollingController>(
		weak_this,
		_dispatch_ptr,
		_apiClient,
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
	_hostPollingController->setup();
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
	std::function<void(optional<string> err_str)> fn
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
	auto logIn_cb_fn = [
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
	};
	_logIn_requestHandle = _apiClient->logIn(
		_public_address,
		_view_sec_key,
		*_local_wasAGeneratedWallet,
		std::move(logIn_cb_fn)
	);
}
//
void Object::Boot_byLoggingIn_givenNewlyCreatedWallet(
	const string &walletLabel,
	SwatchColor swatchColor,
	std::function<void(optional<string> err_str)> fn
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
	std::function<void(optional<string> err_str)> fn
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
	std::function<void(optional<string> err_str)> fn
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
// Runtime (Booted) - Imperatives - Updates
optional<string/*err_str*/> Object::SetValuesAndSave(
	string walletLabel,
	SwatchColor swatchColor
) {
	bool isChanging__walletLabel = _walletLabel != walletLabel;
	bool isChanging__swatchColor = _swatchColor != swatchColor;
	{
		_walletLabel = walletLabel;
		_swatchColor = swatchColor;
	}
	optional<string> err_str = saveToDisk();
	if (err_str != none) {
		return std::move(*err_str);
	}
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_dispatch_ptr->async([weak_this, isChanging__walletLabel, isChanging__swatchColor]() {
		if (auto inner_spt = weak_this.lock()) {
			if (isChanging__walletLabel) {
				inner_spt->labelChanged_signal();
			}
			if (isChanging__swatchColor) {
				inner_spt->swatchColorChanged_signal();
			}
		}
	});
	return none;
}
//
// Imperatives - Local tx CRUD
void Object::_manuallyInsertTransactionRecord(
	const HostedMonero::HistoricalTxRecord &transaction
) {
	if (_transactions == none) {
		_transactions = std::vector<HostedMonero::HistoricalTxRecord>();
	}
	(*_transactions).push_back(transaction); // pushing a copy
	optional<string> err_str = saveToDisk();
	if (err_str != none) {
		return; // TODO: anything to do here? maybe saveToDisk should implement retry logic
	}
	// notify/yield
	___didReceiveActualChangeTo_transactions();
}



// TODO


////
//// Runtime (Booted) - Imperatives - Sending Funds
//func sendFunds(
//			   enteredAddressValue: MoneroAddress?, // currency-ready wallet address, but not an OpenAlias address (resolve before calling)
//			   resolvedAddress: MoneroAddress?,
//			   manuallyEnteredPaymentID: MoneroPaymentID?,
//			   resolvedPaymentID: MoneroPaymentID?,
//			   hasPickedAContact: Bool,
//			   resolvedAddress_fieldIsVisible: Bool,
//			   manuallyEnteredPaymentID_fieldIsVisible: Bool,
//			   resolvedPaymentID_fieldIsVisible: Bool,
//			   //
//			   contact_payment_id: MoneroPaymentID?,
//			   cached_OAResolved_address: String?,
//			   contact_hasOpenAliasAddress: Bool?,
//			   contact_address: String?,
//			   //
//			   raw_amount_string: String?, // human-understandable number, e.g. input 0.5 for 0.5 XMR
//			   isSweeping: Bool, // when true, amount will be ignored
//			   simple_priority: MoneroTransferSimplifiedPriority,
//			   //
//			   didUpdateProcessStep_fn: @escaping ((_ msg: String) -> Void),
//			   success_fn: @escaping (
//									  _ sentTo_address: MoneroAddress,
//									  _ isXMRAddressIntegrated: Bool,
//									  _ integratedAddressPIDForDisplay_orNil: MoneroPaymentID?,
//									  _ final_sentAmountWithoutFee: MoneroAmount,
//									  _ sentPaymentID_orNil: MoneroPaymentID?,
//									  _ tx_hash: MoneroTransactionHash,
//									  _ tx_fee: MoneroAmount,
//									  _ tx_key: MoneroTransactionSecKey,
//									  _ mockedTransaction: MoneroHistoricalTransactionRecord
//									  ) -> Void,
//			   canceled_fn: @escaping () -> Void,
//			   failWithErr_fn: @escaping (
//										  _ err_str: String
//										  ) -> Void
//			   ) {
//	if self.shouldDisplayImportAccountOption != nil && self.shouldDisplayImportAccountOption! {
//		failWithErr_fn(NSLocalizedString("This wallet must first be imported.", comment: ""))
//		return
//	}
//	func __isLocked() -> Bool { return self.isSendingFunds || self.submitter != nil }
//	if __isLocked() {
//		failWithErr_fn(NSLocalizedString("Currently sending funds. Please try again when complete.", comment: ""))
//		return // TODO nil
//	}
//	assert(self._current_sendFunds_request == nil)
//	let statusMessage_prefix = isSweeping
//	? NSLocalizedString("Sending wallet balance…", comment: "")
//	: String(
//			 format: NSLocalizedString("Sending %@ XMR…", comment: "Sending {amount} XMR…"),
//			 FormattedString(fromMoneroAmount: MoneroAmount.new( // converting it from string back to string so as to get the locale-specific separator character
//																withMoneyAmountDoubleString: raw_amount_string!
//																))
//			 )
//	self.submitter = SendFundsFormSubmissionHandle.init(_canceled_fn: { [weak self] in
//		guard let thisSelf = self else {
//			return
//		}
//		thisSelf.__unlock_sending()
//		canceled_fn()
//		thisSelf.submitter = nil // free
//	}, authenticate_fn: { [weak self] in
//		guard let thisSelf = self else {
//			return
//		}
//		PasswordController.shared.initiate_verifyUserAuthenticationForAction(
//																			 customNavigationBarTitle: NSLocalizedString("Authenticate", comment: ""),
//																			 canceled_fn: { [weak thisSelf] in
//																				 guard let thisThisSelf = thisSelf else {
//																					 return
//																				 }
//																				 thisThisSelf.submitter!.cb__authentication(false)
//																			 },
//																			 // all failures show in entry UI
//																			 entryAttempt_succeeded_fn: { [weak thisSelf] in
//																				 guard let thisThisSelf = thisSelf else {
//																					 return
//																				 }
//																				 thisThisSelf.submitter!.cb__authentication(true)
//																			 }
//																			 )
//	}, willBeginSending_fn: { [weak self] in
//		guard let thisSelf = self else {
//			return
//		}
//		thisSelf.__lock_sending()
//		didUpdateProcessStep_fn(statusMessage_prefix)
//	}, status_update_fn: { (processStep_code) in
//		let str = statusMessage_prefix + " " + Wallet.statusMessage_suffixesByCode[processStep_code]! // TODO: localize this concatenation
//		didUpdateProcessStep_fn(str)
//	}, get_unspent_outs_fn: { [weak self] (req_params_json_string) in
//		guard let thisSelf = self else {
//			return
//		}
//		var parameters: [String: Any]
//		do {
//			let json_data = req_params_json_string.data(using: .utf8)!
//			parameters = try JSONSerialization.jsonObject(with: json_data) as! [String: Any]
//		} catch let e {
//			fatalError("req_params_json_string parse error … \(e)")
//		}
//		thisSelf._current_sendFunds_request = HostedMonero.APIClient.shared.UnspentOuts(
//																						parameters: parameters,
//																						{ [weak thisSelf] (err_str, response_data) in
//																							guard let thisThisSelf = thisSelf else {
//																								return
//																							}
//																							thisThisSelf._current_sendFunds_request = nil
//																							var args_string: String? = nil
//																							if response_data != nil {
//																								args_string = String(data: response_data!, encoding: .utf8)
//																							}
//																							thisThisSelf.submitter!.cb_I__got_unspent_outs(err_str, args_string: args_string)
//																						}
//																						)
//	}, get_random_outs_fn: { [weak self] (req_params_json_string) in
//		guard let thisSelf = self else {
//			return
//		}
//		var parameters: [String: Any]
//		do {
//			let json_data = req_params_json_string.data(using: .utf8)!
//			parameters = try JSONSerialization.jsonObject(with: json_data) as! [String: Any]
//		} catch let e {
//			fatalError("req_params_json_string parse error … \(e)")
//		}
//		thisSelf._current_sendFunds_request = HostedMonero.APIClient.shared.RandomOuts(
//																					   parameters: parameters,
//																					   { [weak thisSelf] (err_str, response_data) in
//																						   guard let thisThisSelf = thisSelf else {
//																							   return
//																						   }
//																						   thisThisSelf._current_sendFunds_request = nil
//																						   var args_string: String? = nil
//																						   if response_data != nil {
//																							   args_string = String(data: response_data!, encoding: .utf8)
//																						   }
//																						   thisThisSelf.submitter!.cb_II__got_random_outs(err_str, args_string: args_string)
//																					   }
//																					   )
//	}, submit_raw_tx_fn: { [weak self] (req_params_json_string) in
//		guard let thisSelf = self else {
//			return
//		}
//		var parameters: [String: Any]
//		do {
//			let json_data = req_params_json_string.data(using: .utf8)!
//			parameters = try JSONSerialization.jsonObject(with: json_data) as! [String: Any]
//		} catch let e {
//			fatalError("req_params_json_string parse error … \(e)")
//		}
//		thisSelf._current_sendFunds_request = HostedMonero.APIClient.shared.SubmitSerializedSignedTransaction(
//																											  parameters: parameters,
//																											  { [weak thisSelf] (err_str, response_data) in
//																												  guard let thisThisSelf = thisSelf else {
//																													  return
//																												  }
//																												  thisThisSelf._current_sendFunds_request = nil
//																												  thisThisSelf.submitter!.cb_III__submitted_tx(err_str)
//																											  }
//																											  )
//	}, error_fn: { [weak self] (code, optl_errMsg, optl_createTx_errCode, optl__spendable_balance, optl__required_balance) in
//		guard let thisSelf = self else {
//			return
//		}
//		thisSelf.__unlock_sending()
//		var errStr: String?
//		if code == 0 { // msgProvided
//			errStr = optl_errMsg! // ought to exist…
//		} else if code == 11 { // errInServerResponse_withMsg
//			errStr = optl_errMsg!
//		} else if code == 12 { // createTransactionCode_balancesProvided
//			if optl_createTx_errCode == 90 { // needMoreMoneyThanFound
//				errStr = String(format:
//								NSLocalizedString("Spendable balance too low. Have %@ %@; need %@ %@.", comment: "Spendable balance too low. Have {amount} {XMR}; need {amount} {XMR}."),
//								FormattedString(fromMoneroAmount: MoneroAmount("\(optl__spendable_balance)")!),
//								MoneroConstants.currency_symbol,
//								FormattedString(fromMoneroAmount: MoneroAmount("\(optl__required_balance)")!),
//								MoneroConstants.currency_symbol
//								);
//			} else {
//				errStr = Wallet.createTxErrCodeMessage_byEnumVal[optl_createTx_errCode]!
//			}
//		} else if code == 13 { // createTranasctionCode_noBalances
//			errStr = Wallet.createTxErrCodeMessage_byEnumVal[optl_createTx_errCode]!
//		} else {
//			errStr = Wallet.failureCodeMessage_byEnumVal[code]
//		}
//		failWithErr_fn(errStr!)
//		thisSelf.submitter = nil // free
//	}, success_fn: { [weak self] (used_fee, total_sent, mixin, optl__final_payment_id, signed_serialized_tx_string, tx_hash_string, tx_key_string, tx_pub_key_string, target_address, final_total_wo_fee, isXMRAddressIntegrated, optl__integratedAddressPIDForDisplay) in
//		guard let thisSelf = self else {
//			return
//		}
//		thisSelf.__unlock_sending()
//		//
//		var outgoingAmountForDisplay = MoneroAmount.init("\(final_total_wo_fee + used_fee)")!
//		outgoingAmountForDisplay.sign = .minus // make negative as it's outgoing
//		//
//		let mockedTransaction = MoneroHistoricalTransactionRecord(
//																  amount: outgoingAmountForDisplay,
//																  totalSent: MoneroAmount.init("\(final_total_wo_fee + used_fee)")!,
//																  totalReceived: MoneroAmount("0"),
//																  approxFloatAmount: DoubleFromMoneroAmount(moneroAmount: outgoingAmountForDisplay),
//																  spent_outputs: nil, // TODO: is this ok?
//																  timestamp: Date(), // faking this
//																  hash: tx_hash_string,
//																  paymentId: optl__final_payment_id ?? optl__integratedAddressPIDForDisplay, // transaction.paymentId will be nil for integrated addresses but we show it here anyway and, in the situation where they used a std xmr addr and a short pid, an int addr would get fabricated anyway, leaving sentWith_paymentID nil even though user is expecting a pid - so we want to make sure it gets saved in either case
//																  mixin: MyMoneroCore.fixedMixin,
//																  //
//																  mempool: true, // is this correct?
//																  unlock_time: 0,
//																  height: nil, // mocking the initial value -not- to exist (rather than to erroneously be 0) so that isconfirmed -> false
//																  //
//																  //					coinbase: false, // TODO
//																  //
//																  isFailed: nil, // since we've just created it
//																  //
//																  cached__isConfirmed: false, // important
//																  cached__isUnlocked: true, // TODO: not sure about this
//																  cached__lockedReason: nil,
//																  //
//																  isJustSentTransientTransactionRecord: true,
//																  //
//																  tx_key: tx_key_string,
//																  tx_fee: MoneroAmount.init("\(used_fee)")!,
//																  to_address: target_address
//																  //				contact: hasPickedAContact ? self.pickedContact : null, // TODO?
//																  )
//		success_fn(
//				   target_address,
//				   isXMRAddressIntegrated,
//				   optl__integratedAddressPIDForDisplay,
//				   MoneroAmount.init("\(final_total_wo_fee)")!,
//				   optl__final_payment_id,
//				   tx_hash_string,
//				   MoneroAmount.init("\(used_fee)")!,
//				   tx_key_string,
//				   mockedTransaction
//				   )
//		// manually insert .. and subsequent fetches from the server will be
//		// diffed against this, preserving the tx_fee, tx_key, to_address...
//		thisSelf._manuallyInsertTransactionRecord(mockedTransaction);
//		//
//		thisSelf.submitter = nil // free
//	})
//	self.submitter!.setupWith_fromWallet_didFail(
//												 toInitialize: self.didFailToInitialize_flag == true,
//												 fromWallet_didFailToBoot: self.didFailToBoot_flag == true,
//												 fromWallet_needsImport: self.shouldDisplayImportAccountOption == true,
//												 requireAuthentication: SettingsController.shared.authentication__requireWhenSending != false,
//												 sending_amount_double_NSString: raw_amount_string,
//												 is_sweeping: isSweeping,
//												 priority: simple_priority.cppRepresentation,
//												 hasPickedAContact: hasPickedAContact,
//												 optl__contact_payment_id: contact_payment_id,
//												 optl__contact_hasOpenAliasAddress: contact_hasOpenAliasAddress ?? false,
//												 optl__cached_OAResolved_address: cached_OAResolved_address,
//												 optl__contact_address: contact_address,
//												 nettype: MM_MAINNET,
//												 from_address_string: self.public_address,
//												 sec_viewKey_string: self.private_keys.view,
//												 sec_spendKey_string: self.private_keys.spend,
//												 pub_spendKey_string: self.public_keys.spend,
//												 optl__enteredAddressValue: enteredAddressValue,
//												 optl__resolvedAddress: resolvedAddress,
//												 resolvedAddress_fieldIsVisible: resolvedAddress_fieldIsVisible,
//												 optl__manuallyEnteredPaymentID: manuallyEnteredPaymentID,
//												 manuallyEnteredPaymentID_fieldIsVisible: manuallyEnteredPaymentID_fieldIsVisible,
//												 optl__resolvedPaymentID: resolvedPaymentID,
//												 resolvedPaymentID_fieldIsVisible: resolvedPaymentID_fieldIsVisible
//												 )
//	self.submitter!.handle()
//}
void Object::__lock_sending()
{
	_isSendingFunds = true;
	//
	_userIdleController->temporarilyDisable_userIdle();
//	ScreenSleep.temporarilyDisable_screenSleep() // TODO: implement bridge notifier somehow
}
void Object::__unlock_sending()
{
	_isSendingFunds = false;
	//
	_userIdleController->reEnable_userIdle();
//	ScreenSleep.reEnable_screenSleep() // TODO: implement bridge notifier somehow
}
//
// HostPollingController - Delegation / Protocol
void Object::_HostPollingController_didFetch_addressInfo(
	const HostedMonero::ParsedResult_AddressInfo &parsedResult
) {
	auto xmrToCcyRatesByCcy = parsedResult.xmrToCcyRatesByCcy; // copy
	std::shared_ptr<Object> shared_this = shared_from_this();
	std::weak_ptr<Object> weak_this = shared_this;
	_dispatch_ptr->async([
		weak_this, xmrToCcyRatesByCcy
	] { // just to let wallet stuff finish first
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->_ccyConversionRatesController->set_xmrToCcyRatesByCcy(xmrToCcyRatesByCcy);
		}
	});
	bool didActuallyChange_accountBalance = (_totalReceived == none || parsedResult.totalReceived != *_totalReceived)
		|| (_totalSent == none || parsedResult.totalSent != *_totalSent)
		|| (_lockedBalance == none || parsedResult.lockedBalance != *_lockedBalance);
	_totalReceived = parsedResult.totalReceived;
	_totalSent = parsedResult.totalSent;
	_lockedBalance = parsedResult.lockedBalance;
	//
	bool didActuallyChange_spentOutputs = _spentOutputs == none || (parsedResult.spentOutputs != *_spentOutputs);
	_spentOutputs = std::move(parsedResult.spentOutputs);
	//
	bool didActuallyChange_heights = (_account_scanned_tx_height == none || *_account_scanned_tx_height != parsedResult.account_scanned_tx_height)
		|| (_account_scanned_block_height == none || *_account_scanned_block_height != parsedResult.account_scanned_block_height)
		|| (_account_scan_start_height == none || *_account_scan_start_height != parsedResult.account_scan_start_height)
		|| (_transaction_height == none || *_transaction_height != parsedResult.transaction_height)
		|| (_blockchain_height == none || *_blockchain_height != parsedResult.blockchain_height);
	_account_scanned_tx_height = parsedResult.account_scanned_tx_height;
	_account_scanned_block_height = parsedResult.account_scanned_block_height;
	_account_scan_start_height = parsedResult.account_scan_start_height;
	_transaction_height = parsedResult.transaction_height;
	_blockchain_height = parsedResult.blockchain_height;
	//
	bool wasFirstFetchOf_accountInfo = _dateThatLast_fetchedAccountInfo == none;
	_dateThatLast_fetchedAccountInfo = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	//
	// Write:
	optional<string> err_str = saveToDisk();
	if (err_str != none) {
		return; // there was an issue saving update… TODO: silence here ok for now?
	}
	//
	// Now notify/emit/yield any actual changes
	bool anyChanges = false; // to finalize
	if (didActuallyChange_accountBalance || wasFirstFetchOf_accountInfo) {
		anyChanges = true;
		___didReceiveActualChangeTo_balance();
	}
	if (didActuallyChange_spentOutputs || wasFirstFetchOf_accountInfo) {
		anyChanges = true;
		___didReceiveActualChangeTo_spentOutputs();
	}
	if (didActuallyChange_heights || wasFirstFetchOf_accountInfo) {
		anyChanges = true;
		regenerate_shouldDisplayImportAccountOption(); // scan start height may have changed
		___didReceiveActualChangeTo_heights();
	}
	if (anyChanges == false) {
//		MDEBUG("No actual changes to balance, heights, or spent outputs");
	}
}
void Object::_HostPollingController_didFetch_addressTransactions(
	const HostedMonero::ParsedResult_AddressTransactions &parsedResult
) {
	bool didActuallyChange_heights = (_account_scanned_height == none || *_account_scanned_height != parsedResult.account_scanned_height)
		|| (_account_scanned_block_height == none || *_account_scanned_block_height != parsedResult.account_scanned_block_height)
		|| (_account_scan_start_height == none || *_account_scan_start_height != parsedResult.account_scan_start_height)
		|| (_transaction_height == none || *_transaction_height != parsedResult.transaction_height)
		|| (_blockchain_height == none || *_blockchain_height != parsedResult.blockchain_height);
	_account_scanned_height = parsedResult.account_scanned_height;
	_account_scanned_block_height = parsedResult.account_scanned_block_height;
	_account_scan_start_height = parsedResult.account_scan_start_height;
	_transaction_height = parsedResult.transaction_height;
	_blockchain_height = parsedResult.blockchain_height;
	//
	bool didActuallyChange_transactions = false; // we'll see if anything actually changed and only emit if so
	// We will construct the txs from the incoming txs here as follows.
	// Doing this allows us to selectively preserve already-cached info.
	size_t numberOfTransactionsAdded = 0; // to be finalized…
	//		var newTransactions = [MoneroHistoricalTransactionRecord]()
	std::vector<HostedMonero::HistoricalTxRecord> existing_transactions; // to be finalized…
	if (_transactions != none) {
		existing_transactions = std::move(*_transactions); // copy! (or rather, a move … because we're just about to reconstruct it)
	}
	//
	// Always make sure to construct new array so we have the old set
	std::unordered_map<string, HostedMonero::HistoricalTxRecord> txs_by_hash;
	//
	//
	// TODO: optimize this by using raw ptrs or smart ptrs
	//
	//
	
	for (auto it = existing_transactions.begin(); it != existing_transactions.end(); it++) {
		// in JS here we delete the 'id' field but we don't have it in Swift - in JS, the comment is: "not expecting an id but just in case .. so we don't break diffing"
		txs_by_hash[(*it).hash] = *it; // start with old one
	}
	for (auto incoming_tx__it = parsedResult.transactions.begin(); incoming_tx__it != parsedResult.transactions.end(); incoming_tx__it++) {
		// in JS here we delete the 'id' field but we don't have it in Swift - in JS, the comment is: "because this field changes while sending funds, even though hash stays the same, and because we don't want `id` messing with our ability to diff. so we're not even going to try to store this"
		std::unordered_map<
			string,
			HostedMonero::HistoricalTxRecord
		>::const_iterator existing_tx__it = txs_by_hash.find((*incoming_tx__it).hash);
		bool isNewTransaction = existing_tx__it == txs_by_hash.end();
		// ^- If any existing tx is also in incoming txs, this will cause
		// the (correct) deletion of e.g. isJustSentTransaction=true.
		HostedMonero::HistoricalTxRecord final_incoming_tx = *incoming_tx__it; // a mutable copy
		if (isNewTransaction) { // This is generally now only going to be hit when new incoming txs happen - or outgoing txs done on other logins
			didActuallyChange_transactions = true;
			numberOfTransactionsAdded += 1;
		} else {
			if (*incoming_tx__it != existing_tx__it->second) {
				didActuallyChange_transactions = true; // this is likely to happen if tx.height changes while pending confirmation
			}
			// Check if existing tx has any cached info which we
			// want to bring into the finalized_tx before setting;
			if ((existing_tx__it->second).tx_key != none) {
				final_incoming_tx.tx_key = std::move(*((existing_tx__it->second).tx_key));
			}
			if ((existing_tx__it->second).to_address != none) {
				final_incoming_tx.to_address = std::move(*((existing_tx__it->second).to_address));
			}
			if ((existing_tx__it->second).tx_fee != none) {
				final_incoming_tx.tx_fee = (existing_tx__it->second).tx_fee;
			}
			if ((*incoming_tx__it).paymentId == none || (*incoming_tx__it).paymentId->size() > 0) {
				if ((existing_tx__it->second).paymentId != none) {
					final_incoming_tx.paymentId = std::move(*((existing_tx__it->second).paymentId)); // if the tx lost it.. say, while it's being scanned, keep pid
				}
			}
			if ((*incoming_tx__it).mixin == none || *((*incoming_tx__it).mixin) == 0) {
				if ((existing_tx__it->second).mixin != none && *(existing_tx__it->second).mixin != 0) {
					final_incoming_tx.mixin = (existing_tx__it->second).mixin; // if the tx lost it.. say, while it's being scanned, keep mixin
				}
			}
			if ((*incoming_tx__it).mempool == true) { // since the server has an issue sending the spent outputs at present, and only sends the (positive) change amount, this is a workaround to always prefer the existing cached tx's amounts rather than the ones sent by the server 
				// NOTE: This will also apply to *incoming* txs just due to the naiveness of the logic
				final_incoming_tx.totalSent = (existing_tx__it->second).totalSent;
				final_incoming_tx.totalReceived = (existing_tx__it->second).totalReceived;
				final_incoming_tx.amount = (existing_tx__it->second).amount;
				final_incoming_tx.approxFloatAmount = (existing_tx__it->second).approxFloatAmount;
			}
		}
		// always overwrite existing ones:
		txs_by_hash[(*incoming_tx__it).hash] = final_incoming_tx;
		// Commented b/c we don't use this yet:
//		if isNewTransaction { // waiting so we have the finalized incoming_tx obj
//			newTransactions.append(finalized_incoming_tx)
//		}
	}
	//
	std::vector<HostedMonero::HistoricalTxRecord> finalized_transactions;
	for (auto it = txs_by_hash.begin(); it != txs_by_hash.end(); it++) {
		finalized_transactions.push_back(it->second); // TODO: this is also a copy …… optimize this by using a pointer
	}
	sort(finalized_transactions.begin(), finalized_transactions.end(), HostedMonero::sorting_historicalTxRecords_byTimestamp_walletsList);
	//
	_transactions = std::move(finalized_transactions);
	//
	bool wasFirstFetchOf_transactions = _dateThatLast_fetchedAccountTransactions == none;
	_dateThatLast_fetchedAccountTransactions = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	//
	// Write:
	optional<string> err_str = saveToDisk();
	if (err_str != none) {
		return; // there was an issue saving update… TODO: silence here ok for now?
	}
	//
	// Now notify/emit/yield any actual changes
	if (didActuallyChange_transactions || wasFirstFetchOf_transactions) {
		___didReceiveActualChangeTo_transactions();
	}
	if (didActuallyChange_heights || wasFirstFetchOf_transactions) {
		regenerate_shouldDisplayImportAccountOption(); // scan start height may have changed
		___didReceiveActualChangeTo_heights();
	}
}
//
// Delegation - Internal - Data value property update events
void Object::___didReceiveActualChangeTo_balance()
{
	balanceChanged_signal();
}
void Object::___didReceiveActualChangeTo_spentOutputs()
{
	spentOutputsChanged_signal();
}
void Object::___didReceiveActualChangeTo_heights()
{
	heightsUpdated_signal();
}
void Object::___didReceiveActualChangeTo_transactions()
{
	transactionsChanged_signal();
}


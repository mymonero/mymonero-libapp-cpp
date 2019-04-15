//
//  WalletsListController_Base.cpp
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
#include "WalletsListController_Base.hpp"
#include <boost/foreach.hpp>
#include "misc_log_ex.h"
#include "../mymonero-core-cpp/src/monero_wallet_utils.hpp"
using namespace Wallets;
//
// Lifecycle - Init
void ListController_Base::setup()
{
	if (documentsPath == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected documentsPath != nullptr"));
	}
	// check deps *first* before calling on super
	Lists::Controller::setup();
}
void ListController_Base::setup_startObserving()
{
	Lists::Controller::setup_startObserving();
	//
	connection__HostedMonero_initializedWithNewServerURL = apiClient->initializedWithNewServerURL_signal.connect(
		std::bind(&ListController_Base::HostedMonero_initializedWithNewServerURL, this)
	);
}
void ListController_Base::stopObserving()
{
	Lists::Controller::stopObserving();
	//
	connection__HostedMonero_initializedWithNewServerURL.disconnect();
}
//
// Accessors - Derived properties
std::vector<Wallets::SwatchColor> ListController_Base::givenBooted_swatchesInUse()
{
	if (_hasBooted != true) {
		BOOST_THROW_EXCEPTION(logic_error("givenBooted_swatchesInUse called when \(self) not yet booted."));
		return std::vector<Wallets::SwatchColor>(); // this may be for the first wallet creation - let's say nothing in use yet
	}
	std::vector<Wallets::SwatchColor> inUseSwatches;
	for (std::vector<std::shared_ptr<Persistable::Object>>::iterator it = _records.begin(); it != _records.end(); ++it) {
		inUseSwatches.push_back(std::dynamic_pointer_cast<Wallets::Object>(*it)->swatchColor());
	}
	return inUseSwatches;
}
//
// Booted - Imperatives - Public - Wallets list
void ListController_Base::CreateNewWallet_NoBootNoListAdd(
	string localeCode,
	std::function<void(optional<string> err, std::shared_ptr<Wallets::Object> walletInstance)> fn
) {
	monero_wallet_utils::WalletDescriptionRetVals retVals;
	bool r = monero_wallet_utils::convenience__new_wallet_with_language_code(
		localeCode,
		retVals,
		_nettype
	);
	bool did_error = retVals.did_error;
	if (!r) {
		fn(std::move(*(retVals.err_string)), nullptr);
		return;
	}
	if (did_error) {
		string err_str = "Illegal success flag but did_error";
		BOOST_THROW_EXCEPTION(logic_error(err_str));
		return;
	}
	fn(none, std::make_shared<Wallets::Object>(
		documentsPath,
		passwordController,
		std::move(*(retVals.optl__desc)),
		_nettype,
		apiClient,
		dispatch_ptr
	));
}
void ListController_Base::OnceBooted_ObtainPW_AddNewlyGeneratedWallet(
	std::shared_ptr<Wallets::Object> walletInstance,
	string walletLabel,
	Wallets::SwatchColor swatchColor,
	std::function<void(optional<string> err_str, std::shared_ptr<Wallets::Object> walletInstance)>&& fn,
	std::function<void()>&& userCanceledPasswordEntry_fn
) {
	std::weak_ptr<Wallets::Object> weak_wallet(walletInstance);
	std::shared_ptr<ListController_Base> shared_this = shared_from_this();
	std::weak_ptr<ListController_Base> weak_this = shared_this;
	onceBooted([
		weak_this, weak_wallet, walletLabel, swatchColor,
		fn = std::move(fn), userCanceledPasswordEntry_fn = std::move(userCanceledPasswordEntry_fn)
	] () {
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->passwordController->onceBootedAndPasswordObtained([
				weak_this, weak_wallet, walletLabel, swatchColor,
				 fn = std::move(fn), userCanceledPasswordEntry_fn = std::move(userCanceledPasswordEntry_fn)
				] (
					Passwords::Password password,
					Passwords::Type type
				) {
					if (auto inner_inner_spt = weak_this.lock()) {
						auto walletInstance = weak_wallet.lock();
						if (walletInstance) {
							walletInstance->Boot_byLoggingIn_givenNewlyCreatedWallet(
								walletLabel,
								swatchColor,
								[weak_this, fn = std::move(fn), weak_wallet](optional<string> err_str)
								{
									if (err_str != none) {
										fn(*err_str, nullptr);
										return;
									}
									if (auto inner_inner_inner_spt = weak_this.lock()) {
										auto walletInstance = weak_wallet.lock();
										if (walletInstance) {
											inner_inner_inner_spt->_atRuntime__record_wasSuccessfullySetUp(walletInstance);
											//
											fn(none, walletInstance);
										}
										MWARNING("Wallet instance freed during Boot_byLoggingIn_givenNewlyCreatedWallet");
									}
								}
							);
						} else {
							MWARNING("Wallet instance freed during onceBootedAndPasswordObtained");
						}
					}
				},
				[
					userCanceledPasswordEntry_fn = std::move(userCanceledPasswordEntry_fn)
				] (void) { // user canceled
					userCanceledPasswordEntry_fn();
				}
			);
		}
	});
}
//void OnceBooted_ObtainPW_AddExtantWalletWith_MnemonicString(
//															walletLabel: String,
//															swatchColor: Wallet.SwatchColor,
//															mnemonicString: MoneroSeedAsMnemonic,
//															_ fn: @escaping (
//																			 _ err_str: String?,
//																			 _ walletInstance: Wallet?,
//																			 _ wasWalletAlreadyInserted: Bool?
//																			 ) -> Void,
//															userCanceledPasswordEntry_fn: (() -> Void)? = {}
//															) -> Void {
//	self.onceBooted({ [unowned self] in
//		PasswordController.shared.OnceBootedAndPasswordObtained( // this will 'block' until we have access to the pw
//																{ [unowned self] (password, passwordType) in
//																	do { // check if wallet already entered
//																		for (_, record) in self.records.enumerated() {
//																			let wallet = record as! Wallet
//																			guard let wallet_mnemonicString = wallet.mnemonicString else {
//																				// TODO: solve limitation of this code - check if wallet with same address (but no mnemonic) was already added
//																				continue
//																			}
//																			let areMnemonicsEqual = MyMoneroCore.shared_objCppBridge.areEqualMnemonics(
//																																					   wallet_mnemonicString,
//																																					   mnemonicString
//																																					   )
//																			if areMnemonicsEqual { // must use this comparator to support partial-word mnemomnic strings
//																				fn(nil, wallet, true) // wasWalletAlreadyInserted: true
//																				return
//																			}
//																		}
//																	}
//																	do {
//																		guard let wallet = try Wallet(ifGeneratingNewWallet_walletDescription: nil) else {
//																			fn("Unknown error while adding wallet.", nil, nil)
//																			return
//																		}
//																		wallet.Boot_byLoggingIn_existingWallet_withMnemonic(
//																															walletLabel: walletLabel,
//																															swatchColor: swatchColor,
//																															mnemonicString: mnemonicString,
//																															persistEvenIfLoginFailed_forServerChange: false, // not forServerChange
//																															{ [unowned self] (err_str) in
//																																if err_str != nil {
//																																	fn(err_str, nil, nil)
//																																	return
//																																}
//																																self._atRuntime__record_wasSuccessfullySetUp(wallet)
//																																fn(nil, wallet, false) // wasWalletAlreadyInserted: false
//																															}
//																															)
//																	} catch let e {
//																		fn(e.localizedDescription, nil, nil)
//																		return
//																	}
//																},
//																{ // user canceled
//																	(userCanceledPasswordEntry_fn ?? {})()
//																}
//																)
//	})
//}
//void OnceBooted_ObtainPW_AddExtantWalletWith_AddressAndKeys(
//															walletLabel: String,
//															swatchColor: Wallet.SwatchColor,
//															address: MoneroAddress,
//															privateKeys: MoneroKeyDuo,
//															_ fn: @escaping (
//																			 _ err_str: String?,
//																			 _ wallet: Wallet?,
//																			 _ wasWalletAlreadyInserted: Bool?
//																			 ) -> Void,
//															userCanceledPasswordEntry_fn: (() -> Void)? = {}
//															) {
//	self.onceBooted({ [unowned self] in
//		PasswordController.shared.OnceBootedAndPasswordObtained( // this will 'block' until we have access to the pw
//																{ [unowned self] (password, passwordType) in
//																	do {
//																		for (_, record) in self.records.enumerated() {
//																			let wallet = record as! Wallet
//																			if wallet.public_address == address {
//																				// simply return existing wallet; note: this wallet might have mnemonic and thus seed
//																				// so might not be exactly what consumer of GivenBooted_ObtainPW_AddExtantWalletWith_AddressAndKeys is expecting
//																				fn(nil, wallet, true) // wasWalletAlreadyInserted: true
//																				return
//																			}
//																		}
//																	}
//																	do {
//																		guard let wallet = try Wallet(ifGeneratingNewWallet_walletDescription: nil) else {
//																			fn("Unknown error while adding wallet.", nil, nil)
//																			return
//																		}
//																		wallet.Boot_byLoggingIn_existingWallet_withAddressAndKeys(
//																																  walletLabel: walletLabel,
//																																  swatchColor: swatchColor,
//																																  address: address,
//																																  privateKeys: privateKeys,
//																																  persistEvenIfLoginFailed_forServerChange: false, // not forServerChange
//																																  { [unowned self] (err_str) in
//																																	  if err_str != nil {
//																																		  fn(err_str, nil, nil)
//																																		  return
//																																	  }
//																																	  self._atRuntime__record_wasSuccessfullySetUp(wallet)
//																																	  fn(nil, wallet, false) // wasWalletAlreadyInserted: false
//																																  }
//																																  )
//																	} catch let e {
//																		fn(e.localizedDescription, nil, nil)
//																		return
//																	}
//																},
//																{ // user canceled
//																	(userCanceledPasswordEntry_fn ?? {})()
//																}
//																)
//	})
//}
//
// Delegation - Signals
void ListController_Base::HostedMonero_initializedWithNewServerURL()
{
	// 'log out' all wallets by deleting their runtime state, then reboot them
	if (_hasBooted == false) {
		if (_records.size() != 0) {
			BOOST_THROW_EXCEPTION(logic_error("Expected _records.size == 0"));
		}
		return; // nothing to do
	}
	if (_records.size() == 0) {
		return; // nothing to do
	}
	if (passwordController->hasUserEnteredValidPasswordYet() == false) {
		BOOST_THROW_EXCEPTION(logic_error("App expected password to exist as wallets exist"));
		return;
	}
	for (std::vector<std::shared_ptr<Persistable::Object>>::iterator it = _records.begin(); it != _records.end(); ++it) {
		std::dynamic_pointer_cast<Wallets::Object>(*it)->logOutThenSaveAndLogIn();
	}
	__dispatchAsync_listUpdated_records(); // probably not necessary
}

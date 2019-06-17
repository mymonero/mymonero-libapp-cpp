//
//  WalletsListController.Lite.cpp
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

#ifndef WalletsListController_hpp // here we go with WalletsListController_hpp instead of WalletsListController_Full_hpp since only one, Full or Lite, should be included
#define WalletsListController_hpp

#include "./WalletsListController_Base.hpp"

namespace Wallets
{
	using namespace std;
	using namespace boost;
	//
	// Controllers
	class ListController: public ListController_Base
	{
	public:
		//
		// Lifecycle - Init
		ListController(cryptonote::network_type nettype):
			ListController_Base(nettype)
		{
		}
		~ListController() {
			cout << "Destructor for Wallets::ListController_Lite" << endl;
			tearDown();
		}
		//
		// Dependencies

		// Overrides
		virtual void CreateNewWallet_NoBootNoListAdd(
			string localeCode,
			std::function<void(
				optional<string> err,
				std::shared_ptr<Wallets::Object> walletInstance
			)> fn
		) override {
			if (_records.size() > 0) {
				fn(string("Browser app only supports one wallet at a time"), nullptr);
				// 'Lite' mode available in the browser ... this string needs to get localized though …… return error code instead?
				return;
			}
			ListController_Base::CreateNewWallet_NoBootNoListAdd(
				std::move(localeCode),
				std::move(fn)
			);
		}
		virtual void OnceBooted_ObtainPW_AddNewlyGeneratedWallet_externallyTmpRetained(
			std::shared_ptr<Wallets::Object> walletInstance,
			string walletLabel,
			Wallets::SwatchColor swatchColor,
			std::function<void(optional<string> err_str, std::shared_ptr<Wallets::Object> walletInstance)>&& fn,
			std::function<void()>&& userCanceledPasswordEntry_fn = {}
		) override {
			if (_records.size() > 0) {
				fn(string("Browser app only supports one wallet at a time"), nullptr);
				// 'Lite' mode available in the browser ... this string needs to get localized though …… return error code instead?
				return;
			}
			ListController_Base::OnceBooted_ObtainPW_AddNewlyGeneratedWallet_externallyTmpRetained(
				walletInstance,
				std::move(walletLabel),
				swatchColor,
				std::move(fn),
				std::move(userCanceledPasswordEntry_fn)
			);
		}
		virtual void OnceBooted_ObtainPW_AddExtantWalletWith_MnemonicString(
			string walletLabel,
			Wallets::SwatchColor swatchColor,
			string mnemonicString,
			std::function<void(
				optional<string> err_str,
				std::shared_ptr<Wallets::Object> walletInstance,
				optional<bool> wasWalletAlreadyInserted
			)> fn,
			std::function<void()> userCanceledPasswordEntry_fn = {}
		) override {
			if (_records.size() > 0) {
				fn(string("Browser app only supports one wallet at a time"), nullptr, none);
				// 'Lite' mode available in the browser ... this string needs to get localized though …… return error code instead?
				return;
			}
			ListController_Base::OnceBooted_ObtainPW_AddExtantWalletWith_MnemonicString(
				std::move(walletLabel),
				swatchColor,
				std::move(mnemonicString),
				std::move(fn),
				std::move(userCanceledPasswordEntry_fn)
			);
		}
		virtual void OnceBooted_ObtainPW_AddExtantWalletWith_AddressAndKeys(
			string walletLabel,
			Wallets::SwatchColor swatchColor,
			string address,
			string sec_view_key,
			string sec_spend_key,
			std::function<void(
				optional<string> err_str,
				std::shared_ptr<Wallets::Object> walletInstance,
				optional<bool> wasWalletAlreadyInserted
			)> fn,
			std::function<void()> userCanceledPasswordEntry_fn = {}
		) override {
			if (_records.size() > 0) {
				fn(string("Browser app only supports one wallet at a time"), nullptr, none);
				// 'Lite' mode available in the browser ... this string needs to get localized though …… return error code instead?
				return;
			}
			ListController_Base::OnceBooted_ObtainPW_AddExtantWalletWith_AddressAndKeys(
				std::move(walletLabel),
				swatchColor,
				std::move(address),
				std::move(sec_view_key),
				std::move(sec_spend_key),
				std::move(fn),
				std::move(userCanceledPasswordEntry_fn)
			);
		}
		//
		// Properties

		//
		// Signals

		//
		// Imperatives

	private:
		//
		// Properties

		//
		// Imperatives

	};
}

#endif /* WalletsListController_hpp */

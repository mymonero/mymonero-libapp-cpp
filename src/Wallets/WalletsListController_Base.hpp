//
//  WalletsListController_Base.hpp
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

#ifndef WalletsListController_Base_hpp
#define WalletsListController_Base_hpp

#include <iostream>
#include <boost/signals2.hpp>
#include <mutex>
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Settings/SettingsProviders.hpp"
#include "../Lists/PersistedObjectListController.hpp"
#include "./Wallet.hpp"
#include "../APIClient/HostedMonero.hpp"
#include "cryptonote_config.h"
//
//
namespace Wallets
{
	using namespace std;
	using namespace boost;
	//
	// Controllers
	class ListController_Base: public Lists::Controller
	{
	public:
		ListController_Base(cryptonote::network_type nettype):
			Lists::Controller(Wallets::collectionName),
			_nettype(nettype)
		{
		}
		virtual ~ListController_Base() {}
		//
		// Overrides
		std::shared_ptr<Persistable::Object> new_record(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
			const document_persister::DocumentJSON &plaintext_documentJSON
		) override {
			if (apiClient == nullptr) {
				BOOST_THROW_EXCEPTION(logic_error("Expected non-nullptr apiClient"));
			}
			return std::make_shared<Wallets::Object>(
				documentsPath,
				passwordProvider,
				plaintext_documentJSON,
				_nettype,
				apiClient,
				dispatch_ptr
			);
		}
		//
		// Dependencies
		std::shared_ptr<HostedMonero::APIClient> apiClient;
		//
		// Lifecycle
		void setup();
		//
		// Properties
		
		//
		// Signals
		//
		// Accessors - Virtual overrides
		bool overridable_shouldSortOnEveryRecordAdditionAtRuntime() override
		{
			return true;
		}
		bool overridable_wantsRecordsAppendedNotPrepended() override
		{
			return true;
		}
		void overridable_finalizeAndSortRecords() override
		{
			sort(
				_records.begin(), _records.end(),
				Lists::comparePersistableObjectSharedPtrBy_insertedAt_asc
			);
		}
		//
		// Accessors
		std::vector<Wallets::SwatchColor> givenBooted_swatchesInUse();
		
		//
		// Imperatives
		void CreateNewWallet_NoBootNoListAdd( // call this first, then call OnceBooted_ObtainPW_AddNewlyGeneratedWallet
			string localeCode,
			std::function<void(optional<string> err, std::shared_ptr<Wallets::Object> walletInstance)> fn
		);
		void OnceBooted_ObtainPW_AddNewlyGeneratedWallet(
			std::shared_ptr<Wallets::Object> walletInstance,
			string walletLabel,
			Wallets::SwatchColor swatchColor,
			std::function<void(optional<string> err_str, std::shared_ptr<Wallets::Object> walletInstance)>&& fn,
			std::function<void()>&& userCanceledPasswordEntry_fn = {} // default
		);
		//
		// Delegation - Overrides - Booting reconstitution - Instance setup
		void overridable_booting_didReconstitute(
			std::shared_ptr<Persistable::Object> listedObjectInstance
		) override {
			//
			// TODO:
			//
//			let wallet = listedObjectInstance as! Wallet
//			if wallet.isLoggedIn {
//				wallet.Boot_havingLoadedDecryptedExistingInitDoc(
//																 { err_str in
//																	 if let err_str = err_str {
//																		 DDLog.Error("Wallets", "Error while booting wallet: \(err_str)")
//																	 }
//																 }
//																 )
//			} else {
//				assert(wallet.isLoggingIn == false) // jic
//				DDLog.Do("Wallets", "Wallet which was unable to log in was loaded. Attempting to reboot.")
//				// going to treat this as a wallet which was saved but which failed to log in
//				wallet.logOutThenSaveAndLogIn() // this method can handle being called when the wallet is not logged in
//			}
		}
	private:
		//
		// Properties
		const cryptonote::network_type _nettype;
		boost::signals2::connection connection__HostedMonero_initializedWithNewServerURL;
		//
		// Lifecycle
		void setup_startObserving();
		void stopObserving();
		//
		// Imperatives
		//
		// Delegation
		void HostedMonero_initializedWithNewServerURL();
	};
}

#endif /* WalletsListController_Base_hpp */

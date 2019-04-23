//
//  SettingsController.cpp
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

#ifndef SettingsController_hpp
#define SettingsController_hpp

#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include <boost/uuid/uuid.hpp> // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <memory>
#include <mutex>
#include "../Persistence/document_persister.hpp"
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Passwords/PasswordController.hpp"
#include "./SettingsProviders.hpp"
#include "../Currencies/Currencies.hpp"
//
namespace Settings
{
	using namespace std;
	using namespace boost;
	using namespace document_persister;
	//
	// Constants
	enum Settings_DictKey
	{
		__min = 0,
		//
		_id = 1,
		specificAPIAddressURLAuthority = 2,
		appTimeoutAfterS_nilForDefault_orNeverValue = 3,
		displayCurrencySymbol = 4,
		authentication__requireWhenSending = 5,
		authentication__requireToShowWalletSecrets = 6,
		authentication__tryBiometric = 7,
		//
		__max = 8
	};
	static Currencies::CurrencySymbol default_displayCurrencySymbol = Currencies::CurrencySymbolFrom(Currencies::Currency::XMR);
	static bool default_authentication__requireWhenSending = true;
	static bool default_authentication__requireToShowWalletSecrets = true;
	static bool default_authentication__tryBiometric = true;
	//
	// Accessory types
	typedef boost::variant<
		optional<double>,
		bool,
		optional<Currencies::CurrencySymbol>,
		optional<string>
	> prop_val_arg_type;
	//
	// Controllers
	class Controller:
		public IdleTimeoutAfterS_SettingsProvider,
		public Passwords::DeleteEverythingRegistrant,
		public Passwords::ChangePasswordRegistrant,
		public std::enable_shared_from_this<Controller>
	{
	public:
		//
		// Lifecycle - Init
		Controller() {
			// set dependencies then call setup()
		}
		Controller(const Controller&) = delete; // disable copy constructor to prevent inadvertent temporary in pointer
		Controller& operator=(const Controller&) = delete;
		~Controller() {
			cout << "Destructing a Settings::Controller" << endl;
			teardown();
		}
		//
		// Dependencies
		std::shared_ptr<string> documentsPath;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
		std::shared_ptr<Passwords::Controller> passwordController;
		// Then call:
		void setup();
		//
		// Signals
		boost::signals2::signal<void()> specificAPIAddressURLAuthority_signal;
		boost::signals2::signal<void()> appTimeoutAfterS_noneForDefault_orNeverValue_signal;
		boost::signals2::signal<void()> displayCurrencySymbol_signal;
		boost::signals2::signal<void()> authentication__requireWhenSending_signal;
		boost::signals2::signal<void()> authentication__requireToShowWalletSecrets_signal;
		boost::signals2::signal<void()> authentication__tryBiometric_signal;
		//
		// Accessors
		bool hasExisting_saved_document() const;
		bool hasBooted();
		//
		// Protocols - PasswordControllerEventParticipant
		std::string identifier() const
		{
			return uuid_string;
		}
		// Protocols - DeleteEverythingRegistrant
		optional<string> passwordController_DeleteEverything();
		// Protocols - ChangePasswordRegistrant
		optional<Passwords::EnterPW_Fn_ValidationErr_Code> passwordController_ChangePassword();
		//
		// Accessors - IdleTimeoutAfterS_SettingsProvider
		double default_appTimeoutAfterS();
		optional<double> appTimeoutAfterS_noneForDefault_orNeverValue();
		// Accessors - Other properties - Synchronized
		optional<string> specificAPIAddressURLAuthority();
		bool authentication__requireWhenSending();
		bool authentication__requireToShowWalletSecrets();
		bool authentication__tryBiometric();
		Currencies::CurrencySymbol displayCurrencySymbol();
		Currencies::Currency displayCurrency();
		//
		// Imperatives
		bool set_appTimeoutAfterS_noneForDefault_orNeverValue(optional<double> value);
		bool set_authentication__requireWhenSending(bool value);
		bool set_authentication__requireToShowWalletSecrets(bool value);
		bool set_authentication__tryBiometric(bool value);
		bool set_displayCurrencySymbol(optional<Currencies::CurrencySymbol> value);
		bool set_specificAPIAddressURLAuthority(optional<string> value);
	private:
		//
		// Properties - Runtime
		std::string uuid_string = boost::uuids::to_string((boost::uuids::random_generator())()); // cached
		std::mutex property_mutex;
		bool _hasBooted = false;
		// Properties - Saved values
		optional<DocumentId> _id = none;
		optional<double> _appTimeoutAfterS_noneForDefault_orNeverValue;
		optional<string> _specificAPIAddressURLAuthority;
		bool _authentication__requireWhenSending;
		bool _authentication__requireToShowWalletSecrets;
		bool _authentication__tryBiometric;
		Currencies::CurrencySymbol _displayCurrencySymbol;
		//
		// Accessors
		optional<string> _givenLocked_existing_saved_documentContentString() const;
		bool shouldInsertNotUpdate() const;
		document_persister::DocumentJSON _givenLocked_new_dictRepresentation() const;
		//
		// Imperatives
		void startObserving();
		void _setup_loadState();
		//
		void teardown();
		void stopObserving();
		//
		bool _locked_set_save_and_emit(Settings_DictKey key, prop_val_arg_type val);
		void _set(Settings_DictKey key, prop_val_arg_type val);
		void _onAsync_invokeEmitterFor_changed(Settings_DictKey key);
		void _onAsync_invokeEmitterFor_changed__all();
		void _givenLocked_initWithDefaults();
		void _givenLocked_setup_loadState(
			optional<DocumentId> arg_id,
			optional<string> specificAPIAddressURLAuthority,
			optional<double> appTimeoutAfterS_noneForDefault_orNeverValue,
			bool authentication__requireWhenSending,
			bool authentication__requireToShowWalletSecrets,
			bool authentication__tryBiometric,
			Currencies::CurrencySymbol displayCurrencySymbol
		);
		//
		bool _givenLocked_saveToDisk();
		bool __givenLocked_saveToDisk_insert();
		bool __givenLocked_saveToDisk_update();
		bool ___givenLocked_saveToDisk_write();
		//
	};
}

#endif /* SettingsController_hpp */

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
#include "SettingsController.hpp"
#include <boost/variant.hpp>
#include "../Persistence/PersistableObject.hpp"
#include "misc_log_ex.h"
#include "../mymonero-core-cpp/src/serial_bridge_utils.hpp"
using namespace std;
using namespace boost;
using namespace document_persister;
using namespace Settings;
//
// Constants - Persistence
static string collectionName = string("Settings");
std::string _dictKey(Settings_DictKey fromKey)
{
	switch (fromKey) {
		case Settings_DictKey::_id:
			return "_id";
		case Settings_DictKey::specificAPIAddressURLAuthority:
			return "specificAPIAddressURLAuthority";
		case Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue:
			return "appTimeoutAfterS_nilForDefault_orNeverValue";
		case Settings_DictKey::displayCurrencySymbol:
			return "displayCurrencySymbol";
		case Settings_DictKey::authentication__requireWhenSending:
			return "authentication__requireWhenSending";
		case Settings_DictKey::authentication__requireToShowWalletSecrets:
			return "authentication__requireToShowWalletSecrets";
		case Settings_DictKey::authentication__tryBiometric:
			return "authentication__tryBiometric";
		case __max: case __min:
			BOOST_THROW_EXCEPTION(logic_error("Never expected _dictKey() to be called on __min, __max"));
			return "";
	}
}
//
// Imperatives - Lifecycle - Setup
void Controller::setup()
{
	if (documentsPath == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected documentsPath != nullptr"));
	}
	if (dispatch_ptr == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("PasswordController: expected dispatch_ptr != nullptr"));
	}
	if (passwordController == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("PasswordController: expected passwordController != nullptr"));
	}
	//
	startObserving();
	_setup_loadState();
}
void Controller::startObserving()
{
	passwordController->addRegistrantForDeleteEverything(*this);
	passwordController->addRegistrantForChangePassword(*this);
}
void Controller::_setup_loadState()
{
	property_mutex.lock();
	{
		optional<string> documentContentString = _givenLocked_existing_saved_documentContentString();
		if (documentContentString == none) {
			_givenLocked_initWithDefaults();
		} else { // rather than returning directly from the if, so as to share the mutex.unlock()
			property_tree::ptree documentJSON = Persistable::new_plaintextDocumentDictFromJSONString(*documentContentString);
			DocumentId this_id = documentJSON.get<DocumentId>(_dictKey(Settings_DictKey::_id));
			optional<string> specificAPIAddressURLAuthority = documentJSON.get_optional<string>(_dictKey(Settings_DictKey::specificAPIAddressURLAuthority));
			optional<double> existingValueFor_appTimeoutAfterS = serial_bridge_utils::none_or_double_from(documentJSON, _dictKey(Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue));
			optional<bool> authentication__requireWhenSending = serial_bridge_utils::none_or_bool_from(documentJSON, _dictKey(Settings_DictKey::authentication__requireWhenSending));
			optional<bool> authentication__requireToShowWalletSecrets = serial_bridge_utils::none_or_bool_from(documentJSON, _dictKey(Settings_DictKey::authentication__requireToShowWalletSecrets));
			optional<bool> authentication__tryBiometric = serial_bridge_utils::none_or_bool_from(documentJSON, _dictKey(Settings_DictKey::authentication__tryBiometric));
			optional<Currencies::CurrencySymbol> displayCurrencySymbol = documentJSON.get_optional<Currencies::CurrencySymbol>(_dictKey(Settings_DictKey::displayCurrencySymbol));
			//
			_givenLocked_setup_loadState(
				this_id,
				specificAPIAddressURLAuthority,
				existingValueFor_appTimeoutAfterS != none
					? *existingValueFor_appTimeoutAfterS
					: default_appTimeoutAfterS(),
				(authentication__requireWhenSending != none)
					? *authentication__requireWhenSending
					: default_authentication__requireWhenSending,
				(authentication__requireToShowWalletSecrets != none)
					? *authentication__requireToShowWalletSecrets
					: default_authentication__requireToShowWalletSecrets,
				(authentication__tryBiometric != none)
					? *authentication__tryBiometric
					: default_authentication__tryBiometric,
				displayCurrencySymbol != none
					? *displayCurrencySymbol
					: default_displayCurrencySymbol
			);
		}
	}
	property_mutex.unlock();
}
//
// Imperatives - Lifecycle - Teardown
void Controller::teardown()
{
	stopObserving();
}
void Controller::stopObserving()
{
	passwordController->removeRegistrantForDeleteEverything(*this);
	passwordController->removeRegistrantForChangePassword(*this);
}
//
// Accessors
bool Controller::hasExisting_saved_document() const
{ // Exposed for Testing (Not necessary for app integration)
	// Here, this is not synchronized, because it's assumed that whoever is calling it is not doing so from an application code context, and is probably calling it prior to the construction of any instance
	return _givenLocked_existing_saved_documentContentString() != none;
}
optional<string> Controller::_givenLocked_existing_saved_documentContentString() const
{
	auto result = allDocuments(*documentsPath, collectionName);
	if (result.err_str) {
		ostringstream ss;
		ss << "Settings: Fatal error while loading " << collectionName << ": " << *result.err_str << "" << endl;
		BOOST_THROW_EXCEPTION(logic_error(ss.str()));
		return none;
	}
	auto numDocuments = (*(result.strings)).size();
	if (numDocuments > 1) {
		ostringstream ss;
		ss << "Settings: Unexpected state while loading " << collectionName << ": more than one saved doc." << endl;
		BOOST_THROW_EXCEPTION(logic_error(ss.str()));
		//
		return none;
	}
	if (numDocuments == 0) {
		return none;
	}
	return (*(result.strings))[0];
}
//
// Accessors - Properties / Synchronized
bool Controller::hasBooted()
{
	bool val;
	property_mutex.lock();
	val = _hasBooted;
	property_mutex.unlock();
	//
	return val;
}
optional<string> Controller::specificAPIAddressURLAuthority()
{
	optional<string> val;
	property_mutex.lock();
	val = _specificAPIAddressURLAuthority;
	property_mutex.unlock();
	//
	return val;
}
bool Controller::authentication__requireWhenSending()
{
	bool val;
	property_mutex.lock();
	val = _authentication__requireWhenSending;
	property_mutex.unlock();
	//
	return val;
}
bool Controller::authentication__requireToShowWalletSecrets()
{
	bool val;
	property_mutex.lock();
	val = _authentication__requireToShowWalletSecrets;
	property_mutex.unlock();
	//
	return val;
}
bool Controller::authentication__tryBiometric()
{
	bool val;
	property_mutex.lock();
	val = _authentication__tryBiometric;
	property_mutex.unlock();
	//
	return val;
}
Currencies::CurrencySymbol Controller::displayCurrencySymbol()
{
	Currencies::CurrencySymbol val;
	property_mutex.lock();
	val = _displayCurrencySymbol;
	property_mutex.unlock();
	//
	return val;
}
Currencies::Currency Controller::displayCurrency()
{
	return Currencies::CurrencyFrom(displayCurrencySymbol());
}
//
// Accessors - IdleTimeoutAfterS_SettingsProvider
double Controller::default_appTimeoutAfterS()
{
	return 90; // s …… 30 was a bit short for new users
}
optional<double> Controller::appTimeoutAfterS_noneForDefault_orNeverValue() 
{
	return _appTimeoutAfterS_noneForDefault_orNeverValue;
}
//
// Accessors
bool Controller::shouldInsertNotUpdate() const
{
	return _id == none;
}
property_tree::ptree Controller::_givenLocked_new_dictRepresentation() const
{
	property_tree::ptree dict;
	dict.put(_dictKey(Settings_DictKey::_id), *(_id));
	if (_specificAPIAddressURLAuthority != none) {
		dict.put(_dictKey(Settings_DictKey::specificAPIAddressURLAuthority), *_specificAPIAddressURLAuthority);
	}
	if (_appTimeoutAfterS_noneForDefault_orNeverValue != none) {
		dict.put(_dictKey(Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue), *_appTimeoutAfterS_noneForDefault_orNeverValue);
	}
	dict.put(_dictKey(Settings_DictKey::authentication__requireWhenSending), _authentication__requireWhenSending);
	dict.put(_dictKey(Settings_DictKey::authentication__tryBiometric), _authentication__tryBiometric);
	dict.put(_dictKey(Settings_DictKey::authentication__requireToShowWalletSecrets), _authentication__requireToShowWalletSecrets);
	dict.put(_dictKey(Settings_DictKey::displayCurrencySymbol), _displayCurrencySymbol);
	//
	return dict;
}
//
// Imperatives - State
bool Controller::set_appTimeoutAfterS_noneForDefault_orNeverValue(optional<double> value)
{
	return _locked_set_save_and_emit(Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue, value);
}
bool Controller::set_authentication__requireWhenSending(bool value)
{
	return _locked_set_save_and_emit(Settings_DictKey::authentication__requireWhenSending, value);
}
bool Controller::set_authentication__requireToShowWalletSecrets(bool value)
{
	return _locked_set_save_and_emit(Settings_DictKey::authentication__requireToShowWalletSecrets, value);
}
bool Controller::set_authentication__tryBiometric(bool value)
{
	return _locked_set_save_and_emit(Settings_DictKey::authentication__tryBiometric, value);
}
bool Controller::set_displayCurrencySymbol(optional<Currencies::CurrencySymbol> value)
{
	return _locked_set_save_and_emit(Settings_DictKey::displayCurrencySymbol, value);
}
bool Controller::set_specificAPIAddressURLAuthority(optional<string> value)
{
	return _locked_set_save_and_emit(Settings_DictKey::specificAPIAddressURLAuthority, value);
}
//
// Imperatives - State - Private
bool Controller::_locked_set_save_and_emit(Settings_DictKey key, prop_val_arg_type val)
{
	bool r;
	property_mutex.lock();
	{
		_set(key, val);
		r = _givenLocked_saveToDisk();
	}
	property_mutex.unlock();
	if (!r) { // return *after* unlock
		// TODO? revert member var value here?
		return false;
	}
	dispatch_ptr->async([this, key]() {
		_onAsync_invokeEmitterFor_changed(key);
	});
	return true;
}
void Controller::_set(Settings_DictKey key, prop_val_arg_type val)
{
	switch (key) {
		case Settings_DictKey::specificAPIAddressURLAuthority:
			_specificAPIAddressURLAuthority = boost::get<optional<string>>(val);
			break;
			
		case Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue:
			_appTimeoutAfterS_noneForDefault_orNeverValue = boost::get<optional<double>>(val);
			break;
			
		case Settings_DictKey::displayCurrencySymbol:
		{
			optional<Currencies::CurrencySymbol> ccySymbol_orNone = boost::get<optional<Currencies::CurrencySymbol>>(val);
			_displayCurrencySymbol = ccySymbol_orNone != boost::none ? *ccySymbol_orNone : default_displayCurrencySymbol;
		}
		break;
			
		case Settings_DictKey::authentication__requireWhenSending:
			_authentication__requireWhenSending = boost::get<bool>(val);
			break;
			
		case Settings_DictKey::authentication__requireToShowWalletSecrets:
			_authentication__requireToShowWalletSecrets = boost::get<bool>(val);
			break;
			
		case Settings_DictKey::authentication__tryBiometric:
			_authentication__tryBiometric = boost::get<bool>(val);
			break;
			
		default:
			BOOST_THROW_EXCEPTION(logic_error("Unhandled Settings_DictKey in _set"));
			break;
	}
}
void Controller::_onAsync_invokeEmitterFor_changed(Settings_DictKey key)
{
	switch (key) {
		case Settings_DictKey::specificAPIAddressURLAuthority:
			specificAPIAddressURLAuthority_signal();
			break;
			
		case Settings_DictKey::appTimeoutAfterS_nilForDefault_orNeverValue:
			appTimeoutAfterS_noneForDefault_orNeverValue_signal();
			break;
			
		case Settings_DictKey::displayCurrencySymbol:
			displayCurrencySymbol_signal();
			break;
			
		case Settings_DictKey::authentication__requireWhenSending:
			authentication__requireWhenSending_signal();
			break;
			
		case Settings_DictKey::authentication__requireToShowWalletSecrets:
			authentication__requireToShowWalletSecrets_signal();
			break;
			
		case Settings_DictKey::authentication__tryBiometric:
			authentication__tryBiometric_signal();
			break;
			
		default:
			BOOST_THROW_EXCEPTION(logic_error("Unhandled Settings_DictKey in _onAsync_invokeEmitterFor_changed"));
			break;
	}
}
void Controller::_onAsync_invokeEmitterFor_changed__all()
{
	for (int i = (Settings_DictKey::__min)+1 ; i < Settings_DictKey::__max ; i++) {
		Settings_DictKey key = static_cast<Settings_DictKey>(i);
		if (key != Settings_DictKey::_id) { // since we have no emitter for this
			_onAsync_invokeEmitterFor_changed(key);
		}
	}
}
//
void Controller::_givenLocked_initWithDefaults()
{
	_givenLocked_setup_loadState(
		none, // _id
		none, // specificAPIAddressURLAuthority
		default_appTimeoutAfterS(),
		default_authentication__requireWhenSending,
		default_authentication__requireToShowWalletSecrets,
		default_authentication__tryBiometric,
		default_displayCurrencySymbol
	);
}
void Controller::_givenLocked_setup_loadState(
	optional<DocumentId> arg_id,
	optional<string> specificAPIAddressURLAuthority,
	optional<double> appTimeoutAfterS_noneForDefault_orNeverValue,
	bool authentication__requireWhenSending,
	bool authentication__requireToShowWalletSecrets,
	bool authentication__tryBiometric,
	Currencies::CurrencySymbol displayCurrencySymbol
) {
//	throwUnlessOnSyncThread()
	//
	_id = arg_id;
	_specificAPIAddressURLAuthority = specificAPIAddressURLAuthority;
	_appTimeoutAfterS_noneForDefault_orNeverValue = appTimeoutAfterS_noneForDefault_orNeverValue;
	_authentication__requireWhenSending = authentication__requireWhenSending;
	_authentication__requireToShowWalletSecrets = authentication__requireToShowWalletSecrets;
	_authentication__tryBiometric = authentication__tryBiometric;
	_displayCurrencySymbol = displayCurrencySymbol;
	//
	_hasBooted = true;
}
//
// Imperatives - Persistence
bool Controller::_givenLocked_saveToDisk()
{
	if (shouldInsertNotUpdate()) {
		return __givenLocked_saveToDisk_insert();
	}
	return __givenLocked_saveToDisk_update();
}
bool Controller::__givenLocked_saveToDisk_insert()
{
//	throwUnlessOnSyncThread()
	//
	if (_id != none) {
		BOOST_THROW_EXCEPTION(logic_error("non-nil _id in _saveToDisk_insert"));
	}
	_id = document_persister::new_documentId();
	//
	return ___givenLocked_saveToDisk_write();
}
bool Controller::__givenLocked_saveToDisk_update()
{
//	throwUnlessOnSyncThread()
	//
	if (_id == none) {
		BOOST_THROW_EXCEPTION(logic_error("nil _id in _saveToDisk_update"));
	}
	return ___givenLocked_saveToDisk_write();
}
bool Controller::___givenLocked_saveToDisk_write()
{
//	throwUnlessOnSyncThread()
	//
	optional<string> err_str = document_persister::write(
		*documentsPath,
		Persistable::new_plaintextJSONStringFromDocumentDict(_givenLocked_new_dictRepresentation()),
		*_id,
		collectionName
	);
	if (err_str != none) {
		MERROR("Settings: Error while saving " << this << ": " << err_str);
		return false;
	}
	MDEBUG("Settings: Saved " << this << ".");
	return true;
}
//
// Protocols - DeleteEverythingRegistrant
optional<string> Controller::passwordController_DeleteEverything()
{
	optional<string> err_str = none;
	property_mutex.lock();
	{ // so as not to race with any saves
		errOr_numRemoved result = document_persister::removeAllDocuments(*documentsPath, collectionName);
		if (result.err_str != none) {
			err_str = std::move(*result.err_str);
		} else { // if delete succeeded
			_givenLocked_initWithDefaults();
		}
	}
	property_mutex.unlock();
	//
	dispatch_ptr->async([this]() {
		_onAsync_invokeEmitterFor_changed__all(); // so UI et al. may update
	});
	//
	return err_str;
}
// Protocols - ChangePasswordRegistrant
optional<Passwords::EnterPW_Fn_ValidationErr_Code> Controller::passwordController_ChangePassword()
{
	optional<Passwords::EnterPW_Fn_ValidationErr_Code> err_str = none;
	property_mutex.lock();
	{
		if (_hasBooted != true) {
			MWARNING("Settings: asked to change password but not yet booted.");
			err_str = Passwords::EnterPW_Fn_ValidationErr_Code::notBootedYet;
		} else {
			bool r = _givenLocked_saveToDisk();
			if (!r) {
				err_str = Passwords::EnterPW_Fn_ValidationErr_Code::saveError;
			}
		}
	}
	property_mutex.unlock();
	//
	return err_str;
}

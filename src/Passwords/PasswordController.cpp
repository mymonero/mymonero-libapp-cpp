//
//  PasswordController.cpp
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
#include "PasswordController.hpp"
#include <boost/foreach.hpp>
#include "../UserIdle/UserIdle.hpp"
#include "../Persistence/PersistableObject.hpp"
#include "misc_log_ex.h"
using namespace std;
using namespace boost;
using namespace document_persister;
using namespace Passwords;
//
// Constants - Persistence
static string collectionName = string("PasswordMeta");
static string plaintextMessageToSaveForUnlockChallenges = string("this is just a string that we'll use for checking whether a given password can unlock an encrypted version of this very message");
//
enum class DictKey
{
	_id,
	passwordType,
	messageAsEncryptedDataForUnlockChallenge_base64String
};
std::string _dictKey(DictKey fromKey)
{
	switch (fromKey) {
		case DictKey::_id:
			return "_id";
		case DictKey::passwordType:
			return "passwordType";
		case DictKey::messageAsEncryptedDataForUnlockChallenge_base64String:
			return "messageAsEncryptedDataForUnlockChallenge_base64String";
	}
}
//
// Accessors - Interfaces - PasswordProvider
optional<Password> Controller::getPassword() const
{
   return _password;
}
Passwords::Type Controller::getPasswordType() const
{
	return _passwordType;
}
bool Controller::hasUserSavedAPassword() const
{ // this obviously has a file I/O hit, which is not optimal; alternatives are use sparingly or cache at appropriate locations
	errOr_documentIds result = idsOfAllDocuments(*documentsPath, collectionName);
	if (result.err_str) {
		BOOST_THROW_EXCEPTION(logic_error(*result.err_str));
		return false;
	}
	size_t numberOfIds = result.ids->size();
	if (numberOfIds > 1) {
		BOOST_THROW_EXCEPTION(logic_error("Illegal: Should be only one document"));
		return false;
	} else if (numberOfIds == 0) {
		return false;
	}
	return true;
}
//
// Accessors - Derived properties
bool Controller::hasUserEnteredValidPasswordYet() const
{
	return _password != none;
}
bool Controller::isUserChangingPassword() const
{
	return hasUserEnteredValidPasswordYet() && (_isAlreadyGettingExistingOrNewPWFromUser != none && *_isAlreadyGettingExistingOrNewPWFromUser == true);
}
// Accessors - Common
bool Controller::withExistingPassword_isCorrect(const string &enteredPassword) const
{ // NOTE: This function should most likely remain private so that it is not cheap to check PW and must be done through the PW entry UI (by way of methods on Passwords::Controller)
	if (_password == none) {
		BOOST_THROW_EXCEPTION(logic_error("withExistingPassword_isCorrect expected non-none _password"));
		return false;
	}
	// FIXME/TODO: is this check too weak? is it better to try decrypt and check hmac mismatch?
	return *_password == enteredPassword;
}
//
// Imperatives - PasswordEntryDelegate
void Controller::setPasswordEntryDelegate(PasswordEntryDelegate &to_delegate)
{
	if (_passwordEntryDelegate != nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("setPasswordEntryDelegate called but self.passwordEntryDelegate already exists"));
	}
	_passwordEntryDelegate = &to_delegate;
}
void Controller::clearPasswordEntryDelegate(PasswordEntryDelegate &from_existing_delegate)
{
	if (_passwordEntryDelegate == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("clearPasswordEntryDelegate called but no passwordEntryDelegate exists"));
		return;
	}
	if (*_passwordEntryDelegate != from_existing_delegate) {
		BOOST_THROW_EXCEPTION(logic_error("clearPasswordEntryDelegate called but passwordEntryDelegate does not match"));
		return;
	}
	_passwordEntryDelegate = nullptr;
}
void Controller::TEST_bypassCheckAndClear_passwordEntryDelegate()
{
	_passwordEntryDelegate = nullptr; // bypass identity check here since it went out of scope and therefore can't be used to check identifier()
}
void Controller::TEST_clearUnlockTimer()
{
	if (_pw_entry_unlock_timer_handle != nullptr) {
		_pw_entry_unlock_timer_handle->cancel(); // invalidate timer
		_pw_entry_unlock_timer_handle = nullptr; // release / free
	}
}
void Controller::TEST_resetPasswordControllerInitAndObservers()
{
	TEST_bypassCheckAndClear_passwordEntryDelegate();
	{
		obtainedNewPassword_signal.disconnect_all_slots();
		obtainedCorrectExistingPassword_signal.disconnect_all_slots();
		//
		erroredWhileGettingExistingPassword_signal.disconnect_all_slots();
		erroredWhileSettingNewPassword_signal.disconnect_all_slots();
		canceledWhileEnteringExistingPassword_signal.disconnect_all_slots();
		canceledWhileEnteringNewPassword_signal.disconnect_all_slots();
		//
		setFirstPasswordDuringThisRuntime_signal.disconnect_all_slots();
		registrantsAllChangedPassword_signal.disconnect_all_slots();
		//
		canceledWhileChangingPassword_signal.disconnect_all_slots();
		errorWhileChangingPassword_signal.disconnect_all_slots();
		//
		duringAuthentication_tryBiometrics_signal.disconnect_all_slots();
		errorWhileAuthorizingForAppAction_signal.disconnect_all_slots();
		successfullyAuthenticatedForAppAction_signal.disconnect_all_slots();
		//
		willDeconstructBootedStateAndClearPassword_signal.disconnect_all_slots();
		didDeconstructBootedStateAndClearPassword_signal.disconnect_all_slots();
		havingDeletedEverything_didDeconstructBootedStateAndClearPassword_signal.disconnect_all_slots();
	}
	{ // might seem like a bad idea to do this but since some tests exit in the middle of a failed PW entry, this is necessary:
		isWaitingFor_enterExistingPassword_cb = false;
		enterExistingPassword_final_fn = none;
		//
		isWaitingFor_enterNewPassword_cb = false;
	}
	TEST_clearUnlockTimer();
	lockDownAppAndRequirePassword();
}
//
// Imperatives - Lifecycle
void Controller::setup()
{
	if (documentsPath == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected documentsPath != nullptr"));
	}
	if (dispatch_ptr == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("PasswordController: expected dispatch_ptr != nullptr"));
	}
	if (userIdleController == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("PasswordController: expected userIdleController != nullptr"));
	}
	//
	startObserving();
	initializeRuntimeAndBoot();
}
void Controller::startObserving()
{
	userIdleController->userDidBecomeIdle_signal.connect(
		std::bind(&Controller::UserIdle_userDidBecomeIdle, this)
	);
}
void Controller::initializeRuntimeAndBoot()
{
	if (hasBooted != false) {
		BOOST_THROW_EXCEPTION(logic_error("initializeRuntimeAndBoot called while already booted"));
		return;
	}
	auto result = allDocuments(*documentsPath, collectionName);
	if (result.err_str) {
		ostringstream ss;
		ss << "Passwords: Fatal error while loading " << collectionName << ": " << *result.err_str << "" << endl;
		BOOST_THROW_EXCEPTION(logic_error(ss.str()));
		return;
	}
	auto numDocuments = (*(result.strings)).size();
	if (numDocuments > 1) {
		ostringstream ss;
		ss << "Passwords: Unexpected state while loading " << collectionName << ": more than one saved doc." << endl;
		BOOST_THROW_EXCEPTION(logic_error(ss.str()));
		return;
	}
	if (numDocuments == 0) {
		property_tree::ptree fabricated_documentJSON;
		fabricated_documentJSON.put(_dictKey(DictKey::passwordType), Type::password); // default (at least for now)
		_proceedTo_load(fabricated_documentJSON);
		return;
	}
	_proceedTo_load(
		Persistable::new_plaintextDocumentDictFromJSONString((*(result.strings))[0])
	);
}
void Controller::_proceedTo_load(const DocumentJSON &documentJSON)
{
	optional<DocumentId> id = documentJSON.get_optional<DocumentId>(_dictKey(DictKey::_id));
	if (id != none) {
		_id = std::move(*id);
	}
	int raw_passwordType_val = documentJSON.get<int>(_dictKey(DictKey::passwordType));
	if (raw_passwordType_val <= Passwords::Type::minBound || raw_passwordType_val >= Passwords::Type::maxBound) {
		BOOST_THROW_EXCEPTION(logic_error("Found undefined encrypted msg for unlock challenge in saved password model document"));
		return;
	}
	_passwordType = (Passwords::Type)raw_passwordType_val;
	_messageAsEncryptedDataForUnlockChallenge_base64String = documentJSON.get_optional<string>(_dictKey(DictKey::messageAsEncryptedDataForUnlockChallenge_base64String));
	if (_id != none) { // existing doc
		if (_messageAsEncryptedDataForUnlockChallenge_base64String == none || _messageAsEncryptedDataForUnlockChallenge_base64String->empty()) {
			// ^-- but it was saved w/o an encrypted challenge str
			// TODO: not sure how to handle this case. delete all local info? would suck but otoh when would this happen if not for a cracking attempt, some odd/fatal code fault, or a known migration?
			BOOST_THROW_EXCEPTION(logic_error("Found undefined encrypted msg for unlock challenge in saved password model document"));
			return;
		}
	}
	hasBooted = true;
	_callAndFlushAllBlocksWaitingForBootToExecute();
	MDEBUG("Passwords: Booted \(self) and called all waiting blocks. Waiting for unlock.");
}
//
// Imperatives - Lifecycle - Teardown
void Controller::teardown()
{
	if (_pw_entry_unlock_timer_handle != nullptr) {
		_pw_entry_unlock_timer_handle->cancel();
		_pw_entry_unlock_timer_handle = nullptr;
	}
}
//
// Imperatives - Execution Deferment
void Controller::onceBooted(std::function<void()> fn)
{
	if (hasBooted) {
		fn();
		return;
	}
	if (__blocksWaitingForBootToExecute == none) {
		__blocksWaitingForBootToExecute = vector<std::function<void()>>();
	}
	(*__blocksWaitingForBootToExecute).push_back(fn);
}
void Controller::_callAndFlushAllBlocksWaitingForBootToExecute()
{
	if (__blocksWaitingForBootToExecute == none) {
		return;
	}
	BOOST_FOREACH(std::function<void()> &fn, (*__blocksWaitingForBootToExecute))
	{
		fn();
	}
	__blocksWaitingForBootToExecute = none; // free
}
//
// Accessors - Deferring execution convenience methods
struct once_booted_callback_info_container
{
public:
	bool guardAllCallBacks()
	{
		if (_hasCalledBack == true) {
			auto err_str = string("Passwords::Controller::onceBootedAndPasswordObtained hasCalledBack already true");
			MERROR(err_str);
			BOOST_THROW_EXCEPTION(logic_error(err_str));
			return false; // ^- shouldn't happen but just in case…
		}
		_hasCalledBack = true;
		return true;

	}
	void stopListening()
	{
		if (_hasStoppedListening) {
			BOOST_THROW_EXCEPTION(logic_error("Already stopped listening"));
			return;
		}
		connection__obtainedNewPassword.disconnect();
		connection__obtainedCorrectExistingPassword.disconnect();
		connection__canceledWhileEnteringExistingPassword.disconnect();
		connection__canceledWhileEnteringNewPassword.disconnect();
		_hasStoppedListening = true;
	}
	boost::signals2::connection connection__obtainedNewPassword;
	boost::signals2::connection connection__obtainedCorrectExistingPassword;
	boost::signals2::connection connection__canceledWhileEnteringExistingPassword;
	boost::signals2::connection connection__canceledWhileEnteringNewPassword;
private:
	bool _hasCalledBack = false;
	bool _hasStoppedListening = false;
};
void Controller::onceBootedAndPasswordObtained(
	std::function<void(Password password, Passwords::Type passwordType)> fn,
	std::function<void()> userCanceled_fn = {}
) {
	if (hasUserEnteredValidPasswordYet()) {
		fn(*_password, _passwordType);
		return;
	}
	// then we have to wait for it
	auto callback_info_container = std::make_shared<once_booted_callback_info_container>();
	auto _aPasswordWasObtained = [this, fn = std::move(fn), callback_info_container]()
	{
		if (callback_info_container->guardAllCallBacks() != false) {
			callback_info_container->stopListening(); // immediately unsubscribe
			fn(*_password, _passwordType);
		}
	};
	auto _obtainingPasswordWasCanceled = [userCanceled_fn, callback_info_container]()
	{
		if (callback_info_container->guardAllCallBacks() != false) {
			callback_info_container->stopListening(); // immediately unsubscribe
			userCanceled_fn();
		}
	};
	onceBooted([&callback_info_container, this, _aPasswordWasObtained, _obtainingPasswordWasCanceled] () {
		// hang onto connections so we can unsub
		callback_info_container->connection__obtainedNewPassword = obtainedNewPassword_signal.connect(_aPasswordWasObtained);
		callback_info_container->connection__obtainedCorrectExistingPassword = obtainedCorrectExistingPassword_signal.connect(_aPasswordWasObtained);
		callback_info_container->connection__canceledWhileEnteringExistingPassword = canceledWhileEnteringExistingPassword_signal.connect(_obtainingPasswordWasCanceled);
		callback_info_container->connection__canceledWhileEnteringNewPassword = canceledWhileEnteringNewPassword_signal.connect(_obtainingPasswordWasCanceled);
		//
		// now that we're subscribed, initiate the pw request
		givenBooted_initiateGetNewOrExistingPasswordFromUserAndEmitIt();
	});
}
void Controller::givenBooted_initiateGetNewOrExistingPasswordFromUserAndEmitIt()
{
	if (hasUserEnteredValidPasswordYet()) {
		MWARNING("Passwords: Asked to givenBooted_initiateGetNewOrExistingPasswordFromUserAndEmitIt but already has password.");
		return; // already got it
	}
	{ // guard
		if (_isAlreadyGettingExistingOrNewPWFromUser != none && *(_isAlreadyGettingExistingOrNewPWFromUser) == true) {
			return; // only need to wait for it to be obtained
		}
		_isAlreadyGettingExistingOrNewPWFromUser = true; // this is reverted by unguard_getNewOrExistingPassword
	}
	// we'll use this in a couple places
	bool isForChangePassword = false; // this is simply for requesting to have the existing or a new password from the user
	bool isForAuthorizingAppActionOnly = false; // "
	//
	if (_id == none) { // if the user is not unlocking an already pw-protected app
		// then we need to get a new PW from the user
		obtainNewPasswordFromUser(isForChangePassword); // this will also call self.unguard_getNewOrExistingPassword()
		return;
	} else { // then we need to get the existing PW and check it against the encrypted message
		if (_messageAsEncryptedDataForUnlockChallenge_base64String == none) {
			auto err_str = string("Code fault: Existing document but no messageAsEncryptedDataForUnlockChallenge_base64String");
			MERROR(err_str);
			unguard_getNewOrExistingPassword(); // TODO: is this correct? don't we un-guard only on cancel or success?
			BOOST_THROW_EXCEPTION(logic_error(err_str));
			return;
		}
		_getUserToEnterTheirExistingPassword(
			isForChangePassword,
			isForAuthorizingAppActionOnly, // false
			none, // customNavigationBarTitle
			[this] (
				optional<bool> didCancel_orNone,
				optional<EnterPW_Fn_ValidationErr_Code> validationErr_orNone,
				optional<Password> obtainedPasswordString
			) -> void {
				if (validationErr_orNone != none) { // takes precedence over cancel
					unguard_getNewOrExistingPassword(); // TODO/FIXME: is this correct? don't we un-guard only on cancel or success?
					erroredWhileGettingExistingPassword_signal(*validationErr_orNone);
					return;
				}
				if (didCancel_orNone != none && *didCancel_orNone == true) {
					// free/unset these at terminus of process ... slightly janky (entropic) to do it out-of-setting-function like this
					isWaitingFor_enterExistingPassword_cb = false;
					enterExistingPassword_final_fn = none;
					//
					canceledWhileEnteringExistingPassword_signal();
					unguard_getNewOrExistingPassword();
					return; // just silently exit after unguarding
				}
				optional<std::string> decryptedString = Persistable::new_plaintextStringFrom(
					*_messageAsEncryptedDataForUnlockChallenge_base64String,
					*obtainedPasswordString
				);
				if (decryptedString == none) {
					unguard_getNewOrExistingPassword(); // TODO/FIXME: is this correct? don't we un-guard only on cancel or success?
					MERROR("Passwords: Error while decrypting message for unlock challenge");
					erroredWhileGettingExistingPassword_signal(incorrectPassword);
					return;
				}
				if (*decryptedString != plaintextMessageToSaveForUnlockChallenges) {
					unguard_getNewOrExistingPassword(); // TODO/FIXME: is this correct? don't we un-guard only on cancel or success?
					erroredWhileGettingExistingPassword_signal(incorrectPassword);
					return;
				}
				// free/unset these at terminus of process ... slightly janky (entropic) to do it out-of-setting-function like this
				isWaitingFor_enterExistingPassword_cb = false;
				enterExistingPassword_final_fn = none;
				//
				// then it's correct
				// hang onto pw and set state
				_didObtainPassword(*obtainedPasswordString);
				// all done
				unguard_getNewOrExistingPassword();
				obtainedCorrectExistingPassword_signal();
			}
		);
	}
}
//
// Runtime - Imperatives - Password verification
void Controller::initiate_verifyUserAuthenticationForAction(
	bool tryBiometrics,
	optional<string> customNavigationBarTitle_orNone,
	std::function<void()> canceled_fn,
	std::function<void()> entryAttempt_succeeded_fn
) {
	onceBooted([
		this, customNavigationBarTitle_orNone,
		canceled_fn = std::move(canceled_fn), entryAttempt_succeeded_fn = std::move(entryAttempt_succeeded_fn), tryBiometrics
	] (void) {
		if (hasUserEnteredValidPasswordYet() == false) {
			BOOST_THROW_EXCEPTION(logic_error("initiate_verifyUserAuthenticationForAction called but hasUserEnteredValidPasswordYet == false. This should be disallowed in the UI"));
			return;
		}
		{ // guard
			if (_isAlreadyGettingExistingOrNewPWFromUser == true) {
				BOOST_THROW_EXCEPTION(logic_error("initiate_changePassword called but isAlreadyGettingExistingOrNewPWFromUser == true. This should be precluded in the UI"));
				// only need to wait for it to be obtained
				return;
			}
			_isAlreadyGettingExistingOrNewPWFromUser = true;
		}
		_waitingForAuth_customNavigationBarTitle_orNone = customNavigationBarTitle_orNone;
		_waitingForAuth_canceled_fn = canceled_fn;
		_waitingForAuth_entryAttempt_succeeded_fn = entryAttempt_succeeded_fn;
		//
		// ^-- we're relying on having checked above that user has entered a valid pw already
		// now see if we can use biometrics
		if (tryBiometrics == false) {
			_proceedTo_authenticateVia_passphrase();
			return; // so we don't have to wrap the whole following branch in an if
		}
		duringAuthentication_tryBiometrics_signal(); // then wait for reply
	});
}
void Controller::_proceedTo_authenticateVia_passphrase()
{
   _getUserToEnterTheirExistingPassword(
		false,
		true, // isForAuthorizingAppActionOnly
		_waitingForAuth_customNavigationBarTitle_orNone,
		[this] (
			optional<bool> didCancel_orNone,
			optional<EnterPW_Fn_ValidationErr_Code> validationErrCode_orNone,
			optional<Password> entered_existingPassword
		) {
			if (validationErrCode_orNone != none) { // takes precedence over cancel
				unguard_getNewOrExistingPassword(); // TODO: is this correct?
				errorWhileAuthorizingForAppAction_signal(*validationErrCode_orNone);
				// no clear of callbacks here since user can still enter - they have to succeed or cancel
				return;
			}
			if (didCancel_orNone != none && *didCancel_orNone == true) {
				_authentication__callBackWithEntryAttempt_canceled();
				//
				return; // just silently exit after unguarding
			}
			if (withExistingPassword_isCorrect(*entered_existingPassword) == false) {
				unguard_getNewOrExistingPassword();
				errorWhileAuthorizingForAppAction_signal(incorrectPassword);
				// no clear of callbacks here since user can still enter - they have to succeed or cancel
				return;
			}
			successfullyAuthenticatedForAppAction_signal(); // this must be posted so the PresentationController can dismiss the entry modal
			_authentication__callBackWithEntryAttempt_succeeded();
		}
	);
};
void Controller::_authentication__callBackWithEntryAttempt_succeeded()
{
	unguard_getNewOrExistingPassword(); // must be called at function terminus
	(*_waitingForAuth_entryAttempt_succeeded_fn)(); // consider this an authentication
	//
	_waitingForAuth_canceled_fn = none;
	_waitingForAuth_entryAttempt_succeeded_fn = none;
	_waitingForAuth_customNavigationBarTitle_orNone = none;
}
void Controller::_authentication__callBackWithEntryAttempt_canceled()
{
	// currently there's no need of a .canceledWhileAuthorizingForAppAction signal here
	unguard_getNewOrExistingPassword(); // must be called at function terminus
	(*_waitingForAuth_canceled_fn)();
	//
	_waitingForAuth_canceled_fn = none;
	_waitingForAuth_entryAttempt_succeeded_fn = none;
	_waitingForAuth_customNavigationBarTitle_orNone = none;
}
void Controller::authenticationCB_biometricsSucceeded()
{
	_authentication__callBackWithEntryAttempt_succeeded();
}
void Controller::authenticationCB_biometricsFailed()
{ // fall back to password auth
	_proceedTo_authenticateVia_passphrase();
}
void Controller::authenticationCB_biometricsUnavailable()
{
	_proceedTo_authenticateVia_passphrase();
}
void Controller::authenticationCB_biometricsCanceled()
{
	_authentication__callBackWithEntryAttempt_canceled();
}

//
// Runtime - Imperatives - Private - Requesting password from user
void Controller::unguard_getNewOrExistingPassword()
{
	_isAlreadyGettingExistingOrNewPWFromUser = false;
}
void Controller::_getUserToEnterTheirExistingPassword(
	bool isForChangePassword,
	bool isForAuthorizingAppActionOnly,
	optional<string> customNavigationBarTitle,
	std::function<void(
		optional<bool> didCancel_orNone,
		optional<EnterPW_Fn_ValidationErr_Code> validationErr_orNone,
		optional<Password> obtainedPasswordString
	)> fn
) {
	if (isWaitingFor_enterExistingPassword_cb) {
		BOOST_THROW_EXCEPTION(logic_error("_getUserToEnterTheirExistingPassword: expected isWaitingFor_enterExistingPassword_cb = false"));
		return;
	}
	if (enterExistingPassword_final_fn != none) {
		BOOST_THROW_EXCEPTION(logic_error("_getUserToEnterTheirExistingPassword: expected enterExistingPassword_final_fn = none"));
		return;
	}
	_isCurrentlyLockedOutFromPWEntryAttempts = false;
	_numberOfTriesDuringThisTimePeriod = 0;
	//
	if (isForChangePassword && isForAuthorizingAppActionOnly) { // both shouldn't be true
		BOOST_THROW_EXCEPTION(logic_error("Expected isForChangePassword == false || isForAuthorizingAppActionOnly == false"));
		return;
	}
	if (_passwordEntryDelegate == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("Expected non-null _passwordEntryDelegate in _getUserToEnterTheirExistingPassword"));
		return;
	}
	_dateOf_firstPWTryDuringThisTimePeriod = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // initialized to current time
	//
	isWaitingFor_enterExistingPassword_cb = true;
	enterExistingPassword_final_fn = std::move(fn);
	// Now put request out
	(*_passwordEntryDelegate).getUserToEnterExistingPassword(
		isForChangePassword,
		isForAuthorizingAppActionOnly,
		customNavigationBarTitle
	);
}
void Controller::__cancelAnyAndRebuildUnlockTimer()
{ // allows them to try again every T sec, but resets timer if they submit w/o waiting
	bool wasAlreadyLockedOut = _pw_entry_unlock_timer_handle != nullptr;
	if (_pw_entry_unlock_timer_handle != nullptr) {
		// DDLog.Info("Passwords", "clearing existing unlock timer")
		_pw_entry_unlock_timer_handle->cancel(); // invalidate timer
		_pw_entry_unlock_timer_handle = nullptr; // release / free
	}
	MDEBUG("Passwords: Too many password entry attempts within " << pwEntrySpamming_unlockInT_s << "s. " << (!wasAlreadyLockedOut ? "Locking out" : "Extending lockout.") << ".");
	_pw_entry_unlock_timer_handle = this->dispatch_ptr->after(pwEntrySpamming_unlockInT_ms, [this]()
	{
		_pw_entry_unlock_timer_handle = nullptr; // clear ... TODO: do we need a mutex around this to prevent race condition?
		//
		MDEBUG("Passwords:  Unlocking password entry.");
		_isCurrentlyLockedOutFromPWEntryAttempts = false;
		(*enterExistingPassword_final_fn)(none, clearValidationErrorAndAllowRetry, none); 
	});
}
void Controller::enterExistingPassword_cb(boost::optional<bool> didCancel_orNone, boost::optional<Password> obtainedPasswordString)
{
	if (isWaitingFor_enterExistingPassword_cb != true) {
		BOOST_THROW_EXCEPTION(logic_error("enterExistingPassword_cb: expected isWaitingFor_enterExistingPassword_cb = true"));
		return;
	}
	if (isWaitingFor_enterNewPassword_cb == true) {
		BOOST_THROW_EXCEPTION(logic_error("enterExistingPassword_cb: expected isWaitingFor_enterNewPassword_cb = false"));
		return;
	}
	if (enterExistingPassword_final_fn == none) {
		BOOST_THROW_EXCEPTION(logic_error("_getUserToEnterTheirExistingPassword: expected enterExistingPassword_final_fn != none"));
		return;
	}
	optional<EnterPW_Fn_ValidationErr_Code> validationErrCode_orNone = none; // so far…
	if (didCancel_orNone == none || *didCancel_orNone == false) { // so user did NOT cancel
		// user did not cancel… let's check if we need to send back a pre-emptive validation err (such as because they're trying too much)
		if (_isCurrentlyLockedOutFromPWEntryAttempts == false) {
			if (_numberOfTriesDuringThisTimePeriod == 0) {
				_dateOf_firstPWTryDuringThisTimePeriod = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			}
			_numberOfTriesDuringThisTimePeriod += 1;
			size_t maxLegal_numberOfTriesDuringThisTimePeriod = 5;
			if (_numberOfTriesDuringThisTimePeriod > maxLegal_numberOfTriesDuringThisTimePeriod) { // rhs must be > 0
				_numberOfTriesDuringThisTimePeriod = 0;
				// ^- no matter what, we're going to need to reset the above state for the next 'time period'
				//
				time_t now;
				time(&now); // get current time; same as: now = time(NULL)
				double s_since_firstPWTryDuringThisTimePeriod = difftime(now, *_dateOf_firstPWTryDuringThisTimePeriod);
				double noMoreThanNTriesWithin_s = 30;
				if (s_since_firstPWTryDuringThisTimePeriod > noMoreThanNTriesWithin_s) { // enough time has passed since this group began - only reset the "time period" with tries->0 and let this pass through as valid check
					_dateOf_firstPWTryDuringThisTimePeriod = none; // not strictly necessary to do here as we reset the number of tries during this time period to zero just above
					MDEBUG("There were more than " << maxLegal_numberOfTriesDuringThisTimePeriod << " password entry attempts during this time period but the last attempt was more than " << noMoreThanNTriesWithin_s << "s ago, so letting this go.");
				} else { // simply too many tries!…
					// lock it out for the next time (supposing this try does not pass)
					MDEBUG("Locking user out from PW entry attempts for " << pwEntrySpamming_unlockInT_s << "s");
					_isCurrentlyLockedOutFromPWEntryAttempts = true;
				}
			}
		}
		if (_isCurrentlyLockedOutFromPWEntryAttempts == true) { // do not try to check pw - return as validation err
			MDEBUG("Passwords: Received password entry attempt but currently locked out.");
			validationErrCode_orNone = pleaseWaitBeforeTryingAgain;
			// setup or extend unlock timer - NOTE: this is pretty strict - we don't strictly need to extend the timer each time to prevent spam unlocks
			__cancelAnyAndRebuildUnlockTimer();
		}
	}
	// then regardless of whether user canceled…
	(*enterExistingPassword_final_fn)(
		didCancel_orNone,
		validationErrCode_orNone,
		obtainedPasswordString
	);
}
void Controller::enterNewPasswordAndType_cb(boost::optional<bool> didCancel_orNone, boost::optional<Password> obtainedPasswordString, boost::optional<Type> userSelectedTypeOfPassword)
{
	if (isWaitingFor_enterNewPassword_cb != true) {
		BOOST_THROW_EXCEPTION(logic_error("enterNewPasswordAndType_cb: expected isWaitingFor_enterNewPassword_cb = true"));
		return;
	}
	if (isWaitingFor_enterExistingPassword_cb == true) {
		BOOST_THROW_EXCEPTION(logic_error("enterNewPasswordAndType_cb: expected isWaitingFor_enterExistingPassword_cb = false"));
		return;
	}
	if (didCancel_orNone != none && *didCancel_orNone == true) {
		isWaitingFor_enterNewPassword_cb = false; // terminal state - no longer waiting
		//
		canceledWhileEnteringNewPassword_signal();
		unguard_getNewOrExistingPassword();
		return; // just silently exit after unguarding
	}
	//
	// I. Validate features of pw before trying and accepting
	if (*userSelectedTypeOfPassword == PIN) {
		if (obtainedPasswordString->size() < 6) { // this is too short. get back to them with a validation err by re-entering obtainPasswordFromUser_cb
			unguard_getNewOrExistingPassword();
			erroredWhileSettingNewPassword_signal(enterLongerPIN);
			return; // bail
		}
		// TODO: check if all numbers
		// TODO: check that numbers are not all just one number
	} else if (*userSelectedTypeOfPassword == password) {
		if (obtainedPasswordString->size() < 6) { // this is too short. get back to them with a validation err by re-entering obtainPasswordFromUser_cb
			unguard_getNewOrExistingPassword();
			erroredWhileSettingNewPassword_signal(enterLongerPassword);
			return; // bail
		}
		// TODO: check if password content too weak?
	} else { // this is weird - code fault or cracking attempt?
		unguard_getNewOrExistingPassword();
		erroredWhileSettingNewPassword_signal(unrecognizedPasswordType);
		BOOST_THROW_EXCEPTION(logic_error("Unexpected unrecognized password type"));
		return;
	}
	if (_preexistingBeforeSetNew_isForChangePassword == true) {
		if (_password == *obtainedPasswordString) { // they are disallowed from using change pw to enter the same pw… despite that being convenient for dev ;)
			unguard_getNewOrExistingPassword();
			//
			EnterPW_Fn_ValidationErr_Code code;
			if (*userSelectedTypeOfPassword == password) {
				code = enterFreshPassword;
			} else if (*userSelectedTypeOfPassword == PIN) {
				code = enterFreshPIN;
			} else {
				code = unrecognizedPasswordType;
				BOOST_THROW_EXCEPTION(logic_error("Unexpected unrecognized password type"));
			}
			erroredWhileSettingNewPassword_signal(code);
			return; // bail
		}
	}
	//
	// II. hang onto new pw, pw type, and state(s)
	MDEBUG("Passwords: Obtained " << *userSelectedTypeOfPassword << " " << obtainedPasswordString->size() << " chars long");
	_didObtainPassword(*obtainedPasswordString);
	_passwordType = *userSelectedTypeOfPassword;
	//
	// III. finally, save doc (and unlock on success) so we know a pw has been entered once before
	auto err_str = saveToDisk();
	if (err_str != none) {
		unguard_getNewOrExistingPassword();
		if (_preexistingBeforeSetNew_wasFirstSetOfPasswordAtRuntime == true && _preexistingBeforeSetNew_password != none) {
			BOOST_THROW_EXCEPTION(logic_error("Expected password to be 'none' on first set of password at runtime"));
			return;
		}
		// they'll have to try again - and revert to old pw rather than nil for changePassword (should be nil for first pw set)
		if (_preexistingBeforeSetNew_password == none) {
			_password = none;
		} else {
			_password = *_preexistingBeforeSetNew_password;
		}
		_passwordType = _preexistingBeforeSetNew_passwordType;
		erroredWhileSettingNewPassword_signal(saveError);
		return;
	}
	// detecting & emiting first set or handling result of change saves
	if (_preexistingBeforeSetNew_wasFirstSetOfPasswordAtRuntime == true) {
		isWaitingFor_enterNewPassword_cb = false; // terminal state - no longer waiting
		//
		unguard_getNewOrExistingPassword();
		// specific emit
		setFirstPasswordDuringThisRuntime_signal();
		// general purpose emit
		obtainedNewPassword_signal();
		//
		return; // prevent fallthrough
	}
	// then, it's a change password
	optional<EnterPW_Fn_ValidationErr_Code> changePassword_err_orNone = _changePassword_tellRegistrants_doChangePassword(); // returns error
	if (changePassword_err_orNone == none) { // actual success - we can return early
		isWaitingFor_enterNewPassword_cb = false; // terminal state - no longer waiting
		//
		unguard_getNewOrExistingPassword();
		//
		registrantsAllChangedPassword_signal();
		// general purpose emit
		obtainedNewPassword_signal();
		//
		return;
	}
	// try to revert save files to old password...
	// first revert, so consumers can read reverted value
	if (_preexistingBeforeSetNew_password == none) {
		_password = none;
	} else {
		_password = *_preexistingBeforeSetNew_password;
	}
	_passwordType = _preexistingBeforeSetNew_passwordType;
	//
	optional<string> revert_save_errStr_orNone = saveToDisk();
	if (revert_save_errStr_orNone != none) {
		BOOST_THROW_EXCEPTION(logic_error("Couldn't saveToDisk to revert failed changePassword")); // in debug mode, treat this as fatal
	} else { // continue trying to revert
		optional<EnterPW_Fn_ValidationErr_Code> revert_registrantsChangePw_err_orNone = _changePassword_tellRegistrants_doChangePassword(); // this may well fail
		if (revert_registrantsChangePw_err_orNone != none) {
			BOOST_THROW_EXCEPTION(logic_error("Some registrants couldn't revert failed changePassword")); // in debug mode, treat this as fatal
		} else {
			// revert successful
		}
	}
	// finally, notify of error while changing password
	unguard_getNewOrExistingPassword(); // important
	erroredWhileSettingNewPassword_signal(*changePassword_err_orNone); // the original changePassword_err_orNone
}
//
// Imperatives - Persistence
optional<string>/*err_str*/ Controller::saveToDisk()
{
	if (_password == none) {
		return string("Code fault: saveToDisk musn't be called until a password has been set");
	}
	_messageAsEncryptedDataForUnlockChallenge_base64String = Persistable::new_encryptedBase64StringFrom( // it's important that we hang onto this in memory so we can access it if we need to change the password later
		plaintextMessageToSaveForUnlockChallenges,
		*_password
	);
	if (_id == none) {
		_id = document_persister::new_documentId();
	}
	property_tree::ptree persistableDocument;
	persistableDocument.put(_dictKey(DictKey::_id), *_id);
	persistableDocument.put(_dictKey(DictKey::passwordType), _passwordType);
	persistableDocument.put(_dictKey(DictKey::messageAsEncryptedDataForUnlockChallenge_base64String), *_messageAsEncryptedDataForUnlockChallenge_base64String);
	const std::string plaintextString = Persistable::new_plaintextJSONStringFromDocumentDict(persistableDocument);
	optional<string> err_str = document_persister::write(*documentsPath, plaintextString, *_id, collectionName);
	if (err_str != none) {
		MERROR("Passwords: Error while persisting " << *err_str);
	}
	return err_str;
}
//
// Runtime - Imperatives - Private - Setting/changing Password
void Controller::obtainNewPasswordFromUser(bool isForChangePassword)
{
	_preexistingBeforeSetNew_wasFirstSetOfPasswordAtRuntime = hasUserEnteredValidPasswordYet() == false; // it's ok if we derive this here instead of in obtainNewPasswordFromUser because this fn will only be called, if setting the pw for the first time, if we have not yet accepted a valid PW yet
	// for possible revert:
	_preexistingBeforeSetNew_password = _password; // this may be 'none'
	_preexistingBeforeSetNew_passwordType = _passwordType;
	_preexistingBeforeSetNew_isForChangePassword = isForChangePassword;
	//
	isWaitingFor_enterNewPassword_cb = true;
	//
	(*_passwordEntryDelegate).getUserToEnterNewPasswordAndType(isForChangePassword);
}
//
// Runtime - Imperatives - Password change
void Controller::addRegistrantForChangePassword(ChangePasswordRegistrant &registrant)
{
	MDEBUG("Passwords: Adding registrant for 'ChangePassword'");
	ptrsTo_changePasswordRegistrants.push_back(&registrant);
}
void Controller::removeRegistrantForChangePassword(ChangePasswordRegistrant &registrant)
{
	size_t index = 0;
	bool found = false;
	for (std::vector<ChangePasswordRegistrant *>::iterator it = ptrsTo_changePasswordRegistrants.begin(); it != ptrsTo_changePasswordRegistrants.end(); ++it) {
		if (**it == registrant) {
			found = true;
			break;
		}
		index++;
	}
	if (!found) {
		BOOST_THROW_EXCEPTION(logic_error("registrant is not registered"));
		return;
	}
	MDEBUG("Passwords: Removing registrant for 'ChangePassword'");
	ptrsTo_changePasswordRegistrants.erase(ptrsTo_changePasswordRegistrants.begin() + index);
}
void Controller::initiate_changePassword()
{
	onceBooted([this]()
	{
		if (hasUserEnteredValidPasswordYet() == false) {
			BOOST_THROW_EXCEPTION(logic_error("initiate_changePassword called but hasUserEnteredValidPasswordYet == false. This should be disallowed in the UI"));
			return;
		}
		{ // guard
			if (_isAlreadyGettingExistingOrNewPWFromUser == true) {
				BOOST_THROW_EXCEPTION(logic_error("initiate_changePassword called but _isAlreadyGettingExistingOrNewPWFromUser == true. This should be precluded in the UI"));
				// only need to wait for it to be obtained
				return;
			}
			_isAlreadyGettingExistingOrNewPWFromUser = true;
		}
		// ^-- we're relying on having checked above that user has entered a valid pw already
		bool isForChangePassword = true; // we'll use this in a couple places
		_getUserToEnterTheirExistingPassword(
			isForChangePassword,
			false, // isForAuthorizingAppActionOnly
			none, // customNavigationBarTitle
			[this, isForChangePassword] (
				optional<bool> didCancel_orNone,
				optional<EnterPW_Fn_ValidationErr_Code> validationErr_orNone,
				optional<Password> obtainedPasswordString
			) -> void {
				if (validationErr_orNone != none) { // takes precedence over cancel
					unguard_getNewOrExistingPassword();
					errorWhileChangingPassword_signal(*validationErr_orNone);
					return;
				}
				if (didCancel_orNone != none && *didCancel_orNone == true) {
					{ // these must be cleared since it's a cancelation
						isWaitingFor_enterExistingPassword_cb = false;
						enterExistingPassword_final_fn = none;
					}
					unguard_getNewOrExistingPassword();
					canceledWhileChangingPassword_signal();
					return; // just silently exit after unguarding
				}
				if (withExistingPassword_isCorrect(*obtainedPasswordString) == false) {
					unguard_getNewOrExistingPassword();
					errorWhileChangingPassword_signal(incorrectPassword);
					return;
				}
				{ // these must be cleared since we can proceed and it's not a cancelation
					isWaitingFor_enterExistingPassword_cb = false;
					enterExistingPassword_final_fn = none;
				}
				//
				// passwords match checked as necessary, we can proceed
				obtainNewPasswordFromUser(isForChangePassword);
			}
		);
	});
}
optional<EnterPW_Fn_ValidationErr_Code> Controller::_changePassword_tellRegistrants_doChangePassword()
{
	for (std::vector<ChangePasswordRegistrant *>::iterator it = ptrsTo_changePasswordRegistrants.begin(); it != ptrsTo_changePasswordRegistrants.end(); ++it) {
		optional<EnterPW_Fn_ValidationErr_Code> registrant__err_code = (*it)->passwordController_ChangePassword();
		if (registrant__err_code != none) {
			return *registrant__err_code;
		}
	}
	return none;
}
//
// Imperatives - Delete everything
void Controller::addRegistrantForDeleteEverything(DeleteEverythingRegistrant &registrant)
{
	MDEBUG("Passwords: Adding registrant for 'DeleteEverything'");
	ptrsTo_deleteEverythingRegistrants.push_back(&registrant);
}
void Controller::removeRegistrantForDeleteEverything(DeleteEverythingRegistrant &registrant)
{
	size_t index = 0;
	bool found = false;
	for (std::vector<DeleteEverythingRegistrant *>::iterator it = ptrsTo_deleteEverythingRegistrants.begin(); it != ptrsTo_deleteEverythingRegistrants.end(); ++it) {
		if (**it == registrant) {
			found = true;
			break;
		}
		index++;
	}
	if (!found) {
		BOOST_THROW_EXCEPTION(logic_error("registrant is not registered"));
		return;
	}
	MDEBUG("Passwords: Removing registrant for 'DeleteEverything'");
	ptrsTo_deleteEverythingRegistrants.erase(ptrsTo_deleteEverythingRegistrants.begin() + index);
}
void Controller::initiateDeleteEverything()
{ // this is used as a central initiation/sync point for delete everything like user idle
	// maybe it should be moved, maybe not.
	// And note we're assuming here the PW has been entered already.
	if (hasUserSavedAPassword() != true) {
		BOOST_THROW_EXCEPTION(logic_error("initiateDeleteEverything() called but hasUserSavedAPassword != true. This should be disallowed in the UI."));
		return;
	}
	_deconstructBootedStateAndClearPassword(
		true, // isForADeleteEverything
		[this](std::function<void(optional<string> err_str)> cb) {
			// reset state cause we're going all the way back to pre-boot
			hasBooted = false; // require this pw controller to boot
			_password = none; // this is redundant but is here for clarity
			_id = none;
			_messageAsEncryptedDataForUnlockChallenge_base64String = none;
			//
			// first have registrants delete everything
			for (std::vector<DeleteEverythingRegistrant *>::iterator it = ptrsTo_deleteEverythingRegistrants.begin(); it != ptrsTo_deleteEverythingRegistrants.end(); ++it) {
				optional<string> registrant__err_str = (*it)->passwordController_DeleteEverything();
				if (registrant__err_str != none) {
					cb(registrant__err_str);
					return;
				}
			}
			//
			// then delete pw record
			errOr_numRemoved result = document_persister::removeAllDocuments(*documentsPath, collectionName);
			if (result.err_str != none) {
				cb(std::move(*result.err_str));
				return;
			}
			MDEBUG("Passwords: Deleted password record.");
			//
			initializeRuntimeAndBoot(); // now trigger a boot before we call cb (tho we could do it after - consumers will wait for boot)
			cb(none);
		},
		[this](optional<string> err_str) {
			if (err_str != none) {
				MERROR("Passwords: Error while deleting everything: " << *err_str);
				BOOST_THROW_EXCEPTION(logic_error("Error while deleting everything"));
				// we probably want to just fatalError here since password etc has been un-set - user can always relaunch
//				fatalError("Error while deleting everything");
				return;
			}
			havingDeletedEverything_didDeconstructBootedStateAndClearPassword_signal();
		}
	);
}
//
// Runtime - Imperatives - App lock down interface (special case usage only)
void Controller::lockDownAppAndRequirePassword()
{ // just a public interface for this - special-case-usage only!
	if (hasUserEnteredValidPasswordYet() == false) { // this is fine, but should be used to bail
		MWARNING("Passwords: Asked to lockDownAppAndRequirePassword but no password entered yet.");
		return;
	}
	MDEBUG("Passwords: Will lockDownAppAndRequirePassword");
	_deconstructBootedStateAndClearPassword(
		false, // not for a delete-everything
		{},
		{}
	);
}
//
// Runtime - Imperatives - Boot-state deconstruction/teardown
void Controller::_deconstructBootedStateAndClearPassword(
	bool isForADeleteEverything,
	std::function<void(std::function<void(optional<string> err_str)> cb)> optl__hasFiredWill_fn,
	std::function<void(optional<string> err_str)> optl__fn
) {
	auto hasFiredWill_fn = optl__hasFiredWill_fn
		? optl__hasFiredWill_fn
		: [](std::function<void(optional<string> err_str)> cb) { cb(none); };
	auto fn = optl__fn ? optl__fn : [](optional<string> err_str) {};
	//
	// TODO:? do we need to cancel any waiting functions here? not sure it would be possible to have any (unless code fault)…… we'd only deconstruct the booted state and pop the enter pw screen here if we had already booted before - which means there shouldn't be such waiting functions - so maybe assert that here - which requires hanging onto those functions somehow
	willDeconstructBootedStateAndClearPassword_signal(isForADeleteEverything); // indicate to consumers they should tear down and await the "did" event to re-request
	//
	hasFiredWill_fn([fn = std::move(fn), this](optional<string> err_str) {
		if (err_str != none) {
			fn(std::move(*err_str));
			return;
		}
		{ // trigger deconstruction of booted state and require password
			_password = none; // clear pw in memory
			hasBooted = false; // require this pw controller to boot
			_id = none;
			_messageAsEncryptedDataForUnlockChallenge_base64String = none;
		}
		{ // we're not going to call WhenBootedAndPasswordObtained_PasswordAndType because consumers will call it for us after they tear down their booted state with the "will" event and try to boot/decrypt again when they get this "did" event
			didDeconstructBootedStateAndClearPassword_signal();
		}
		initializeRuntimeAndBoot(); // now trigger a boot before we call cb (tho we could do it after - consumers will wait for boot)
		fn(none);
	});
}
//
// Delegation - Password
void Controller::_didObtainPassword(Password password)
{
	_password = password;
}
//
// Delegation - User Idle
void Controller::UserIdle_userDidBecomeIdle()
{
	if (hasUserSavedAPassword() == false) {
		// nothing to do here because the app is not unlocked and/or has no data which would be locked
		MDEBUG("Passwords: User became idle but no password has ever been entered/no saved data should exist.");
		return;
	} else if (hasUserEnteredValidPasswordYet() == false) {
		// user has saved data but hasn't unlocked the app yet
		MDEBUG("Passwords: User became idle and saved data/pw exists, but user hasn't unlocked app yet.");
		return;
	}
	_didBecomeIdleAfterHavingPreviouslyEnteredPassword();
}
//
// Delegation - User having become idle -> teardown booted state and require pw
void Controller::_didBecomeIdleAfterHavingPreviouslyEnteredPassword()
{
	_deconstructBootedStateAndClearPassword(
		false, // not for a delete-everything
		{},
		{}
	);
}

//
//  PasswordController.hpp
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

#ifndef PasswordController_hpp
#define PasswordController_hpp

#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include "../Persistence/document_persister.hpp"
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../UserIdle/UserIdle.hpp"

namespace Passwords
{
	using namespace std;
	//
	typedef string Password;
}
namespace Passwords
{
	using namespace std;
	//
	enum Type
	{ // These are given specific values for the purpose of serialization through the app lib bridge and more critically for persistence, so they should not be changed
		minBound = -1,
		//
		PIN = 0,
		password = 1,
		//
		maxBound = 2
	};
	//
	static inline std::string new_humanReadableString(Type type)
	{ // TODO: this will probably need to be localized at the application layer anyway
		return std::string(
			type == PIN ? "PIN" : "password" // TODO: return localized
		);
	}
	static inline std::string capitalized_humanReadableString(Type type)
	{ // TODO: this will probably need to be localized at the application layer anyway
		std::string str = new_humanReadableString(type);
		str[0] = (char)toupper(str[0]);
		//
		return str;
	}
	static inline std::string new_invalidEntry_humanReadableString(Type type) // TOOD: return localized
	{ // TODO: maybe just keep this function at the Android level so it can be localized .. or maybe better to ship localizations with app lib so they're not duplicated everywhere? haven't confirmed C++ lib best practice yet
		return type == PIN ? "Incorrect PIN" : "Incorrect password";
	}
	static inline Type new_detectedFromPassword(Password &password)
	{
		std::string copyOf_password = password;
		copyOf_password.erase(boost::remove_if(copyOf_password, ::isdigit), copyOf_password.end());

		return copyOf_password.empty() ? Type::PIN : Type::password;
	}
}
namespace Passwords
{
	class PasswordControllerEventParticipant
	{ // abstract interface - implement with another interface
	public:
		virtual ~PasswordControllerEventParticipant() {}
		//
		virtual std::string identifier() const = 0; // To support isEqual, ==
		bool operator==(PasswordControllerEventParticipant const &rhs) const
		{
			return identifier() == rhs.identifier();
		}
		bool operator!=(PasswordControllerEventParticipant const &rhs) const
		{
			return (*this == rhs) == false;
		}
	};
}
namespace Passwords
{
	using namespace std;
	using namespace boost;
	//
	class PasswordEntryDelegate : public PasswordControllerEventParticipant
	{
	public:
		virtual ~PasswordEntryDelegate() {}
		//
		virtual void getUserToEnterExistingPassword(
			bool isForChangePassword,
			bool isForAuthorizingAppActionOnly, // normally no - this is for things like SendFunds
			optional<string> customNavigationBarTitle
		) = 0;
		// then call Controller::enterExistingPassword_cb
		//
		virtual void getUserToEnterNewPasswordAndType(
			bool isForChangePassword
		) = 0;
		// then call Controller::enterNewPasswordAndType_cb
	};
}
namespace Passwords
{ // Constants
	static const uint32_t minPasswordLength = 6;
	static const uint32_t maxLegal_numberOfTriesDuringThisTimePeriod = 5;
	static const size_t pwEntrySpamming_unlockInT_s = 10;
	static const size_t pwEntrySpamming_unlockInT_ms = pwEntrySpamming_unlockInT_s * 1000;
	//
	enum EnterPW_Fn_ValidationErr_Code
	{
		pleaseWaitBeforeTryingAgain = 0,
		incorrectPassword = 1,
		enterLongerPIN = 2,
		enterLongerPassword = 3,
		unrecognizedPasswordType = 4,
		enterFreshPIN = 5,
		enterFreshPassword = 6,
		saveError = 7,
		changePasswordError = 8,
		clearValidationErrorAndAllowRetry = 9,
		notBootedYet = 10,
		unexpectedState = 11
	};
}
namespace Passwords
{
	using namespace std;
	using namespace boost;
	//
	class ChangePasswordRegistrant: public PasswordControllerEventParticipant
	{ // Implement this function to support change-password events as well as revert-from-failed-change-password
	public:
		virtual optional<EnterPW_Fn_ValidationErr_Code> passwordController_ChangePassword() = 0; // return err_str:String if error - it will abort and try to revert the changepassword process. at time of writing, this was able to be kept synchronous.
		// TODO: ^-- maybe make this return a code instead of an error string
	};
	class DeleteEverythingRegistrant: public PasswordControllerEventParticipant
	{
	public:
		virtual optional<string> passwordController_DeleteEverything() = 0; // return err_str:String if error. at time of writing, this was able to be kept synchronous.
		// TODO: ^-- maybe make this return a code instead of an error string
	};
}
namespace Passwords
{
	using namespace std;
	using namespace boost;
	using namespace document_persister;
	//
	// Interfaces
	class PasswordProvider
	{ // you can use this type for dependency-injecting a Passwords::Controller implementation; see PersistableObject
	public:
		virtual ~PasswordProvider() {}
		virtual boost::optional<Password> getPassword() const = 0;
	};
	//
	// Controllers
	class Controller: public PasswordProvider, std::enable_shared_from_this<Controller>
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
			cout << "Destructing a Passwords::Controller" << endl;
			teardown();
		}
		//
		// Dependencies
		std::shared_ptr<string> documentsPath;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
		std::shared_ptr<UserIdle::Controller> userIdleController;
		// Then call:
		void setup();
		//
		// Signals
		boost::signals2::signal<void()> obtainedNewPassword_signal;
		boost::signals2::signal<void()> obtainedCorrectExistingPassword_signal;
		//
		boost::signals2::signal<void(EnterPW_Fn_ValidationErr_Code code)> erroredWhileGettingExistingPassword_signal;
		boost::signals2::signal<void(EnterPW_Fn_ValidationErr_Code code)> erroredWhileSettingNewPassword_signal;
		boost::signals2::signal<void()> canceledWhileEnteringExistingPassword_signal;
		boost::signals2::signal<void()> canceledWhileEnteringNewPassword_signal;
		//
		boost::signals2::signal<void()> setFirstPasswordDuringThisRuntime_signal;
		boost::signals2::signal<void()> registrantsAllChangedPassword_signal;
		//
		boost::signals2::signal<void()> canceledWhileChangingPassword_signal;
		boost::signals2::signal<void(EnterPW_Fn_ValidationErr_Code code)> errorWhileChangingPassword_signal;
		//
		boost::signals2::signal<void()> duringAuthentication_tryBiometrics_signal;
		boost::signals2::signal<void(EnterPW_Fn_ValidationErr_Code code)> errorWhileAuthorizingForAppAction_signal;
		boost::signals2::signal<void()> successfullyAuthenticatedForAppAction_signal;
		//
		boost::signals2::signal<void(bool isForADeleteEverything)> willDeconstructBootedStateAndClearPassword_signal;
		boost::signals2::signal<void()> didDeconstructBootedStateAndClearPassword_signal;
		boost::signals2::signal<void()> havingDeletedEverything_didDeconstructBootedStateAndClearPassword_signal;
		//
		// Accessors - Interfaces - PasswordProvider
		optional<Password> getPassword() const;
		// Accessors - Members
		Passwords::Type getPasswordType() const;
		// Accessors - Persistence state
		bool hasUserSavedAPassword() const;
		//
		// Accessors - Derived properties
		bool hasUserEnteredValidPasswordYet() const;
		bool isUserChangingPassword() const;
		//
		// Imperatives - PasswordEntryDelegate
		void setPasswordEntryDelegate(PasswordEntryDelegate &to_delegate);
		void clearPasswordEntryDelegate(PasswordEntryDelegate &from_existing_delegate);
		//
		void TEST_resetPasswordControllerInitAndObservers();
		void TEST_clearUnlockTimer();
		void TEST_bypassCheckAndClear_passwordEntryDelegate();
		//
		// Imperatives - Execution Deferment
		void onceBooted(std::function<void()> fn);
		//
		void onceBootedAndPasswordObtained(
			std::function<void(Password password, Passwords::Type passwordType)>&& fn,
			std::function<void()> userCanceled_fn
		);
		//
		void addRegistrantForChangePassword(ChangePasswordRegistrant &registrant);
		void removeRegistrantForChangePassword(ChangePasswordRegistrant &registrant);
		void initiate_changePassword();
		//
		void addRegistrantForDeleteEverything(DeleteEverythingRegistrant &registrant);
		void removeRegistrantForDeleteEverything(DeleteEverythingRegistrant &registrant);
		void initiateDeleteEverything();
		//
		void lockDownAppAndRequirePassword();
		//
		// Imperatives - Password entry attempt callbacks - to be called by the PasswordEntryDelegate
		void enterExistingPassword_cb( // this will throw an exception if called while the controller is not waiting for it
			boost::optional<bool> didCancel,
			boost::optional<Password> obtainedPasswordString
		);
		void enterNewPasswordAndType_cb( // this will throw an exception if called while the controller is not waiting for it
			boost::optional<bool> didCancel,
			boost::optional<Password> obtainedPasswordString,
			boost::optional<Type> userSelectedTypeOfPassword
		);
		//
		void initiate_verifyUserAuthenticationForAction(
			bool tryBiometrics,
			optional<string> customNavigationBarTitle_orNone,
			std::function<void()> canceled_fn,
			std::function<void()> entryAttempt_succeeded_fn // required
		);
		void authenticationCB_biometricsSucceeded();
		void authenticationCB_biometricsFailed();
		void authenticationCB_biometricsUnavailable();
		void authenticationCB_biometricsCanceled();
		//
	private:
		//
		// Properties - Instance members
		bool hasBooted = false;
		optional<DocumentId> _id = none;
		optional<Password> _password = none;
		Passwords::Type _passwordType;
		boost::optional<string> _messageAsEncryptedDataForUnlockChallenge_base64String = none;
		boost::optional<bool> _isAlreadyGettingExistingOrNewPWFromUser = none;
		//
		bool _preexistingBeforeSetNew_wasFirstSetOfPasswordAtRuntime;
		optional<Password> _preexistingBeforeSetNew_password = none;
		Passwords::Type _preexistingBeforeSetNew_passwordType;
		bool _preexistingBeforeSetNew_isForChangePassword;
		//
		void _callAndFlushAllBlocksWaitingForBootToExecute();
		optional<vector<std::function<void()>>> __blocksWaitingForBootToExecute = none;
		// NOTE: onceBooted() exists because even though init()->setup() is synchronous, we need to be able to tear down and reconstruct the passwordController booted state, e.g. on user idle and delete everything
		//
		Passwords::PasswordEntryDelegate *_passwordEntryDelegate = nullptr; // someone in the app must set this by calling setPasswordEntryDelegate()
		//
		bool isWaitingFor_enterExistingPassword_cb = false;
		optional<std::function<void(
			optional<bool> didCancel_orNone,
			optional<EnterPW_Fn_ValidationErr_Code> validationErr_orNone,
			optional<Password> obtainedPasswordString
		)>> enterExistingPassword_final_fn = none;
		bool _isCurrentlyLockedOutFromPWEntryAttempts = false;
		size_t _numberOfTriesDuringThisTimePeriod = 0;
		optional<time_t> _dateOf_firstPWTryDuringThisTimePeriod;
		std::unique_ptr<Dispatch::CancelableTimerHandle> _pw_entry_unlock_timer_handle; // initialized to nullptr
		bool isWaitingFor_enterNewPassword_cb = false;
		//
		// Authentication
		optional<string> _waitingForAuth_customNavigationBarTitle_orNone;
		optional<std::function<void()>> _waitingForAuth_canceled_fn;
		optional<std::function<void()>> _waitingForAuth_entryAttempt_succeeded_fn;
		void _proceedTo_authenticateVia_passphrase();
		void _authentication__callBackWithEntryAttempt_succeeded();
		void _authentication__callBackWithEntryAttempt_canceled();
		//
		// Accessors
		bool withExistingPassword_isCorrect(const string &enteredPassword) const;
		//
		// Imperatives
		void teardown();
		void startObserving();
		void initializeRuntimeAndBoot();
		void _proceedTo_load(const DocumentJSON &documentJSON);
		// Imperatives - Persistence
		optional<string>/*err_str*/ saveToDisk();
		//
		void givenBooted_initiateGetNewOrExistingPasswordFromUserAndEmitIt();
		//
		void unguard_getNewOrExistingPassword();
		void _getUserToEnterTheirExistingPassword(
			bool isForChangePassword,
			bool isForAuthorizingAppActionOnly,
			optional<string> customNavigationBarTitle,
			std::function<void(
				optional<bool> didCancel_orNone,
				optional<EnterPW_Fn_ValidationErr_Code> validationErr_orNone,
				optional<Password> obtainedPasswordString
			)> fn
		);
		void __cancelAnyAndRebuildUnlockTimer();
		//
		void obtainNewPasswordFromUser(bool isForChangePassword);
		//
		std::vector<ChangePasswordRegistrant *> ptrsTo_changePasswordRegistrants;
		optional<EnterPW_Fn_ValidationErr_Code> _changePassword_tellRegistrants_doChangePassword();
		//
		std::vector<DeleteEverythingRegistrant *> ptrsTo_deleteEverythingRegistrants;
		void _deconstructBootedStateAndClearPassword(
			bool isForADeleteEverything,
			std::function<void(std::function<void(optional<string> err_str)> cb)> optl__hasFiredWill_fn,
			std::function<void(optional<string> err_str)> optl__fn
		);
		//
		// Delegation - State
		void _didObtainPassword(Password password);
		//
		// Delegation - Events
		void UserIdle_userDidBecomeIdle();
		void _didBecomeIdleAfterHavingPreviouslyEnteredPassword();
	};
}

#endif /* PasswordController_hpp */

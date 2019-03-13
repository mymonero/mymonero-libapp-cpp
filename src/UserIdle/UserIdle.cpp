//
//  UserIdle.cpp
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
#include "UserIdle.hpp"
using namespace UserIdle;
#include "misc_log_ex.h"
#include "../Settings/SettingsController.hpp"
//
// Imperatives - Lifecycle
void Controller::setup()
{
	_initiate_userIdle_intervalTimer();
}
//
// Imperatives - Interface
void Controller::temporarilyDisable_userIdle()
{
	_numberOfRequestsToLockUserIdleAsDisabled += 1;
	if (_numberOfRequestsToLockUserIdleAsDisabled == 1) { // if we're requesting to disable without it already having been disabled, i.e. was 0, now 1
		MDEBUG("App.UserIdle: Temporarily disabling the user idle timer.");
		__disable_userIdle();
	} else {
		MDEBUG("App.UserIdle: Requested to temporarily disable user idle but already disabled. Incremented lock.");
	}
}
void Controller::reEnable_userIdle()
{
	if (_numberOfRequestsToLockUserIdleAsDisabled == 0) {
		MDEBUG("App.UserIdle: ReEnable_userIdle, self._numberOfRequestsToLockUserIdleAsDisabled 0");
		return; // don't go below 0
	}
	_numberOfRequestsToLockUserIdleAsDisabled -= 1;
	if (_numberOfRequestsToLockUserIdleAsDisabled == 0) {
		MDEBUG("App.UserIdle: Re-enabling the user idle timer.");
		__reEnable_userIdle();
	} else {
		MDEBUG("App.UserIdle: Requested to re-enable user idle but other locks still exist.");
	}
}
//
// Imperatives - Internal
void Controller::__disable_userIdle()
{
	if (_userIdle_intervalTimer == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("__disable_userIdle called but already have nil self.userIdle_intervalTimer"));
		return;
	}
	_userIdle_intervalTimer->cancel();
	_userIdle_intervalTimer = nullptr;
}
void Controller::__reEnable_userIdle()
{
	if (_userIdle_intervalTimer != nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("__reEnable_userIdle called but non-nil self.userIdle_intervalTimer"));
		return;
	}
	_initiate_userIdle_intervalTimer();
}
//
void Controller::_initiate_userIdle_intervalTimer()
{
	if (_userIdle_intervalTimer != nullptr) { // necessary?
		BOOST_THROW_EXCEPTION(logic_error("Expected _userIdle_intervalTimer == nullptr"));
		return;
	}
	//
	// TOOD: synchronize this to main thread ?
//	DispatchQueue.main.async
//	{ [unowned self] in
		_dateOfLastUserInteraction = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // reset this in case the app disabled user idle at a time at all different from when the last idle breaking action occurred - s since last user interaction should always be 0 when the userIdle timer starts anyway
		//
		__create_repeating_timer();
//	}
}
void Controller::__create_repeating_timer()
{
	_userIdle_intervalTimer = dispatch_ptr->after(1 * 1000, [this]()
	{
		checkIdleTimeout();
		//
		// we will never get this callback called if the timer is canceled, so it's safe to just re-enter (recreate) here
		__create_repeating_timer();
	});
}
//
// Imperatives - Publicly callable
void Controller::checkIdleTimeout()
{
	optional<double> appTimeoutAfterS_orNone_orNeverValue = settingsController->appTimeoutAfterS_noneForDefault_orNeverValue;
	double appTimeoutAfterS = appTimeoutAfterS_orNone_orNeverValue != none ? *appTimeoutAfterS_orNone_orNeverValue : 20.0; // use default on no pw entered / no settings info yet
	if (appTimeoutAfterS == Settings::appTimeoutAfterS_neverValue) { // then idle timer is specifically disabled
		return; // do nothing
	}
	time_t now;
	time(&now); // get current time; same as: now = time(NULL)
	double sSinceLastInteraction = difftime(now, _dateOfLastUserInteraction);
	if (sSinceLastInteraction >= appTimeoutAfterS) {
		if (isUserIdle == false) { // not already idle (else redundant)
			_userDidBecomeIdle();
		}
	}
}
//
// TODO: subscribe to a bridge?
////
//// Delegation - Notifications
//void application_didSendEvent(_ notification: Notification)
//{
//	self._idleBreakingActionOccurred()
//	// TODO: also detect when app is being controlled w/o touching the screen - e.g. via Simulator keyboard (or perhaps external)…
//}
//
// Delegation - Internal
void Controller::_idleBreakingActionOccurred()
{
	bool wasUserIdle = isUserIdle;
	_userDidInteract();
	if (wasUserIdle) { // emit after we have set isUserIdle back to false
		_userDidComeBackFromIdle();
	}
}
void Controller::_userDidInteract()
{
	_dateOfLastUserInteraction = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); // reset counter
}
void Controller::_userDidComeBackFromIdle()
{
	isUserIdle = false; // in case they were
	MDEBUG("App.UserIdle: User came back from having been idle.");
	userDidComeBackFromIdle_signal();
}
void Controller::_userDidBecomeIdle()
{
	isUserIdle = true;
	MDEBUG("App.UserIdle: User became idle.");
	userDidBecomeIdle_signal();
}

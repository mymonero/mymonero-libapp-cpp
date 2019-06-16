//
//  Wallet_HostPollingController.cpp
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
#include "Wallet_HostPollingController.hpp"
using namespace Wallets;
#include "../APIClient/HostedMonero.hpp"
#include "misc_log_ex.h"
#include "Wallet.hpp"
//
// Lifecycle
void HostPollingController::setup()
{
	if (_dispatch_ptr == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("HostPollingController: expected dispatch_ptr != nullptr"));
	}
	startPollingTimer();
	// ^ just immediately going to jump into the runtime - so only instantiate self when you're ready to do this
	//
	performRequests();
}
void HostPollingController::tearDown()
{
	invalidateTimer();
	{
		if (_requestHandleFor_addressInfo != nullptr) {
			_requestHandleFor_addressInfo->cancel();
			_requestHandleFor_addressInfo = nullptr;
		}
		if (_requestHandleFor_addressTransactions != nullptr) {
			_requestHandleFor_addressTransactions->cancel();
			_requestHandleFor_addressTransactions = nullptr;
		}
	}
	_didUpdate_factorOf_isFetchingAnyUpdates(); // unsure if emitting is desired here but it probably isn't harmful
	_didUpdate_factorOf_isFetchingAnyUpdates_fn = {}; // zero (though not strictly necessary)
}
//
// Imperatives - Timer
void HostPollingController::startPollingTimer()
{
	timer_mutex.lock();
	{
		if (_timer != nullptr) { // necessary?
			BOOST_THROW_EXCEPTION(logic_error("HostPollingController: Expected _timer == nullptr"));
			return;
		}
		__givenLocked_create_repeating_timer();
	}
	timer_mutex.unlock();
}
static const size_t pollingTimerPeriod = 30;
void HostPollingController::__givenLocked_create_repeating_timer()
{
	if (_timer != nullptr) { // necessary?
		BOOST_THROW_EXCEPTION(logic_error("HostPollingController: __givenLocked_create_repeating_timer: Expected _timer == nullptr"));
		return;
	}
//	if (isTornDown) {
//		BOOST_THROW_EXCEPTION(logic_error("__givenLocked_create_repeating_timer: Expected isTornDown == false"));
//		return;
//	}
	std::shared_ptr<HostPollingController> shared_this = shared_from_this();
	std::weak_ptr<HostPollingController> weak_this = shared_this;
	_timer = _dispatch_ptr->after(pollingTimerPeriod * 1000, [weak_this]()
	{
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->performRequests();
			//
			// we will never get this callback called if the timer is canceled, so it's safe to just re-enter (recreate) here
			inner_spt->timer_mutex.lock();
			{
				inner_spt->_timer = nullptr; // since we're finished with it
				inner_spt->__givenLocked_create_repeating_timer(); // re-enter
			}
			inner_spt->timer_mutex.unlock();
		}
	});
}
void HostPollingController::invalidateTimer()
{
	timer_mutex.lock();
	{
		if (_timer != nullptr) {
			_timer->cancel();
			_timer = nullptr;
		}
	}
	timer_mutex.unlock();
}
//
// Imperatives - Requests
void HostPollingController::performRequests()
{
	_fetch_addressInfo();
	_fetch_addressTransactions();
}
std::shared_ptr<Wallets::Object> HostPollingController::__walletSPTIfAbleToPerformRequests()
{
	auto wallet_spt = _wallet.lock();
	if (wallet_spt) {
		if (wallet_spt->isLoggedIn() != true) {
			MERROR("Wallets: Unable to do request while not isLoggedIn");
		} else  if (wallet_spt->public_address().empty()) {
			MERROR("Wallets: Unable to do request for wallet w/o public_address");
		} else if (wallet_spt->view_sec_key().empty() || wallet_spt->spend_sec_key().empty()) {
			MERROR("Wallets: Unable to do request for wallet w/o private_keys");
		}
	} else {
		MWARNING("Wallets.Wallet_HostPollingController: Wallet already freed");
	}
	return wallet_spt; // will return nullptr if unable to lock weak_ptr
}
void HostPollingController::_fetch_addressInfo()
{
	if (_requestHandleFor_addressInfo != nullptr) {
		MWARNING("Wallets: _fetch_addressInfo called but request already exists");
		return;
	}
	auto wallet_spt = __walletSPTIfAbleToPerformRequests();
	if (!wallet_spt) {
		return; // already been logged
	}
	std::shared_ptr<HostPollingController> shared_this = shared_from_this();
	std::weak_ptr<HostPollingController> weak_this = shared_this;
	_requestHandleFor_addressInfo = _apiClient->addressInfo(
		wallet_spt->lazy_keyImageCache(),
		wallet_spt->public_address(),
		wallet_spt->view_sec_key(),
		wallet_spt->spend_pub_key(),
		wallet_spt->spend_sec_key(),
		[weak_this] (
			optional<string> err_str,
			optional<HostedMonero::ParsedResult_AddressInfo> result
		) {
			if (auto inner_spt = weak_this.lock()) {
				if (inner_spt->_requestHandleFor_addressInfo == nullptr) {
					BOOST_THROW_EXCEPTION(logic_error("Already canceled"));
					return;
				}
				inner_spt->_dateOfLast_fetch_addressInfo = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				inner_spt->_requestHandleFor_addressInfo = nullptr; // first/immediately unlock this request fetch
				inner_spt->_didUpdate_factorOf_isFetchingAnyUpdates();
				//
				if (err_str != none) {
					return; // already logged err
				}
				auto wallet_spt = inner_spt->_wallet.lock();
				if (!wallet_spt) {
					MWARNING("Wallets: Wallet host polling request response returned but wallet already freed.");
					return;
				}
				wallet_spt->_HostPollingController_didFetch_addressInfo(*result);
			} else {
				MWARNING("Wallets.Wallet_HostPollingController: self already nil");
				return;
			}
		}
	);
	_didUpdate_factorOf_isFetchingAnyUpdates();
}
void HostPollingController::_fetch_addressTransactions()
{
	if (_requestHandleFor_addressTransactions != nullptr) {
		MWARNING("Wallets: _fetch_addressInfo called but request already exists");
		return;
	}
	auto wallet_spt = __walletSPTIfAbleToPerformRequests();
	if (!wallet_spt) {
		return; // already been logged
	}
	std::shared_ptr<HostPollingController> shared_this = shared_from_this();
	std::weak_ptr<HostPollingController> weak_this = shared_this;
	_requestHandleFor_addressTransactions = _apiClient->addressTransactions(
		wallet_spt->lazy_keyImageCache(),
		wallet_spt->public_address(),
		wallet_spt->view_sec_key(),
		wallet_spt->spend_pub_key(),
		wallet_spt->spend_sec_key(),
		[weak_this] (
			optional<string> err_str,
			optional<HostedMonero::ParsedResult_AddressTransactions> result
		) {
			if (auto inner_spt = weak_this.lock()) {
				if (inner_spt->_requestHandleFor_addressTransactions == nullptr) {
					BOOST_THROW_EXCEPTION(logic_error("Already canceled"));
					return;
				}
				inner_spt->_dateOfLast_fetch_addressTransactions = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
				inner_spt->_requestHandleFor_addressTransactions = nullptr; // first/immediately unlock this request fetch
				inner_spt->_didUpdate_factorOf_isFetchingAnyUpdates();
				//
				if (err_str != none) {
					return; // already logged err
				}
				auto wallet_spt = inner_spt->_wallet.lock();
				if (!wallet_spt) {
					MWARNING("Wallets: Wallet host polling request response returned but wallet already freed.");
					return;
				}
				wallet_spt->_HostPollingController_didFetch_addressTransactions(*result);
			} else {
				MWARNING("Wallets.Wallet_HostPollingController: self already nil");
				return;
			}
		}
	);
	_didUpdate_factorOf_isFetchingAnyUpdates();
}
//
// Imperatives - Manual refresh
static const double manualRefreshCoolDownMinimum_s = 10;
void HostPollingController::requestFromUI_manualRefresh()
{
	if (_requestHandleFor_addressInfo != nullptr || _requestHandleFor_addressTransactions != nullptr) {
		return; // still refreshing.. no need
	}
	// now since addressInfo and addressTransactions are nearly happening at the same time (with failures and delays unlikely), I'm just going to use time since addressTransactions to approximate length since last collective refresh
	auto current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	cout << "current_time " << current_time << endl;
	cout << "*_dateOfLast_fetch_addressTransactions " << _dateOfLast_fetch_addressTransactions << endl;
	cout << "difftime(current_time, *_dateOfLast_fetch_addressTransactions) " << difftime(current_time, *_dateOfLast_fetch_addressTransactions) << endl;
	bool hasBeenLongEnoughSinceLastRefreshToRefresh = _dateOfLast_fetch_addressTransactions == none /* we know a request is not _currently_ happening, so nil date means one has never happened */
		|| difftime(current_time, *_dateOfLast_fetch_addressTransactions) >= manualRefreshCoolDownMinimum_s;
	if (hasBeenLongEnoughSinceLastRefreshToRefresh) {
		// and here we again know we don't have any requests to cancel
		performRequests(); // approved manual refresh
		//
		invalidateTimer(); // clear and reset timer to push next fresh out by timer period
		startPollingTimer();
	}
}
//
// Delegation - isFetchingAnyUpdates
void HostPollingController::_didUpdate_factorOf_isFetchingAnyUpdates() // must be called manually
{
	optional<bool> previous_lastRecorded_isFetchingAnyUpdates = _lastRecorded_isFetchingAnyUpdates;
	bool current_isFetchingAnyUpdates = isFetchingAnyUpdates();
	_lastRecorded_isFetchingAnyUpdates = current_isFetchingAnyUpdates;
	if (previous_lastRecorded_isFetchingAnyUpdates == none || *previous_lastRecorded_isFetchingAnyUpdates != current_isFetchingAnyUpdates) { // Emit
		_didUpdate_factorOf_isFetchingAnyUpdates_fn();
	}
}

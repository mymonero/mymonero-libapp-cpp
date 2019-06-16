//
//  Wallet_TxCleanupController.cpp
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
#include "Wallet_TxCleanupController.hpp"
#include "./Wallet.hpp"
using namespace Wallets;
#include "misc_log_ex.h"
//
// Lifecycle
void TxCleanupController::setup()
{
	if (_dispatch_ptr == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("TxCleanupController: expected dispatch_ptr != nullptr"));
	}
	__do_localTxCleanupJob();
	startPollingTimer();
}
void TxCleanupController::tearDown()
{
	invalidateTimer();
}
//
// Imperatives - Timer
void TxCleanupController::startPollingTimer()
{
	timer_mutex.lock();
	{
		if (_timer != nullptr) { // necessary?
			BOOST_THROW_EXCEPTION(logic_error("TxCleanupController: Expected _timer == nullptr"));
			return;
		}
		__givenLocked_create_repeating_timer();
	}
	timer_mutex.unlock();
}
static const size_t pollingTimerPeriod = 60; // every minute
void TxCleanupController::__givenLocked_create_repeating_timer()
{
	if (_timer != nullptr) { // necessary?
		BOOST_THROW_EXCEPTION(logic_error("TxCleanupController: __givenLocked_create_repeating_timer: Expected _timer == nullptr"));
		return;
	}
	//	if (isTornDown) {
	//		BOOST_THROW_EXCEPTION(logic_error("__givenLocked_create_repeating_timer: Expected isTornDown == false"));
	//		return;
	//	}
	std::shared_ptr<TxCleanupController> shared_this = shared_from_this();
	std::weak_ptr<TxCleanupController> weak_this = shared_this;
	_timer = _dispatch_ptr->after(pollingTimerPeriod * 1000, [weak_this]()
	{
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->__do_localTxCleanupJob();
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
void TxCleanupController::invalidateTimer()
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
// Imperatives - Runtime
void TxCleanupController::__do_localTxCleanupJob()
{
	auto wallet_spt = _wallet.lock();
	if (!wallet_spt) {
		MWARNING("Wallets::TxCleanupController: Asked to __do_localTxCleanupJob() but self.wallet==nil; skipping.");
		return;
	}
	auto txs = wallet_spt->actual_noLock_transactions();
	if (txs == none || txs->size() == 0) {
		cout << "Wallet " << wallet_spt << " has no txs to look within to cleanup." << endl;
		return; // nothing to do
	}
	bool didChangeAny = false; // mutable
	double oneDayAndABit_s = double(60 * 60 * (24 + 1/*bit=1hr*/)); // and a bit to avoid possible edge cases
	auto tilConsideredRejected_s = oneDayAndABit_s; // to be clear
	auto time_now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	for (auto tx__it = txs->begin(); tx__it != txs->end(); tx__it++) {
		double sSinceCreation = difftime(time_now, (*tx__it).timestamp);
		if (sSinceCreation < 0) {
			BOOST_THROW_EXCEPTION(logic_error("Expected non-negative sSinceCreation"));
		}
		if (sSinceCreation > tilConsideredRejected_s) {
			if ((*tx__it).cached__isConfirmed == false || (*tx__it).mempool) {
				if ((*tx__it).isFailed != true/*already*/) {
					MDEBUG("Wallets::TxCleanupController: Marking transaction as dead: " << (*tx__it).hash);
					//
					didChangeAny = true;
					(*tx__it).isFailed = true;  // this flag does not need to get preserved on existing_txs when overwritten by an incoming_tx because if it's returned by the server, it can't be dead
					(*tx__it).isJustSentTransientTransactionRecord = false;
				}
			}
		}
	}
	if (didChangeAny) {
		wallet_spt->saveToDisk();
	}
}

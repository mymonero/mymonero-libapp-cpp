//
//  Wallet_HostPollingController.hpp
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

#ifndef HostPollingController_hpp
#define HostPollingController_hpp
//
#include <functional>
#include <mutex>
#include "../APIClient/HTTPRequests_Interface.hpp"
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../APIClient/HostedMonero.hpp"
//
namespace Wallets
{
	using namespace std;
	using namespace boost;
	//
	// Forward-declarations
	class Object;
	//
	// Controllers
	class HostPollingController: public std::enable_shared_from_this<HostPollingController>
	{
	public:
		//
		// Lifecycle - Init
		HostPollingController(
			std::weak_ptr<Object> wallet,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr,
			std::shared_ptr<HostedMonero::APIClient> apiClient,
			std::function<void()>&& didUpdate_factorOf_isFetchingAnyUpdates_fn
		):
		_wallet(wallet),
		_dispatch_ptr(dispatch_ptr),
		_apiClient(apiClient),
		_didUpdate_factorOf_isFetchingAnyUpdates_fn(std::move(didUpdate_factorOf_isFetchingAnyUpdates_fn))
		{
			// you MUST call setup after you instantiate the HostPollingController
		}
		~HostPollingController()
		{
			cout << "Destructor for a Wallets::HostPollingController" << endl;
			tearDown();
		}
		//
		// Dependencies
		//
		// Properties
		bool isFetchingAnyUpdates()
		{
			return _requestHandleFor_addressInfo != nullptr || _requestHandleFor_addressTransactions != nullptr;
		}
		//
		// Imperatives - Lifecycle / Instantiation
		void setup();
		// Imperatives - Runtime
		void requestFromUI_manualRefresh();
		//
	private:
		//
		// Properties
		std::weak_ptr<Object> _wallet;
		std::shared_ptr<Dispatch::Dispatch> _dispatch_ptr;
		std::shared_ptr<HostedMonero::APIClient> _apiClient;
		std::function<void()> _didUpdate_factorOf_isFetchingAnyUpdates_fn;
		//
		std::mutex timer_mutex;
		std::unique_ptr<Dispatch::CancelableTimerHandle> _timer; // initialized to nullptr
		//
		std::shared_ptr<HTTPRequests::Handle> _requestHandleFor_addressInfo;
		std::shared_ptr<HTTPRequests::Handle> _requestHandleFor_addressTransactions;
		//
		optional<time_t> _dateOfLast_fetch_addressInfo;
		optional<time_t> _dateOfLast_fetch_addressTransactions;
		optional<bool> _lastRecorded_isFetchingAnyUpdates;
		//
		// Imperatives - Lifecycle
		void tearDown();
		//
		// Accessors - Utility
		std::shared_ptr<Wallets::Object> __walletSPTIfAbleToPerformRequests();
		//
		// Imperatives - Timer
		void startPollingTimer();
		void invalidateTimer();
		void __givenLocked_create_repeating_timer();
		//
		// Imperatives - Requests
		void performRequests();
		void _fetch_addressInfo();
		void _fetch_addressTransactions();
		//
		// Delegation
		void _didUpdate_factorOf_isFetchingAnyUpdates();
	};
}

#endif /* HostPollingController_hpp */

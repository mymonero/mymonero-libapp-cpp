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

#ifndef TxCleanupController_hpp
#define TxCleanupController_hpp
//
//
#include <iostream>
#include "../Dispatch/Dispatch_Interface.hpp"
//
namespace Wallets
{
	using namespace std;
//	using namespace boost;
	//
	// Forward-declarations
	class Object;
	//
	// Controllers
	class TxCleanupController: public std::enable_shared_from_this<TxCleanupController>
	{
	public:
		//
		// Lifecycle - Init
		TxCleanupController(
			std::weak_ptr<Object> wallet,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr
		):
		_wallet(wallet),
		_dispatch_ptr(dispatch_ptr)
		{
		}
		~TxCleanupController()
		{
			cout << "Destructor for a Wallets::TxCleanupController" << endl;
		}
		//
		// Imperatives - Lifecycle / Instantiation
		void setup();
		//
		// Dependencies
		
		
		//
		// Properties
		
		//
		// Signals
		
		//
		// Imperatives
		
	private:
		//
		// Properties
		std::weak_ptr<Object> _wallet;
		std::shared_ptr<Dispatch::Dispatch> _dispatch_ptr;
		//
		std::mutex timer_mutex;
		std::unique_ptr<Dispatch::CancelableTimerHandle> _timer; // initialized to nullptr
		//
		// Imperatives - Lifecycle
		void tearDown();
		//
		// Imperatives - Timer
		void startPollingTimer();
		void invalidateTimer();
		void __givenLocked_create_repeating_timer();
		//
		// Imperatives
		void __do_localTxCleanupJob();
	};
}

#endif /* TxCleanupController_hpp */

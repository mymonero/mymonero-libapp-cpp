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

#ifndef UserIdle_hpp
#define UserIdle_hpp

#include <iostream>
#include <boost/signals2.hpp>
#include <mutex>
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Settings/SettingsProviders.hpp"

namespace UserIdle
{
	using namespace std;
	using namespace boost;
	//
	// Controllers
	class Controller
	{
	public:
		//
		// Lifecycle - Init
		Controller() {
			// set dependencies then call setup()
		}
		~Controller() {
			teardown();
			cout << "Destructed user idle" << endl;
		}
		//
		// Dependencies
		string documentsPath;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
		std::shared_ptr<Settings::IdleTimeoutAfterS_SettingsProvider> idleTimeoutAfterS_SettingsProvider;
		// Then call:
		void setup();
		//
		// Properties
		bool isUserIdle = false;
		//
		// Signals
		boost::signals2::signal<void()> userDidComeBackFromIdle_signal;
		boost::signals2::signal<void()> userDidBecomeIdle_signal;
		//
		// Imperatives
		void temporarilyDisable_userIdle();
		void reEnable_userIdle();
		void checkIdleTimeout();
		void breakIdle();
	private:
		//
		// Properties
		bool isTornDown = false;
		std::mutex timer_mutex;
		time_t _dateOfLastUserInteraction = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		uint32_t _numberOfRequestsToLockUserIdleAsDisabled = 0;
		std::unique_ptr<Dispatch::CancelableTimerHandle> _userIdle_intervalTimer; // initialized to nullptr
		//
		// Imperatives
		void teardown();
		//
		void __lockMutexAnd_disable_userIdle();
		void __reEnable_userIdle();
		void _lockMutexAnd_initiate_userIdle_intervalTimer();
		void __givenLocked_create_repeating_timer();
		//
		void _idleBreakingActionOccurred();
		void _userDidInteract();
		void _userDidComeBackFromIdle();
		void _userDidBecomeIdle();
	};
}

#endif /* UserIdle_hpp */

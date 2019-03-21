//
//  AppServiceLocator.hpp
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
//  conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice, this list
//  of conditions and the following disclaimer in the documentation and/or other
//  materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors may be
//  used to endorse or promote products derived from this software without specific
//  prior written permission.
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
//
#ifndef AppServiceLocator_HPP_
#define AppServiceLocator_HPP_
//
#include <iostream> // TODO: this is to obtain stdlib.. what should be imported instead of this?
//
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Passwords/PasswordController.hpp"
#include "../Settings/SettingsController.hpp"
#include "../UserIdle/UserIdle.hpp"
//
namespace App
{
	using namespace std;
	//
	class ServiceLocator_SpecificImpl;
	//
	class ServiceLocator
	{
		private:
			ServiceLocator() {}
			ServiceLocator(const ServiceLocator&) = delete;
			ServiceLocator& operator=(const ServiceLocator&) = delete;
			//
			ServiceLocator_SpecificImpl *_pImpl;
			//
			ServiceLocator &_shared_build( // use this for platform-specific implementations of ServiceLocator::build
				const string &this_documentsPath,
				std::shared_ptr<Dispatch::Dispatch> this_dispatch_ptr
			) {
				documentsPath = this_documentsPath;
				// TODO: assert existence of deps here? -- documentsPath etc
				//
				dispatch_ptr = this_dispatch_ptr; // store - it got std::moved
				//
				settingsController = std::make_shared<Settings::Controller>();
				userIdleController = std::make_shared<UserIdle::Controller>();
				passwordController = std::make_shared<Passwords::Controller>();
				//
				passwordController->documentsPath = documentsPath;
				passwordController->dispatch_ptr = dispatch_ptr;
				passwordController->userIdleController = userIdleController;
				passwordController->setup();
				//
				userIdleController->documentsPath = documentsPath;
				userIdleController->dispatch_ptr = dispatch_ptr;
				userIdleController->idleTimeoutAfterS_SettingsProvider = settingsController;
				userIdleController->setup();
				//
				settingsController->documentsPath = documentsPath;
				settingsController->dispatch_ptr = dispatch_ptr;
				settingsController->passwordController = passwordController;
				settingsController->setup();
				//
				built = true;
				//
				return *this;
			}
		//
		public:
			~ServiceLocator();
			void teardown()
			{
				dispatch_ptr = nullptr;
				settingsController = nullptr;
				passwordController = nullptr;
				userIdleController = nullptr;
			}
			//
			static ServiceLocator& instance()
			{
				static ServiceLocator pInstance;
				return pInstance;
			}
			bool uniqueFlag = false;
			//
			// Properties - Initial: Status
			bool built = false;
			// Properties - Initial: Required for build()
			string documentsPath;
			// Properties - Services: Built and retained dependencies
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
			std::shared_ptr<Passwords::Controller> passwordController;
			std::shared_ptr<Settings::Controller> settingsController;
			std::shared_ptr<UserIdle::Controller> userIdleController;
			//
			// Lifecycle - Init
			ServiceLocator &build(
				const string &documentsPath
			); // simply returns the singleton for convenience
   };
}

#endif /* AppServiceLocator_HPP_ */

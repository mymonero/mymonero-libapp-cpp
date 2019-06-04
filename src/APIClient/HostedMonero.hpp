//
//  HostedMonero.hpp
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
#ifndef HostedMonero_HPP_
#define HostedMonero_HPP_
//
#include <iostream> // TODO: this is to obtain stdlib.. what should be imported instead of this?
#include <boost/signals2.hpp>
//
#include "../Settings/SettingsController.hpp"
#include "./HTTPRequests_Interface.hpp"
#include "./parsing.hpp"
#include "../Wallets/Wallet_KeyImageCache.hpp"
//
namespace HostedMonero
{
	using namespace std;
	//
	// Principal Types
	class APIClient: public std::enable_shared_from_this<APIClient>
	{
	public:
		APIClient() {}
		APIClient(const APIClient&) = delete;
		APIClient& operator=(const APIClient&) = delete;
		~APIClient() {}
		//
		// Dependencies
		std::shared_ptr<HTTPRequests::RequestFactory> requestFactory;
		std::shared_ptr<Settings::Controller> settingsController;
		//
		// Then call:
		void setup();
		//
		// Signals
		boost::signals2::signal<void()> initializedWithNewServerURL_signal;
		//
		// Endpoints
		std::shared_ptr<HTTPRequests::Handle> logIn(
			const string &address,
			const string &sec_view_key,
			bool generated_locally,
			std::function<void(
				optional<string> err_str,
				optional<HostedMonero::ParsedResult_Login> result
			)> fn
		);
		std::shared_ptr<HTTPRequests::Handle> addressInfo(
			std::shared_ptr<Wallets::KeyImageCache> keyImageCache,
			const string &address,
			const string &sec_view_key,
			const string &pub_spend_key,
			const string &sec_spend_key,
			std::function<void(
				optional<string> err_str,
				optional<HostedMonero::ParsedResult_AddressInfo> result
			)> fn
		);
		std::shared_ptr<HTTPRequests::Handle> addressTransactions(
			std::shared_ptr<Wallets::KeyImageCache> keyImageCache,
			const string &address,
			const string &sec_view_key,
			const string &pub_spend_key,
			const string &sec_spend_key,
			std::function<void(
				optional<string> err_str,
				optional<HostedMonero::ParsedResult_AddressTransactions> result
			)> fn
		);
	private:
		//
		// Lifecycle
		void startObserving();
		void teardown();
		void stopObserving();
		//
		// Connections
		boost::signals2::connection connection__SettingsController__specificAPIAddressURLAuthority_changed;
		//
		// Accessors
		string final_apiAddress_authority();
		//
		// Delegation
		void SettingsController__specificAPIAddressURLAuthority_changed();
   };
}

#endif /* HostedMonero_HPP_ */

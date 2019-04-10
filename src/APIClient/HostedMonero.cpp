//
//  HostedMonero.cpp
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
#include "HostedMonero.hpp"
using namespace HostedMonero;
//
// Imperatives - Lifecycle
void APIClient::setup()
{
	if (requestFactory == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("APIClient: expected requestFactory != nullptr"));
	}
	if (settingsController == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("APIClient: expected settingsController != nullptr"));
	}
	//
	startObserving();
}
void APIClient::startObserving()
{
	connection__SettingsController__specificAPIAddressURLAuthority_changed = settingsController->specificAPIAddressURLAuthority_signal.connect(
		std::bind(&APIClient::SettingsController__specificAPIAddressURLAuthority_changed, this)
	);
}
//
// Imperatives - Lifecycle - Teardown
void APIClient::teardown()
{
	stopObserving();
}
void APIClient::stopObserving()
{
	connection__SettingsController__specificAPIAddressURLAuthority_changed.disconnect();
}
//
// Endpoints
std::unique_ptr<HTTPRequests::Handle> APIClient::logIn(
	const string &address,
	const string &view_pub_key,
	bool generated_locally,
	std::function<void(
		optional<string> err_str,
		optional<HostedMonero::ParsedResult_Login> result
	)> fn
) {
	// TODO
	return requestFactory->new_request("login", []() {
		
	});
}
//
// Delegation
void APIClient::SettingsController__specificAPIAddressURLAuthority_changed()
{
	// TODO: implement this as some sort of synchronous emit
//	initializeManagerWithFinalServerAuthority();
	//
	// Notify consumers to avoid race condition with anyone trying to make a request just before the manager gets de-initialized and re-initialized
	initializedWithNewServerURL_signal();
}

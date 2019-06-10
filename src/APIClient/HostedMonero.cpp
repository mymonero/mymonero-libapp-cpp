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
#include "rapidjson_defines.hpp" // must be included before rapidjson include
#include "rapidjson/document.h"
using namespace rapidjson;
//
// Constants - Endpoints
enum Endpoint
{
	LogIn,
	//
	AddressInfo,
	AddressTransactions,
	//
	UnspentOuts,
	RandomOuts,
	SubmitSerializedSignedTransaction,
	//
	ImportRequestInfoAndStatus,
};
static string endpoint__login = string("login");
static string endpoint__addressInfo = string("get_address_info");
static string endpoint__addressTxs = string("get_address_txs");
static string endpoint__unspentOuts = string("get_unspent_outs");
static string endpoint__randomOuts = string("get_random_outs");
static string endpoint__submitTx = string("submit_raw_tx");
static string endpoint__importRequest = string("import_wallet_request");
static inline const string &_endpoint_path_from(Endpoint endpoint)
{
	switch (endpoint) {
		case LogIn:
			return endpoint__login;
		case AddressInfo:
			return endpoint__addressInfo;
		case AddressTransactions:
			return endpoint__addressTxs;
		case UnspentOuts:
			return endpoint__unspentOuts;
		case RandomOuts:
			return endpoint__randomOuts;
		case SubmitSerializedSignedTransaction:
			return endpoint__submitTx;
		case ImportRequestInfoAndStatus:
			return endpoint__importRequest;
	}
}
//
// Accessory functions - Requests
static string reqParams_key__address = string("address");
static string reqParams_key__viewKey = string("view_key");
static inline HTTPRequests::ReqParams new_parameters_forWalletRequest(
	const string &address,
	const string &sec_view_key
) {
	HTTPRequests::ReqParams params;
	params.SetObject();
	{
		Value k(StringRef(reqParams_key__address));
		Value v(address, params.GetAllocator()); // copy string
		params.AddMember(k, v, params.GetAllocator());
	}
	{
		Value k(StringRef(reqParams_key__viewKey));
		Value v(sec_view_key, params.GetAllocator()); // copy string
		params.AddMember(k, v, params.GetAllocator());
	}
	return params;
}
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
	//
	requestFactory = nullptr;
	settingsController = nullptr;
}
void APIClient::stopObserving()
{
	connection__SettingsController__specificAPIAddressURLAuthority_changed.disconnect();
}
//
// Accessors
static string mymonero_apiAddress_authority = "api.mymonero.com:8443";
string APIClient::final_apiAddress_authority()
{ // authority means [subdomain.]host.…[:…]
	assert(settingsController->hasBooted());
	optional<string> settings_authorityValue = settingsController->specificAPIAddressURLAuthority();
	if (settings_authorityValue == none) {
		return mymonero_apiAddress_authority;
	}
	if (settings_authorityValue->empty()) { // NOTE: This should not technically be here. See SettingsController's set(valuesByDictKey:)'s failed attempt at nil detection in normalizing "" to nil
		return mymonero_apiAddress_authority;
	}
	assert(settings_authorityValue->empty() == false);
	//
	return *settings_authorityValue;
}
//
// Endpoints
static string reqParams_key__createAccount = "create_account";
static string reqParams_key__generatedLocally = "generated_locally";
std::shared_ptr<HTTPRequests::Handle> APIClient::logIn(
	const string &address,
	const string &sec_view_key,
	bool generated_locally,
	std::function<void(
		optional<string> err_str,
		optional<HostedMonero::ParsedResult_Login> result
	)> fn
) {
	auto params = new_parameters_forWalletRequest(address, sec_view_key);
	{
		Value k(StringRef(reqParams_key__createAccount));
		params.AddMember(k, Value(true).Move(), params.GetAllocator());
	}
	{
		Value k(StringRef(reqParams_key__generatedLocally));
		params.AddMember(k, Value(generated_locally).Move(), params.GetAllocator());
	}
	std::shared_ptr<APIClient> shared_this = shared_from_this();
	std::weak_ptr<APIClient> weak_this = shared_this;
	return requestFactory->new_request(
		HTTPRequests::HTTPS,
		final_apiAddress_authority(),
		_endpoint_path_from(LogIn),
		std::move(params),
		[weak_this, fn = std::move(fn)](optional<string> err_str, std::shared_ptr<HTTPRequests::ResponseJSON> res)
		{
			if (auto inner_spt = weak_this.lock()) {
				if (err_str != none) {
					fn(std::move(*err_str), none);
					return;
				}
				fn(none, new_ParsedResult_Login(std::move(*res)));
			} else { /* if APIClient gone, can assume no need to call back */ }
		}
	);
}
std::shared_ptr<HTTPRequests::Handle> APIClient::addressInfo(
	std::shared_ptr<Wallets::KeyImageCache> keyImageCache,
	const string &address,
	const string &sec_view_key,
	const string &pub_spend_key,
	const string &sec_spend_key,
	std::function<void(
		optional<string> err_str,
		optional<HostedMonero::ParsedResult_AddressInfo> result
	)> fn
) {
	auto params = new_parameters_forWalletRequest(address, sec_view_key);
	std::shared_ptr<APIClient> shared_this = shared_from_this();
	std::weak_ptr<APIClient> weak_this = shared_this;
	return requestFactory->new_request(
		HTTPRequests::HTTPS,
		final_apiAddress_authority(),
		_endpoint_path_from(AddressInfo),
		std::move(params),
		[
			weak_this, fn = std::move(fn),
			address, sec_view_key, pub_spend_key, sec_spend_key,
			keyImageCache
		] (
			optional<string> err_str,
			std::shared_ptr<HTTPRequests::ResponseJSON> res
		) {
			if (auto inner_spt = weak_this.lock()) {
				if (err_str != none) {
					fn(std::move(*err_str), none);
					return;
				}
				fn(none, new_ParsedResult_AddressInfo(
					std::move(*res),
					address,
					sec_view_key,
					pub_spend_key,
					sec_spend_key,
					keyImageCache
				));
			} else { /* if APIClient gone, can assume no need to call back */ }
		}
	);
}
std::shared_ptr<HTTPRequests::Handle> APIClient::addressTransactions(
	std::shared_ptr<Wallets::KeyImageCache> keyImageCache,
	const string &address,
	const string &sec_view_key,
	const string &pub_spend_key,
	const string &sec_spend_key,
	std::function<void(
		optional<string> err_str,
		optional<HostedMonero::ParsedResult_AddressTransactions> result
	)> fn
) {
	auto params = new_parameters_forWalletRequest(address, sec_view_key);
	std::shared_ptr<APIClient> shared_this = shared_from_this();
	std::weak_ptr<APIClient> weak_this = shared_this;
	return requestFactory->new_request(
		HTTPRequests::HTTPS,
		final_apiAddress_authority(),
		_endpoint_path_from(AddressTransactions),
		std::move(params),
		[
			weak_this, fn = std::move(fn),
			address, sec_view_key, pub_spend_key, sec_spend_key,
			keyImageCache
		] (
			optional<string> err_str,
			std::shared_ptr<HTTPRequests::ResponseJSON> res
		) {
		if (auto inner_spt = weak_this.lock()) {
			if (err_str != none) {
				fn(std::move(*err_str), none);
				return;
			}
			fn(none, new_ParsedResult_AddressTransactions(
				std::move(*res),
				address,
				sec_view_key,
				pub_spend_key,
				sec_spend_key,
				keyImageCache
			));
		} else { /* if APIClient gone, can assume no need to call back */ }
		}
	);
}
//
// TODO
//
// Import request info
//func ImportRequestInfoAndStatus(
//								address: MoneroAddress,
//								view_key__private: MoneroKey,
//								_ fn: @escaping (
//												 _ err_str: String?,
//												 _ result: ParsedResult_ImportRequestInfoAndStatus?
//												 ) -> Void
//								) -> RequestHandle? {
//	let parameters: [String: Any] =
//	[
//	 "address": address,
//	 "view_key": view_key__private
//	 ]
//	let endpoint = HostedMoneroAPI_Endpoint.ImportRequestInfoAndStatus
//	let requestHandle = self._request(endpoint, parameters)
//	{ [unowned self] (err_str, response_data, response_jsonDict) in
//		if let err_str = err_str {
//			self._shared_onMain_callBackFromRequest(err_str, nil, fn)
//			return
//		}
//		let result = ParsedResult_ImportRequestInfoAndStatus(
//															 payment_id: response_jsonDict!["payment_id"] as! MoneroPaymentID,
//															 payment_address: response_jsonDict!["payment_address"] as! MoneroAddress,
//															 import_fee: MoneroAmount(response_jsonDict!["import_fee"] as! String)!,
//															 feeReceiptStatus: response_jsonDict!["status"] as? String
//															 )
//		self._shared_onMain_callBackFromRequest(err_str, result, fn)
//	}
//	return requestHandle
//	}
//	
//	
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

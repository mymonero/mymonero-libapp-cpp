//
//  AppBridge.cpp
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

#include "AppBridge.hpp"
using namespace App;
using namespace Bridge_modules;
using namespace Bridge_event;
using namespace Bridge_exec;
using namespace std;
using namespace boost;
using namespace rapidjson;
//
// Shared - Utility - can be kept in here for now or copied easily … not necessarily bad given stasis
//
// i.e. assert() + exception msg.:
#define BOOST_THROW_EXCEPTION_IF_NOT(cond, err) \
	if (cond) \
	{ \
		BOOST_THROW_EXCEPTION(#err); \
	}
//
//
// Lifecycle - Setup



void Bridge::setup(
	ServiceLocator_SpecificImpl *pImpl_ptr__orNULL,
	std::shared_ptr<string> documentsPath,
	network_type nettype,
	std::shared_ptr<Dispatch::Dispatch> this_dispatch_ptr
) {
	dispatch_ptr = this_dispatch_ptr;
	assert(dispatch_ptr != nullptr);
	//
	locator = std::make_shared<App::ServiceLocator>();
	std::shared_ptr<App::Bridge> shared_this = shared_from_this();
	locator->shared_build(
		pImpl_ptr__orNULL,
		documentsPath,
		nettype, // TODO: eventually have this configurable via Settings
		shared_this, // httprequest factory
		this_dispatch_ptr,
		shared_this // initial pw entry delegate
	);
	//
	startObserving();
}
void Bridge::startObserving()
{
	cout << "TODO: Start observing the locator here" << endl;
	{ // password / password entry 
		
	}
	{ // wallet
		
	}
}
//
// Lifecycle - Destructor
void Bridge::teardown()
{
	locator = nullptr;
	dispatch_ptr = nullptr;
}
//
// Imperatives - Public
void Bridge::exec(const string &msg)
{
	rapidjson::Document d;
	ParseResult ok = d.Parse(msg.c_str(), msg.size());
	if (!ok) {
		cout << "Bridge exec parse error code: " << ok.Code() << endl;
	}
	BOOST_ASSERT_MSG(ok, "Failed to parse exec payload msg");
	//
	string module_name;
	string cmdName;
	{
		Value::ConstMemberIterator itr = d.FindMember(Bridge_exec::_msg_key__moduleName);
		BOOST_ASSERT_MSG(itr != d.MemberEnd(), "Didn't find _msg_key__moduleName");
		BOOST_ASSERT_MSG(itr->value.IsString(), "Expected string at _msg_key__moduleName");
		module_name = string(itr->value.GetString(), itr->value.GetStringLength());
		BOOST_ASSERT_MSG(module_name.size() != 0, "Expected non-zero _msg_key__moduleName");
	}
	{
		Value::ConstMemberIterator itr = d.FindMember(Bridge_exec::_msg_key__cmdName);
		BOOST_ASSERT_MSG(itr != d.MemberEnd(), "Didn't find _msg_key__cmdName");
		BOOST_ASSERT_MSG(itr->value.IsString(), "Expected string at _msg_key__cmdName");
		cmdName = string(itr->value.GetString(), itr->value.GetStringLength());
		BOOST_ASSERT_MSG(module_name.size() != 0, "Expected non-zero _msg_key__cmdName");
	}
	{
		Value::ConstMemberIterator itr = d.FindMember(Bridge_exec::_msg_key__params);
		BOOST_ASSERT_MSG(itr != d.MemberEnd(), "Didn't find _msg_key__params");
		BOOST_ASSERT_MSG(itr->value.IsObject(), "Expected object at _msg_key__params");
		//
		_exec(module_name, cmdName, itr->value);
	}
}
//
// Imperatives - Private
void Bridge::_emitWith(
	const ModuleName &moduleName,
	const EventName &eventName,
	std::function<void(Value &params, Document::AllocatorType &a)> optl__construct_fn
) {
	auto msg = Bridge_event::new_msg_with(
		moduleName,
		eventName,
		optl__construct_fn
	);
	std::shared_ptr<App::Bridge> shared_this = shared_from_this();
	std::weak_ptr<App::Bridge> weak_this = shared_this;
	dispatch_ptr->async([weak_this, msg] () {
		if (auto inner_spt = weak_this.lock()) {
			inner_spt->evented_signal(msg);
		}
	});
}
//
// Imperatives - Exec
//  - Native Extensions - PasswordController
void _exec_PasswordController(
	std::shared_ptr<App::ServiceLocator> locator,
	const CmdName &cmdName,
	const Value &params
) {
	if (cmdName == PasswordController__enterExistingPassword_cb) {
		optional<bool> v_1 = none;
		Value::ConstMemberIterator itr_1 = params.GetObject().FindMember(PasswordController_k__didCancel_orNone);
		if (itr_1 != params.GetObject().MemberEnd()) {
			v_1 = itr_1->value.GetBool();
		}
		//
		optional<string> v_2 = none;
		Value::ConstMemberIterator itr_2 = params.GetObject().FindMember(PasswordController_k__obtainedPasswordString);
		if (itr_2 != params.GetObject().MemberEnd()) {
			v_2 = string(itr_2->value.GetString(), itr_2->value.GetStringLength());
		}
		//
		BOOST_ASSERT_MSG(v_1 || v_2, "Expected at least one value for enterExistingPassword_cb");
		locator->passwordController->enterExistingPassword_cb(v_1, v_2);
		//
		// finally, exit so as not to trip the assert
		return;
	} else if (cmdName == PasswordController__enterNewPasswordAndType_cb) {
		optional<bool> v_1 = none;
		Value::ConstMemberIterator itr_1 = params.GetObject().FindMember(PasswordController_k__didCancel_orNone);
		if (itr_1 != params.GetObject().MemberEnd()) {
			v_1 = itr_1->value.GetBool();
		}
		//
		optional<string> v_2 = none;
		Value::ConstMemberIterator itr_2 = params.GetObject().FindMember(PasswordController_k__obtainedPasswordString);
		if (itr_2 != params.GetObject().MemberEnd()) {
			v_2 = string(itr_2->value.GetString(), itr_2->value.GetStringLength());
		}
		//
		optional<Passwords::Type> v_3 = none;
		Value::ConstMemberIterator itr_3 = params.GetObject().FindMember(PasswordController_k__userSelectedTypeOfPassword);
		if (itr_3 != params.GetObject().MemberEnd()) {
			v_3 = Passwords::new_Type_with_bridge_serialized(itr_3->value.GetUint());
		}
		//
		BOOST_ASSERT_MSG(v_1 || v_2 || v_3, "Expected at least one value for enterNewPasswordAndType_cb");
		locator->passwordController->enterNewPasswordAndType_cb(v_1, v_2, v_3);
		//
		// finally, exit so as not to trip the assert
		return;
	}
	BOOST_ASSERT_MSG(false, "_exec_PasswordController: Unrecognized cmdName");

}
void Bridge::_exec(
	const ModuleName &module_name,
	const CmdName &cmdName,
	const Value &params/*kObjectType*/
) {
	
	assert(params.IsObject());
	if (module_name == Bridge_modules::Name__PasswordController) {
		_exec_PasswordController(locator, cmdName, params);
		return;
	}
	cout << "TODO: handle _exec(…) of " << module_name << "::" << cmdName << endl;
//	if ( … )  { // TODO
//	}
	// …
	BOOST_ASSERT_MSG(false, "Unrecognized module_name");
}
//
//
// Password Entry
void Bridge::getUserToEnterExistingPassword(
	bool isForChangePassword,
	bool isForAuthorizingAppActionOnly, // normally no - this is for things like SendFunds
	boost::optional<std::string> customNavigationBarTitle
) {
	_emitWith(
		Bridge_modules::Name__PasswordController,
		Bridge_event::Name__getUserToEnterExistingPassword,
		[
			isForChangePassword, isForAuthorizingAppActionOnly, customNavigationBarTitle
		] (Value &params, Document::AllocatorType &a) {
			{
				Value v(isForChangePassword);
				params.AddMember("isForChangePassword", v.Move(), a);
			}
			{
				Value v(isForAuthorizingAppActionOnly);
				params.AddMember("isForAuthorizingAppActionOnly", v.Move(), a);
			}
			if (customNavigationBarTitle) {
				Value v(*customNavigationBarTitle, a);
				params.AddMember("customNavigationBarTitle", v, a); // copy ?
			}
		}
	);
}
void Bridge::getUserToEnterNewPasswordAndType(
	bool isForChangePassword
) {
	_emitWith(
		Bridge_modules::Name__PasswordController,
		Bridge_event::Name__getUserToEnterNewPasswordAndType,
		[
			isForChangePassword
		] (Value &params, Document::AllocatorType &a) {
			{
				Value v(isForChangePassword);
				params.AddMember("isForChangePassword", v.Move(), a);
			}
		}
	);
}
//
// HTTPRequests::RequestFactory
std::shared_ptr<HTTPRequests::Handle> Bridge::new_request(
	Scheme scheme,
	string authority, // host+':'+port
	string endpoint_path,
	ReqParams params,
	std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)> fn
) {
	vector<string> authority_components;
	string str = authority; // an undesired copy
	string delim = ":";
	{ // split str by delim
		size_t prev = 0, pos = 0;
		do {
			pos = str.find(delim, prev);
			if (pos == string::npos) {
				pos = str.length();
			}
			string token = str.substr(prev, pos-prev);
			if (!token.empty()) {
				authority_components.push_back(token);
			}
			prev = pos + delim.length();
		} while (pos < str.length() && prev < str.length());
	}
	if (authority_components.size() < 1 || authority_components.size() > 2) {
		BOOST_THROW_EXCEPTION(logic_error("Unexpected number of authority_components"));
		return nullptr;
	}
	const char *host = authority_components[0].c_str();
	const char *port = authority_components.size() == 1
		? port_from_scheme(scheme).c_str()
		: authority_components[1].c_str();
	//
	//			auto verb = string("post"); // here, always POST, for now
	std::shared_ptr<App::Bridge> shared_this = shared_from_this();
	std::weak_ptr<App::Bridge> weak_this = shared_this;
	auto handle_ptr = std::make_shared<Handle_bridge>(
		weak_this,
		string(host), string(port), endpoint_path,
		std::move(fn)
	);
	handle_ptr->run(
		std::move(params)
	);
	//
	return handle_ptr;
}

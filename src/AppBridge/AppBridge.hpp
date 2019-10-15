//
//  AppBridge.hpp
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
#ifndef AppBridge_HPP_
#define AppBridge_HPP_
//
#include <string>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include "rapidjson/writer.h"
#include "rapidjson_defines.hpp" // must be included before rapidjson include
#include "rapidjson/document.h"
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../APIClient/HTTPRequests_Interface.hpp"
#include "../App/AppServiceLocator.hpp"
#include "./HTTPRequests_Handle_bridge.hpp"
//
// not currently used
//namespace
//{
//	//
//	enum BridgeComponent
//	{ // these Int values must never be changed and form part of the protocol
//		PasswordEntryDelegate = 0
//		//,
//		// TODO
//	};
//	typedef std::vector<BridgeComponent> BridgeComponents;
//	BridgeComponents _all_bridgeComponents;
//	bool hasBeenInitialized__all_bridgeComponents = false;
//	static inline BridgeComponents all_bridgeComponents()
//	{
//		if (!hasBeenInitialized__all_bridgeComponents) {
//			hasBeenInitialized__all_bridgeComponents = true;
//			//
//			_all_bridgeComponents.push_back(PasswordEntryDelegate);
//		}
//		return _all_bridgeComponents;
//	}
//}

namespace App
{
	using namespace std;
	using namespace boost;
	using namespace rapidjson;
	//
	namespace Bridge_modules
	{
		typedef string ModuleName;
		//
		static const ModuleName Name__PasswordController = "PC";
	}
	//
	namespace Bridge_event
	{
		using namespace Bridge_modules;
		//
		static const string _msg_key__moduleName = "m";
		static const string _msg_key__eventName = "e";
		static const string _msg_key__params = "ps";
		//
		typedef string EventName;
		//
		static inline string new_msg_with(
			const ModuleName &moduleName,
			const EventName &eventName,
			std::function<void(Value &params, Document::AllocatorType &a)> construct_params_fn
		) {
			Document root;
			root.SetObject();
			Document::AllocatorType &a = root.GetAllocator();
			{
				Value k(StringRef(_msg_key__moduleName));
				Value v(moduleName, a); // any safe optimizations here?
				root.AddMember(k, v.Move(), a);
			}
			{
				Value k(StringRef(_msg_key__eventName));
				Value v(eventName, a); // any safe optimizations here?
				root.AddMember(k, v.Move(), a);
			}
			rapidjson::Value params(kObjectType);
			construct_params_fn(params, a);
			{
				Value k(StringRef(_msg_key__params));
				root.AddMember(k, params.Move(), a); // TODO: .Move() ok on params here?
			}
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);
			root.Accept(writer);
			//
			return string(buffer.GetString());
		}
		//
		struct _Convenience__Event
		{
			string moduleName;
			string eventName;
			const Value &params;
		};
		static inline _Convenience__Event new_convenience__event_with(const string &msg)
		{
			rapidjson::Document d;
			ParseResult ok = d.Parse(msg.c_str(), msg.size());
			if (!ok) {
				cout << "JSON parse err with code: " << ok.Code() << endl;
//				fprintf(stderr, "JSON parse error: %s (%u)", GetParseError_En(ok.Code()), ok.Offset());
			}
			BOOST_ASSERT_MSG(ok, "Failed to parse Bridge_event payload msg");
			auto e = _Convenience__Event{
				string(d[_msg_key__moduleName].GetString(), d[_msg_key__moduleName].GetStringLength()),
				string(d[_msg_key__eventName].GetString(), d[_msg_key__eventName].GetStringLength()),
				d[_msg_key__params] // hopefully this exists ..
			};
			return e;
		}
	}
}
namespace App
{
	namespace Bridge_event
	{ // Extensions for specific components of the Bridge, including the Locator
		//
		// Password
		static const EventName Name__getUserToEnterExistingPassword = "getUserToEnterExistingPassword";
		static const EventName Name__getUserToEnterNewPasswordAndType = "getUserToEnterNewPasswordAndType";
		//
	}
}
namespace App
{
	namespace Bridge_exec
	{
		using namespace Bridge_modules;
		//
		static const string _msg_key__moduleName = "m";
		static const string _msg_key__cmdName = "c";
		static const string _msg_key__params = "ps";
		//
		typedef string CmdName;
		//
		static inline string new_msg_with( // this is purely a convenience method since, technically, no code within AppBridge is going to need to use it...
			const ModuleName &moduleName,
			const CmdName &cmdName,
			std::function<void(Value &params, Document::AllocatorType &a)> construct_params_fn
		) {
			Document root;
			root.SetObject();
			Document::AllocatorType &a = root.GetAllocator();
			{
				Value k(StringRef(_msg_key__moduleName));
				Value v(moduleName, a); // any safe optimizations here?
				root.AddMember(k, v.Move(), a);
			}
			{
				Value k(StringRef(_msg_key__cmdName));
				Value v(cmdName, a); // any safe optimizations here?
				root.AddMember(k, v.Move(), a);
			}
			rapidjson::Value params(kObjectType);
			construct_params_fn(params, a);
			{
				Value k(StringRef(_msg_key__params));
				root.AddMember(k, params.Move(), a); // TODO: .Move() ok on params here?
			}
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);
			root.Accept(writer);
			//
			return string(buffer.GetString());
		}
	}
}
namespace App
{
	namespace Bridge_exec
	{ // Extensions for specific components of the Bridge, including the Locator
		//
		// Passwords
		static const CmdName PasswordController__enterExistingPassword_cb = "enterExistingPassword_cb";
		static const CmdName PasswordController__enterNewPasswordAndType_cb = "enterNewPasswordAndType_cb";
		//
		static const string PasswordController_k__didCancel_orNone = "didCancel_orNone";
		static const string PasswordController_k__obtainedPasswordString = "obtainedPasswordString";
		static const string PasswordController_k__userSelectedTypeOfPassword = "userSelectedTypeOfPassword";
	}
}
namespace App
{
	using namespace std;
	using namespace boost;
	using namespace HTTPRequests;
	using namespace Bridge_event;
	using namespace Bridge_exec;
	//
	class Bridge:
		public std::enable_shared_from_this<Bridge>,
		public Passwords::PasswordEntryDelegate,
		public HTTPRequests::RequestFactory
	{
	public:
		~Bridge()
		{ // must be public for shared ptr
			cout << "Destructor for App::Bridge" << endl;
			teardown();
		}
		//
		// Setup - Methods
		void setup(
			ServiceLocator_SpecificImpl *pImpl_ptr__orNULL,
			std::shared_ptr<string> documentsPath,
			network_type nettype,
			std::shared_ptr<Dispatch::Dispatch> this_dispatch_ptr
		);
		void teardown();
		// Setup - Deps - Will be created by setup(…)
		std::shared_ptr<App::ServiceLocator> locator = nullptr;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr = nullptr;
		//
		// Properties - Interface
		boost::signals2::signal<void(string msg)> evented_signal;
		//
		// Accessors
		//
		// Imperatives
		void exec(const string &msg); // call this with a payload to tell a Locator component to do something
	private:
		//
		// Lifecycle - Init
		void startObserving();
		//
		// Imperatives - Convenience
		void _emitWith(
			const ModuleName &moduleName,
			const EventName &eventName,
			std::function<void(Value &params, Document::AllocatorType &a)> optl__construct_fn = {}
		);
		// Imperatives - Implementations
		void _exec(
			const ModuleName &module_name,
			const CmdName &cmdName,
			const Value &params/*kObjectType*/
		);
		//
		// Password Entry
		std::string uuid_string = boost::uuids::to_string((boost::uuids::random_generator())()); // cached
		//
		std::string identifier() const
		{
			return uuid_string;
		}
		void getUserToEnterExistingPassword(
			bool isForChangePassword,
			bool isForAuthorizingAppActionOnly, // normally no - this is for things like SendFunds
			boost::optional<std::string> customNavigationBarTitle
		);
		void getUserToEnterNewPasswordAndType(
			bool isForChangePassword
		);
		//
		// HTTPRequests RequestFactory
		std::shared_ptr<HTTPRequests::Handle> new_request(
			Scheme scheme,
			string authority, // host+':'+port
			string endpoint_path,
			ReqParams params,
			std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)> fn
		); // implemented in cpp
	};
}
//

#endif /* AppBridge_HPP_ */

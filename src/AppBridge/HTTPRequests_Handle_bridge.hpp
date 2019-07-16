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
#ifndef HTTPRequests_Handle_bridge_HPP_
#define HTTPRequests_Handle_bridge_HPP_
//
#include <string>
#include <boost/optional/optional.hpp>
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../APIClient/HTTPRequests_Interface.hpp"
#include "rapidjson/writer.h"
//

namespace App
{
	class Bridge; // forward-declaration … import in a cpp file if linking desired
}
namespace HTTPRequests
{
	using namespace std;
	//
	struct Handle_bridge: public Handle, public std::enable_shared_from_this<Handle_bridge>
	{
	public:
		Handle_bridge(
			std::weak_ptr<App::Bridge> bridge,
			string host,
			string port,
			string endpoint_path,
			std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)> fn
		):
		_bridge(bridge),
		_host(host),
		_port(port),
		_endpoint_path(endpoint_path),
		_fn(std::move(fn))
		{}
		~Handle_bridge() {}
		//
		// Accessors
		auto atomic_get() const
		{
			return std::atomic_load(&_result);
		}
		//
		// Imperatives - Override
		void cancel() override
		{
			cout << "Canceling a HTTPRequests::Handle_bridge<" << this << ">" << endl;
			if (_isConnectionClosed) {
				return;
			}
			_isConnectionClosed = true;
			//
			// TODO: broadcast to cancel the request with the handle UUID
		}
		//
		// Imperatives - Public
		void run(ReqParams postbody)
		{
			// TODO: actually emit this for the Bridge or rather ... lock the weak_ptr to the bridge and then call the 'emitToRunHTTPRequest(…)' convenience function
			
			cout << " TODO: actually emit this for the Bridge or rather ... lock the weak_ptr to the bridge and then call the 'emitToRunHTTPRequest(…)' convenience function" << endl;
			
			
			//			ostringstream target_ss;
			//			target_ss << "/" << _endpoint_path; // prepend "/"
			//
			//			_request.set(boost::beast::http::field::content_type, "application/json");
			//			_request.set(boost::beast::http::field::accept, "application/json");
			//
		}
	private:
		//
		// Initialized
		std::weak_ptr<App::Bridge> _bridge;
		string _host;
		string _port;
		string _endpoint_path;
		std::function<void(optional<string> err_str, std::shared_ptr<rapidjson::Document> res)> _fn;
		//
		// Runtime
		bool _isConnectionClosed = false;
		bool _hasFNBeenCalled = false;
		std::shared_ptr<rapidjson::Document> _result;
		// TODO: uuid for bridge-comms?
		//
		// Imperatives
		void _call_fn_with_success()
		{
			_call_fn_with(none);
		}
		void _call_fn_with(
						   optional<string> err_str
						   ) {
			assert(_hasFNBeenCalled == false);
			_hasFNBeenCalled = true;
			if (err_str != none) {
				cout << "in HTTPRequests::Handle_bridge<" << this << ">, err_str is… " << err_str << endl;
				_fn(std::move(*err_str), nullptr);
				return;
			}
			_fn(none, atomic_get());
		}
		//
		// Delegation
		//			if (ec || _response.result() != status::ok) {
		//				_call_fn_with(string(ec.message()));
		//				return;
		//			}
		//			auto data = std::make_shared<rapidjson::Document>();
		//			auto r = !(*data).Parse(_response.body().c_str(), _response.body().size()).HasParseError();
		//			//			cout << "_response.body().c_str(): " << _response.body().c_str() << endl;
		//			assert(r);
		//			std::atomic_store(&_result, data);
		//			//
		//			// TODO: perhaps dispatch->async?
		//			//
		//			_isBeingShutDown = true;
		//			_isShutDown = true;
		//			_isBeingShutDown = false;
		//			_isStreamInUse = false; // may as well unset this
		//
		// and this will free 'this'
		//			_call_fn_with_success();
		//		}
	};
}
//

#endif /* HTTPRequests_Handle_bridge_HPP_ */

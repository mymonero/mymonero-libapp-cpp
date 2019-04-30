//
//  HTTPRequests_Interface.hpp
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

#ifndef HTTPRequests_Interface_hpp
#define HTTPRequests_Interface_hpp

#include <string>
#include <functional>
#include <boost/optional/optional.hpp>
#include "rapidjson_defines.hpp" // must be included before rapidjson include
#include "rapidjson/document.h"

namespace HTTPRequests
{
	using namespace std;
	using namespace boost;
	using namespace rapidjson;
	//
	// Accessory Types
	typedef Document ReqParams;
	typedef Document ResponseJSON;
	enum Scheme
	{
		HTTP,
		HTTPS
	};
	static inline string port_from_scheme(Scheme scheme)
	{
		switch (scheme) {
			case HTTPS:
				return "8080";
			case HTTP:
				return "80";
		}
	}
	//
	// Principal Types
	struct Handle
	{
		virtual ~Handle() {}
		//
		virtual void cancel() = 0;
	};
	//
	struct RequestFactory
	{
		virtual ~RequestFactory() {}
		//
		virtual std::shared_ptr<Handle> new_request(
			Scheme scheme,
			string authority, // host+':'+port
			string endpoint_path,
			ReqParams params,
			std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)>&& fn
		) = 0;
	private:
	};
}

#endif /* HTTPRequests_Interface_hpp */

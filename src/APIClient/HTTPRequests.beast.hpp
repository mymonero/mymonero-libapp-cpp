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

#ifndef HTTPRequests_beast_hpp
#define HTTPRequests_beast_hpp

#include "./HTTPRequests_Interface.hpp"
#include <boost/beast.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/optional/optional_io.hpp>
#include "rapidjson/writer.h"
#include "./root_certificates.hpp"
//
namespace HTTPRequests
{
	using boost::beast::http::verb;
	using boost::beast::http::status;
	using tcp = boost::asio::ip::tcp;
	using body_type = boost::beast::http::string_body;
	using request = boost::beast::http::request<body_type>;
	using response = boost::beast::http::response<body_type>;
	using boost::asio::ip::tcp;
	
	struct Handle_beast: public Handle, public std::enable_shared_from_this<Handle_beast>
	{
	public:
		Handle_beast(
			boost::asio::io_context &io_ctx,
			boost::asio::ssl::context &ssl_ctx,
			boost::beast::http::verb verb,
			string host,
			string port,
			string endpoint_path,
			std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)>&& fn
		):	_strand(io_ctx),
			_stream(io_ctx, ssl_ctx),
			_resolver(io_ctx),
			_verb(verb),
			_host(host),
			_port(port),
			_endpoint_path(endpoint_path),
			_fn(std::move(fn))
		{}
		~Handle_beast() {}
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
			cout << "Canceling a HTTPRequests::Handle_beast" << endl;
			if (_isConnectionClosed) {
				return;
			}
			_isConnectionClosed = true;
			_strand.dispatch([
				c = std::weak_ptr<Handle_beast>(shared_from_this())
			] () {
				if (auto cl = c.lock()) {
					cl->_stream.shutdown(); // possible FIXME: this may need to be something other than shutdown()
				}
			});
			
		}
		//
		// Imperatives - Public
		void run(bool set_postbody, ReqParams postbody)
		{
			if (SSL_set_tlsext_host_name(_stream.native_handle(), _host.c_str()) == false) { // Set SNI Hostname (many hosts need this to handshake successfully)
				boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
				_call_fn_with(string(ec.message()));
				return;
			}
			ostringstream target_ss;
			target_ss << "/" << _endpoint_path; // prepend "/"
			//
			_request.version(10);
			_request.method(_verb);
			_request.target(target_ss.str());
			_request.set(boost::beast::http::field::content_type, "application/json");
			_request.set(boost::beast::http::field::accept, "application/json");
			//
			if (set_postbody) { // the API for this could definitely be better designed since not every request is going to be a post …… the param for this is not as elegant as it could be
				assert(_verb == boost::beast::http::verb::post);
				//
				StringBuffer buffer;
				Writer<StringBuffer> writer(buffer);
				postbody.Accept(writer);
				//
				_request.body() = buffer.GetString();
//				cout << "~~~~> body: " << _request.body() << endl;
				_request.prepare_payload(); // set body size in header
			}
			_resolver.async_resolve(
				_host.c_str(),
				_port.c_str(),
				_strand.wrap(std::bind(
					&Handle_beast::on_resolve,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2
				))
			);
		}
	private:
		//
		// Initialized
		boost::asio::io_context::strand _strand;
		boost::asio::ssl::stream<boost::asio::ip::tcp::socket> _stream;
		tcp::resolver _resolver;
		boost::beast::http::verb _verb;
		string _host;
		string _port;
		string _endpoint_path;
		std::function<void(optional<string> err_str, std::shared_ptr<rapidjson::Document> res)>&& _fn;
		//
		// Runtime
		bool _isConnectionClosed = false;
		bool _hasFNBeenCalled = false;
		boost::beast::flat_buffer _buffer;
		boost::beast::http::request<boost::beast::http::string_body> _request;
		boost::beast::http::response<boost::beast::http::string_body> _response;
		std::shared_ptr<rapidjson::Document> _result;
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
				cout << "err_str is… " << err_str << endl;
				_fn(std::move(*err_str), nullptr);
				return;
			}
			_fn(none, atomic_get());
		}
		//
		// Delegation
		void on_resolve(
			beast::error_code ec,
			tcp::resolver::results_type resolver_results
		) {
			if (ec) {
				// TODO: pass status code
				_call_fn_with(string(ec.message()));
				return;
			}
			boost::asio::async_connect( // Make the connection on the IP address we get from a lookup
				_stream.next_layer(),
				resolver_results.begin(),
				resolver_results.end(),
				_strand.wrap(std::bind(
					&Handle_beast::on_connect,
					shared_from_this(),
					std::placeholders::_1
				))
			);
		}
		void on_connect(boost::beast::error_code ec)
		{
			if (ec) {
				// TODO: set _isConnectionClosed to true??
				_call_fn_with(string(ec.message()));
				return;
			}
			_stream.async_handshake(
				ssl::stream_base::client,
				_strand.wrap(std::bind(
					&Handle_beast::on_handshake,
					shared_from_this(),
					std::placeholders::_1
				))
			);
		}
		void on_handshake(beast::error_code ec)
		{
			if (ec) {
				// TODO: set _isConnectionClosed to true??
				_call_fn_with(string(ec.message()));
				return;
			}
			boost::beast::http::async_write(
				_stream, _request,
				_strand.wrap(std::bind(
					&Handle_beast::on_write,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2
				))
			);
		}
		void on_write(boost::beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);
			//
			if (ec) {
				_call_fn_with(string(ec.message()));
				return;
			}
			boost::beast::http::async_read(
				_stream, _buffer, _response,
				_strand.wrap(std::bind(
					&Handle_beast::on_read,
					shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2
				))
			);
		}
		void on_read(
			beast::error_code ec,
			std::size_t bytes_transferred
		) {
			boost::ignore_unused(bytes_transferred);
			//
			if (ec || _response.result() != status::ok) {
				_call_fn_with(string(ec.message()));
				return;
			}
			auto data = std::make_shared<rapidjson::Document>();
			auto r = !(*data).Parse(_response.body().c_str(), _response.body().size()).HasParseError();
			assert(r);
			std::atomic_store(&_result, data);
			//
			// TODO: perhaps dispatch->async?
			_call_fn_with_success();
			//
			// Gracefully close the stream
			_stream.async_shutdown(
				_strand.wrap(std::bind(
					&Handle_beast::on_shutdown,
					shared_from_this(),
					std::placeholders::_1
				))
			);
		}
		void on_shutdown(beast::error_code ec)
		{
			if (ec == boost::asio::error::eof) {
				// Rationale:
				// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
				ec = {};
			}
			if (ec) {
				// probably don't need to call back on fn here
			}
			// If we get here then the connection is closed gracefully
		}
	};
	//
	struct RequestFactory_beast: public RequestFactory
	{
		RequestFactory_beast(
			boost::asio::io_context &io_ctx
		):	_io_ctx(io_ctx)
		{}
		~RequestFactory_beast() {}
		//
		std::shared_ptr<Handle> new_request(
			Scheme scheme,
			string authority, // host+':'+port
			string endpoint_path,
			ReqParams params,
			std::function<void(optional<string> err_str, std::shared_ptr<ResponseJSON> res)>&& fn
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
			ssl::context ssl_ctx{ssl::context::tlsv12_client};
			load_root_certificates(ssl_ctx);
			ssl_ctx.set_verify_mode(ssl::verify_peer);
			//
			auto verb = boost::beast::http::verb::post;
			auto handle_ptr = std::make_shared<Handle_beast>(
				_io_ctx, ssl_ctx,
				verb, string(host), string(port), endpoint_path,
				std::move(fn)
			);
			handle_ptr->run(
				// v------- This API could probably be designed better
				verb == boost::beast::http::verb::post ? true : false,
				std::move(params)
			);
			//
			return handle_ptr;
		}
	private:
		boost::asio::io_context &_io_ctx;
	};
}

#endif /* HTTPRequests_beast_hpp */

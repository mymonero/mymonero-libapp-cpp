//
//  Dispatch.asio.hpp
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

#ifndef Dispatch_asio_hpp
#define Dispatch_asio_hpp

#include <string>
#include "./Dispatch_Interface.hpp"
#include <boost/asio.hpp>

namespace Dispatch
{
	using namespace std;
	using namespace boost::asio;
	//
	struct io_ctx_thread_holder
	{
		io_context &_ctx;
		//
		io_ctx_thread_holder(io_context& ctx):
		_ctx(ctx)
		{
		}
		~io_ctx_thread_holder()
		{
			_thread.join();
		}
		//
		void setup_after_work_guard()
		{ // this must be called after init - and it's broken out into a separate function so that the holder of the work_guard can place it before calling _ctx.run() (to avoid a race condition)
			_thread = std::thread([this]() {
				_ctx.run();
				if (true) {
					BOOST_THROW_EXCEPTION(logic_error("io_ctx_thread_holder's _ctx.run() should never return."));
				}
			});
		}
	private:
		std::thread _thread;
	};
	//
	struct CancelableTimerHandle_asio: public CancelableTimerHandle
	{
		CancelableTimerHandle_asio(steady_timer *t):
			_t(t)
		{
		}
		~CancelableTimerHandle_asio() {}
		//
		void cancel()
		{ // don't need to worry about lifecycle of _t since it's only deleted upon async_wait call
			_t->cancel();
		}
	private:
		steady_timer *_t;
	};
	//
	struct Dispatch_asio: public Dispatch
	{
		Dispatch_asio(io_ctx_thread_holder& ctx_thread_holder):
			_ctx(ctx_thread_holder._ctx)
		{
			//
			// Now that the work_guard is in place:
			ctx_thread_holder.setup_after_work_guard(); // this *must* get called or no thread will be spawned nor will the ctx have run() called
		}
		~Dispatch_asio() {}
		//
		std::unique_ptr<CancelableTimerHandle> after(uint32_t ms, std::function<void()>&& fn)
		{
			auto t = new steady_timer(_ctx, boost::asio::chrono::milliseconds(ms));
			t->async_wait([fn = std::move(fn), t](const boost::system::error_code &e)
			{
				if (e != boost::asio::error::operation_aborted) { // timer not canceled
					fn();
				}
				delete t;
			});
			return std::make_unique<CancelableTimerHandle_asio>(t);
		}
		void async(std::function<void()>&& fn)
		{
			boost::asio::post(_ctx, [fn = std::move(fn)]()
			{
				fn();
			});
		}
	private:
		io_context &_ctx;
		const executor_work_guard<io_context::executor_type> _guard = make_work_guard(_ctx); // const just to make sure no one gets rid of the work guard
	};
}

#endif /* Dispatch_asio_hpp */

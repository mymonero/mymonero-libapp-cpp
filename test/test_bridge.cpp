//
//  test_bridge.cpp
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
// Test module setup
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE LibAppTests_Bridge
#include <boost/test/unit_test.hpp> // last
#include <boost/assert.hpp>
#include <boost/asio.hpp>
using namespace boost::asio;
#include "../Dispatch/Dispatch.asio.hpp"
// Includes & namespaces
using namespace std;
using namespace boost;
//
namespace utf = boost::unit_test;
//
#include "tests_common.hpp"
#include "../src/AppBridge/AppBridge.hpp"
//
class App::ServiceLocator_SpecificImpl
{
public:
	io_context io_ctx;
	Dispatch::io_ctx_thread_holder ctx_thread_holder{io_ctx};
	//
	ServiceLocator_SpecificImpl() {}
	~ServiceLocator_SpecificImpl() {}
};
//
App::ServiceLocator_SpecificImpl *pImpl_ptr = new App::ServiceLocator_SpecificImpl();
static auto dispatch_ptr = std::make_shared<Dispatch::Dispatch_asio>(pImpl_ptr->ctx_thread_holder);
static std::shared_ptr<App::Bridge> bridge_ptr = nullptr;
static boost::signals2::connection evented_signal__connection;
bool has_seen_bridge_emit = false;
//
string changed_mock_password_string = string("a changed mock password");
void handle_event(const string &msg)
{
	using namespace App;
	using namespace Bridge_event;
	using namespace Bridge_exec;
	//
	has_seen_bridge_emit = true;
	cout << "Seen event msg: " << msg << endl;
	//
	_Convenience__Event ev = new_convenience__event_with(msg);
	if (ev.eventName == Name__getUserToEnterExistingPassword) {
		auto rep_msg = Bridge_exec::new_msg_with(
			Module__PasswordController,
			PasswordController__enterExistingPassword_cb,
			[] (Value &params, Document::AllocatorType &a)
			{
				params.AddMember("didCancel_orNone", false, a);
				params.AddMember("obtainedPasswordString", changed_mock_password_string, a);
			}
		);
		bridge_ptr->exec(rep_msg);
	} else {
		BOOST_ASSERT_MSG(false, "Unrecognized event name");
	}
}
//
BOOST_AUTO_TEST_CASE(setup)
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "setup" << endl;
	using namespace App;
	//
	bridge_ptr = std::make_shared<App::Bridge>();
//	std::weak_ptr<App::Bridge> weak_bridge_ptr = bridge_ptr;
	evented_signal__connection = bridge_ptr->evented_signal.connect([
//		weak_bridge_ptr
	] (string msg) {
		dispatch_ptr->async([msg](){
			handle_event(msg);
		});
	});
	bridge_ptr->setup(
		pImpl_ptr,
		new_documentsPath(),
		MAINNET,
		dispatch_ptr
	);
	//
	// NOTE: and, intentionally, NOT disconnecting from evented_signal__connection ... til the end of the tests!!
}
BOOST_AUTO_TEST_CASE(sleepBriefly, *utf::depends_on("setup"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "sleepBriefly" << endl;
	using namespace App;
	//
	
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	cout << "… done sleeping." << endl;

}
//
BOOST_AUTO_TEST_CASE(teardownRuntime, *utf::depends_on("sleepBriefly"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "teardownRuntime" << endl;
	using namespace App;
	//
	evented_signal__connection.disconnect();
	//
	bridge_ptr = nullptr; // trigger teardown
	dispatch_ptr = nullptr;
	delete pImpl_ptr; // must manage this ourselves … and it seems it needs to be done near/at the end 
	cout << "done with teardown…" << endl;
}

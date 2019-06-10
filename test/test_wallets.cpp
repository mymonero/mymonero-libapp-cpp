//
//  test_all.cpp
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
#define BOOST_TEST_MODULE LibAppTests_Wallets
#include <boost/test/unit_test.hpp> // last
#include <boost/assert.hpp>
//
// Includes & namespaces
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/uuid/uuid.hpp> // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp> // streaming operators etc.
#include <boost/asio.hpp>
using namespace std;
using namespace boost;
//
#include "cryptonote_config.h"
using namespace cryptonote;
//
namespace utf = boost::unit_test;
//
#include "tests_common.hpp"
//
// Test Suites - ServiceLocator
#include "../src/App/AppServiceLocator.hpp"
//
//
// NOTE: It's expected that test_all is run before these tests so that the password-change has already been performed
//
//
string changed_mock_password_string = string("a changed mock password");
class Mocked_EnterExistingOrNewCorrectChanged_PasswordEntryDelegate: public Passwords::PasswordEntryDelegate
{
public:
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
	) {
		BOOST_REQUIRE(isForChangePassword == false);
		BOOST_REQUIRE(isForAuthorizingAppActionOnly == false);
		BOOST_REQUIRE(customNavigationBarTitle == none);
		BOOST_REQUIRE(App::ServiceLocator::instance().passwordController->getPassword() == none);
		//
		cout << "Being asked for the existing pw" << endl;
		//
		App::ServiceLocator::instance().passwordController->enterExistingPassword_cb(false, changed_mock_password_string);
	}
	void getUserToEnterNewPasswordAndType(
		bool isForChangePassword
	) {
		
		cout << "Being asked for a new pw" << endl;
		
		// in case test_wallets is the first test to run…
		App::ServiceLocator::instance().passwordController->enterNewPasswordAndType_cb(false, changed_mock_password_string, Passwords::Type::password);
	}
};
//
auto initial_pwEntryDelegate_spt = std::make_shared<Mocked_EnterExistingOrNewCorrectChanged_PasswordEntryDelegate>();
App::ServiceLocator &builtSingleton(
	cryptonote::network_type nettype
) {
	using namespace App;
	return ServiceLocator::instance().build(
		new_documentsPath(),
		nettype,
		initial_pwEntryDelegate_spt
	);
}
BOOST_AUTO_TEST_CASE(serviceLocator_build)
{
	using namespace App;

	builtSingleton(MAINNET);
	
	BOOST_REQUIRE(ServiceLocator::instance().built == true);
	BOOST_REQUIRE(App::ServiceLocator::instance().dispatch_ptr != nullptr);
}
//
//
BOOST_AUTO_TEST_CASE(walletsListController_addByMnemonic, *utf::depends_on("serviceLocator_build"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "walletsListController_addByMnemonic" << endl;
	using namespace App;
	//
	BOOST_REQUIRE(ServiceLocator::instance().settingsController->set_specificAPIAddressURLAuthority(
		string("api.mymonero.com:8443")
	));
	//
	auto wlc_spt = ServiceLocator::instance().walletsListController;
	bool hasAdded = false;
	size_t nthCallOfListUpdated = 0;
	auto connection = wlc_spt->list__updated_signal.connect(
		[&nthCallOfListUpdated, &hasAdded, &wlc_spt]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); // sleeping to wait for the givenBooted_delete to return - since we cannot guarantee that the call will return before we enter this
			//
			nthCallOfListUpdated += 1;
			size_t expectingNumRecords = hasAdded ? 1 : 0; // we can rely on hasBooted being set to true before the first list__updated signal is fired because the onceBooted cb is sync and called before the signal is executed asynchronously
			BOOST_REQUIRE_MESSAGE(wlc_spt->records().size() >= expectingNumRecords, "Expected number of records to be at least " << expectingNumRecords << " in call " << nthCallOfListUpdated << " of walletsListController_addByMnemonic but it was " << wlc_spt->records().size());
		}
	);
	//
	BOOST_REQUIRE(wlc_spt->hasBooted() == true); // asserting this because .setup() is all synchronous … which is why onceBooted's fn gets called -after- the first list__updated!! rather than before it ……… we could make the _flushWaitingBlocks… call asynchronously executed as well, but that might mess up other architecture assumptions…… hmm
	size_t num_records_before_add = wlc_spt->records().size();
	//
	wlc_spt->OnceBooted_ObtainPW_AddExtantWalletWith_MnemonicString(
		string("Dark grey"), Wallets::SwatchColor::darkGrey,
		string("fox sel hum nex juv dod pep emb bis ela jaz vib bis"),
		[&hasAdded, num_records_before_add] (
			optional<string> err_str,
			optional<std::shared_ptr<Wallets::Object>> wallet_spt,
			optional<bool> wasWalletAlreadyInserted
		) {
			if (err_str != none) {
				cout << "Error while adding wallet: " << *err_str << endl;
			}
			BOOST_REQUIRE_MESSAGE(err_str == none, "Error while adding wallet");
			if (err_str == none) {
				hasAdded = true;
			}
			BOOST_REQUIRE_MESSAGE(wallet_spt != none, "Expected non-none wallet_spt");
			BOOST_REQUIRE(wasWalletAlreadyInserted != none);
			if (*wasWalletAlreadyInserted == true) {
				if (num_records_before_add > 0) {
					cout << "A wallet with that address was already added" << endl;
				} else {
					BOOST_REQUIRE_MESSAGE(false, "Expected pre-existing wallets if the wallet has already been added by that mnemonic");
				}
			}
		}
	);
	//
	std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // wait for login network request completion, async notifies……
	connection.disconnect(); // critical
}
//
BOOST_AUTO_TEST_CASE(walletsListController_addByAddressAndKeys, *utf::depends_on("walletsListController_addByMnemonic"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "walletsListController_addByAddressAndKeys" << endl;
	using namespace App;
	//
	BOOST_REQUIRE(ServiceLocator::instance().settingsController->set_specificAPIAddressURLAuthority(
		string("api.mymonero.com:8443")
	));
	//
	auto wlc_spt = ServiceLocator::instance().walletsListController;
	bool hasAdded = false;
	size_t nthCallOfListUpdated = 0;
	auto connection = wlc_spt->list__updated_signal.connect(
		[&nthCallOfListUpdated, &hasAdded, &wlc_spt]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50)); // sleeping to wait for the givenBooted_delete to return - since we cannot guarantee that the call will return before we enter this
			//
			nthCallOfListUpdated += 1;
			size_t expectingNumRecords = hasAdded ? 1 : 0; // we can rely on hasBooted being set to true before the first list__updated signal is fired because the onceBooted cb is sync and called before the signal is executed asynchronously
			BOOST_REQUIRE_MESSAGE(wlc_spt->records().size() >= expectingNumRecords, "Expected number of records to be at least " << expectingNumRecords << " in call " << nthCallOfListUpdated << " of walletsListController_addByAddressAndKeys but it was " << wlc_spt->records().size());
		}
	);
	//
	BOOST_REQUIRE(wlc_spt->hasBooted() == true); // asserting this because .setup() is all synchronous … which is why onceBooted's fn gets called -after- the first list__updated!! rather than before it ……… we could make the _flushWaitingBlocks… call asynchronously executed as well, but that might mess up other architecture assumptions…… hmm
	
	size_t num_records_before_add = wlc_spt->records().size();
	//
	wlc_spt->OnceBooted_ObtainPW_AddExtantWalletWith_AddressAndKeys(
		string("Dark grey (addr and keys)"), Wallets::SwatchColor::darkGrey,
		string("43zxvpcj5Xv9SEkNXbMCG7LPQStHMpFCQCmkmR4u5nzjWwq5Xkv5VmGgYEsHXg4ja2FGRD5wMWbBVMijDTqmmVqm93wHGkg"),
		string("7bea1907940afdd480eff7c4bcadb478a0fbb626df9e3ed74ae801e18f53e104"),
		string("4e6d43cd03812b803c6f3206689f5fcc910005fc7e91d50d79b0776dbefcd803"),
		[&hasAdded, num_records_before_add] (
			optional<string> err_str,
			optional<std::shared_ptr<Wallets::Object>> wallet_spt,
			optional<bool> wasWalletAlreadyInserted
		) {
			if (err_str != none) {
				cout << "Error while adding wallet: " << *err_str << endl;
			}
			BOOST_REQUIRE_MESSAGE(err_str == none, "Error while adding wallet");
			if (err_str == none) {
				hasAdded = true;
			}
			BOOST_REQUIRE_MESSAGE(wallet_spt != none, "Expected non-none wallet_spt");
			BOOST_REQUIRE(wasWalletAlreadyInserted != none);
			if (*wasWalletAlreadyInserted == true) {
				if (num_records_before_add > 0) {
					cout << "Wallet with that addr was already added" << endl;
				} else {
					BOOST_REQUIRE_MESSAGE(false, "Expected pre-existing wallets if the wallet has already been added by that addr and keys");
				}
			}
		}
	);
	//
	std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // wait for login network request completion, async notifies……
	connection.disconnect(); // critical
}
BOOST_AUTO_TEST_CASE(walletsListController_createNewWallet, *utf::depends_on("walletsListController_addByAddressAndKeys"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "walletsListController_createNewWallet" << endl;
	using namespace App;
	//
	BOOST_REQUIRE(ServiceLocator::instance().settingsController->set_specificAPIAddressURLAuthority(
		string("api.mymonero.com:8443")
	));
	//
	auto wlc_spt = ServiceLocator::instance().walletsListController;
	bool hasAdded = false;
	size_t nthCallOfListUpdated = 0;
	auto connection = wlc_spt->list__updated_signal.connect(
		[&nthCallOfListUpdated, &hasAdded, &wlc_spt]()
		{
		std::this_thread::sleep_for(std::chrono::milliseconds(50)); // sleeping to wait for the givenBooted_delete to return - since we cannot guarantee that the call will return before we enter this
		//
		nthCallOfListUpdated += 1;
//		size_t expectingNumRecords = hasAdded ? 1 : 0; // we can rely on hasBooted being set to true before the first list__updated signal is fired because the onceBooted cb is sync and called before the signal is executed asynchronously
//		BOOST_REQUIRE_MESSAGE(wlc_spt->records().size() >= expectingNumRecords, "Expected number of records to be at least " << expectingNumRecords << " in call " << nthCallOfListUpdated << " of walletsListController_addByAddressAndKeys but it was " << wlc_spt->records().size());
		}
	);
		//
	BOOST_REQUIRE(wlc_spt->hasBooted() == true); // asserting this because .setup() is all synchronous … which is why onceBooted's fn gets called -after- the first list__updated!! rather than before it ……… we could make the _flushWaitingBlocks… call asynchronously executed as well, but that might mess up other architecture assumptions…… hmm
	
	size_t num_records_before_add = wlc_spt->records().size();
	cout << "num_records_before_add " << num_records_before_add << endl;
	//
	std::shared_ptr<Wallets::Object> cached__wallet_spt = nullptr;
	//
	wlc_spt->CreateNewWallet_NoBootNoListAdd(
		string("en-US"),
		[&hasAdded, &wlc_spt, num_records_before_add, &cached__wallet_spt] (
			optional<string> err_str,
			optional<std::shared_ptr<Wallets::Object>> wallet_spt
		) {
			if (err_str != none) {
				cout << "Error while creating new wallet: " << *err_str << endl;
			}
			BOOST_REQUIRE_MESSAGE(err_str == none, "Error while creating new wallet");
			if (err_str == none) {
				hasAdded = true;
			}
			BOOST_REQUIRE_MESSAGE(wallet_spt != none, "Expected non-none wallet_spt");
			BOOST_REQUIRE(wlc_spt->records().size() == num_records_before_add); // assert didn't add it to the list yet
			//
			cached__wallet_spt = *wallet_spt;
			cout << "Created wallet " << cached__wallet_spt << endl;
			//
			wlc_spt->OnceBooted_ObtainPW_AddNewlyGeneratedWallet_externallyTmpRetained(
				cached__wallet_spt,
				string("Blue (new)"), Wallets::SwatchColor::blue,
				[&hasAdded, num_records_before_add, &wlc_spt, &cached__wallet_spt] (
					optional<string> err_str,
					optional<std::shared_ptr<Wallets::Object>> wallet_spt
				) {
					if (err_str != none) {
						cout << "Error while adding wallet: " << *err_str << endl;
					}
					cached__wallet_spt = nullptr; // release from here
					BOOST_REQUIRE_MESSAGE(err_str == none, "Error while adding wallet");
					if (err_str == none) {
						hasAdded = true;
					}
					BOOST_REQUIRE_MESSAGE(wallet_spt != none, "Expected non-none wallet_spt");
					BOOST_REQUIRE(wlc_spt->records().size() == num_records_before_add + 1); // assert did add it to the list
				}
			);
		}
	);
	//
	std::this_thread::sleep_for(std::chrono::milliseconds(10000)); // wait for login network request completion, async notifies……
	connection.disconnect(); // critical
}
//
BOOST_AUTO_TEST_CASE(teardownRuntime, *utf::depends_on("walletsListController_createNewWallet"))
{
	cout << endl;
	cout << "---------------------------" << endl;
	cout << "teardownRuntime" << endl;
	using namespace App;
	//
	ServiceLocator::instance().teardown();
}

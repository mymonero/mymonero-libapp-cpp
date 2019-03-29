//
//  PersistedObjectListController.cpp
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
#include "PersistedObjectListController.hpp"
#include "../Persistence/PersistableObject.hpp"
#include "misc_log_ex.h"
#include <boost/foreach.hpp>
using namespace std;
using namespace boost;
using namespace document_persister;
using namespace Lists;
//
// Imperatives - Lifecycle - Setup
void Controller::setup()
{
	if (documentsPath == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected documentsPath != nullptr"));
	}
	if (dispatch_ptr == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected dispatch_ptr != nullptr"));
	}
	if (passwordController == nullptr) {
		BOOST_THROW_EXCEPTION(logic_error("ListController: expected passwordController != nullptr"));
	}
	setup_startObserving();
	setup_tryToBoot();
}
void Controller::setup_startObserving()
{
	startObserving_passwordController();
}
void Controller::setup_tryToBoot()
{
	overridable_deferBootUntil([this](optional<string> err_str)
	{
		if (err_str != none) {
			BOOST_THROW_EXCEPTION(logic_error(*err_str));
			return;
		}
		setup_fetchAndReconstituteExistingRecords();
	});
}
void Controller::startObserving_passwordController()
{
	passwordController->addRegistrantForDeleteEverything(*this);
	passwordController->addRegistrantForChangePassword(*this);
	//
	connection__PasswordController_willDeconstructBootedStateAndClearPassword = passwordController->willDeconstructBootedStateAndClearPassword_signal.connect(
		std::bind(&Controller::PasswordController_willDeconstructBootedStateAndClearPassword, this)
	);
	connection__PasswordController_didDeconstructBootedStateAndClearPassword = passwordController->didDeconstructBootedStateAndClearPassword_signal.connect(
		std::bind(&Controller::PasswordController_didDeconstructBootedStateAndClearPassword, this)
	);
}
void Controller::setup_fetchAndReconstituteExistingRecords()
{
	_records_mutex.lock();
	{
		_records.clear(); // zeroing
	}
	_records_mutex.unlock();
	//
	auto err_orIds = _new_idsOfPersistedRecords();
	if (err_orIds.err_str != boost::none) {
		_setup_didFailToBoot(*err_orIds.err_str);
		return;
	}
	if (err_orIds.ids->size() == 0) { // we must check before requesting password so pw entry not enforced when not necessary
		_setup_didBoot();
		return;
	}
	passwordController->onceBootedAndPasswordObtained(
		[this](Passwords::Password password, Passwords::Type type)
		{
			// now we actually want to load the ids again after we have the password - or we'll have stale ids on having deleted all data in the app and subsequently adding a record!
			// now proceed to load and boot all extant records
			//
			auto inner_err_orIds = _new_idsOfPersistedRecords();
			if (inner_err_orIds.err_str != boost::none) {
				_setup_didFailToBoot(*inner_err_orIds.err_str);
				return;
			}
			if (inner_err_orIds.ids->size() == 0) { // we must check before requesting password so pw entry not enforced when not necessary
				_setup_didBoot();
				return;
			}
			auto err_orJSONStrings = document_persister::documentsWith(*documentsPath, _listedObjectTypeCollectionName, *inner_err_orIds.ids);
			if (err_orJSONStrings.err_str != boost::none) {
				_setup_didFailToBoot(*err_orJSONStrings.err_str);
				return;
			}
			if (err_orJSONStrings.strings->size() == 0) { // just in case
				_setup_didBoot();
				return;
			}
			for (auto it = (*(err_orJSONStrings.strings)).begin(); it != (*(err_orJSONStrings.strings)).end(); it++) {
				optional<string> plaintext_documentContentString = Persistable::new_plaintextStringFrom(
					*it,
					*(passwordController->getPassword())
				);
				if (plaintext_documentContentString == none) {
					_setup_didFailToBoot("ListController: Incorrect password");
					return; // sort of a fatal condition, though
				}
				property_tree::ptree plaintext_documentJSON;
				try {
					plaintext_documentJSON = Persistable::new_plaintextDocumentDictFromJSONString(*plaintext_documentContentString);
				} catch (const std::exception & e) {
					_setup_didFailToBoot(e.what());
					return;
				}
				std::shared_ptr<Persistable::Object> listedObjectInstance = new_record(
					documentsPath,
					passwordController,
					plaintext_documentJSON
				);
				_records_mutex.lock();
				{ // TODO: should we lock outside of the for loop?
					_records.push_back(listedObjectInstance);
				}
				_records_mutex.unlock();
				overridable_booting_didReconstitute(listedObjectInstance);
			}
			_setup_didBoot();
		},
		[](void) {
			// canceled function
		}
	); // this will 'block' until we have access to the pw
}
void Controller::_setup_didBoot()
{
//	MDEBUG("Lists: " << this << " booted.");
	overridable_finalizeAndSortRecords(); // finalize on every boot - so that overriders can opt to do CRUD even with 0 extant records
	_hasBooted = true; // all done!
	_callAndFlushAllBlocksWaitingForBootToExecute(); // after hasBooted=true; this can probably go in the ->async
	dispatch_ptr->async([this]()
	{ // on next tick to avoid instantiator missing this
		boot__did_signal();
		_listUpdated_records(); // notify every boot, after did-boot notification
	});
}
void Controller::_setup_didFailToBoot(const string &err_str)
{
	MERROR("Lists: ListController " << this << " failed to boot with err: " << err_str);
	dispatch_ptr->async([this]()
	{ // on next tick to avoid instantiator missing this
		boot__failed_signal();
	});
}
//
// Runtime - Accessors - Private - Lookups - Documents & instances
errOr_documentIds Controller::_new_idsOfPersistedRecords()
{
	return document_persister::idsOfAllDocuments(*documentsPath, _listedObjectTypeCollectionName);
}
//
// Imperatives - Lifecycle - Teardown
void Controller::tearDown()
{ // overridable but call on super obvs
	stopObserving();
}
void Controller::stopObserving()
{ // overridable but call on super obvs
	_stopObserving_passwordController();
}
void Controller::_stopObserving_passwordController()
{
	passwordController->removeRegistrantForDeleteEverything(*this);
	passwordController->removeRegistrantForChangePassword(*this);
	//
	connection__PasswordController_willDeconstructBootedStateAndClearPassword.disconnect();
	connection__PasswordController_didDeconstructBootedStateAndClearPassword.disconnect();
}
//
// Imperatives - Execution Deferment
void Controller::onceBooted(std::function<void()> fn)
{
	if (_hasBooted) {
		fn();
		return;
	}
	if (__blocksWaitingForBootToExecute == none) {
		__blocksWaitingForBootToExecute = vector<std::function<void()>>();
	}
	(*__blocksWaitingForBootToExecute).push_back(fn);
}
void Controller::_callAndFlushAllBlocksWaitingForBootToExecute()
{
	if (__blocksWaitingForBootToExecute == none) {
		return;
	}
	BOOST_FOREACH(std::function<void()> &fn, (*__blocksWaitingForBootToExecute))
	{
		fn();
	}
	__blocksWaitingForBootToExecute = none; // free
}
//
// Imperatives - Delete
optional<string> Controller::givenBooted_delete(Persistable::Object &object)
{
	if (!_hasBooted) {
		BOOST_THROW_EXCEPTION(logic_error("Expected _hasBooted by givenBooted_delete call"));
		return string("Logic error");
	}
	optional<string> err_str = object.deleteFromDisk();
	if (err_str == none) { // remove / release
		_removeFromList(object);
		//
		return none;
	} else {
		return std::move(*err_str);
	}
}
optional<string> Controller::givenBooted_delete_noListUpdatedNotify(Persistable::Object &object)
{
	if (!_hasBooted) {
		BOOST_THROW_EXCEPTION(logic_error("Expected _hasBooted by givenBooted_delete call"));
		return string("Logic error");
	}
	optional<string> err_str = object.deleteFromDisk();
	if (err_str == none) { // remove / release
		_removeFromList_noListUpdatedNotify(object);
	}
	return std::move(*err_str);
}
void Controller::_removeFromList(Persistable::Object &object)
{
	_removeFromList_noListUpdatedNotify(object);
	__dispatchAsync_listUpdated_records(); // this needs to be kept asynchronous so that observers of the signal will be called after sync return from _removeFromList
}
void Controller::_removeFromList_noListUpdatedNotify(Persistable::Object &object)
{
	// self.stopObserving(record: record) // if observation added later…
	if (object._id == none) {
		BOOST_THROW_EXCEPTION(logic_error("_removeFromList_noListUpdatedNotify can only remove an object with a non-none _id"));
		return;
	}
	_records_mutex.lock();
	{
		size_t index = 0;
		bool found = false;
		for (std::vector<std::shared_ptr<Persistable::Object>>::iterator it = _records.begin(); it != _records.end(); ++it) {
			if ((*it)->_id == none) {
				continue; // no way to compare reliably, so skip
			}
			if ((*(*it)->_id) == *(object._id)) {
				found = true;
				break;
			}
			index++;
		}
		if (!found) {
			BOOST_THROW_EXCEPTION(logic_error("object is not in _records"));
			return;
		}
		_records.erase(_records.begin() + index);
	}
	_records_mutex.unlock();
}
//
// Delegation - Updates
void Controller::_atRuntime__record_wasSuccessfullySetUp(std::shared_ptr<Persistable::Object> listedObject)
{
	_atRuntime__lockMutexAnd_record_wasSuccessfullySetUp_noSortNoListUpdated(listedObject);
	if (overridable_shouldSortOnEveryRecordAdditionAtRuntime() == true) { // this is no longer actually used by anything
		overridable_finalizeAndSortRecords();
	}
	__dispatchAsync_listUpdated_records();
	// ^-- so control can be passed back before all observers of notification handle their work - which is done synchronously
}
void Controller::_atRuntime__lockMutexAnd_record_wasSuccessfullySetUp_noSortNoListUpdated(std::shared_ptr<Persistable::Object> listedObject)
{
	_records_mutex.lock();
	{
		if (overridable_wantsRecordsAppendedNotPrepended() == true) {
			_records.push_back(listedObject);
		} else {
			_records.insert(_records.begin(), listedObject); // so we add it to the top
		}
	}
	_records_mutex.unlock();
//	overridable_startObserving(listedObject_ptr) // TODO if necessary - but shouldn't be at the moment - if implemented, be sure to add corresponding stopObserving in _removeFromList_noListUpdatedNotify
}
//
void Controller::_listUpdated_records()
{
	list__updated_signal();
}
void Controller::__dispatchAsync_listUpdated_records()
{
	dispatch_ptr->async([this]()
	{
		_listUpdated_records();
	});
}
//
// Delegation - Notifications - Password Controller
void Controller::PasswordController_willDeconstructBootedStateAndClearPassword()
{
	_records_mutex.lock();
	{
		_records.clear();
	}
	_records_mutex.unlock();
	_hasBooted = false;
	// now we'll wait for the "did" event ---v before emiting anything like list updated, etc
}
void Controller::PasswordController_didDeconstructBootedStateAndClearPassword()
{
	__dispatchAsync_listUpdated_records(); // manually emit so that the UI updates to empty list after the pw entry screen is shown
	setup_tryToBoot(); // this will re-request the pw and lead to loading records & booting self
}
//
// Protocols - DeleteEverythingRegistrant
optional<string> Controller::passwordController_DeleteEverything()
{
	errOr_numRemoved result = document_persister::removeAllDocuments(*documentsPath, _listedObjectTypeCollectionName);
	if (result.err_str != none) {
		MERROR("Lists: Error while deleting everything: " << *result.err_str);
	} else {
		MDEBUG("Lists: Deleted all " << _listedObjectTypeCollectionName << ".");
	}
	return std::move(*result.err_str);
}
//
// Protocols - ChangePasswordRegistrant
optional<Passwords::EnterPW_Fn_ValidationErr_Code> Controller::passwordController_ChangePassword()
{
	if (_hasBooted != true) {
		MWARNING("Lists: Controller asked to change password but not yet booted.");
		//
		return Passwords::EnterPW_Fn_ValidationErr_Code::notBootedYet; // critical: not ready to get this
	}
	// change all record passwords by re-saving
	_records_mutex.lock();
	{
		for (std::vector<std::shared_ptr<Persistable::Object>>::iterator it = _records.begin(); it != _records.end(); ++it) {
			if (((*it)->didFailToInitialize_flag != none && *((*it)->didFailToInitialize_flag) == true)
				|| ((*it)->didFailToBoot_flag != none && *((*it)->didFailToBoot_flag) == true)) {
				MERROR("Lists: This record failed to boot. Not messing with its saved data");
				BOOST_THROW_EXCEPTION(logic_error("Expected record to have been booted during change-password.")); // not considering this a runtime error.. app can deal with it.. plus we don't want to abort a save just for this - if we did, change pw revert would not work anyway
				//
				_records_mutex.unlock(); // Critical: must unlock
				return Passwords::EnterPW_Fn_ValidationErr_Code::unexpectedState;
			}
			// by here, we've checked if the record had a problem booting or initializing
			optional<string> err_str = (*it)->saveToDisk();
			if (err_str != none) { // err_str is logged
				_records_mutex.unlock(); // Critical: must unlock
				return Passwords::EnterPW_Fn_ValidationErr_Code::saveError;
			}
		}
	}
	_records_mutex.unlock(); // Critical: must unlock
	return none; // success
}

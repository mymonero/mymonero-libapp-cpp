//
//  PersistedObjectListController.hpp
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

#ifndef PersistedObjectListController_hpp
#define PersistedObjectListController_hpp

#include <string>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include <boost/uuid/uuid.hpp> // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <memory>
#include "../Persistence/document_persister.hpp"
#include "../Persistence/PersistableObject.hpp"
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Passwords/PasswordController.hpp"

namespace Lists
{
	using namespace std;
	using namespace boost;
	using namespace document_persister;
	//
	// Comparators
	static inline bool comparePersistableObjectSharedPtrBy_insertedAt_asc(
		std::shared_ptr<Persistable::Object> l,
		std::shared_ptr<Persistable::Object> r
	) {
		if (l->insertedAt_sSinceEpoch == none) {
			return false;
		}
		if (r->insertedAt_sSinceEpoch == none) {
			return true;
		}
		return *(l->insertedAt_sSinceEpoch) <= *(r->insertedAt_sSinceEpoch);
	}
	//
	// Controllers
	class Controller:
		public Passwords::DeleteEverythingRegistrant,
		public Passwords::ChangePasswordRegistrant
	{
	public:
		//
		// Lifecycle - Init
		Controller(const CollectionName &collectionName):
			_listedObjectTypeCollectionName(collectionName) // copy
		{
			// set dependencies then call setup()
		}
		Controller(const Controller&) = delete; // disable copy constructor to prevent inadvertent temporary in pointer
		Controller& operator=(const Controller&) = delete;
		virtual ~Controller()
		{
			cout << "Destructing a ListController" << endl;
			tearDown();
		}
		//
		// Dependencies
		std::shared_ptr<string> documentsPath;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
		std::shared_ptr<Passwords::Controller> passwordController;
		// Then call:
		void setup();
		//
		virtual std::shared_ptr<Lists::Controller> get_shared_ptr_from_this() = 0; // Child classes must override and implement this with shared_from_this() and by inheriting std::enabled_shared_from_this<Child>
		//
		virtual void overridable_deferBootUntil( // overridable
			std::function<void(optional<string> err_str)> fn
		) {
			fn(boost::none); // make sure to call this
		}
		//
		// Signals
		boost::signals2::signal<void()> boot__did_signal;
		boost::signals2::signal<void()> boot__failed_signal;
		boost::signals2::signal<void()> list__updated_signal;
		boost::signals2::signal<void()> record__deleted_signal;
		//
		// Protocols - PasswordControllerEventParticipant
		std::string identifier() const
		{
			return uuid_string;
		}
		//
		// Accessors
		std::vector<std::shared_ptr<Persistable::Object>> records()
		{ // accessing within the mutex and returning a copy so as to resolve possible mutability and consistency issues
			// TODO: is this too expensive? are the pointers themselves being copied?
			_records_mutex.lock();
			auto r_copy = _records;
			_records_mutex.unlock();
			return r_copy;
		}
		bool hasBooted() const { return _hasBooted; }
		//
		// Accessors - Override
		virtual std::shared_ptr<Persistable::Object> new_record(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
			const document_persister::DocumentJSON &plaintext_documentJSON
		) = 0;
		//
		// Accessors - Overridable
		virtual bool overridable_shouldSortOnEveryRecordAdditionAtRuntime()
		{
			return false; // default
		}
		virtual bool overridable_wantsRecordsAppendedNotPrepended()
		{
			return false; // default
		}
		//
		// Imperatives - Execution Deferment
		void onceBooted(std::function<void()> fn);
		//
		// Imperatives - CRUD
		optional<string> givenBooted_delete(Persistable::Object &object);
		optional<string> givenBooted_delete_noListUpdatedNotify(Persistable::Object &object);
		//
		// Imperatives - Overridable
		virtual void overridable_finalizeAndSortRecords() {}
		//
		// Delegation - Overridable
		virtual void overridable_booting_didReconstitute(std::shared_ptr<Persistable::Object> listedObjectInstance) {} // somewhat intentionally ignores errors and values which would be returned asynchronously, e.g. by way of a callback/block
		//
		// Delegation - Updates
		void _atRuntime__record_wasSuccessfullySetUp(std::shared_ptr<Persistable::Object> listedObject); // this is to be called by subclasses
		void _atRuntime__lockMutexAnd_record_wasSuccessfullySetUp_noSortNoListUpdated(std::shared_ptr<Persistable::Object> listedObject);
		//
	protected:
		bool _hasBooted = false;
		std::vector<std::shared_ptr<Persistable::Object>> _records;
		std::mutex _records_mutex;
		//
		// Lifecycle
		void setup_startObserving();
		void stopObserving();
		//
		// Delegation
		void _listUpdated_records();
		void __dispatchAsync_listUpdated_records();
		//
	private:
		//
		// Properties - Instance members
		std::string uuid_string = boost::uuids::to_string((boost::uuids::random_generator())()); // cached
		const CollectionName _listedObjectTypeCollectionName;
		//
		boost::signals2::connection connection__PasswordController_willDeconstructBootedStateAndClearPassword;
		boost::signals2::connection connection__PasswordController_didDeconstructBootedStateAndClearPassword;
		//
		// Lifecycle
		void setup_tryToBoot();
		void startObserving_passwordController();
		void setup_fetchAndReconstituteExistingRecords();
		void _setup_didBoot();
		void _setup_didFailToBoot(const string &err_str);
		//
		void tearDown();
		void _stopObserving_passwordController();
		//
		// Accessors
		errOr_documentIds _new_idsOfPersistedRecords();
		//
		// Execution deferment
		void _callAndFlushAllBlocksWaitingForBootToExecute();
		optional<vector<std::function<void()>>> __blocksWaitingForBootToExecute = none;
		//
		// CRUD
		void _removeFromList(Persistable::Object &object);
		void _removeFromList_noListUpdatedNotify(Persistable::Object &object);
		//
		// Protocols - DeleteEverythingRegistrant
		optional<string> passwordController_DeleteEverything();
		// Protocols - ChangePasswordRegistrant
		optional<Passwords::EnterPW_Fn_ValidationErr_Code> passwordController_ChangePassword();
		//
		// Delegation - Notifications
		void PasswordController_willDeconstructBootedStateAndClearPassword();
		void PasswordController_didDeconstructBootedStateAndClearPassword();
	};
}

#endif /* PersistedObjectListController_hpp */

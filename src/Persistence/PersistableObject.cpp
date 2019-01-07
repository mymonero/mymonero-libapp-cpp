//
//  PersistableObject.cpp
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
#include "PersistableObject.hpp"
//
// Private - Lifecycle - Teardown
Persistable::Object::~Object()
{
	// TODO: emit .willBeDeinitialized for observers
}
//
// Private - Accessors
std::string Persistable::Object::new_encrypted_serializedFileRepresentation() const
{
	if ((*this->passwordProvider.get()).getPassword() == none) {
		BOOST_THROW_EXCEPTION(logic_error("new_encrypted_serializedFileRepresentation called with nil password"));
		// nothing more should be executed... is there any need to ensure that?
	}
	const property_tree::ptree plaintextDict = this->new_dictRepresentation();
	const std::string plaintextString = new_plaintextJSONStringFromDocumentDict(plaintextDict);
	//
	return Persistable::new_encryptedBase64StringFrom(
		plaintextString,
		*((*this->passwordProvider.get()).getPassword()) // we assume password exists here
	);
}
bool Persistable::Object::_shouldInsertNotUpdate() const
{
	return this->_id == none;
}
//
// Public - Imperatives
optional<std::string> Persistable::Object::saveToDisk()
{
	if (this->_shouldInsertNotUpdate()) {
		return this->_saveToDisk_insert();
	} else {
		return this->_saveToDisk_update();
	}
}
optional<std::string> Persistable::Object::deleteFromDisk()
{
	if ((*this->passwordProvider.get()).getPassword() == none) {
		BOOST_THROW_EXCEPTION(logic_error("deleteFromDisk called without password having been entered"));
		return none; // Asked to delete when no password exists. Unexpected.
	}
	if (this->insertedAt_sSinceEpoch == none || this->_id == none) {
		cout << "Persistence" << ": " << "Asked to delete() but had not yet been saved." << endl; // TODO: maybe call central logging lib which then can be routed to Android log if we want
		//
		// posting notifications so UI updates, e.g. to pop views etc
//		this->willBeDeleted_fns.invoke(this);
//		this->wasDeleted_fns.invoke(this);
		return none; // no error
	}
	if (this->_id == none) {
		BOOST_THROW_EXCEPTION(logic_error("deleteFromDisk called with nil _id"));
	}
//	this.willBeDeleted_fns.invoke(this, "")
	vector<DocumentId> ids;
	ids.push_back(*(this->_id));
	//
	const errOr_numRemoved result = document_persister::removeDocuments(
		*(this->documentsPath.get()),
		this->collectionName(),
		ids
	);
	if (result.err_str != none) {
		cout << "Persistence" << ": " << "Error while deleting object: " << *(result.err_str) << endl;
	} else if (result.numRemoved == none || *(result.numRemoved) < 1) {
		cout << "Persistence" << ": " << "No error while deleting object but numRemoved none or 0." << endl; // TODO: error log or ....
	} else if (*(result.numRemoved) > 1) {
		cout << "Persistence" << ": " << "No error while deleting object but numRemoved > 1." << endl; // TODO: error log or ...
	} else {
		cout << "Persistence" << ": " << "Deleted " << this << endl;
		// NOTE: handlers of this should dispatch async so err_str can be returned -- it would be nice to post this on next-tick but self might have been released by then
//		this.wasDeleted_fns.invoke(this, "")
	}
	return std::move(result.err_str); // does this move do anything?
}
//
// Internal - Imperatives - Writing
// For these, we presume consumers/parents/instantiators have only created this wallet if they have gotten the password
optional<std::string> Persistable::Object::_saveToDisk_insert()
{
	if (this->_id != none) {
		BOOST_THROW_EXCEPTION(logic_error("_saveToDisk_insert called with non-nil _id"));
	}
	if ((*this->passwordProvider.get()).getPassword() == none) {
		BOOST_THROW_EXCEPTION(logic_error("_saveToDisk_insert called without password"));
		return none; // Asked to insert new when no password exists. Probably ok if currently tearing down logged-in runtime. Ensure self is not being prevented from being freed.
	}
	// only generate _id here after checking shouldInsertNotUpdate since that relies on _id
	this->_id = document_persister::new_documentId(); // generating a new UUID
	{ // and since we know this is an insertion, let's any other initial centralizable data
		posix_time::ptime t0(gregorian::date(1970,1,1));
		posix_time::ptime t(posix_time::second_clock::universal_time());
		posix_time::time_duration dur = t - t0; // s since epoch
		this->insertedAt_sSinceEpoch = (dur.total_milliseconds() / 1000);
	}
	// and now that those values have been placed, we can generate the dictRepresentation
	optional<std::string> errStr = this->__write();
	if (errStr != none) {
		cout << "Persistence" << ": " << "Error while saving new object: " << *errStr << endl; // TODO: error log
	} else {
		cout << "Persistence" << ": " << "Inserted new member of " << this->collectionName() << endl;
	}
	return errStr;
}
optional<std::string> Persistable::Object::_saveToDisk_update()
{
	if (this->_id == none) {
		BOOST_THROW_EXCEPTION(logic_error("_saveToDisk_update called with nil _id"));
	}
	if ((*this->passwordProvider.get()).getPassword() == none) {
		BOOST_THROW_EXCEPTION(logic_error("_saveToDisk_update called without password"));
		return none; // Asked to update new when no password exists. Probably ok if currently tearing down logged-in runtime. Ensure self is not being prevented from being freed.
	}
	optional<std::string> errStr = this->__write();
	if (errStr != none) {
		cout << "Persistence" << ": " << "Error while saving new object: " << *errStr << endl; // TODO: err log
	} else {
		cout << "Persistence" << ": " << "Updated member of " << this->collectionName() << "." << endl;
	}
	return errStr;
}
optional<std::string> Persistable::Object::__write()
{
	std::string stringToWrite = this->new_encrypted_serializedFileRepresentation();
	if (stringToWrite == "") {
		BOOST_THROW_EXCEPTION(std::runtime_error("stringToWrite should never be empty"));
	}
	if (this->_id == none) {
		BOOST_THROW_EXCEPTION(logic_error("__write called with nil _id"));
	}
	return document_persister::write(
		*(this->documentsPath.get()),
		stringToWrite,
		*(this->_id),
		this->collectionName()
	);
}

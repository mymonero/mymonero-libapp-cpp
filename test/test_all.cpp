//
//  test_all.cpp
//  MyMonero
//
//  Copyright (c) 2014-2018, MyMonero.com
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
#define BOOST_TEST_MODULE LibAppTests
#include <boost/test/unit_test.hpp> // last
//
// Includes & namespaces
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/filesystem.hpp>
using namespace std;
using namespace boost;
//
// Shared code
inline std::string _new_documentsPathString()
{
	std::string thisfile_path = std::string(__FILE__);
	std::string tests_dir = thisfile_path.substr(0, thisfile_path.find_last_of("\\/"));
	std::string srcroot_dir = tests_dir.substr(0, tests_dir.find_last_of("\\/"));
	boost::filesystem::path dir(srcroot_dir);
	boost::filesystem::path file("build");
    boost::filesystem::path full_path = dir / file;
	//
	boost::filesystem::create_directory(full_path); // in case it doesn't exist
	//
	return full_path.string();
}
inline std::shared_ptr<string> new_documentsPath()
{
	return std::make_shared<string>(
		_new_documentsPathString()
	);
}
//
// Test suites - document_persister
#include "../src/Persistence/document_persister.hpp"
using namespace document_persister;
const CollectionName mockedPlainStringDocs__CollectionName = "TestDocuments";
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_insert)
{
	const DocumentId id = new_documentId();
	std::stringstream jsonSS;
	jsonSS << "{\"a\":2,\"id\":\"" << id << "\"}";
	const std::string jsonString = jsonSS.str();
	//
	boost::optional<std::string> err_str = document_persister::write(
		*new_documentsPath(),
		jsonString,
		id,
		mockedPlainStringDocs__CollectionName
	);
	if (err_str) {
		std::cout << *err_str << std::endl;
		BOOST_REQUIRE(!err_str);
	}
	std::cout << "Inserted " << id << std::endl;
}
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_allIds)
{
	errOr_documentIds result  = idsOfAllDocuments(
		*(new_documentsPath().get()),
		mockedPlainStringDocs__CollectionName
	);
	if (result.err_str) {
		std::cout << *result.err_str << std::endl;
		BOOST_REQUIRE(!result.err_str);
	}
	BOOST_REQUIRE_MESSAGE(result.ids, "Expected non-nil ids with nil err_str");
	string joinedIdsString = algorithm::join(*(result.ids), ", ");
	cout << "mockedPlainStringDoc_allIds: ids: " << joinedIdsString << endl;
	BOOST_REQUIRE_MESSAGE((*(result.ids)).size() > 0, "Expected to find at least one id."); // from __insert
}
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_allDocuments)
{
	errOr_contentStrings result = allDocuments(
		*new_documentsPath(),
		mockedPlainStringDocs__CollectionName
	);
	if (result.err_str) {
		std::cout << *result.err_str << endl;
		BOOST_REQUIRE(!result.err_str);
	}
	BOOST_REQUIRE_MESSAGE(result.strings, "Expected result.strings");
	BOOST_REQUIRE_MESSAGE((*(result.strings)).size() > 0, "Expected result.string.count() > 0"); // from __insert
	string joinedValuesString = algorithm::join(*(result.strings), ", ");
	cout << "mockedPlainStringDoc_allDocuments: document content strings: " << joinedValuesString << endl;
}
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_removeAllDocuments)
{
	string parentPath = std::move(*new_documentsPath()); // is this move right to avoid a copy?
	//
	errOr_documentIds result_1 = idsOfAllDocuments(parentPath, mockedPlainStringDocs__CollectionName);
	if (result_1.err_str) {
		std::cout << *result_1.err_str << endl;
		BOOST_REQUIRE(!result_1.err_str);
	}
	BOOST_REQUIRE(result_1.ids);
	BOOST_REQUIRE((*(result_1.ids)).size() > 0); // from __insert
	//
	errOr_numRemoved result_2 = removeAllDocuments(parentPath, mockedPlainStringDocs__CollectionName);
	if (result_2.err_str) {
		std::cout << *result_2.err_str << endl;
		BOOST_REQUIRE(!result_2.err_str);
	}
	BOOST_REQUIRE((*result_2.numRemoved) == (*(result_1.ids)).size()); // from __insert
	cout << "mockedPlainStringDoc_removeAllDocuments: removed " << (*result_2.numRemoved) << " document(s)" << endl;
}
//
// Test suites - PersistableObject
#include "../src/Passwords/PasswordController.hpp"
#include "../src/Persistence/PersistableObject.hpp"
const CollectionName mockedSavedObjects__CollectionName = "MockedSavedObjects";
const string mockedSavedObjects__addtlVal_ = "Some extra test data";
class MockedPasswordProvider: public Passwords::PasswordProvider
{
public:
	boost::optional<Passwords::Password> getPassword() const
	{
	   return std::string("123123");
	}
};
class MockedSavedObject: public Persistable::Object
{
public:
	boost::optional<std::string> addtlVal = none; // set this after init to avoid test fail
	//
	MockedSavedObject(
		std::shared_ptr<std::string> documentsPath,
		std::shared_ptr<const Passwords::PasswordProvider> passwordProvider
	): Persistable::Object(
		std::move(documentsPath),
		std::move(passwordProvider)
	) {
	}
	MockedSavedObject(
		std::shared_ptr<std::string> documentsPath,
		std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
		const property_tree::ptree &plaintextData
	): Persistable::Object(
		std::move(documentsPath),
		std::move(passwordProvider),
		plaintextData
	) {
		BOOST_REQUIRE(this->_id != boost::none);
		BOOST_REQUIRE(this->insertedAt_sSinceEpoch != boost::none);
		//
		this->addtlVal = plaintextData.get<std::string>("addtlVal");
		BOOST_REQUIRE(this->addtlVal != boost::none);
	}
	virtual property_tree::ptree new_dictRepresentation() const
	{
		property_tree::ptree dict = Persistable::Object::new_dictRepresentation();
		if (this->addtlVal) {
			dict.put("addtlVal", *(this->addtlVal));
		}
		//
		return dict;
	}
	CollectionName collectionName() const { return mockedSavedObjects__CollectionName; }
};
//
//
const std::string _persistableObject_encryptedBase64String = "AwGNOxB4XmBTWlinIWg0sCPrfjwKGxwtshvmN4jMi3YBXYp6SAzx4WWMe0gNTV6vBT4iROJXMwpKyW6n+uzc2/nTrOPaLh0Dk8obNjeN1S2Rz7fMuiil2JHerFj2YM2vYpOPsaUg92mUojN8s1wNcfkpWtCF7oFD/VCKV3QYfRnFJyvmqD4LjFXg+ENB5um1bFdrk+36LbV9TGhVqjttYUMQtSUWOKtR+VqpuoEuRAVK4zxVSfzjAF3yHi85UaJLEi8=";
const std::string _persistableObject_decryptedString = "this is just a string that we'll use for checking whether a given password can unlock an encrypted version of this very message";
BOOST_AUTO_TEST_CASE(persistableObject_decryptBase64)
{
	const Passwords::Password password = "123123";
	const std::string decryptedString = Persistable::new_plaintextStringFrom(_persistableObject_encryptedBase64String, password);
	
	cout << "decryptedString: " << decryptedString << endl;	
	BOOST_REQUIRE(decryptedString == _persistableObject_decryptedString);
}
//
BOOST_AUTO_TEST_CASE(mockedSavedObjects_insertNew)
{
	MockedPasswordProvider passwordProvider_itself;
	auto passwordProvider = std::make_shared<MockedPasswordProvider>(passwordProvider_itself);
	MockedSavedObject obj{
		new_documentsPath(),
		passwordProvider
	};
	obj.addtlVal = mockedSavedObjects__addtlVal_;
	//
	boost::optional<std::string> err_str = obj.saveToDisk();
	BOOST_REQUIRE(err_str == none);
	BOOST_REQUIRE(obj._id != none);
}
BOOST_AUTO_TEST_CASE(mockedSavedObjects_loadExisting)
{
	std::shared_ptr<string> documentsPath_ptr = new_documentsPath();
	errOr_documentIds result = document_persister::idsOfAllDocuments(
		*documentsPath_ptr,
		mockedSavedObjects__CollectionName
	);
	if (result.err_str) {
		std::cout << *result.err_str << endl;
		BOOST_REQUIRE(!result.err_str);
	}
	BOOST_REQUIRE(result.ids != none);
	BOOST_REQUIRE((*(result.ids)).size() > 0); // from __insertNew
	//
	errOr_contentStrings load__result = document_persister::documentsWith(
		*documentsPath_ptr,
		mockedSavedObjects__CollectionName,
		(*(result.ids))
	);
	if (load__result.err_str) {
		std::cout << *load__result.err_str << endl;
		BOOST_REQUIRE(!load__result.err_str);
	}
	BOOST_REQUIRE(load__result.strings != none);
	BOOST_REQUIRE((*(load__result.strings)).size() > 0);
	MockedPasswordProvider passwordProvider_itself;
	auto passwordProvider = std::make_shared<MockedPasswordProvider>(passwordProvider_itself);
	for (auto it = (*(load__result.strings)).begin(); it != (*(load__result.strings)).end(); it++) {
		string plaintext_documentContentString = Persistable::new_plaintextStringFrom(
			*it,
			*(passwordProvider_itself.getPassword())
		);
		property_tree::ptree plaintext_documentJSON = Persistable::new_plaintextDocumentDictFromJSONString(
			plaintext_documentContentString
		);
		MockedSavedObject listedObjectInstance{
			documentsPath_ptr,
			passwordProvider,
			plaintext_documentJSON
		};
		BOOST_REQUIRE(listedObjectInstance._id != none);
		BOOST_REQUIRE(listedObjectInstance.insertedAt_sSinceEpoch != none);
		BOOST_REQUIRE(listedObjectInstance.addtlVal == mockedSavedObjects__addtlVal_);
	}
}
BOOST_AUTO_TEST_CASE(mockedSavedObjects_deleteExisting)
{
	std::shared_ptr<string> documentsPath_ptr = new_documentsPath();
	errOr_documentIds result = document_persister::idsOfAllDocuments(
		*documentsPath_ptr, mockedSavedObjects__CollectionName);
	BOOST_REQUIRE(result.err_str == none);
	BOOST_REQUIRE(result.ids != none);
	BOOST_REQUIRE((*(result.ids)).size() > 0); // from __insertNew
	//
	errOr_contentStrings load__result = document_persister::documentsWith(
		*documentsPath_ptr,
		mockedSavedObjects__CollectionName,
		*(result.ids)
	);
	BOOST_REQUIRE(load__result.err_str == none);
	BOOST_REQUIRE(load__result.strings != none);
	BOOST_REQUIRE((*(load__result.strings)).size() > 0);
	MockedPasswordProvider passwordProvider_itself;
	auto passwordProvider = std::make_shared<MockedPasswordProvider>(passwordProvider_itself);
	for (auto it = (*(load__result.strings)).begin(); it != (*(load__result.strings)).end(); it++) {
		string plaintext_documentContentString = Persistable::new_plaintextStringFrom(
			*it,
			*(passwordProvider_itself.getPassword())
		);
		property_tree::ptree plaintext_documentJSON = Persistable::new_plaintextDocumentDictFromJSONString(
			plaintext_documentContentString
		);
		MockedSavedObject listedObjectInstance{
			documentsPath_ptr,
			passwordProvider,
			plaintext_documentJSON
		};
		BOOST_REQUIRE(listedObjectInstance._id != none);
		BOOST_REQUIRE(listedObjectInstance.insertedAt_sSinceEpoch != none);
		//
		boost::optional<string> delete__errStr = listedObjectInstance.deleteFromDisk();
		BOOST_REQUIRE(delete__errStr == none);
	}
}

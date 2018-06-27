//
//  test_document_persister.cpp
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

//Link to Boost
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "MockedPlainStringDocs"
// IMPORTANT: include unit_test this last
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/filesystem.hpp>
using namespace std;
using namespace boost;
//
#include "../src/Persistence/document_persister.hpp"
using namespace document_persister;
//
const document_persister::CollectionName mockedPlainStringDocs__CollectionName = "TestDocuments";
static inline const std::string documentsPath()
{
	std::string thisfile_path = std::string(__FILE__);
	std::string tests_dir = thisfile_path.substr(0, thisfile_path.find_last_of("\\/"));
	std::string srcroot_dir = tests_dir.substr(0, tests_dir.find_last_of("\\/"));
	boost::filesystem::path dir(srcroot_dir);
	boost::filesystem::path file("build");
    boost::filesystem::path full_path = dir / file;
	//
	return full_path.string();
}
//
// Tests
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_insert)
{
	const DocumentId id = new_documentId();
	std::stringstream jsonSS;
	jsonSS << "{\"a\":2,\"id\":\"" << id << "\"}";
	const std::string jsonString = jsonSS.str();
	//
	boost::optional<std::string> err_str = document_persister::write(
		documentsPath(),
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
		documentsPath(),
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
		documentsPath(),
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
	string parentPath = documentsPath();
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

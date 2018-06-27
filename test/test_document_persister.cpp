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
//
#include "../src/Persistence/document_persister.hpp"
//
const document_persister::CollectionName mockedPlainStringDocs__CollectionName = "TestDocuments";
const std::string documentsPath = "./build"; // TODO?
//
// Tests
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_insert)
{
	const document_persister::DocumentId id = document_persister::new_documentId();
	std::stringstream jsonSS;
	jsonSS << "{\"a\":2,\"id\":\"" << id << "\"}";
	const std::string jsonString = jsonSS.str();
	//
	boost::optional<std::string> err_str = document_persister::write(
		documentsPath,
		jsonString,
		id,
		mockedPlainStringDocs__CollectionName
	);
	BOOST_CHECK(!err_str);
	std::cout << "Inserted " << id << std::endl;
}
//BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_allIds)
//{
//	BOOST_CHECK('a' == 'a');
//	document_persister::errOr_documentIds result  = document_persister::idsOfAllDocuments(
//		documentsPath,
//		mockedPlainStringDocs__CollectionName
//	);
//	boost::optional<std::string> err_str = result.get<0>();
//	BOOST_CHECK_MESSAGE(!err_str, *err_str);
//	boost::optional<std::vector<document_persister::DocumentId>> ids = result.get<1>();
//	BOOST_CHECK_MESSAGE(ids, "Expected non-nil ids with nil err_str");
//	BOOST_CHECK_MESSAGE((*ids).size() > 0, "Expected to find at least one id."); // from __insert
//	std::stringstream joinedIdsString_ss;
//	std::copy((*ids).begin(), (*ids).end(), std::ostream_iterator<std::string>(joinedIdsString_ss, ", "));
//	std::cout << "ids: " << joinedIdsString_ss.str() << std::endl;
//}
/*
	@Test fun mockedPlainStringDoc__allDocuments()
	{
		val applicationContext = MainApplication.instance.applicationContext
		val documentPersister = DocumentPersister(applicationContext = applicationContext)
		val (err_str, strings) = documentPersister.AllDocuments(mockedPlainStringDocs__CollectionName)
		assertEquals(null, err_str)
		assertTrue(strings != null)
		assertTrue(strings!!.count() > 0) // from __insert
		Log.d(mockedPlainStringDocs__LogTag, "strings: " + strings)
	}
	@Test fun mockedPlainStringDoc__removeAllDocuments()
	{
		val applicationContext = MainApplication.instance.applicationContext
		val documentPersister = DocumentPersister(applicationContext = applicationContext)
		val (fetch__err_str, ids) = documentPersister.IdsOfAllDocuments(mockedPlainStringDocs__CollectionName)
		assertEquals(null, fetch__err_str)
		assertTrue(ids != null)
		assertTrue(ids!!.count() > 0) // from __insert
		//
		val (remove__err_str, numRemoved) = documentPersister.RemoveAllDocuments(mockedPlainStringDocs__CollectionName)
		assertEquals(null, remove__err_str)
		assertTrue(numRemoved!! == ids!!.count()) // from __insert
	}

*/

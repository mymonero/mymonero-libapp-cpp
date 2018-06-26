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
const std::string documentsPath = "/Users/paulshapiro/Documents/Repos/mymonero-libapp-cpp/build"; // TODO?
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
BOOST_AUTO_TEST_CASE(mockedPlainStringDoc_allIds)
{
	document_persister::errOr_documentIds result  = document_persister::idsOfAllDocuments(
		documentsPath,
		mockedPlainStringDocs__CollectionName
	);
	boost::optional<std::string> err_str = result.get<0>();
	BOOST_CHECK_MESSAGE(!err_str, *err_str);
	boost::optional<std::vector<document_persister::DocumentId>> ids = result.get<1>();
	BOOST_CHECK_MESSAGE(ids, "Expected non-nil ids with nil err_str");
	BOOST_CHECK_MESSAGE((*ids).size() > 0, "Expected to find at least one id."); // from __insert
	std::stringstream joinedIdsString_ss;
	std::copy((*ids).begin(), (*ids).end(), std::ostream_iterator<std::string>(joinedIdsString_ss, ", "));
	std::cout << "ids: " << joinedIdsString_ss.str() << std::endl;
}
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
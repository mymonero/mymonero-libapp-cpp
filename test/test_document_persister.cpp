//Link to Boost
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE "MockedPlainStringDocs"
// IMPORTANT: include unit_test this last
#include <boost/test/unit_test.hpp>
//
#include "../src/Persistence/document_persister.hpp"
//
const char *CollectionName = "TestDocuments";
//
// Tests
BOOST_AUTO_TEST_CASE(constructors)
{
	BOOST_CHECK('a' == 'a');
}
/*
	@Test fun mockedPlainStringDoc__insert()
	{
		val id = DocumentFileDescription.new_documentId()
		val applicationContext = MainApplication.instance.applicationContext
		val documentPersister = DocumentPersister(applicationContext = applicationContext)
		val err_str = documentPersister.Write(
			documentFileWithString = "\"a\":2,\"id\":\"${id}\"",
			id = id,
			collectionName = mockedPlainStringDocs__CollectionName
		)
		assertEquals(null, err_str)
		Log.d(mockedPlainStringDocs__LogTag, "Inserted " + id)
	}
	@Test fun mockedPlainStringDoc__allIds()
	{
		val applicationContext = MainApplication.instance.applicationContext
		val documentPersister = DocumentPersister(applicationContext = applicationContext)
		val (err_str, ids) = documentPersister.IdsOfAllDocuments(mockedPlainStringDocs__CollectionName)
		assertEquals(null, err_str)
		assertTrue(ids != null)
		assertTrue(ids!!.count() > 0) // from __insert
		Log.d(mockedPlainStringDocs__LogTag, "ids: " + ids)
	}
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
//
//  document_persister.cpp
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
#include <iostream>
#include <fstream>
#include <vector>
#include <boost/atomic.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost;

namespace document_persister
{
   typedef string DocumentId;
   typedef string CollectionName;
   typedef string DocumentContentString;
   typedef property_tree::ptree DocumentJSON;
   typedef property_tree::ptree MutableDocumentJSON;
}
namespace document_persister
{
   struct  DocumentFileDescription
   {
       CollectionName inCollectionName;
       DocumentId documentId;
   };
   const std::string fileKeyComponentDelimiterString = "__";
   const std::string filenameExt = std::string(".MMDBDoc"); // just trying to pick something fairly unique, and short
   //
   static inline DocumentId new_documentId()
   {
       boost::uuids::uuid uuid = (boost::uuids::random_generator())();
       //
       return boost::uuids::to_string(uuid);
   }
   static inline std::stringstream new_fileKey_ss(const DocumentFileDescription &description)
   {
       std::stringstream ss;
       ss << description.inCollectionName << fileKeyComponentDelimiterString << description.documentId;
       //
       return ss;
   }
   static inline std::string new_fileKey(const DocumentFileDescription &description)
   {
       return new_fileKey_ss(description).str();
   }
   static inline std::string new_filename(const DocumentFileDescription &description)
   {
       return (new_fileKey_ss(description) << filenameExt).str();
   }
}
namespace document_persister
{
	namespace fs = boost::filesystem;
	//
	struct errOr_documentFileDescriptions
	{
		boost::optional<std::string> err_str;
		boost::optional<std::vector<DocumentFileDescription>> descriptions;
	};
	struct errOr_documentIds
	{
		boost::optional<std::string> err_str;
		boost::optional<std::vector<DocumentId>> ids;
	};
	struct errOr_contentString
	{
		boost::optional<std::string> err_str;
		boost::optional<std::string> string;
	};
	struct errOr_contentStrings
	{
		boost::optional<std::string> err_str;
		boost::optional<std::vector<std::string>> strings;
	};
	struct errOr_numRemoved
	{
		boost::optional<std::string> err_str;
		boost::optional<uint32_t> numRemoved;
	};
	//
	// Accessors - Internal
	static inline const errOr_documentFileDescriptions __read_documentFileDescriptions(
		const string &documentsPath,
		const CollectionName &collectionName
	) {
		if (!fs::exists(documentsPath)) {
			return {std::string("documentsPath doesn't exist"), boost::none};
		}
		if (!fs::is_directory(documentsPath)) {
			return {std::string("documentsPath isn't a directory"), boost::none};
		}
	    vector<DocumentFileDescription> fileDescriptions;
		fs::directory_iterator end_itr; // "default construction yields past-the-end"
		for (fs::directory_iterator itr(documentsPath); itr != end_itr; itr++) {
			try {
				// filtering to what should be app document_persister files..
				if (is_directory(itr->status())) {
					continue;
				}
				if (itr->path().extension()/* has '.' */.string() != filenameExt) {
					continue;
				}
				string fileKey = itr->path().stem()/* assumption */.string();
				vector<string> fileKey_components;
				split(fileKey_components, fileKey, is_any_of(fileKeyComponentDelimiterString), token_compress_on);
				if (fileKey_components.size() != 2) {
					return { string("Unrecognized filename format in db data directory."), none };
				}
				string fileKey_collectionName = fileKey_components[0]; // CollectionName
				if (fileKey_collectionName != collectionName) {
					continue;
				}
				string fileKey_id  = fileKey_components[1]; // DocumentId
				fileDescriptions.push_back({
					fileKey_collectionName,
					fileKey_id
				}); // ought to be a JSON doc file
			} catch (const std::exception &e) {
				return {string(e.what()), none};
			}
		}
		return {none, fileDescriptions};
	}
	static inline const errOr_contentString __read_existentDocumentContentString(
		const std::string &documentsPath,
		const DocumentFileDescription &fileDescription
	) {
		if (!boost::filesystem::exists(documentsPath)) {
			return {std::string("documentsPath doesn't exist"), boost::none};
		}
		if (!boost::filesystem::is_directory(documentsPath)) {
			return {std::string("documentsPath isn't a directory"), boost::none};
		}
		std::stringstream path_ss;
		path_ss << documentsPath << "/" << new_filename(fileDescription);
		std::ifstream ifs{path_ss.str()};
		if (!ifs.is_open()) { // TODO: return err (as code is best)
			return {std::string("Couldn't open file"), boost::none};
		}
		std::stringstream contents_ss;
		std::string line;
		while (std::getline(ifs, line)) {
			contents_ss << line << std::endl;
		}
		return {boost::none, contents_ss.str() };
	}
	static inline const errOr_contentStrings _read_existentDocumentContentStrings(
		const std::string &documentsPath,
		const std::vector<DocumentFileDescription> &documentFileDescriptions
	) {
		std::vector<std::string> documentContentStrings;
		if (documentFileDescriptions.size() == 0) {
			return { boost::none, documentContentStrings };
		}
		for (auto it = documentFileDescriptions.begin(); it != documentFileDescriptions.end(); it++) {
			errOr_contentString result = __read_existentDocumentContentString(documentsPath, *it);
			if (result.err_str) {
				return { std::move(*result.err_str), boost::none };
			}
			documentContentStrings.push_back(
				std::move(*result.string)
			);
		}
		return { boost::none, documentContentStrings };
	}
	//
	// Imperatives - Internal
	static inline boost::optional<std::string> _write_fileDescriptionDocumentString(
		const std::string &documentsPath,
		const DocumentFileDescription &fileDescription,
		const std::string &contentString
	) {
		boost::filesystem::path dir(documentsPath);
		boost::filesystem::path file(new_filename(fileDescription));
	    boost::filesystem::path full_path = dir / file;
		std::ofstream ofs{full_path.string()};
		if(!ofs.is_open()) {
			return std::string("Couldn't open file for writing."); // TODO: return error code instead
		}
		ofs << contentString;
		ofs.close();
		//
		return boost::none;
	}
}
namespace document_persister {
	//
	// Accessors
	static inline const errOr_documentIds idsOfAllDocuments(
		const std::string &documentsPath,
		const CollectionName &collectionName
	) {
		errOr_documentFileDescriptions result = __read_documentFileDescriptions(documentsPath, collectionName);
		if (result.err_str) {
			return { std::move(*result.err_str), boost::none };
		}
		std::vector<DocumentId> ids;
		for (auto it = (*result.descriptions).begin(); it != (*result.descriptions).end(); it++) {
			ids.push_back((*it).documentId);
		}
		//
		return { boost::none, ids };
	}
	//
	// Imperatives - Interface
	static inline const boost::optional<std::string> write( // returning optl err str
		const std::string &documentsPath,
		const std::string &contentString, // if you're using this for Documents, be sure to set field _id to id within your fileData
		const DocumentId &id, // consumer must supply the document ID since we can't make assumptions about fileData
		const CollectionName &collectionName
	) {
		DocumentFileDescription fileDescription{
			collectionName,
			id
		};
		return document_persister::_write_fileDescriptionDocumentString(
			documentsPath,
			fileDescription,
			contentString
		);
	}
}

//
//  document_persister.hpp
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

#ifndef document_persister_hpp
#define document_persister_hpp
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <boost/atomic.hpp>
#include <boost/optional/optional.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string.hpp>
//
#include "rapidjson_defines.hpp" // must be included before rapidjson include
#include "rapidjson/document.h"
using namespace std;
using namespace boost;
using namespace rapidjson;
//
//
namespace document_persister
{
   typedef string DocumentId;
   typedef string CollectionName;
   typedef string DocumentContentString;
   typedef Document DocumentJSON;
}
namespace document_persister
{
   struct  DocumentFileDescription
   {
       CollectionName inCollectionName;
       DocumentId documentId;
   };
   const string fileKeyComponentDelimiterString = "__";
   const string filenameExt = string(".mmdbdoc_v1"); // just trying to pick something fairly unique, and short
   //
   static inline DocumentId new_documentId()
   {
       boost::uuids::uuid uuid = (boost::uuids::random_generator())();
       //
       return boost::uuids::to_string(uuid);
   }
   static inline stringstream new_fileKey_ss(const DocumentFileDescription &description)
   {
       stringstream ss;
       ss << description.inCollectionName << fileKeyComponentDelimiterString << description.documentId;
       //
       return ss;
   }
   static inline string new_fileKey(const DocumentFileDescription &description)
   {
       return new_fileKey_ss(description).str();
   }
   static inline string new_filename(const DocumentFileDescription &description)
   {
	   auto ss = new_fileKey_ss(description);
	   ss << filenameExt;
	   //
	   return ss.str();
   }
}
namespace document_persister
{
	//
	// Shared - Parsing - Values
	static inline optional<string> none_or_string_from(
		const DocumentJSON &json,
		const string &key
	) {
		Value::ConstMemberIterator itr = json.FindMember(key);
		if (itr != json.MemberEnd()) {
			return string(itr->value.GetString(), itr->value.GetStringLength());
		}
		return none;
	}
	static inline optional<double> none_or_double_from(
		const DocumentJSON &json,
		const string &key
	) {
		Value::ConstMemberIterator itr = json.FindMember(key);
		if (itr != json.MemberEnd()) {
			if (itr->value.IsString()) {
				return stod(itr->value.GetString()); // this may throw an exception - allowing it to bubble up here
			} else if (itr->value.IsNumber()) {
				if (itr->value.IsInt()) {
					return (double)itr->value.GetInt();
				}
				assert(itr->value.IsDouble());
				return itr->value.GetDouble();
			}
		}
		return none;
	}
	static inline optional<bool> none_or_bool_from(
		const document_persister::DocumentJSON &json,
		const string &key
	) {
		Value::ConstMemberIterator itr = json.FindMember(key);
		if (itr != json.MemberEnd()) {
			if (itr->value.IsString()) {
				string str = string(itr->value.GetString(), itr->value.GetStringLength());
				if (str == "true" || str == "1") {
					return true;
				} else if (str == "false" || str == "0") {
					return false;
				} else {
					BOOST_THROW_EXCEPTION(logic_error("Unable to parse bool string"));
					return none;
				}
			} else if (itr->value.IsBool()) {
				return itr->value.GetBool();
			}
		}
		return none;
	}
}
namespace document_persister
{
	namespace fs = boost::filesystem;
	//
	struct errOr_documentFileDescriptions
	{
		boost::optional<string> err_str;
		boost::optional<vector<DocumentFileDescription>> descriptions;
	};
	struct errOr_documentIds
	{
		boost::optional<string> err_str;
		boost::optional<vector<DocumentId>> ids;
	};
	struct errOr_contentString
	{
		boost::optional<string> err_str;
		boost::optional<string> content_string;
	};
	struct errOr_contentStrings
	{
		boost::optional<string> err_str;
		boost::optional<vector<string>> content_strings;
	};
	struct errOr_numRemoved
	{
		boost::optional<string> err_str;
		boost::optional<uint32_t> numRemoved;
	};
	//
	// Accessors - Internal
	static inline const errOr_documentFileDescriptions _read_documentFileDescriptions(
		const string &documentsPath,
		const CollectionName &collectionName
	) {
		if (!fs::exists(documentsPath)) {
			return {string("documentsPath doesn't exist"), boost::none};
		}
		if (!fs::is_directory(documentsPath)) {
			return {string("documentsPath isn't a directory"), boost::none};
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
		const string &documentsPath,
		const DocumentFileDescription &fileDescription
	) {
		if (!boost::filesystem::exists(documentsPath)) {
			return {string("documentsPath doesn't exist"), boost::none};
		}
		if (!boost::filesystem::is_directory(documentsPath)) {
			return {string("documentsPath isn't a directory"), boost::none};
		}
		stringstream path_ss;
		path_ss << documentsPath << "/" << new_filename(fileDescription);
		ifstream ifs{path_ss.str()};
		if (!ifs.is_open()) { // TODO: return err (as code is best)
			return {string("Couldn't open file"), boost::none};
		}
		stringstream contents_ss;
		string line;
		bool isFirstLine = true;
		while (getline(ifs, line)) {
			if (isFirstLine) {
				isFirstLine = false;
			} else {
				contents_ss << endl;
			}
			contents_ss << line;
		}
		return {boost::none, contents_ss.str() };
	}
	static inline const errOr_contentStrings _read_existentDocumentContentStrings(
		const string &documentsPath,
		const vector<DocumentFileDescription> &documentFileDescriptions
	) {
		vector<string> documentContentStrings;
		if (documentFileDescriptions.size() == 0) {
			return { boost::none, documentContentStrings };
		}
		for (auto it = documentFileDescriptions.begin(); it != documentFileDescriptions.end(); it++) {
			errOr_contentString result = __read_existentDocumentContentString(documentsPath, *it);
			if (result.err_str) {
				return { std::move(*result.err_str), boost::none };
			}
			documentContentStrings.push_back(std::move(*result.content_string));
		}
		return { boost::none, documentContentStrings };
	}
	//
	// Imperatives - Internal
	static inline boost::optional<string> _write_fileDescriptionDocumentString(
		const string &documentsPath,
		const DocumentFileDescription &fileDescription,
		const string &contentString
	) {
		fs::path dir(documentsPath);
		fs::path file(new_filename(fileDescription));
	    fs::path full_path = dir / file;
		ofstream ofs{full_path.string()};
		if(!ofs.is_open()) {
			stringstream err_ss;
			err_ss << "Couldn't open file for writing at " << full_path.string();
			return err_ss.str(); // TODO: return error code instead
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
		const string &documentsPath,
		const CollectionName &collectionName
	) {
		errOr_documentFileDescriptions result = _read_documentFileDescriptions(documentsPath, collectionName);
		if (result.err_str) {
			return { std::move(*result.err_str), none };
		}
		if (!result.descriptions) {
			BOOST_THROW_EXCEPTION(runtime_error("nil fileDescriptions"));
		}
		vector<DocumentId> ids;
		for (auto it = (*result.descriptions).begin(); it != (*result.descriptions).end(); it++) {
			ids.push_back((*it).documentId);
		}
		return { boost::none, ids };
	}
	static inline const errOr_contentStrings documentsWith(
		const string &documentsPath,
		const CollectionName &collectionName,
		const vector<DocumentId> ids
	) {
		vector<DocumentFileDescription> descriptions;
		for (auto it = ids.begin(); it != ids.end(); it++) {
			descriptions.push_back({collectionName, *it/*id*/});
		}
		return _read_existentDocumentContentStrings(documentsPath, descriptions);
	}
	static inline const errOr_contentStrings allDocuments(
		const string &documentsPath,
		const CollectionName &collectionName
	) {
		errOr_documentFileDescriptions result = _read_documentFileDescriptions(documentsPath, collectionName);
		if (result.err_str) {
			return { std::move(*result.err_str), none };
		}
		if (!result.descriptions) {
			BOOST_THROW_EXCEPTION(runtime_error("nil fileDescriptions"));
		}
		return _read_existentDocumentContentStrings(
			documentsPath,
			*(result.descriptions)
		);
	}
	//
	// Imperatives - Interface
	static inline const boost::optional<string> write( // returning optl err str
		const string &documentsPath,
		const string &contentString, // if you're using this for Documents, be sure to set field _id to id within your fileData
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
	//
	static inline errOr_numRemoved removeDocuments(
		const string &documentsPath,
		const CollectionName &collectionName,
		const vector<DocumentId> &ids
	) {
		uint numRemoved = 0;
		fs::path dir(documentsPath);
		for (auto it = ids.begin(); it != ids.end(); it++) {
			DocumentFileDescription fileDescription = {collectionName, *it/*id*/};
			fs::path file(new_filename(fileDescription));
			fs::path full_path = dir / file;
			bool deleted = fs::remove(full_path);
			if (deleted) {
				numRemoved += 1;
			}
		}
		if (numRemoved != ids.size()) {
			BOOST_THROW_EXCEPTION(runtime_error("Expected numRemoved == ids.size()"));
		}
		return {none, numRemoved};
	}
	static inline errOr_numRemoved removeAllDocuments(
		const string &documentsPath,
		const CollectionName &collectionName
	) {
		errOr_documentIds result = idsOfAllDocuments(documentsPath, collectionName);
		if (result.err_str) {
			return { std::move(*result.err_str), none };
		}
		return removeDocuments(documentsPath, collectionName, *result.ids);
	}
}

#endif /* document_persister_hpp */

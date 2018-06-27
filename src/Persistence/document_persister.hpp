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
#ifndef __document_persister_hpp
#define __document_persister_hpp

#include <iostream>
#include <fstream>
#include <boost/atomic.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/optional/optional.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/tuple/tuple.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

namespace document_persister
{
   typedef std::string DocumentId;
   typedef std::string CollectionName;
   typedef std::string DocumentContentString;
   typedef boost::property_tree::ptree DocumentJSON;
   typedef boost::property_tree::ptree MutableDocumentJSON;
}
namespace document_persister
{
   struct  DocumentFileDescription
   {
       CollectionName inCollectionName;
       DocumentId documentId;
   };
   const std::string fileKeyComponentDelimiterString = "__";
   const std::string filenameExt = std::string("MMDBDoc"); // just trying to pick something fairly unique, and short
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
	typedef boost::tuple<
		boost::optional<std::string>,
		boost::optional<std::vector<DocumentFileDescription>>
	> errOr_documentFileDescriptions;
	typedef boost::tuple<
		boost::optional<std::string>,
		boost::optional<std::vector<DocumentId>>
	> errOr_documentIds;
	typedef boost::tuple<
		boost::optional<std::string>,
		boost::optional<std::string>
	> errOr_contentString;
	typedef boost::tuple<
		boost::optional<std::string>,
		boost::optional<std::vector<std::string>>
	> errOr_contentStrings;
	typedef boost::tuple<
		boost::optional<std::string>,
		boost::optional<uint32_t>
	> errOr_numRemoved;
	//
	// Accessors - Internal
	static inline const errOr_documentFileDescriptions __read_documentFileDescriptions(
		const std::string &documentsPath,
		const CollectionName *collectionName
	) {
		if (!boost::filesystem::exists(documentsPath)) {
			return errOr_documentFileDescriptions(std::string("documentsPath doesn't exist"), boost::none);
		}
		if (!boost::filesystem::is_directory(documentsPath)) {
			return errOr_documentFileDescriptions(std::string("documentsPath isn't a directory"), boost::none);
		}
	    std::vector<DocumentFileDescription> fileDescriptions;
		//
		boost::filesystem::directory_iterator end_itr; // "default construction yields past-the-end"
		for (boost::filesystem::directory_iterator itr(documentsPath); itr != end_itr; itr++) {
			// filtering to what should be app document_persister files..
			if (is_directory(itr->status())) {
				continue;
			}
			if (itr->path().extension().string() != filenameExt) {
				continue;
			}
			std::string fileKey = itr->path().stem().string(); // assumption
//			val fileKey_components = fileKey.split(DocumentFileDescription.fileKeyComponentDelimiterString)
//			if (fileKey_components.count() != 2) {
//				return ErrorOr_DocumentFileDescriptions(
//						"Unrecognized filename format in db data directory.",
//						null
//				)
//			}
//			val fileKey_collectionName = fileKey_components[0] // CollectionName
//			if (fileKey_collectionName != collectionName) {
//				continue
//			}
//			val fileKey_id  = fileKey_components[1] // DocumentId
//			auto fileDescription = DocumentFileDescription{
//				fileKey_collectionName,
//				fileKey_id
//			};
//			fileDescriptions.push_back(fileDescription); // ought to be a JSON doc file
		}
		return errOr_documentFileDescriptions(boost::none, fileDescriptions);
	}
	static inline const errOr_contentString __read_existentDocumentContentString(
		const std::string &documentsPath,
		const DocumentFileDescription &fileDescription
	) {
		if (!boost::filesystem::exists(documentsPath)) {
			return errOr_documentFileDescriptions(std::string("documentsPath doesn't exist"), boost::none);
		}
		if (!boost::filesystem::is_directory(documentsPath)) {
			return errOr_documentFileDescriptions(std::string("documentsPath isn't a directory"), boost::none);
		}
		std::stringstream path_ss;
		path_ss << documentsPath << "/" << new_filename(fileDescription);
		std::ifstream ifs{path_ss.str()};
		if (!ifs.is_open()) { // TODO: return err (as code is best)
			return errOr_contentString(std::string("Couldn't open file"), boost::none);
		}
		std::stringstream contents_ss;
		std::string line;
		while (std::getline(ifs, line)) {
			contents_ss << line << std::endl;
		}
		return errOr_contentString(
			boost::none,
			contents_ss.str()
		);
	}
	static inline const errOr_contentStrings _read_existentDocumentContentStrings(
		const std::string &documentsPath,
		const std::vector<DocumentFileDescription> &documentFileDescriptions
	) {
		std::vector<std::string> documentContentStrings;
		if (documentFileDescriptions.size() == 0) {
			return errOr_contentStrings(boost::none, documentContentStrings);
		}
		for (auto it = documentFileDescriptions.begin(); it != documentFileDescriptions.end(); it++) {
			errOr_contentString result = __read_existentDocumentContentString(documentsPath, *it);
			boost::optional<std::string> err_str = result.get<0>();
			if (err_str) {
				return errOr_contentStrings(std::move(err_str), boost::none);
			}
			documentContentStrings.push_back(
				std::move(*(result.get<1>()))
			);
		}
		return errOr_contentStrings(boost::none, documentContentStrings);
	}
	//
	// Imperatives - Internal
	static inline boost::optional<std::string> _write_fileDescriptionDocumentString(
		const std::string &documentsPath,
		const DocumentFileDescription &fileDescription,
		const std::string &contentString
	) {
		std::stringstream path_ss;
		path_ss << documentsPath << "/" << new_filename(fileDescription);
		std::ofstream ofs{path_ss.str()};
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
	static inline errOr_documentIds idsOfAllDocuments(
		const std::string &documentsPath,
		const CollectionName &collectionName
	) {
//		val (err_str, fileDescriptions) = _read_documentFileDescriptions(collectionName)
//		if (err_str != null) {
//			return errOr_documentIds(err_str, boost::none);
		return errOr_documentIds(std::string("TODO"), boost::none);
//		}
//		if (fileDescriptions == null) {
//			throw AssertionError("nil fileDescriptions")
//		}
//		var ids = mutableListOf<DocumentId>()
//		val unwrapped_fileDescriptions = fileDescriptions as List<DocumentFileDescription>
//		for (fileDescription in unwrapped_fileDescriptions) {
//			ids.add(fileDescription.documentId)
//		}
//		//
//		return ErrorOr_DocumentIds(null, ids)
	}
	//
	// Imperatives - Interface
	static inline boost::optional<std::string> write( // returning optl err str
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

#endif 
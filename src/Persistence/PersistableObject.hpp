//
//  PersistableObject.hpp
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

#ifndef PersistableObject_hpp
#define PersistableObject_hpp

#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
//#include <mutex> // TODO: use mutex? (see _property_mutex below)
#include <boost/optional/optional.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/throw_exception.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/signals2.hpp>
using namespace std;
using namespace boost;
//
#include "../RNCryptor-C/rncryptor_c.h"
#include "../RNCryptor-C/mutils.h"
#include "../base64/base64.hpp"
//
#include "rapidjson/writer.h"
#include "document_persister.hpp"
using namespace document_persister;
#include "../Passwords/PasswordController.hpp"
//
namespace Persistable
{
	class Object
	{
		public:
			//
			// Lifecycle - Init
			explicit Object(
				std::shared_ptr<const string> documentsPath,
				std::shared_ptr<const Passwords::PasswordProvider> passwordProvider
			): documentsPath(std::move(documentsPath)),
				passwordProvider(std::move(passwordProvider))
			{}
			explicit Object(
				std::shared_ptr<const string> documentsPath,
				std::shared_ptr<const Passwords::PasswordProvider> passwordProvider,
				const document_persister::DocumentJSON &plaintextData
			): documentsPath(std::move(documentsPath)),
				passwordProvider(std::move(passwordProvider))
			{
				this->_id = string(plaintextData["_id"].GetString(), plaintextData["_id"].GetStringLength());
				{
					Value::ConstMemberIterator itr = plaintextData.FindMember("insertedAt_sSinceEpoch");
					if (itr != plaintextData.MemberEnd()) {
						this->insertedAt_sSinceEpoch = strtoul(itr->value.GetString(), NULL, 0); // must parse from string to u long
					}
				}
				//
				// Subclassers: Override and extract data but call on super
				//
			}
			// Lifecycle - Teardown
		virtual ~Object()
		{
			cout << "A Persistable::Object virtual destructor for " << this << endl;
			// make sure to call 'teardown' in the concrete destructor in your base class inherited from this
		}
		virtual void teardown(); // overridable / callable .... make sure to call Persistable::Object::teardown() in your concrete ~Object(){}
			//
			// Properties - Dependencies
			std::shared_ptr<const string> documentsPath;
			std::shared_ptr<const Passwords::PasswordProvider> passwordProvider;
			std::string instance_uuid_string = boost::uuids::to_string((boost::uuids::random_generator())()); // only the same per-instance - not persisted to disk - used by the walletsListController
			//
			// Properties - State
			boost::optional<std::string> _id;
			boost::optional<unsigned long> insertedAt_sSinceEpoch;
			//
			optional<bool> didFailToInitialize_flag;
			optional<bool> didFailToBoot_flag;
			optional<string> didFailToBoot_errStr;
			//
			// Signals - Boot state change notification declarations for your convenience - not posted for you - see Wallet.cpp
			boost::signals2::signal<void()> booted_signal;
			boost::signals2::signal<void()> failedToBoot_signal;
			// Signals - Posted automatically
			boost::signals2::signal<void(const Persistable::Object &obj)> willBeDeinitialized_signal; // this is necessary since views like UITableView and UIPickerView won't necessarily call .prepareForReuse() on an unused cell (e.g. after logged-in-runtime teardown), leaving PersistableObject instances hanging around
			boost::signals2::signal<void(const Persistable::Object &obj)> willBeDeleted_signal;
			boost::signals2::signal<void(const Persistable::Object &obj)> wasDeleted_signal;
			//
			// Accessors - Overridable - Required
			virtual const CollectionName &collectionName() const = 0;
			virtual document_persister::DocumentJSON new_dictRepresentation() const
			{ // override and implement but call on Persistable::Object
				
				Document dict;
				dict.SetObject();
				{
					Value k("_id", dict.GetAllocator());
					Value v(*(this->_id), dict.GetAllocator()); // copy string
					dict.AddMember(k, v, dict.GetAllocator());
				}
				if (this->insertedAt_sSinceEpoch != none) {
					ostringstream convert_ss;
					convert_ss << *(this->insertedAt_sSinceEpoch);
					Value k("insertedAt_sSinceEpoch", dict.GetAllocator());
					Value v(convert_ss.str(), dict.GetAllocator()); // TODO: copy is unnecessary
					dict.AddMember(k, v, dict.GetAllocator()); // store as string and decode on parse b/c Long gets converted to double
				}
				//
				return dict;
			}
			//
			// Imperatives
			boost::optional<std::string> saveToDisk();
			boost::optional<std::string> deleteFromDisk();
		private:
			//
			// Members
//			std::mutex _property_mutex; // TODO: use this?
			//
			// Accessors
			std::string new_encrypted_serializedFileRepresentation() const;
			bool _shouldInsertNotUpdate() const;
			//
			// Imperatives
			boost::optional<std::string> _saveToDisk_insert();
			boost::optional<std::string> _saveToDisk_update();
			boost::optional<std::string> __write();
	};
}

namespace Persistable
{ // Serialization
	static inline rapidjson::Document new_plaintextDocumentDictFromJSONString(const string &plaintextJSONString)
	{
		rapidjson::Document document;
		document.Parse(plaintextJSONString.c_str());
		
		return document; // move semantics
	}
	static inline string new_plaintextJSONStringFrom_movedDocumentDict(const document_persister::DocumentJSON &dict)
	{
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		dict.Accept(writer);
		//
		return string(buffer.GetString());
	}
	//
	static inline optional<string> new_plaintextStringFrom(
		const string &encryptedBase64String,
		const Passwords::Password &password
	) {
		std::string encryptedString = base64::decodedFromBase64(encryptedBase64String);
		int plaintextBytes_len = 0;
		char errbuf[BUFSIZ];
		memset(errbuf, 0, sizeof(errbuf));
		//
//		rncryptorc_set_debug(1);
		unsigned char *plaintextBytes = rncryptorc_decrypt_data_with_password(
			(unsigned char *)encryptedString.c_str(), // TODO: can this merely be cast?
			encryptedString.size(),
			RNCRYPTOR3_KDF_ITER,
			password.c_str(),
			password.size(),
			&plaintextBytes_len,
			errbuf,
			sizeof(errbuf)-1
		);
		if (plaintextBytes == NULL) {
			return boost::none;
		}
		return std::string(
			reinterpret_cast<const char *>(plaintextBytes), // unsigned char -> char
			plaintextBytes_len
		);
	}
	static inline string new_encryptedBase64StringFrom(
		const string &plaintextString,
		const Passwords::Password &password
	) {
		char errbuf[BUFSIZ];
		//
//		rncryptorc_set_debug(1);
		//
		int encryptedBytes_len = 0;
		unsigned char *encryptedBytes = rncryptorc_encrypt_data_with_password(
			(unsigned char *)plaintextString.c_str(), // TODO: can this merely be cast?
			plaintextString.size(),
			RNCRYPTOR3_KDF_ITER,
			password.c_str(),
			password.size(),
			&encryptedBytes_len,
			errbuf,
			sizeof(errbuf)-1
		);
		std::string encryptedString(
			reinterpret_cast<const char *>(encryptedBytes), // unsigned char -> char
			encryptedBytes_len
		);
		std::string encryptedBase64String = base64::encodedToBase64(encryptedString);
		//
		return encryptedBase64String;
	}
}
#endif /* PersistableObject_hpp */

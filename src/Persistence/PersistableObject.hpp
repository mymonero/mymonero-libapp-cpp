//
//  PersistableObject.cpp
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

#ifndef PersistableObject_hpp
#define PersistableObject_hpp

#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional/optional.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.
#include <boost/throw_exception.hpp>
//#include <boost/algorithm/string.hpp>
using namespace std;
using namespace boost;

#include "document_persister.hpp"
using namespace document_persister;
#include "../Passwords/PasswordController.hpp"

namespace Persistable
{
	class Object
	{
		public:
			//
			// Lifecycle - Init
			Object(
				const string &documentsPath,
				const Passwords::PasswordProvider &passwordProvider
			): documentsPath(documentsPath),
				passwordProvider(passwordProvider)
			{}
			Object(
				const string &documentsPath,
				const Passwords::PasswordProvider &passwordProvider,
				const property_tree::ptree &plaintextData
			): documentsPath(documentsPath),
				passwordProvider(passwordProvider)
			{
				this->_id = plaintextData.get<DocumentId>("_id");
				boost::optional<std::string> insertedAt_sSinceEpoch_string = plaintextData.get_optional<std::string>("insertedAt_sSinceEpoch");
				if (insertedAt_sSinceEpoch_string) {
					this->insertedAt_sSinceEpoch = strtoul((*insertedAt_sSinceEpoch_string).c_str(), NULL, 0); // must parse from string to u long
				}
				// Subclassers: Override and extract data but call on super
			}
			// Lifecycle - Teardown
			~Object();
			//
			// Properties - Dependencies
			const string &documentsPath;
			const Passwords::PasswordProvider &passwordProvider;
			//
			// Properties - State
			boost::optional<std::string> _id;
			boost::optional<unsigned long> insertedAt_sSinceEpoch;
			//
			// Accessors - Overridable - Required
			property_tree::ptree new_dictRepresentation() const
			{ // override and implement but call on Persistable::Object
				property_tree::ptree dict;
				dict.put("_id", *(this->_id));
				if (this->insertedAt_sSinceEpoch) {
					ostringstream convert_ss;
					convert_ss << insertedAt_sSinceEpoch;
					dict.put("insertedAt_sSinceEpoch", convert_ss.str()); // store as string and decode on parse b/c Long gets converted to double
				}
				//
				return dict;
			}
			virtual CollectionName collectionName() const = 0;
			//
			// Imperatives
			boost::optional<std::string> saveToDisk();
		private:
	};
}
namespace Persistable
{ // Serialization
	static inline property_tree::ptree new_plaintextDocumentDictFromJSONString(const string &plaintextJSONString)
	{
		stringstream ss;
		ss << plaintextJSONString;
		property_tree::ptree pt;
        property_tree::read_json(ss, pt);
        //
		return pt;
	}
	static inline string new_plaintextJSONStringFromDocumentDict(const property_tree::ptree &dict)
	{
	    std::stringstream ss;
    	boost::property_tree::json_parser::write_json(ss, dict);
		//
		return ss.str();
	}
	//
	static inline string new_plaintextStringFrom(
		const string &encryptedString,
		const Passwords::Password &password
	) {
//		return RNCryptorNative().decrypt(encryptedString, password)
	}
	static inline string new_encryptedStringFrom(
		const string &plaintextString,
		const Passwords::Password &password
	) {
//		val encryptedString = RNCryptorNative().encrypt(plaintextString, password).toString(Charset.forName("UTF-8"))
//
//		return encryptedString
	}
}
#endif /* PersistableObject_hpp */

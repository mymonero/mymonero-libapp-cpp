//
//  Wallet.hpp
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

#ifndef Wallet_hpp
#define Wallet_hpp

#include <string>
#include "../Persistence/PersistableObject.hpp"

namespace Wallets
{
	using namespace std;
	//
	// Constants
	static const CollectionName collectionName = "Wallets";
	//
	//
	class Object: public Persistable::Object
	{
	public:
		Object(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<const Passwords::PasswordProvider> passwordProvider
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider)
		) {
		}
		Object(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
			const property_tree::ptree &plaintextData
		): Persistable::Object(
			std::move(documentsPath),
			std::move(passwordProvider),
			plaintextData
		) {
			if (this->_id != boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->_id == boost::none"));
			}
			if (this->insertedAt_sSinceEpoch != boost::none) {
				BOOST_THROW_EXCEPTION(logic_error("Didn't expect this->insertedAt_sSinceEpoch == boost::none"));
			}
			//
			// TODO:
//			this->addtlVal = plaintextData.get<std::string>("addtlVal");
//			BOOST_REQUIRE(this->addtlVal != boost::none);
		}
		virtual property_tree::ptree new_dictRepresentation() const
		{
			property_tree::ptree dict = Persistable::Object::new_dictRepresentation();
			// TODO:
//			if (this->addtlVal) {
//				dict.put("addtlVal", *(this->addtlVal));
//			}
			//
			return dict;
		}
		const CollectionName &collectionName() const { return Wallets::collectionName; }
	};
}

#endif /* Wallet_hpp */

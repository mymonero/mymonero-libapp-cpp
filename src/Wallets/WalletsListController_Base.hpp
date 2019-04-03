//
//  WalletsListController_Base.hpp
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

#ifndef WalletsListController_Base_hpp
#define WalletsListController_Base_hpp

#include <iostream>
#include <boost/signals2.hpp>
#include <mutex>
#include "../Dispatch/Dispatch_Interface.hpp"
#include "../Settings/SettingsProviders.hpp"
#include "../Lists/PersistedObjectListController.hpp"
#include "./Wallet.hpp"

namespace Wallets
{
	using namespace std;
	using namespace boost;
	//
	// Controllers
	class ListController_Base: public Lists::Controller
	{
	public:
		ListController_Base(): Lists::Controller(Wallets::collectionName) {}
		~ListController_Base()
		{
			// TODO: ensure parent class destructor is getting called
		}
		//
		// Overrides
		std::shared_ptr<Persistable::Object> new_record(
			std::shared_ptr<std::string> documentsPath,
			std::shared_ptr<Passwords::PasswordProvider> passwordProvider,
			const document_persister::DocumentJSON &plaintext_documentJSON
		) override {
			return std::make_shared<Wallets::Object>(
				documentsPath,
				passwordProvider,
				plaintext_documentJSON
			);
		}
		
		//
		// Dependencies


		//
		// Properties

		//
		// Signals

		//
		// Imperatives

	private:
		//
		// Properties

		//
		// Imperatives

	};
}

#endif /* WalletsListController_Base_hpp */

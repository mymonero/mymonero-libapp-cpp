//
//  SettingsController.cpp
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

#ifndef SettingsController_hpp
#define SettingsController_hpp

#include <string>
#include <boost/range/algorithm.hpp>
#include <boost/optional/optional.hpp>
#include <boost/signals2.hpp>
#include <memory>
#include "../Persistence/document_persister.hpp"
#include "../Dispatch/Dispatch_Interface.hpp"

namespace Settings
{ // Constants
	static const double appTimeoutAfterS_neverValue = -1;
}
namespace Settings
{
	using namespace std;
	using namespace boost;
	using namespace document_persister;
	//
	// Controllers
	class Controller
	{
	public:
		//
		// Lifecycle - Init
		Controller(
			string documentsPath,
			std::shared_ptr<Dispatch::Dispatch> dispatch_ptr
		) {
			this->documentsPath = documentsPath;
			this->dispatch_ptr = dispatch_ptr;
			//
			this->setup();
		}
		~Controller() {
			cout << "Destructed Settings" << endl;
		}
		//
		// Constructor args
		string documentsPath;
		std::shared_ptr<Dispatch::Dispatch> dispatch_ptr;
		//
		// Signals
		//
		// Properties
		optional<double> appTimeoutAfterS_noneForDefault_orNeverValue;
	private:
		//
		// Imperatives
		void setup();
	};
}

#endif /* SettingsController_hpp */

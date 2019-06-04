//
//  Wallet_HostPollingController.hpp
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

#ifndef Wallet_KeyImageCache_hpp
#define Wallet_KeyImageCache_hpp
//
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include "monero_key_image_utils.hpp"
#include "string_tools.h"
//
namespace Wallets
{
	using namespace std;
//	using namespace boost;
	using namespace monero_key_image_utils;
	//
	//
	//
	// Controllers
	struct KeyImageCache
	{
		//
	public:

		//
		// Instance - Interface - Accessors
		string lazy_keyImage(
			const string &tx_pub_key_string,
			uint64_t out_index,
			const string &public_address,
			const string &view_sec_key_string,
			const string &spend_sec_key_string,
			const string &spend_pub_key_string
		) {
			auto cacheKey = _new_keyImage_cacheKey(tx_pub_key_string, public_address, out_index);
			std::unordered_map<string, string>::const_iterator it = _keyImages_byCacheKey.find(cacheKey);
			if (it == _keyImages_byCacheKey.end()) { // does not exist
				crypto::secret_key sec_viewKey{};
				crypto::secret_key sec_spendKey{};
				crypto::public_key pub_spendKey{};
				crypto::public_key tx_pub_key{};
				{
					bool r = false;
					r = epee::string_tools::hex_to_pod(view_sec_key_string, sec_viewKey);
					if (!r) {
						BOOST_THROW_EXCEPTION(logic_error("Invalid secret view key"));
					}
					r = epee::string_tools::hex_to_pod(spend_sec_key_string, sec_spendKey);
					if (!r) {
						BOOST_THROW_EXCEPTION(logic_error("Invalid secret spend key"));
					}
					r = epee::string_tools::hex_to_pod(spend_pub_key_string, pub_spendKey);
					if (!r) {
						BOOST_THROW_EXCEPTION(logic_error("Invalid public spend key"));
					}
					r = epee::string_tools::hex_to_pod(tx_pub_key_string, tx_pub_key);
					if (!r) {
						BOOST_THROW_EXCEPTION(logic_error("Invalid tx pub key"));
					}
				}
				KeyImageRetVals retVals;
				bool r = new__key_image(
					pub_spendKey, sec_spendKey, sec_viewKey, tx_pub_key,
					out_index,
					retVals
				);
				if (!r) {
					BOOST_THROW_EXCEPTION(logic_error("Unable to generate key image"));
					//
					return ""; // since something must be returned…
				}
				_keyImages_byCacheKey[cacheKey] = epee::string_tools::pod_to_hex(retVals.calculated_key_image);
			}
			return _keyImages_byCacheKey[cacheKey]; // copy?
		}
	private:
		//
		std::unordered_map<string, string> _keyImages_byCacheKey;
		//
		static inline string _new_keyImage_cacheKey(
			const string &tx_pub_key,
			const string &public_address,
			uint64_t out_index
		) {
			ostringstream oss;
			oss << tx_pub_key << ":" << public_address << ":" << out_index;
			//
			return oss.str();
		}

	};
}

#endif /* Wallet_KeyImageCache_hpp */

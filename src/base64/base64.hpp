//
//  base64.cpp
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

#ifndef base64_hpp
#define base64_hpp
//
#include <string>
using namespace std;
//
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>
//
namespace base64
{
	//
	// Adapted from 
	// https://stackoverflow.com/questions/7053538/how-do-i-encode-a-string-to-base64-using-only-boost
	static inline std::string decodedFromBase64(const std::string &val)
	{
	    using namespace boost::archive::iterators;
	    using It = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>;
	    //
	    return boost::algorithm::trim_right_copy_if(
	    	std::string(
	    		It(std::begin(val)), 
	    		It(std::end(val))
	    	),
    		[](char c) { return c == '\0'; }
	    );
	}
	static inline std::string encodedToBase64(const std::string &val)
	{
	    using namespace boost::archive::iterators;
	    using It = base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>;
	    auto tmp = std::string(
	    	It(std::begin(val)), 
	    	It(std::end(val))
	    );
	    //
	    return tmp.append((3 - val.size() % 3) % 3, '=');
	}
}
#endif /* base64_hpp */

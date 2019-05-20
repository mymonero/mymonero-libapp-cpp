//
//  test_all.cpp
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
// Includes & namespaces
#include <iostream>
#include <iterator>
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/optional/optional_io.hpp>
using namespace std;
using namespace boost;
//
// Shared code
inline std::string _new_documentsPathString()
{
	std::string thisfile_path = std::string(__FILE__);
	std::string tests_dir = thisfile_path.substr(0, thisfile_path.find_last_of("\\/"));
	std::string srcroot_dir = tests_dir.substr(0, tests_dir.find_last_of("\\/"));
	boost::filesystem::path dir(srcroot_dir);
	boost::filesystem::path file("build");
    boost::filesystem::path full_path = dir / file;
	//
	boost::filesystem::create_directory(full_path); // in case it doesn't exist
	//
	return full_path.string();
}
inline std::shared_ptr<string> new_documentsPath()
{
	return std::make_shared<string>(
		_new_documentsPathString()
	);
}
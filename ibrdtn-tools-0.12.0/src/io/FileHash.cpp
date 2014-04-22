/*
 * FileHash.cpp
 *
 * Copyright (C) 2013 IBR, TU Braunschweig
 *
 * Written-by: David Goltzsche <goltzsch@ibr.cs.tu-bs.de>
 *             Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *  Created on: Oct 17, 2013
 */
#include "FileHash.h"

namespace io
{
	FileHash::FileHash()
	{
	}

	FileHash::FileHash(const std::string &path, const std::string &hash)
	 : _path(path), _hash(hash)
	{
	}

	FileHash::~FileHash()
	{
	}

	const std::string& FileHash::getHash() const
	{
		return _hash;
	}

	const std::string& FileHash::getPath() const
	{
		return _path;
	}

	bool FileHash::operator!=(const FileHash& other) const
	{
		return (_path != other.getPath()) || (_hash != other.getHash());
	}

	bool FileHash::operator==(const FileHash& other) const
	{
		return (_path == other.getPath()) && (_hash == other.getHash());
	}

	bool FileHash::operator<(const FileHash& other) const
	{
		if (_path < other.getPath()) return true;
		if (_path > other.getPath()) return false;

		if (_hash < other.getHash()) return true;
		return false;
	}
}

/*
 * SecurityKey.h
 *
 * Copyright (C) 2011 IBR, TU Braunschweig
 *
 * Written-by: Johannes Morgenroth <morgenroth@ibr.cs.tu-bs.de>
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
 */

#ifndef SECURITYKEY_H_
#define SECURITYKEY_H_

#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Number.h"
#include "ibrdtn/data/DTNTime.h"
#include "ibrdtn/data/BundleString.h"
#include <ibrcommon/data/File.h>
#include <openssl/rsa.h>

#include <string>
#include <iostream>

namespace dtn
{
	namespace security
	{
		class SecurityKey
		{
		public:
			enum KeyType
			{
				KEY_UNSPEC = 0,
				KEY_SHARED = 1,
				KEY_PRIVATE = 2,
				KEY_PUBLIC = 3
			};

			SecurityKey();
			virtual ~SecurityKey();

			// key type
			KeyType type;

			// referencing EID of this key
			dtn::data::EID reference;

			// last update time
			dtn::data::DTNTime lastupdate;

			// key file
			ibrcommon::File file;

			virtual RSA* getRSA() const;

			virtual EVP_PKEY* getEVP() const;

			virtual const std::string getData() const;

			static void free(RSA* key);
			static void free(EVP_PKEY* key);

			friend std::ostream &operator<<(std::ostream &stream, const SecurityKey &key)
			{
				// key type
				stream << dtn::data::Number(key.type);

				// EID reference
				stream << dtn::data::BundleString(key.reference.getString());

				// timestamp of last update
				stream << key.lastupdate;

				// To support concatenation of streaming calls, we return the reference to the output stream.
				return stream;
			}

			friend std::istream &operator>>(std::istream &stream, SecurityKey &key)
			{
				// key type
				dtn::data::Number sdnv_type; stream >> sdnv_type;
				key.type = KeyType(sdnv_type.get<KeyType>());

				// EID reference
				dtn::data::BundleString eid_reference; stream >> eid_reference;
				key.reference = dtn::data::EID(eid_reference);

				// timestamp of last update
				stream >> key.lastupdate;

				// To support concatenation of streaming calls, we return the reference to the input stream.
				return stream;
			}

		private:
			RSA* getPublicRSA() const;
			RSA* getPrivateRSA() const;
		};
	}
}

#endif /* SECURITYKEY_H_ */

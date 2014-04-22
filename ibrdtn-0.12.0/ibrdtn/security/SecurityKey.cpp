/*
 * SecurityKey.cpp
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

#include "ibrdtn/security/SecurityKey.h"
#include <ibrcommon/Logger.h>
#include <fstream>
#include <sstream>

#include <openssl/pem.h>
#include <openssl/err.h>

namespace dtn
{
	namespace security
	{
		SecurityKey::SecurityKey()
		 : type(KEY_UNSPEC)
		{}

		SecurityKey::~SecurityKey()
		{}

		void SecurityKey::free(RSA* key)
		{
			RSA_free(key);
		}

		void SecurityKey::free(EVP_PKEY* key)
		{
			EVP_PKEY_free(key);
		}

		const std::string SecurityKey::getData() const
		{
			std::ifstream stream(file.getPath().c_str(), ios::in);
			std::stringstream ss;

			ss << stream.rdbuf();

			stream.close();

			return ss.str();
		}

		RSA* SecurityKey::getRSA() const
		{
			switch (type)
			{
			case KEY_PRIVATE:
				return getPrivateRSA();
			case KEY_PUBLIC:
				return getPublicRSA();
			default:
				return NULL;
			}
		}

		EVP_PKEY* SecurityKey::getEVP() const
		{
			EVP_PKEY* ret = EVP_PKEY_new();
			FILE * pkey_file = fopen(file.getPath().c_str(), "r");

			switch (type)
			{
				case KEY_PRIVATE:
				{
					ret = PEM_read_PrivateKey(pkey_file, &ret, NULL, NULL);
					break;
				}

				case KEY_PUBLIC:
				{
					ret = PEM_read_PUBKEY(pkey_file, &ret, NULL, NULL);
					break;
				}

				default:
					ret = NULL;
					break;
			}

			fclose(pkey_file);
			return ret;
		}

		RSA* SecurityKey::getPrivateRSA() const
		{
			RSA *rsa = RSA_new();

			FILE * rsa_pkey_file = fopen(file.getPath().c_str(), "r");
			if (!rsa_pkey_file) {
				IBRCOMMON_LOGGER_ex(critical) << "Failed to open " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("Failed to open " + file.getPath());
			}
			if (!PEM_read_RSAPrivateKey(rsa_pkey_file, &rsa, NULL, NULL)) {
				IBRCOMMON_LOGGER_ex(critical) << "Error loading RSA private key file: " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				throw ibrcommon::Exception("Error loading RSA private key file: " + file.getPath());
			}
			fclose(rsa_pkey_file);
			return rsa;
		}

		RSA* SecurityKey::getPublicRSA() const
		{
			RSA *rsa = RSA_new();

			FILE * rsa_pkey_file = fopen(file.getPath().c_str(), "r");
			if (!rsa_pkey_file) {
				IBRCOMMON_LOGGER_ex(critical) << "Failed to open " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				throw ibrcommon::Exception("Failed to open " + file.getPath());
			}
			if (!PEM_read_RSA_PUBKEY(rsa_pkey_file, &rsa, NULL, NULL)) {
				IBRCOMMON_LOGGER_ex(critical) << "Error loading RSA public key file: " << file.getPath() << IBRCOMMON_LOGGER_ENDL;
				ERR_print_errors_fp(stderr);
				throw ibrcommon::Exception("Error loading RSA public key file: " + file.getPath());
			}
			fclose(rsa_pkey_file);
			return rsa;
		}
	}
}

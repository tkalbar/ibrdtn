/*
 * DiscoveryBeacon.h
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

#ifndef DISCOVERYBEACON_H_
#define DISCOVERYBEACON_H_

#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/data/BundleString.h>
#include "net/DiscoveryService.h"
#include <string>
#include <list>

namespace dtn
{
	namespace net
	{
		class DiscoveryBeacon
		{
			enum BEACON_FLAGS_V1
			{
				BEACON_NO_FLAGS = 0x00,
				BEACON_SHORT = 0x01
			};

			enum BEACON_FLAGS_V2
			{
				BEACON_CONTAINS_EID = 0x01,
				BEACON_SERVICE_BLOCK = 0x02,
				BEACON_BLOOMFILTER = 0x04
			};

		public:
			enum Protocol
			{
				DTND_IPDISCOVERY = 0x00,
				DISCO_VERSION_00 = 0x01,
				DISCO_VERSION_01 = 0x02
			};

			DiscoveryBeacon(const Protocol version = DISCO_VERSION_00, const dtn::data::EID &eid = dtn::data::EID());

			virtual ~DiscoveryBeacon();

			typedef std::list<DiscoveryService> service_list;

			void setEID(const dtn::data::EID &eid);
			const dtn::data::EID& getEID() const;

			service_list& getServices();
			const service_list& getServices() const;
			void clearServices();
			void addService(const DiscoveryService &service);
			const DiscoveryService& getService(const std::string &name) const;
			DiscoveryService& getService(const std::string &name);

			std::string toString() const;

			void setSequencenumber(uint16_t sequence);

			bool isShort() const;

		private:
			friend std::ostream &operator<<(std::ostream &stream, const DiscoveryBeacon &announcement);
			friend std::istream &operator>>(std::istream &stream, DiscoveryBeacon &announcement);

			unsigned int _version;
			unsigned char _flags;
			dtn::data::EID _canonical_eid;
			uint16_t _sn;
			dtn::data::BundleString _bloomfilter;

			service_list _services;
		};
	}
}

#endif /* DISCOVERYBEACON_H_ */

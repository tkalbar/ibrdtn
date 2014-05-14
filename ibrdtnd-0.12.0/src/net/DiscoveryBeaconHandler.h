/* 
 * DiscoveryBeaconHandler.h
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

#ifndef _DISCOVERYBEACONHANDLER_H
#define	_DISCOVERYBEACONHANDLER_H

#include "net/DiscoveryBeacon.h"
#include <ibrcommon/net/vinterface.h>
#include <string>

namespace dtn
{
	namespace net
	{
		class DiscoveryBeaconHandler
		{
		public:
			class NoServiceHereException : public ibrcommon::Exception
			{
			public:
				NoServiceHereException(std::string what = "No service available.") throw() : ibrcommon::Exception(what)
				{
				};

				virtual ~NoServiceHereException() throw() {};
			};

			virtual ~DiscoveryBeaconHandler() = 0;

			virtual void onAdvertiseBeacon(const ibrcommon::vinterface &iface, const DiscoveryBeacon &beacon) throw ();

			virtual void onUpdateBeacon(const ibrcommon::vinterface &iface, DiscoveryBeacon &beacon) throw (NoServiceHereException);
		};
	}
}

#endif	/* _DISCOVERYBEACONHANDLER_H */


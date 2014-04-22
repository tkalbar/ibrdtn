/*
 * DiscoveryAgent.cpp
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

#include "Configuration.h"
#include "net/DiscoveryAgent.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryBeacon.h"
#include "core/BundleCore.h"
#include "core/TimeEvent.h"
#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"
#include "core/EventDispatcher.h"
#include <ibrdtn/utils/Utils.h>
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>

using namespace dtn::core;

namespace dtn
{
	namespace net
	{
		DiscoveryAgent::DiscoveryAgent()
		 : _config(dtn::daemon::Configuration::getInstance().getDiscovery()),
		   _enabled(true), _sn(0), _adv_next(0), _last_announce_sent(0)
		{
		}

		DiscoveryAgent::~DiscoveryAgent()
		{
		}

		const std::string DiscoveryAgent::getName() const
		{
			return "DiscoveryAgent";
		}

		void DiscoveryAgent::raiseEvent(const dtn::core::Event *evt) throw ()
		{
			try {
				dynamic_cast<const dtn::core::TimeEvent&>(*evt);
				const dtn::data::Timestamp ts = dtn::utils::Clock::getMonotonicTimestamp();

				if (_config.announce() && (_adv_next <= ts)) {
					// advertise me
					onAdvertise();

					// next advertisement in one second
					_adv_next = ts + 1;
				}
			} catch (const std::bad_cast&) {

			}

			try {
				const dtn::core::GlobalEvent &global = dynamic_cast<const dtn::core::GlobalEvent&>(*evt);
				if (global.getAction() == dtn::core::GlobalEvent::GLOBAL_START_DISCOVERY) {
					// start sending discovery beacons
					_enabled = true;
				}
				else if (global.getAction() == dtn::core::GlobalEvent::GLOBAL_STOP_DISCOVERY) {
					// suspend discovery beacons
					_enabled = false;
				}
			} catch (const std::bad_cast&) {

			}
		}

		void DiscoveryAgent::componentUp() throw ()
		{
			// listen to global events (discovery start/stop)
			dtn::core::EventDispatcher<dtn::core::GlobalEvent>::add(this);

			// listen to time events
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::add(this);
		}

		void DiscoveryAgent::componentDown() throw ()
		{
			// un-listen to global events (discovery start/stop)
			dtn::core::EventDispatcher<dtn::core::GlobalEvent>::remove(this);

			// un-listen to time events
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::remove(this);
		}

		void DiscoveryAgent::registerService(const ibrcommon::vinterface &iface, dtn::net::DiscoveryBeaconHandler *handler)
		{
			ibrcommon::MutexLock l(_provider_lock);
			handler_list &list = _providers[iface];
			list.push_back(handler);
		}

		void DiscoveryAgent::registerService(dtn::net::DiscoveryBeaconHandler *handler)
		{
			ibrcommon::MutexLock l(_provider_lock);
			handler_list &list = _providers[_any_iface];
			list.push_back(handler);
		}

		void DiscoveryAgent::unregisterService(const dtn::net::DiscoveryBeaconHandler *handler)
		{
			ibrcommon::MutexLock l(_provider_lock);

			// walk though all interfaces
			for (handler_map::iterator it_p = _providers.begin(); it_p != _providers.end();)
			{
				handler_list &list = (*it_p).second;

				for (handler_list::iterator it = list.begin(); it != list.end(); ++it) {
					if ((*it) == handler) {
						list.erase(it);
						break;
					}
				}

				if (list.empty())
					_providers.erase(it_p++);
				else
					++it_p;
			}
		}

		void DiscoveryAgent::unregisterService(const ibrcommon::vinterface &iface, const dtn::net::DiscoveryBeaconHandler *handler)
		{
			ibrcommon::MutexLock l(_provider_lock);
			if (_providers.find(iface) == _providers.end()) return;

			handler_list &list = _providers[iface];

			for (handler_list::iterator it = list.begin(); it != list.end(); ++it) {
				if ((*it) == handler) {
					list.erase(it);
					if (list.empty()) _providers.erase(iface);
					return;
				}
			}
		}

		DiscoveryBeacon DiscoveryAgent::obtainBeacon() const
		{
			DiscoveryBeacon::Protocol version;

			switch (_config.version())
			{
			case 2:
				version = DiscoveryBeacon::DISCO_VERSION_01;
				break;

			case 1:
				version = DiscoveryBeacon::DISCO_VERSION_00;
				break;

			case 0:
				IBRCOMMON_LOGGER_TAG("DiscoveryAgent", info) << "DTN2 compatibility mode" << IBRCOMMON_LOGGER_ENDL;
				version = DiscoveryBeacon::DTND_IPDISCOVERY;
				break;
			};

			DiscoveryBeacon beacon(version, dtn::core::BundleCore::local);

			return beacon;
		}

		void DiscoveryAgent::onBeaconReceived(const DiscoveryBeacon &beacon)
		{
			// ignore own beacons
			if (beacon.getEID() == dtn::core::BundleCore::local) return;

			// convert the announcement into NodeEvents
			Node n(beacon.getEID());

			const std::list<DiscoveryService> &services = beacon.getServices();

			for (std::list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); ++iter)
			{
				const dtn::data::Number to_value = _config.timeout();

				const DiscoveryService &s = (*iter);

				// get protocol from tag
				const dtn::core::Node::Protocol p = s.getProtocol();

				if (p == dtn::core::Node::CONN_EMAIL)
				{
					// Set timeout
					dtn::data::Number to_value_mailcl = to_value;
					size_t configTime = dtn::daemon::Configuration::getInstance().getEMail().getNodeAvailableTime();
					if(configTime > 0)
						to_value_mailcl = configTime;

					n.add(Node::URI(Node::NODE_DISCOVERED, Node::CONN_EMAIL, s.getParameters(), to_value_mailcl, 20));
				}
				else if ((p == dtn::core::Node::CONN_UNSUPPORTED) || (p == dtn::core::Node::CONN_UNDEFINED))
				{
					n.add(Node::Attribute(Node::NODE_DISCOVERED, s.getName(), s.getParameters(), to_value, 20));
				}
				else
				{
					n.add(Node::URI(Node::NODE_DISCOVERED, p, s.getParameters(), to_value, 20));
				}
			}

			// announce NodeInfo to ConnectionManager
			dtn::core::BundleCore::getInstance().getConnectionManager().updateNeighbor(n);

			// if continuous announcements are disabled, then reply to this message
			if (!_config.announce())
			{
				// first check if another announcement was sent during the same seconds
				const dtn::data::Timestamp ts = dtn::utils::Clock::getMonotonicTimestamp();

				if (_last_announce_sent != ts)
				{
					IBRCOMMON_LOGGER_DEBUG_TAG("DiscoveryAgent", 55) << "reply with discovery beacon" << IBRCOMMON_LOGGER_ENDL;

					// reply with an own announcement
					onAdvertise();

					// remember timestamp of last sent discovery
					_last_announce_sent = ts;
				}
			}
		}

		void DiscoveryAgent::onAdvertise()
		{
			// check if announcements are enabled
			if (!_enabled) return;

			IBRCOMMON_LOGGER_DEBUG_TAG("DiscoveryAgent", 55) << "advertise discovery beacon" << IBRCOMMON_LOGGER_ENDL;

			DiscoveryBeacon::Protocol version;

			switch (_config.version())
			{
			case 2:
				version = DiscoveryBeacon::DISCO_VERSION_01;
				break;

			case 1:
				version = DiscoveryBeacon::DISCO_VERSION_00;
				break;

			case 0:
				IBRCOMMON_LOGGER_TAG("DiscoveryAgent", info) << "DTN2 compatibility mode" << IBRCOMMON_LOGGER_ENDL;
				version = DiscoveryBeacon::DTND_IPDISCOVERY;
				break;
			};

			DiscoveryBeacon beacon(version, dtn::core::BundleCore::local);

			// set sequencenumber
			beacon.setSequencenumber(_sn);

			ibrcommon::MutexLock l(_provider_lock);

			// get list for ANY interface
			const handler_list &any_list = _providers[_any_iface];

			for (handler_map::const_iterator it_p = _providers.begin(); it_p != _providers.end(); ++it_p)
			{
				const ibrcommon::vinterface &iface = (*it_p).first;
				const handler_list &plist = (*it_p).second;

				// clear all services
				beacon.clearServices();

				// collect services from providers
				if (!_config.shortbeacon())
				{
					for (handler_list::const_iterator iter = plist.begin(); iter != plist.end(); ++iter)
					{
						DiscoveryBeaconHandler &handler = (**iter);

						try {
							// update service information
							handler.onUpdateBeacon(iface, beacon);
						} catch (const dtn::net::DiscoveryBeaconHandler::NoServiceHereException&) {

						}
					}

					// add service information for ANY interface
					if (iface != _any_iface)
					{
						for (handler_list::const_iterator iter = any_list.begin(); iter != any_list.end(); ++iter)
						{
							DiscoveryBeaconHandler &handler = (**iter);

							try {
								// update service information
								handler.onUpdateBeacon(iface, beacon);
							} catch (const dtn::net::DiscoveryBeaconHandler::NoServiceHereException&) {

							}
						}
					}
				}

				// broadcast announcement
				for (handler_list::const_iterator iter = plist.begin(); iter != plist.end(); ++iter)
				{
					DiscoveryBeaconHandler &handler = (**iter);

					// broadcast beacon
					handler.onAdvertiseBeacon(iface, beacon);
				}
			}

			// save the time of the last sent announcement
			_last_announce_sent = dtn::utils::Clock::getTime();

			// increment sequencenumber
			_sn++;
		}
	}
}

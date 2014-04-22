/*
 * NativeDaemon.h
 *
 * Copyright (C) 2013 IBR, TU Braunschweig
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

#include "Component.h"
#include "core/AbstractWorker.h"
#include "net/IPNDAgent.h"
#include "storage/BundleSeeker.h"
#include "storage/BundleIndex.h"
#include "core/EventReceiver.h"
#include <ibrcommon/Exceptions.h>
#include <list>
#include <set>
#include <map>

#ifndef NATIVEDAEMON_H_
#define NATIVEDAEMON_H_

namespace dtn
{
	namespace daemon
	{
		enum DaemonRunLevel {
			RUNLEVEL_ZERO = 0,
			RUNLEVEL_CORE = 1,
			RUNLEVEL_STORAGE = 2,
			RUNLEVEL_ROUTING = 3,
			RUNLEVEL_API = 4,
			RUNLEVEL_NETWORK = 5,
			RUNLEVEL_ROUTING_EXTENSIONS = 6
		};

		class NativeDaemonCallback {
		public:

			virtual ~NativeDaemonCallback() = 0;

			virtual void levelChanged(DaemonRunLevel level) throw () = 0;
		};

		class NativeEventCallback {
		public:
			virtual ~NativeEventCallback() = 0;

			virtual void eventRaised(const std::string &event, const std::string &action, const std::vector<std::string> &data) throw () = 0;
		};

		class NativeNode {
		public:
			enum Type {
				NODE_P2P,
				NODE_CONNECTED,
				NODE_DISCOVERED,
				NODE_INTERNET,
				NODE_STATIC
			};

			NativeNode(const std::string &e)
			: eid(e), type(NODE_STATIC) { };

			~NativeNode() {};

			std::string eid;
			Type type;
		};

		class NativeStats {
		public:
			NativeStats()
			: uptime(0), timestamp(0), neighbors(0), storage_size(0),
			  time_offset(0.0), time_rating(0.0), time_adjustments(0),
			  bundles_stored(0), bundles_expired(0), bundles_generated(0),
			  bundles_received(0), bundles_transmitted(0), bundles_aborted(0),
			  bundles_requeued(0), bundles_queued(0)
			{ };

			~NativeStats() { };

			size_t uptime;
			size_t timestamp;
			size_t neighbors;
			size_t storage_size;

			double time_offset;
			double time_rating;
			size_t time_adjustments;

			size_t bundles_stored;
			size_t bundles_expired;
			size_t bundles_generated;
			size_t bundles_received;
			size_t bundles_transmitted;
			size_t bundles_aborted;
			size_t bundles_requeued;
			size_t bundles_queued;

			const std::vector<std::string>& getTags() {
				return _tags;
			}

			const std::string& getData(int index) {
				return _data[index];
			}

			void addData(const std::string &tag, const std::string &data) {
				_tags.push_back(tag);
				_data.push_back(data);
			}

		private:
			std::vector<std::string> _tags;
			std::vector<std::string> _data;
		};

		class NativeDaemonException : public ibrcommon::Exception
		{
		public:
			NativeDaemonException(std::string what = "An error happened.") throw() : ibrcommon::Exception(what)
			{
			};
		};

		class NativeEventLoop;

		class NativeDaemon : public dtn::core::EventReceiver
		{
			static const std::string TAG;

		public:
			/**
			 * Constructor
			 */
			NativeDaemon(NativeDaemonCallback *statecb = NULL, NativeEventCallback *eventcb = NULL);

			/**
			 * Destructor
			 */
			virtual ~NativeDaemon();

			/**
			 * Get the current runlevel
			 */
			DaemonRunLevel getRunLevel() const throw ();

			/**
			 * Switch the runlevel of the daemon
			 */
			void init(DaemonRunLevel rl) throw (NativeDaemonException);

			/**
			 * Wait until the runlevel is reached
			 */
			void wait(DaemonRunLevel rl) throw ();

			/**
			 * Generate a reload signal
			 */
			void reload() throw ();

			/**
			 * Enable / disable debugging at runtime
			 */
			void setDebug(int level) throw ();

			/**
			 * Set the config file and enable default logging capabilities
			 */
			void setLogging(const std::string &defaultTag, int logLevel) const throw ();

			/**
			 * Set the path to the log file
			 */
			void setLogFile(const std::string &path, int logLevel) const throw ();

			/**
			 * Set the daemon configuration
			 */
			void setConfigFile(const std::string &config_file);

			/** NATIVE DAEMON METHODS **/

			/**
			 * Get the local EID of this node
			 */
			std::string getLocalUri() const throw ();

			/**
			 * Get the neighbors
			 */
			std::vector<std::string> getNeighbors() const throw ();

			/**
			 * Get neighbor info
			 */
			NativeNode getInfo(const std::string &neighbor_eid) const throw (NativeDaemonException);

			/**
			 * Get statistical data
			 */
			NativeStats getStats() throw ();

			/**
			 * Add a static connection to the neighbor with the given EID
			 */
			void addConnection(std::string eid, std::string protocol, std::string address, std::string service, bool local = false) const throw ();

			/**
			 * Remove a static connection of the neighbor with the given EID
			 */
			void removeConnection(std::string eid, std::string protocol, std::string address, std::string service, bool local = false) const throw ();

			/**
			 * initiate a connection to a given neighbor
			 */
			void initiateConnection(std::string eid);

			/**
			 * @see dtn::core::EventReceiver::raiseEvent()
			 */
			virtual void raiseEvent(const dtn::core::Event *evt) throw ();

			/**
			 * Returns version information about the native daemon
			 */
			std::vector<std::string> getVersion() const throw ();

			/**
			 * Delete all bundles in the storage
			 */
			void clearStorage() const throw ();

			/**
			 * Enable discovery mechanisms IPND beacons
			 */
			void startDiscovery() const throw ();

			/**
			 * Disable discovery mechanism like IPND beacons
			 */
			void stopDiscovery() const throw ();

		private:
			void init_up(DaemonRunLevel rl) throw (NativeDaemonException);
			void init_down(DaemonRunLevel rl) throw (NativeDaemonException);

			void addEventData(const dtn::data::Bundle &b, std::vector<std::string> &data) const;
			void addEventData(const dtn::data::MetaBundle &b, std::vector<std::string> &data) const;
			void addEventData(const dtn::data::BundleID &b, std::vector<std::string> &data) const;

			void bindEvents();
			void unbindEvents();

			/**
			 * routines to init/shutdown the daemon core
			 */
			void init_core() throw (NativeDaemonException);
			void shutdown_core() throw (NativeDaemonException);

			/**
			 * routines to init/shutdown the storage subsystem
			 */
			void init_storage() throw (NativeDaemonException);
			void shutdown_storage() const throw (NativeDaemonException);

			/**
			 * routines to init/shutdown the base router
			 */
			void init_routing() throw (NativeDaemonException);
			void shutdown_routing() const throw (NativeDaemonException);

			/**
			 * routines to init/shutdown the API modules and embedded
			 * Apps
			 */
			void init_api() throw (NativeDaemonException);
			void shutdown_api() throw (NativeDaemonException);

			/**
			 * routines to init/shutdown the networking stack
			 */
			void init_network() throw (NativeDaemonException);
			void shutdown_network() throw (NativeDaemonException);

			/**
			 * routines to init/shutdown the extended routing modules
			 */
			void init_routing_extensions() throw (NativeDaemonException);
			void shutdown_routing_extensions() const throw (NativeDaemonException);

			// conditional
			ibrcommon::Conditional _runlevel_cond;
			DaemonRunLevel _runlevel;

			NativeDaemonCallback *_statecb;
			NativeEventCallback *_eventcb;

			// list of components
			typedef std::list< dtn::daemon::Component* > component_list;
			typedef std::map<DaemonRunLevel, component_list > component_map;
			component_map _components;

			// list of embedded apps
			typedef std::list< dtn::core::AbstractWorker* > app_list;
			app_list _apps;

			NativeEventLoop *_event_loop;

			ibrcommon::File _config_file;
		};

		class NativeEventLoop : public ibrcommon::JoinableThread {
		public:
			NativeEventLoop(NativeDaemon &daemon);
			~NativeEventLoop();
			virtual void run(void) throw ();
			virtual void __cancellation() throw ();

		private:
			NativeDaemon &_daemon;
		};
	} /* namespace daemon */
} /* namespace dtn */
#endif /* NATIVEDAEMON_H_ */

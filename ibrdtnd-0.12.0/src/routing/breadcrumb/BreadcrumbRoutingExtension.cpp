/*
 * BreadcrumbRoutingExtension.cpp
 *
 */

#include "routing/breadcrumb/BreadcrumbRoutingExtension.h"

#include "routing/QueueBundleEvent.h"
#include "routing/NodeHandshakeEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include "net/ConnectionEvent.h"
#include "net/ConnectionManager.h"
#include "Configuration.h"
#include "core/BundleCore.h"
#include "core/EventDispatcher.h"
#include "core/BundleEvent.h"
#include "net/BundleReceivedEvent.h"

#include "core/TimeEvent.h"

#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>

#include <ibrdtn/utils/Clock.h>

#include <functional>
#include <list>
#include <algorithm>
#include <cmath>

#include <iomanip>
#include <ios>
#include <iostream>
#include <set>
#include <memory>

#include <stdlib.h>
#include <typeinfo>

#include <time.h>

namespace dtn
{
	namespace routing
	{
		const std::string BreadcrumbRoutingExtension::TAG = "BreadcrumbRoutingExtension";

		/*class Location
		{
			public:
				double _latitude;
				double _longitude;
				Location() : _latitude(0.0), _longitude(0.0) {};
				Location(double latitude, double longitude) : _latitude(latitude), _longitude(longitude) {};
				~Location(){};
		};

		Location nodeLocation;*/

		BreadcrumbRoutingExtension::BreadcrumbRoutingExtension() : _next_exchange_timeout(60), _next_exchange_timestamp(0), _next_loc_update_interval(15), _next_loc_update_timestamp(0)
		{
			// write something to the syslog
			IBRCOMMON_LOGGER_TAG(BreadcrumbRoutingExtension::TAG, info) << "BreadcrumbRoutingExtension()" << IBRCOMMON_LOGGER_ENDL;

			//srand(time(NULL));
			//float latitude = (rand() % 1000000)/100000;
			//float longitude = (rand() % 1000000)/100000;
			//_location._geopoint.set(latitude, longitude);
			updateMyLocation();
			IBRCOMMON_LOGGER_TAG(BreadcrumbRoutingExtension::TAG, info) << "Setting dummy location: " << _location << IBRCOMMON_LOGGER_ENDL;

			_next_exchange_timestamp = dtn::utils::Clock::getMonotonicTimestamp() + _next_exchange_timeout;
			_next_loc_update_timestamp = dtn::utils::Clock::getMonotonicTimestamp() + _next_loc_update_interval;
		}

		BreadcrumbRoutingExtension::~BreadcrumbRoutingExtension()
		{
			//delete &nodeLocation;
			join();
		}

		void BreadcrumbRoutingExtension::responseHandshake(const dtn::data::EID& neighbor, const NodeHandshake& request, NodeHandshake& response)
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "responseHandshake()" << IBRCOMMON_LOGGER_ENDL;

			if (request.hasRequest(GeoLocation::identifier))
			{
				ibrcommon::MutexLock l(_location);
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "addItem(): " << _location << " to response" << IBRCOMMON_LOGGER_ENDL;
				response.addItem(new GeoLocation(_location));
			}
		}

		void BreadcrumbRoutingExtension::processHandshake(const dtn::data::EID& neighbor, NodeHandshake& response)
		{
			/* ignore neighbors, that have our EID */
			//if (neighbor.sameHost(dtn::core::BundleCore::local)) return;
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "processHandshake()" << IBRCOMMON_LOGGER_ENDL;

			try {
				const GeoLocation& neighbor_location = response.get<GeoLocation>();

				// strip possible application part off the neighbor EID
				const dtn::data::EID neighbor_node = neighbor.getNode();

				try {
					NeighborDatabase &db = (**this).getNeighborDB();
					NeighborDataset ds(new GeoLocation(neighbor_location));
					ibrcommon::MutexLock l(db);
					db.get(neighbor_node).putDataset(ds);
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Putting in database: [" << neighbor_node.getString() << "," << neighbor_location << "]" << IBRCOMMON_LOGGER_ENDL;
				} catch (const NeighborNotAvailableException&) { };

				/* update predictability for this neighbor */
				//updateNeighbor(neighbor_node, neighbor_dp_map);

			} catch (std::exception&) { }
		}

		void BreadcrumbRoutingExtension::requestHandshake(const dtn::data::EID&, NodeHandshake &request) const
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "requestHandshake()" << IBRCOMMON_LOGGER_ENDL;
			request.addRequest(GeoLocation::identifier);
			request.addRequest(BloomFilterSummaryVector::identifier);
		}

		void BreadcrumbRoutingExtension::eventTransferCompleted(const dtn::data::EID &peer, const dtn::data::MetaBundle &meta) throw ()
		{
			//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventTransferCompleted()" << IBRCOMMON_LOGGER_ENDL;
			if (dtn::core::BundleCore::getInstance().getStorage().contains(meta)) {
				if (meta.hasgeoroute) {
					dtn::core::BundleCore::getInstance().getStorage().remove(meta);
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventTransferCompleted(): bundle removed" << IBRCOMMON_LOGGER_ENDL;
				}
			}

		}

		void BreadcrumbRoutingExtension::eventDataChanged(const dtn::data::EID &peer) throw ()
		{
			// transfer the next bundle to this destination
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventDataChanged()" << IBRCOMMON_LOGGER_ENDL;
			_taskqueue.push( new SearchNextBundleTask( peer ) );
		}

		void BreadcrumbRoutingExtension::eventBundleQueued(const dtn::data::EID &peer, const dtn::data::MetaBundle &meta) throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventBundleQueued()" << IBRCOMMON_LOGGER_ENDL;

			class UpdateBundleFilter : public dtn::storage::BundleSelector {
						public:
							UpdateBundleFilter(const GeoLocation &myloc) : _myloc(myloc) {};

							virtual ~UpdateBundleFilter() {};

							virtual dtn::data::Size limit() const throw () { return dtn::core::BundleCore::getInstance().getStorage().size(); };

							virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const throw (dtn::storage::BundleSelectorException) {

								if (meta.reacheddest) {
									IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: Destination reached" << IBRCOMMON_LOGGER_ENDL;
									return false;
								}
								if (meta.hasgeoroute) {
									IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: MetaBundle has georoute" << IBRCOMMON_LOGGER_ENDL;

									// check if the new location is the next hop to see if it's worth going through the bundle
									if (checkMargin(_myloc, meta.nextgeohop)) {
										IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: Location match found, adding to list" << IBRCOMMON_LOGGER_ENDL;
										return true;
									} else {
										IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: No location match found" << IBRCOMMON_LOGGER_ENDL;
										return false;
									}
								}
								return false;
							}
						private:
							const GeoLocation &_myloc;
						};

			if (meta.hasgeoroute) {
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventBundleQueued(): has geo route" << IBRCOMMON_LOGGER_ENDL;
				//if (peer.getNode() != dtn::core::BundleCore::local) {
					//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventBundleQueued(): not local," << peer.getHost() << IBRCOMMON_LOGGER_ENDL;
					//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Peer: " << peer.getHost() << IBRCOMMON_LOGGER_ENDL;
					//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Local: " << dtn::core::BundleCore::local.getHost() << IBRCOMMON_LOGGER_ENDL;

					try {
						const UpdateBundleFilter updateFilter(_location);
						dtn::storage::BundleResultList updateList;
						updateList.clear();
						(**this).getSeeker().get(updateFilter, updateList); // use the filter to acquire all bundles that need updates
						updateBundleList(_location, updateList);
					} catch (const dtn::storage::NoBundleFoundException &ex) {
						IBRCOMMON_LOGGER_DEBUG_TAG(TAG, 10) << "NoBundleFound from filter" << IBRCOMMON_LOGGER_ENDL;
					}
					/*dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(meta);
					dtn::data::GeoRoutingBlock &grblock = bundle.find<dtn::data::GeoRoutingBlock>();
					if (grblock.getRoute().empty()) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventBundleQueued(): The geo route block should not be empty!" << IBRCOMMON_LOGGER_ENDL;
					} else {
						grblock.getRoute().pop_back();
						dtn::core::BundleCore::getInstance().getStorage().remove(meta);
						dtn::core::BundleCore::getInstance().getStorage().store(bundle);
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "eventBundleQueued(): Popped off the last entry upon receive" << IBRCOMMON_LOGGER_ENDL;
					}*/
				//}
			}

			// new bundles trigger a recheck for all neighbors
			const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getConnectionManager().getNeighbors();

			for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); ++iter)
			{
				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current peer string: "+peer.getString() << IBRCOMMON_LOGGER_ENDL;
				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current peer host: "+peer.getHost() << IBRCOMMON_LOGGER_ENDL;

				const dtn::core::Node &n = (*iter);

				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current n string: "+n.getEID().getString() << IBRCOMMON_LOGGER_ENDL;
				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current n host: "+n.getEID().getHost() << IBRCOMMON_LOGGER_ENDL;

				if (n.getEID() != peer)
				{
					// trigger all routing modules to search for bundles to forward
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Trigger: eventDataChanged()" << IBRCOMMON_LOGGER_ENDL;
					eventDataChanged(n.getEID());
				}
			}
		}

		void BreadcrumbRoutingExtension::raiseEvent(const dtn::core::Event *evt) throw ()
		{
			//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Received Event: "+(*evt).getName() << IBRCOMMON_LOGGER_ENDL;

			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				ibrcommon::MutexLock l_ex(_next_exchange_mutex);
				const dtn::data::Timestamp now = dtn::utils::Clock::getMonotonicTimestamp();

				if ((_next_exchange_timestamp > 0) && (_next_exchange_timestamp < now))
				{
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Push: NextExchangeTask()" << IBRCOMMON_LOGGER_ENDL;

					_taskqueue.push( new NextExchangeTask() );

					// define the next exchange timestamp
					_next_exchange_timestamp = now + _next_exchange_timeout;
				}

				ibrcommon::MutexLock l_loc(_next_loc_update_mutex);
				if ((_next_loc_update_timestamp > 0) && (_next_loc_update_timestamp < now))
				{
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Push: UpdateMyLocationTask()" << IBRCOMMON_LOGGER_ENDL;

					_taskqueue.push( new UpdateMyLocationTask() );

					// define the next location update timestamp
					_next_loc_update_timestamp = now + _next_loc_update_interval;
				}

				return;
			} catch (const std::bad_cast&) { };

			try {
				const NodeHandshakeEvent &handshake = dynamic_cast<const NodeHandshakeEvent&>(*evt);

				if (handshake.state == NodeHandshakeEvent::HANDSHAKE_REPLIED)
				{
					// transfer the next bundle to this destination
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: handshake replied" << IBRCOMMON_LOGGER_ENDL;
					//_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				else if (handshake.state == NodeHandshakeEvent::HANDSHAKE_UPDATED)
				{
					// transfer the next bundle to this destination
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: handshake updated" << IBRCOMMON_LOGGER_ENDL;
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				else if (handshake.state == NodeHandshakeEvent::HANDSHAKE_COMPLETED)
				{
					// transfer the next bundle to this destination
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: handshake completed" << IBRCOMMON_LOGGER_ENDL;
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				return;
			} catch (const std::bad_cast&) { };

			/*try {
				const BundleReceivedEvent &bundleEvent = dynamic_cast<const BundleReceivedEvent&>(*evt);
				const dtn::data::MetaBundle m = dtn::data::MetaBundle::create(bundleEvent.bundle);
				dtn::data::Bundle bundle = bundleEvent.bundle;
				if (m.hasgeoroute) {
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: received geo route bundle" << IBRCOMMON_LOGGER_ENDL;
					dtn::data::GeoRoutingBlock &grblock = bundle.find<dtn::data::GeoRoutingBlock>();
					if (grblock.getRoute().empty()) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: The geo route block should not be empty!" << IBRCOMMON_LOGGER_ENDL;
						return;
					}
					grblock.getRoute().pop_back();
					dtn::core::BundleCore::getInstance().getStorage().remove(m);
					dtn::core::BundleCore::getInstance().getStorage().store(bundle);
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Event: Popped off the last entry upon receive" << IBRCOMMON_LOGGER_ENDL;
				}
			} catch (const std::bad_cast&) { };*/
		}

		void BreadcrumbRoutingExtension::componentUp() throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "componentUp()" << IBRCOMMON_LOGGER_ENDL;

			dtn::core::EventDispatcher<dtn::routing::NodeHandshakeEvent>::add(this);
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::add(this);
			//dtn::core::EventDispatcher<dtn::net::BundleReceivedEvent>::add(this);

			// reset the task queue
			_taskqueue.reset();

			// routine checked for throw() on 15.02.2013

			try {
				// run the thread
				start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_TAG(BreadcrumbRoutingExtension::TAG, error) << "componentUp failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void BreadcrumbRoutingExtension::componentDown() throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "componentDown()" << IBRCOMMON_LOGGER_ENDL;
			dtn::core::EventDispatcher<dtn::routing::NodeHandshakeEvent>::remove(this);
			dtn::core::EventDispatcher<dtn::core::TimeEvent>::remove(this);
			//dtn::core::EventDispatcher<dtn::net::BundleReceivedEvent>::remove(this);

			try {
				// stop the thread
				stop();
				join();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER_TAG(BreadcrumbRoutingExtension::TAG, error) << "componentDown failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void BreadcrumbRoutingExtension::__cancellation() throw ()
		{
			_taskqueue.abort();
		}

		void BreadcrumbRoutingExtension::run() throw ()
		{
			class BundleFilter : public dtn::storage::BundleSelector
			{
			public:
				BundleFilter(const NeighborDatabase::NeighborEntry &entry, const std::set<dtn::core::Node> &neighbors, const GeoLocation &peerloc, const GeoLocation &myloc)
				 : _entry(entry), _neighbors(neighbors), _peerloc(peerloc), _myloc(myloc)
				{};

				virtual ~BundleFilter() {};

				virtual dtn::data::Size limit() const throw () { return _entry.getFreeTransferSlots(); };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const throw (dtn::storage::BundleSelectorException)
				{

					// check Scope Control Block - do not forward bundles with hop limit == 0
					if (meta.hopcount == 0)
					{
						return false;
					}

					// do not forward local bundles
					if ((meta.destination.getNode() == dtn::core::BundleCore::local)
							&& meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)
						)
					{
						return false;
					}

					// check Scope Control Block - do not forward non-group bundles with hop limit <= 1
					if ((meta.hopcount <= 1) && (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON)))
					{
						return false;
					}

					// do not forward bundles addressed to this neighbor,
					// because this is handled by neighbor routing extension
					if (_entry.eid == meta.destination.getNode())
					{
						return false;
					}

					// if this is a singleton bundle ...
					if (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON))
					{
						const dtn::core::Node n(meta.destination.getNode());

						// do not forward the bundle if the final destination is available
						if (_neighbors.find(n) != _neighbors.end())
						{
							return false;
						}
					}

					// do not forward bundles already known by the destination
					// throws BloomfilterNotAvailableException if no filter is available or it is expired
					try {
						if (_entry.has(meta, true))
						{
							return false;
						}
					} catch (const dtn::routing::NeighborDatabase::BloomfilterNotAvailableException&) {
						throw dtn::storage::BundleSelectorException();
					}

					if (meta.reacheddest) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Destination reached" << IBRCOMMON_LOGGER_ENDL;
						return false;
					}

					// block is a GeoRoutingBlock
					if (meta.hasgeoroute)
					{
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "MetaBundle has georoute" << IBRCOMMON_LOGGER_ENDL;

						// send if closer

						float peerLat = (_peerloc._geopoint.getLatitude() - meta.nextgeohop.geopoint.getLatitude());
						float peerLong = (_peerloc._geopoint.getLongitude() - meta.nextgeohop.geopoint.getLongitude());
						float myLat = (_myloc._geopoint.getLatitude() - meta.nextgeohop.geopoint.getLatitude());
						float myLong = (_myloc._geopoint.getLongitude() - meta.nextgeohop.geopoint.getLongitude());

						if ( sqrt(peerLat*peerLat + peerLong*peerLong) < sqrt(myLat*myLat + myLong*myLong) ) {
							IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Peer is closer: Adding bundle with geo route to send list" << IBRCOMMON_LOGGER_ENDL;
							return true;
						}

						if (checkMargin(_peerloc, meta.nextgeohop)) {
							IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Peer is at location: Adding bundle with geo route to send list" << IBRCOMMON_LOGGER_ENDL;
							return true;
						} else {
							IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Not adding bundle with geo route to send list" << IBRCOMMON_LOGGER_ENDL;
							return false;
						}
					}

					// if entry's (i.e., the potential destination) location does not match,

					return true;
				};

			private:
				const NeighborDatabase::NeighborEntry &_entry;
				const std::set<dtn::core::Node> &_neighbors;
				const dtn::routing::GeoLocation &_peerloc;
				const dtn::routing::GeoLocation &_myloc;
			};

			class UpdateBundleFilter : public dtn::storage::BundleSelector {
			public:
				UpdateBundleFilter(const GeoLocation &myloc) : _myloc(myloc) {};

				virtual ~UpdateBundleFilter() {};

				virtual dtn::data::Size limit() const throw () { return dtn::core::BundleCore::getInstance().getStorage().size(); };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const throw (dtn::storage::BundleSelectorException) {

					if (meta.reacheddest) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: Destination reached" << IBRCOMMON_LOGGER_ENDL;
						return false;
					}
					if (meta.hasgeoroute) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: MetaBundle has georoute" << IBRCOMMON_LOGGER_ENDL;

						// check if the new location is the next hop to see if it's worth going through the bundle
						if (checkMargin(_myloc, meta.nextgeohop)) {
							IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: Location match found, adding to list" << IBRCOMMON_LOGGER_ENDL;
							return true;
						} else {
							IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "UpdateFilter: No location match found" << IBRCOMMON_LOGGER_ENDL;
							return false;
						}
					}
					return false;
				}
			private:
				const GeoLocation &_myloc;
			};

			// list for bundles
			dtn::storage::BundleResultList list;

			// set of known neighbors
			std::set<dtn::core::Node> neighbors;

			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					std::auto_ptr<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Processing: " << t->toString() << IBRCOMMON_LOGGER_ENDL;

					try {
						/**
						 * SearchNextBundleTask triggers a search for a bundle to transfer
						 * to another host. This Task is generated by TransferCompleted, TransferAborted
						 * and node events.
						 */
						try {
							SearchNextBundleTask &task = dynamic_cast<SearchNextBundleTask&>(*t);

							// lock the neighbor database while searching for bundles
							try {
								NeighborDatabase &db = (**this).getNeighborDB();
								ibrcommon::MutexLock l(db);
								NeighborDatabase::NeighborEntry &entry = db.get(task.eid, true);

								// check if enough transfer slots available (threshold reached)
								if (!entry.isTransferThresholdReached())
									throw NeighborDatabase::NoMoreTransfersAvailable();

								if (dtn::daemon::Configuration::getInstance().getNetwork().doPreferDirect()) {
									// get current neighbor list
									//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "get neighbor list" << IBRCOMMON_LOGGER_ENDL;
									neighbors = dtn::core::BundleCore::getInstance().getConnectionManager().getNeighbors();
								} else {
									// "prefer direct" option disabled - clear the list of neighbors
									neighbors.clear();
								}

								// GeoLocation query
								const GeoLocation &gl = entry.getDataset<GeoLocation>();

								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Found location: [" << task.eid.getString() << "," << gl << "]" << IBRCOMMON_LOGGER_ENDL;

								// get the bundle filter of the neighbor
								const BundleFilter filter(entry, neighbors, gl, _location);

								// query some unknown bundle from the storage
								list.clear();
								(**this).getSeeker().get(filter, list);
								if (list.empty()) {
									IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "List is empty!" << IBRCOMMON_LOGGER_ENDL;
								}
							} catch (const NeighborDatabase::DatasetNotAvailableException&) {
								// if there is no GeoLocation for this peer do handshake with them
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "No GeoLocation available from " << task.eid.getString() << ", triggering doHandshake()" << IBRCOMMON_LOGGER_ENDL;
								(**this).doHandshake(task.eid);
							} catch (const dtn::storage::BundleSelectorException&) {
								// query a new summary vector from this neighbor
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "No summary vector available from " << task.eid.getString() << ", triggering doHandshake()" << IBRCOMMON_LOGGER_ENDL;
								(**this).doHandshake(task.eid);
							}

							// send the bundles as long as we have resources
							for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); ++iter)
							{
								try {
									IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer bundle" << IBRCOMMON_LOGGER_ENDL;
									/*if ((*iter).hasgeoroute) {
										dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(*iter);
										IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updating bundle at transfer point" << IBRCOMMON_LOGGER_ENDL;
										try {
											dtn::data::GeoRoutingBlock &grblock = bundle.find<dtn::data::GeoRoutingBlock>();
											// check if there are entries left
											if (grblock.getRoute().empty()) {
												IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer: No more georoute entries for this bundle, but there should be!" << IBRCOMMON_LOGGER_ENDL;
												continue;
											}
											IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer: Popping last entry prior to transfer" << IBRCOMMON_LOGGER_ENDL;
											grblock.getRoute().pop_back();
											IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer: Removing outdated bundle and replacing it in storage" << IBRCOMMON_LOGGER_ENDL;
											dtn::core::BundleCore::getInstance().getStorage().remove(*iter);
											dtn::core::BundleCore::getInstance().getStorage().store(bundle);
											IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer: Successfully updated bundle" << IBRCOMMON_LOGGER_ENDL;
										} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) {
											IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Transfer: No georouting block, but flag is set. Should not be here!" << IBRCOMMON_LOGGER_ENDL;
										}
									}*/
									// transfer the bundle to the neighbor
									transferTo(task.eid, *iter);
								} catch (const NeighborDatabase::AlreadyInTransitException&) { };
							}
						} catch (const NeighborDatabase::NoMoreTransfersAvailable &ex) {
							IBRCOMMON_LOGGER_DEBUG_TAG(TAG, 10) << "task " << t->toString() << " aborted: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
						} catch (const NeighborDatabase::NeighborNotAvailableException &ex) {
							IBRCOMMON_LOGGER_DEBUG_TAG(TAG, 10) << "task " << t->toString() << " aborted: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
						} catch (const dtn::storage::NoBundleFoundException &ex) {
							IBRCOMMON_LOGGER_DEBUG_TAG(TAG, 10) << "task " << t->toString() << " aborted: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
						} catch (const std::bad_cast&) { };

						try {
							NextExchangeTask &task =  dynamic_cast<NextExchangeTask&>(*t);

							std::set<dtn::core::Node> neighbors = dtn::core::BundleCore::getInstance().getConnectionManager().getNeighbors();
							std::set<dtn::core::Node>::const_iterator it;
							for(it = neighbors.begin(); it != neighbors.end(); ++it)
							{
								try{
									(**this).doHandshake(it->getEID());
								} catch (const ibrcommon::Exception &ex) { }
							}
						} catch (const std::bad_cast&) { }

						try {
							UpdateMyLocationTask &task = dynamic_cast<UpdateMyLocationTask&>(*t);
							updateMyLocation();
							const UpdateBundleFilter updateFilter(_location);
							dtn::storage::BundleResultList updateList;
							updateList.clear();
							(**this).getSeeker().get(updateFilter, updateList); // use the filter to acquire all bundles that need updates
							updateBundleList(_location, updateList);
						} catch (const dtn::storage::NoBundleFoundException &ex) {
							IBRCOMMON_LOGGER_DEBUG_TAG(TAG, 10) << "NoBundleFound from filter" << IBRCOMMON_LOGGER_ENDL;
						} catch (const std::bad_cast&) { }

					} catch (const ibrcommon::Exception &ex) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 20) << "task failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
				} catch (const std::exception &ex) {
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 15) << "terminated due to " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					return;
				}

				yield();
			}
		}

		void BreadcrumbRoutingExtension::updateBundleList(GeoLocation myloc, dtn::storage::BundleResultList bundleList) {
			for (std::list<dtn::data::MetaBundle>::const_iterator iter = bundleList.begin(); iter != bundleList.end(); ++iter) {
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update bundle based on location" << IBRCOMMON_LOGGER_ENDL;
				if ((*iter).hasgeoroute) {
					dtn::data::Bundle bundle = dtn::core::BundleCore::getInstance().getStorage().get(*iter);
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updating bundle during location update" << IBRCOMMON_LOGGER_ENDL;
					try {
						dtn::data::GeoRoutingBlock &grblock = bundle.find<dtn::data::GeoRoutingBlock>();

						bool donePruningEntries = false;
						// NOTE: in this case the first lookup is redundant since the bundle would not be in this list if it did not need at least 1 pop
						while (!donePruningEntries) {
							// look at the last entry
							dtn::data::GeoRoutingBlock::GeoRoutingEntry nextgeohop = grblock.getRoute().back();
							if (checkMargin(myloc,nextgeohop)) {
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: Popping entry" << IBRCOMMON_LOGGER_ENDL;
								grblock.getRoute().pop_back();
							} else {
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: Done popping entries" << IBRCOMMON_LOGGER_ENDL;
								donePruningEntries = true;
							}

							if (grblock.getRoute().empty()) {
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: No more entries, done popping" << IBRCOMMON_LOGGER_ENDL;
								donePruningEntries = true;
							} else {
								dtn::data::GeoRoutingBlock::GeoRoutingEntry nextgeohop = grblock.getRoute().back();
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: Iterating to nextgeohop" << IBRCOMMON_LOGGER_ENDL;
							}
						}
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: Removing outdated bundle and replacing it in storage" << IBRCOMMON_LOGGER_ENDL;
						dtn::core::BundleCore::getInstance().getStorage().remove(*iter);
						dtn::core::BundleCore::getInstance().getStorage().store(bundle);

					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) {
						IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Update: No georouting block, but flag is set. Should not be here!" << IBRCOMMON_LOGGER_ENDL;
					}
				}
			}
		}

		bool BreadcrumbRoutingExtension::checkMargin(const dtn::routing::GeoLocation &base_location, const dtn::data::GeoRoutingBlock::GeoRoutingEntry &bundle_location)
		{
			float margin = bundle_location.getMargin();

			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current location: (" << base_location._geopoint.getLatitude() << "," << base_location._geopoint.getLongitude() << ")"<< IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Bundle location: (" << bundle_location.geopoint.getLatitude() << "," << bundle_location.geopoint.getLongitude() << ")"<< IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Margin: " << margin << IBRCOMMON_LOGGER_ENDL;

			if ((abs(base_location._geopoint.getLatitude() - bundle_location.geopoint.getLatitude()) < margin)
					&& (abs(base_location._geopoint.getLongitude() - bundle_location.geopoint.getLongitude()) < margin)) {
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "New location is in appropriate range of entry" << IBRCOMMON_LOGGER_ENDL;

				return true;
			} else {
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Peer NOT in range" << IBRCOMMON_LOGGER_ENDL;
				return false;
			}
		}

		void BreadcrumbRoutingExtension::updateMyLocation() {
			std::string filename = "/usr/local/etc/vmt_gps_coord.txt";
			std::ifstream ifs;
			ifs.open("/usr/local/etc/vmt_gps_coord.txt");
			if (!ifs.is_open()) {
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Could not open vmt_gps_coord file" << IBRCOMMON_LOGGER_ENDL;
				return;
			}
			ifs.seekg(-1,ios_base::end);

			char cur = '\0';
			while( cur==EOF || cur=='\0' || cur=='\n' ) {
				if ((int)ifs.tellg() <= 1) {
					//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "File has no location entries" << IBRCOMMON_LOGGER_ENDL;
					return;
				}
				ifs.get(cur);
				ifs.seekg(-2,ios_base::cur);
			}

			int end = (int)ifs.tellg()+1;
			ifs.seekg(0,ios_base::beg);

			while( cur!='\n' ) {
				if ((int)ifs.tellg() <= 0) {
					ifs.seekg(0,ios_base::beg);
					break;
				}
				ifs.get(cur);
				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Pos2: " << (int)ifs.tellg() << IBRCOMMON_LOGGER_ENDL;
				//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Cur2: " << cur << IBRCOMMON_LOGGER_ENDL;
				ifs.seekg(-2,ios_base::cur);
			}

			int start = ifs.tellg();
			char *entry = new char[end-start+1];
			if (cur==EOF || cur=='\0' || cur=='\n') {
				ifs.seekg(1,ios_base::cur);
			}

			ifs.read(entry,end-start); // read last line from file
			entry[end-start] = 0x00;
			std::string s(entry);
			delete [] entry;
			ifs.close();

			//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "full string to start: "+s << IBRCOMMON_LOGGER_ENDL;
			s = s.substr(0,s.size()-1); // strip off EOF char from string
			size_t pos = 0;
			std::string latitude;
			std::string longitude;
			std::string tstamp;
			std::string delim = ";";

			// prune timestamp
			pos = s.find(delim);
			tstamp = s.substr(0,pos);
			s.erase(0,pos+delim.length());

			// get latitude
			pos = s.find(delim);
			latitude = s.substr(0,pos);

			// get longitude
			s.erase(0,pos+delim.length());
			longitude = s;

			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "GPS timestamp: "+tstamp << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updated latitude: "+latitude << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updated longitude: "+longitude << IBRCOMMON_LOGGER_ENDL;

			//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updated latitude(as float): " << ::atof(latitude.c_str()) << IBRCOMMON_LOGGER_ENDL;
			//IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updated longitude(as float): " << ::atof(longitude.c_str()) << IBRCOMMON_LOGGER_ENDL;

			_location._geopoint.set(::atof(latitude.c_str()), ::atof(longitude.c_str()));
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Updated location (as float): " << _location << IBRCOMMON_LOGGER_ENDL;

		}

		/****************************************/

		BreadcrumbRoutingExtension::SearchNextBundleTask::SearchNextBundleTask(const dtn::data::EID &e)
		 : eid(e)
		{ }

		BreadcrumbRoutingExtension::SearchNextBundleTask::~SearchNextBundleTask()
		{ }

		std::string BreadcrumbRoutingExtension::SearchNextBundleTask::toString()
		{
			return "SearchNextBundleTask: " + eid.getString();
		}

		BreadcrumbRoutingExtension::NextExchangeTask::NextExchangeTask()
		{
		}

		BreadcrumbRoutingExtension::NextExchangeTask::~NextExchangeTask()
		{
		}

		std::string BreadcrumbRoutingExtension::NextExchangeTask::toString()
		{
			return "NextExchangeTask";
		}

		BreadcrumbRoutingExtension::UpdateMyLocationTask::UpdateMyLocationTask()
		{
		}

		BreadcrumbRoutingExtension::UpdateMyLocationTask::~UpdateMyLocationTask()
		{
		}

		std::string BreadcrumbRoutingExtension::UpdateMyLocationTask::toString()
		{
			return "UpdateMyLocationTask";
		}
	}
}

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

#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>

#include <functional>
#include <list>
#include <algorithm>

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

		BreadcrumbRoutingExtension::BreadcrumbRoutingExtension()
		{
			// write something to the syslog
			IBRCOMMON_LOGGER_TAG(BreadcrumbRoutingExtension::TAG, info) << "Initializing breadcrumb routing module" << IBRCOMMON_LOGGER_ENDL;
			srand(time(NULL));
			_location._latitude = rand() % 10;
			_location._longitude = rand() % 10;
		}

		BreadcrumbRoutingExtension::~BreadcrumbRoutingExtension()
		{
			//delete &nodeLocation;
			join();
		}

		void BreadcrumbRoutingExtension::responseHandshake(const dtn::data::EID& neighbor, const NodeHandshake& request, NodeHandshake& response)
		{
			if (request.hasRequest(GeoLocation::identifier))
			{
				ibrcommon::MutexLock l(_location);
				response.addItem(new GeoLocation(_location));
			}
		}

		void BreadcrumbRoutingExtension::processHandshake(const dtn::data::EID& neighbor, NodeHandshake& response)
		{
			/* ignore neighbors, that have our EID */
			//if (neighbor.sameHost(dtn::core::BundleCore::local)) return;
			try {
				const GeoLocation& neighbor_location = response.get<GeoLocation>();

				// strip possible application part off the neighbor EID
				const dtn::data::EID neighbor_node = neighbor.getNode();

				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "location received from " << neighbor_node.getString() << IBRCOMMON_LOGGER_ENDL;


			} catch (std::exception&) { }
		}

		void BreadcrumbRoutingExtension::requestHandshake(const dtn::data::EID&, NodeHandshake &request) const
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "request handshake" << IBRCOMMON_LOGGER_ENDL;

			request.addRequest(GeoLocation::identifier);
			request.addRequest(BloomFilterSummaryVector::identifier);
		}

		void BreadcrumbRoutingExtension::eventDataChanged(const dtn::data::EID &peer) throw ()
		{
			// transfer the next bundle to this destination
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "event data changed" << IBRCOMMON_LOGGER_ENDL;
			_taskqueue.push( new SearchNextBundleTask( peer ) );
		}

		void BreadcrumbRoutingExtension::eventBundleQueued(const dtn::data::EID &peer, const dtn::data::MetaBundle &meta) throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "event bundle queued" << IBRCOMMON_LOGGER_ENDL;

			// new bundles trigger a recheck for all neighbors
			const std::set<dtn::core::Node> nl = dtn::core::BundleCore::getInstance().getConnectionManager().getNeighbors();

			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Looping through peers" << IBRCOMMON_LOGGER_ENDL;

			for (std::set<dtn::core::Node>::const_iterator iter = nl.begin(); iter != nl.end(); ++iter)
			{
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current peer string: "+peer.getString() << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current peer host: "+peer.getHost() << IBRCOMMON_LOGGER_ENDL;



				const dtn::core::Node &n = (*iter);

				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current n string: "+n.getEID().getString() << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "Current n host: "+n.getEID().getHost() << IBRCOMMON_LOGGER_ENDL;


				if (n.getEID() != peer)
				{
					// trigger all routing modules to search for bundles to forward
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "trigger event data changed" << IBRCOMMON_LOGGER_ENDL;
					eventDataChanged(n.getEID());
				}
			}
		}

		void BreadcrumbRoutingExtension::raiseEvent(const dtn::core::Event *evt) throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "received event" << IBRCOMMON_LOGGER_ENDL;
			try {
				const NodeHandshakeEvent &handshake = dynamic_cast<const NodeHandshakeEvent&>(*evt);

				IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "received a node handshake event" << IBRCOMMON_LOGGER_ENDL;

				if (handshake.state == NodeHandshakeEvent::HANDSHAKE_UPDATED)
				{
					// transfer the next bundle to this destination
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "handshake update" << IBRCOMMON_LOGGER_ENDL;
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				else if (handshake.state == NodeHandshakeEvent::HANDSHAKE_COMPLETED)
				{
					// transfer the next bundle to this destination
					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "handshake complete" << IBRCOMMON_LOGGER_ENDL;
					_taskqueue.push( new SearchNextBundleTask( handshake.peer ) );
				}
				return;
			} catch (const std::bad_cast&) { };
		}

		void BreadcrumbRoutingExtension::componentUp() throw ()
		{
			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "componentUp() start (4)" << IBRCOMMON_LOGGER_ENDL;

			dtn::core::EventDispatcher<dtn::routing::NodeHandshakeEvent>::add(this);

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
			dtn::core::EventDispatcher<dtn::routing::NodeHandshakeEvent>::remove(this);

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
				BundleFilter(const NeighborDatabase::NeighborEntry &entry, const std::set<dtn::core::Node> &neighbors)
				 : _entry(entry), _neighbors(neighbors)
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

					// if entry's (i.e., the potential destination) location does not match,

					return true;
				};

			private:
				const NeighborDatabase::NeighborEntry &_entry;
				const std::set<dtn::core::Node> &_neighbors;
			};

			// list for bundles
			dtn::storage::BundleResultList list;

			// set of known neighbors
			std::set<dtn::core::Node> neighbors;

			IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "before while(true) " << IBRCOMMON_LOGGER_ENDL;

			while (true)
			{
				try {
					Task *t = _taskqueue.getnpop(true);
					std::auto_ptr<Task> killer(t);

					IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "processing task " << t->toString() << IBRCOMMON_LOGGER_ENDL;

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
									IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 1) << "get neighbor list" << IBRCOMMON_LOGGER_ENDL;
									neighbors = dtn::core::BundleCore::getInstance().getConnectionManager().getNeighbors();
								} else {
									// "prefer direct" option disabled - clear the list of neighbors
									neighbors.clear();
								}

								// get the bundle filter of the neighbor
								const BundleFilter filter(entry, neighbors);

								// some debug output
								IBRCOMMON_LOGGER_DEBUG_TAG(BreadcrumbRoutingExtension::TAG, 40) << "search some bundles not known by " << task.eid.getString() << IBRCOMMON_LOGGER_ENDL;

								// query some unknown bundle from the storage
								list.clear();
								(**this).getSeeker().get(filter, list);
							} catch (const dtn::storage::BundleSelectorException&) {
								// query a new summary vector from this neighbor
								(**this).doHandshake(task.eid);
							}

							// send the bundles as long as we have resources
							for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); ++iter)
							{
								try {
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
	}
}

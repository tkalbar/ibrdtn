/*
 * BreadcrumbRoutingExtension.h
 *
 */

#ifndef BREADCRUMBROUTINGEXTENSION_H_
#define BREADCRUMBROUTINGEXTENSION_H_

#include "core/Node.h"
#include "core/AbstractWorker.h"
#include "core/EventReceiver.h"

#include "routing/RoutingExtension.h"
#include "routing/NeighborDatabase.h"
#include "routing/breadcrumb/GeoLocation.h"

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/ExtensionBlock.h>
#include <ibrdtn/data/GeoRoutingBlock.h>

#include <ibrcommon/thread/Queue.h>
#include <ibrcommon/thread/Thread.h>

#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class BreadcrumbRoutingExtension : public RoutingExtension, public ibrcommon::JoinableThread, public dtn::core::EventReceiver
		{
			static const std::string TAG;

		public:

			GeoLocation _location;

			ibrcommon::Mutex _next_exchange_mutex; ///< Mutex for the _next_exchange_timestamp.
			dtn::data::Timestamp _next_exchange_timeout; ///< Interval in seconds how often Handshakes should be executed on longer connections.
			dtn::data::Timestamp _next_exchange_timestamp; ///< Unix timestamp, when the next handshake is due.

			BreadcrumbRoutingExtension();
			virtual ~BreadcrumbRoutingExtension();

			virtual void eventDataChanged(const dtn::data::EID &peer) throw ();

			virtual void eventBundleQueued(const dtn::data::EID &peer, const dtn::data::MetaBundle &meta) throw ();

			void raiseEvent(const dtn::core::Event *evt) throw ();
			void componentUp() throw ();
			void componentDown() throw ();

			bool checkMargin(const dtn::routing::GeoLocation &, const dtn::data::GeoRoutingBlock::GeoRoutingEntry &) const;

			/**
			 * @see BaseRouter::requestHandshake()
			 */
			virtual void requestHandshake(const dtn::data::EID&, NodeHandshake&) const;

			virtual void responseHandshake(const dtn::data::EID&, const NodeHandshake&, NodeHandshake&);

			virtual void processHandshake(const dtn::data::EID&, NodeHandshake&);



		protected:
			void run() throw ();
			void __cancellation() throw ();

		private:
			class Task
			{
			public:
				virtual ~Task() {};
				virtual std::string toString() = 0;
			};

			class SearchNextBundleTask : public Task
			{
			public:
				SearchNextBundleTask(const dtn::data::EID &eid);
				virtual ~SearchNextBundleTask();

				virtual std::string toString();

				const dtn::data::EID eid;
			};

			class NextExchangeTask : public Task
			{
			public:
				NextExchangeTask();
				virtual ~NextExchangeTask();

				virtual std::string toString();
			};

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::Queue<BreadcrumbRoutingExtension::Task* > _taskqueue;
		};
	}
}

#endif /* BREADCRUMBROUTINGEXTENSION_H_ */

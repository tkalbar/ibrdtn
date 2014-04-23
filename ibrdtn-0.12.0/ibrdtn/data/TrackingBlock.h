/*
 * TrackingBlock.h
 *
 */

#ifndef TRACKINGBLOCK_H_
#define TRACKINGBLOCK_H_

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/Number.h>
#include <ibrdtn/data/DTNTime.h>
#include <ibrdtn/data/GeoPoint.h>
#include <ibrdtn/data/ExtensionBlock.h>

namespace dtn
{
	namespace data
	{
		class TrackingBlock : public dtn::data::Block
		{
		public:
			class Factory : public dtn::data::ExtensionBlock::Factory
			{
			public:
				Factory() : dtn::data::ExtensionBlock::Factory(TrackingBlock::BLOCK_TYPE) {};
				virtual ~Factory() {};
				virtual dtn::data::Block* create();
			};

			static const dtn::data::block_t BLOCK_TYPE;

			// handling flags
			enum FLAGS
			{
				TRACK_HOPS      = 1 << 1,
				TRACK_GEO       = 1 << 2,
				TRACK_TIMESTAMP = 1 << 3
			};
			Bitset<FLAGS> procflags;

			// the minimum interval on which to track geo data (seconds).
			// if coarser info is available, take what you can get
			// if finer info is available only take it this often
			Number tracking_interval;

			TrackingBlock();
			TrackingBlock(int track_hops, int track_geo, int tr_intvl);
			virtual ~TrackingBlock();

			bool getFlag(FLAGS f) const;
			void setFlag(FLAGS f, bool value);

			void finalizeEntryList();
			virtual Length getLength() const;
			virtual std::ostream &serialize(std::ostream &stream, Length &length) const;
			virtual std::istream &deserialize(std::istream &stream, const Length &length);

			virtual std::ostream &serialize_strict(std::ostream &stream, Length &length) const;
			virtual Length getLength_strict() const;

			class TrackingEntry
			{
			public:
				/*
				enum FLAGS
				{
					TIMESTAMP_PRESENT = 1,
					EID_PRESENT = 2,
					GEODATA_PRESENT = 4
				};
				Bitset<FLAGS> flags;
				*/

				enum ENTRYTYPE
				{
					GEODATA = 1,
					HOPDATA = 2
				};
				Number entry_type;

				TrackingEntry();
				TrackingEntry(const dtn::data::EID &eid);
				TrackingEntry(float lat, float lon);
				TrackingEntry(const dtn::data::EID &eid, float lat, float lon);
				~TrackingEntry();

				dtn::data::EID endpoint;
				dtn::data::Timestamp timestamp;
				dtn::data::GeoPoint geopoint;

				friend std::ostream& operator<<(std::ostream &stream, const TrackingEntry &entry);
				friend std::istream& operator>>(std::istream &stream, TrackingEntry &entry);

				Length getLength() const;
			};

			typedef std::list<TrackingEntry> tracking_list;

			const tracking_list& getTrack() const;

			// append a HOPDATA TrackingEntry
			void append(const dtn::data::EID &eid);

			// append a GEODATA TrackingEntry
			void append(float lat, float lon);

		private:
			tracking_list _entries;
		};

		/**
		 * This creates a static block factory
		 */
		static TrackingBlock::Factory __TrackingBlockFactory__;
	} /* namespace data */
} /* namespace dtn */
#endif /* TRACKINGBLOCK_H_ */

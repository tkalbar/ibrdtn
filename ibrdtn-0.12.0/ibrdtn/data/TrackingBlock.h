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
				TRACK_HOPS = 1 << 0x01,
				TRACK_GEO = 1 << 0x02
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

			virtual Length getLength() const;
			virtual std::ostream &serialize(std::ostream &stream, Length &length) const;
			virtual std::istream &deserialize(std::istream &stream, const Length &length);

			virtual std::ostream &serialize_strict(std::ostream &stream, Length &length) const;
			virtual Length getLength_strict() const;

			class TrackingEntry
			{
			public:
				enum FLAGS
				{
					TIMESTAMP_PRESENT = 1,
					EID_PRESENT = 2,
					GEODATA_PRESENT = 4
				};
				Bitset<FLAGS> flags;

				TrackingEntry();
				TrackingEntry(const dtn::data::EID &eid);
				TrackingEntry(float lat, float lon);
				TrackingEntry(const dtn::data::EID &eid, float lat, float lon);
				~TrackingEntry();

				bool getFlag(FLAGS f) const;
				void setFlag(FLAGS f, bool value);

				dtn::data::EID endpoint;
				dtn::data::DTNTime timestamp;
				dtn::data::GeoPoint geopoint;

				friend std::ostream& operator<<(std::ostream &stream, const TrackingEntry &entry);
				friend std::istream& operator>>(std::istream &stream, TrackingEntry &entry);

				Length getLength() const;
			};

			typedef std::list<TrackingEntry> tracking_list;

			const tracking_list& getTrack() const;

			void append(const dtn::data::EID &eid);

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

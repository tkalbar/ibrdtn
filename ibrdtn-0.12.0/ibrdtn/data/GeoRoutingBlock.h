/*
 * GeoRoutingBlock.h
 *
 */

#ifndef GEOROUTINGBLOCK_H_
#define GEOROUTINGBLOCK_H_

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/Number.h>
#include <ibrdtn/data/DTNTime.h>
#include <ibrdtn/data/GeoPoint.h>
#include <ibrdtn/data/ExtensionBlock.h>

namespace dtn
{
	namespace data
	{
		class GeoRoutingBlock : public dtn::data::Block
		{
		public:
			class Factory : public dtn::data::ExtensionBlock::Factory
			{
			public:
				Factory() : dtn::data::ExtensionBlock::Factory(GeoRoutingBlock::BLOCK_TYPE) {};
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

			GeoRoutingBlock();
			GeoRoutingBlock(int track_hops, int track_geo, int tr_intvl);
			virtual ~GeoRoutingBlock();

			bool getFlag(FLAGS f) const;
			void setFlag(FLAGS f, bool value);

			virtual Length getLength() const;
			virtual std::ostream &serialize(std::ostream &stream, Length &length) const;
			virtual std::istream &deserialize(std::istream &stream, const Length &length);

			virtual std::ostream &serialize_strict(std::ostream &stream, Length &length) const;
			virtual Length getLength_strict() const;

			class GeoRoutingEntry
			{
			public:
				enum FLAGS
				{
					TIMESTAMP_PRESENT = 1,
					GEODATA_PRESENT = 2
				};
				Bitset<FLAGS> flags;

				GeoRoutingEntry();
				GeoRoutingEntry(const dtn::data::EID &eid);
				~GeoRoutingEntry();

				bool getFlag(FLAGS f) const;
				void setFlag(FLAGS f, bool value);

				dtn::data::EID endpoint;
				dtn::data::DTNTime timestamp;
				dtn::data::GeoPoint geopoint;

				friend std::ostream& operator<<(std::ostream &stream, const GeoRoutingEntry &entry);
				friend std::istream& operator>>(std::istream &stream, GeoRoutingEntry &entry);

				Length getLength() const;
			};

			typedef std::list<GeoRoutingEntry> tracking_list;

			const tracking_list& getTrack() const;

			void append(const dtn::data::EID &eid);

		private:
			tracking_list _entries;
		};

		/**
		 * This creates a static block factory
		 */
		static GeoRoutingBlock::Factory __GeoRoutingBlockFactory__;
	} /* namespace data */
} /* namespace dtn */
#endif /* GEOROUTINGBLOCK_H_ */

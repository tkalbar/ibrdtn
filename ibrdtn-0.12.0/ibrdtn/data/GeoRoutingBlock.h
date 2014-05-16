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
				UNUSED          = 1 << 1
			};
			Bitset<FLAGS> procflags;

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
					REQUIRED        = 1 << 0,
					ORDERED         = 1 << 1,
					GEO_REQUIRED    = 1 << 2,
					EID_REQUIRED    = 1 << 3
				};
				Bitset<FLAGS> flags;

				GeoRoutingEntry();
				//GeoRoutingEntry(const dtn::data::EID &eid);
				//GeoRoutingEntry(float lat, float lon);
				//GeoRoutingEntry(const dtn::data::EID &eid, float lat, float lon);
				~GeoRoutingEntry();

				bool getFlag(FLAGS f) const;
				void setFlag(FLAGS f, bool value);

				// the geographic point the data must pass by
				dtn::data::GeoPoint geopoint;

				// the eid of the node the bundle must pass through
				dtn::data::EID eid;


				friend std::ostream& operator<<(std::ostream &stream, const GeoRoutingEntry &entry);
				friend std::istream& operator>>(std::istream &stream, GeoRoutingEntry &entry);

				Length getLength() const;

			private:
				// how near the target point the bundle has to get
				// this is actually representing a float, but we scale it up by 1024*2014 and represent as an SDNV
				Number margin;
				float _scale_factor;

			public:
				float getMargin() const {
					float mar = margin.get();
					return mar/_scale_factor;
				}
				void setMargin(float f) { margin = (int)(_scale_factor*f); }


			};

			typedef std::list<GeoRoutingEntry> tracking_list;

			tracking_list& getRoute();
			const tracking_list& getRoute() const;

			void append(const dtn::data::EID &eid);
			void append(float lat, float lon, float margin);
			void append(const dtn::data::EID &eid, float lat, float lon, float margin);

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

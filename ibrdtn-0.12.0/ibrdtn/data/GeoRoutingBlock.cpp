/*
 * GeoRoutingBlock.cpp
 *
 */

#include "ibrdtn/data/GeoRoutingBlock.h"
#include "ibrdtn/data/BundleString.h"

namespace dtn
{
	namespace data
	{
		const dtn::data::block_t GeoRoutingBlock::BLOCK_TYPE = 194;

		dtn::data::Block* GeoRoutingBlock::Factory::create()
		{
			return new GeoRoutingBlock();
		}

		GeoRoutingBlock::GeoRoutingBlock()
		 : dtn::data::Block(GeoRoutingBlock::BLOCK_TYPE)
		{
			// set the replicate in every fragment bit
			set(REPLICATE_IN_EVERY_FRAGMENT, true);
		}

		GeoRoutingBlock::GeoRoutingBlock(int track_hops, int track_geo, int tr_intvl)
		: dtn::data::Block(GeoRoutingBlock::BLOCK_TYPE)
		{
		}


		GeoRoutingBlock::~GeoRoutingBlock()
		{
		}

		bool GeoRoutingBlock::getFlag(GeoRoutingBlock::FLAGS f) const
		{
			return procflags.getBit(f);
		}

		void GeoRoutingBlock::setFlag(GeoRoutingBlock::FLAGS f, bool value)
		{
			procflags.setBit(f, value);
		}

		Length GeoRoutingBlock::getLength() const
		{
			Length ret = 0;

			// number of elements
			dtn::data::Number count(_entries.size());
			ret += count.getLength();

			// NOTE!!!
			// in order to compute this we need to update the tracking_list
			// and then whatever we return has to remain accurate when someone serializes the block
			// tricky tricky tricky

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				const GeoRoutingEntry &entry = (*iter);
				ret += entry.getLength();
			}

			return ret;
		}

		std::ostream& GeoRoutingBlock::serialize(std::ostream &stream, Length&) const
		{
			// the flags which are currently unused
			stream << procflags;

			// number of elements
			dtn::data::Number count(_entries.size());
			stream << count;
			cout << "serializing GeoRoutingBlock with number of entries: " << count.get() << endl;

			int i=0;
			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				const GeoRoutingEntry &entry = (*iter);
				stream << entry;
				cout << "serializing GeoRoutingEntry " << i++ << endl;
			}

			return stream;
		}

		std::istream& GeoRoutingBlock::deserialize(std::istream &stream, const Length&)
		{
			// the flags which are currently unused
			stream >> procflags;

			// number of elements
			dtn::data::Number count;
			stream >> count;
			cout << "received GeoRoutingBlock with number of entries: " << count.get() << endl;

			for (Number i = 0; i < count; ++i)
			{
				GeoRoutingEntry entry;
				stream >> entry;
				_entries.push_back(entry);
				cout << "read GeoRoutingEntry " << i.get() << endl;
			}

			return stream;
		}

		Length GeoRoutingBlock::getLength_strict() const
		{
			return getLength();
		}

		std::ostream& GeoRoutingBlock::serialize_strict(std::ostream &stream, Length &length) const
		{
			return serialize(stream, length);
		}

		GeoRoutingBlock::tracking_list& GeoRoutingBlock::getRoute()
		{
			return _entries;
		}

		const GeoRoutingBlock::tracking_list& GeoRoutingBlock::getRoute() const
		{
			return _entries;
		}


		GeoRoutingBlock::GeoRoutingEntry::GeoRoutingEntry()
		: _scale_factor(1048576)
		{
		}

		void GeoRoutingBlock::append(const dtn::data::EID &eid)
		{
			GeoRoutingEntry entry;

			entry.setFlag(GeoRoutingBlock::GeoRoutingEntry::EID_REQUIRED, true);
			entry.eid = eid;
			_entries.push_back(entry);
		}

		void GeoRoutingBlock::append(float lat, float lon, int margin)
		{
			GeoRoutingEntry entry;

			entry.setFlag(GeoRoutingBlock::GeoRoutingEntry::GEO_REQUIRED, true);
			entry.geopoint.set(lat,lon);
			entry.setMargin(margin);
			_entries.push_back(entry);
		}

		void GeoRoutingBlock::append(const dtn::data::EID &eid, float lat, float lon, int margin)
		{
			GeoRoutingEntry entry;

			entry.setFlag(GeoRoutingBlock::GeoRoutingEntry::EID_REQUIRED, true);
			entry.eid = eid;
			entry.setFlag(GeoRoutingBlock::GeoRoutingEntry::GEO_REQUIRED, true);
			entry.geopoint.set(lat,lon);
			entry.setMargin(margin);
			_entries.push_back(entry);
		}

		GeoRoutingBlock::GeoRoutingEntry::~GeoRoutingEntry()
		{
		}

		bool GeoRoutingBlock::GeoRoutingEntry::getFlag(GeoRoutingBlock::GeoRoutingEntry::FLAGS f) const
		{
			return flags.getBit(f);
		}

		void GeoRoutingBlock::GeoRoutingEntry::setFlag(GeoRoutingBlock::GeoRoutingEntry::FLAGS f, bool value)
		{
			flags.setBit(f, value);
		}

		Length GeoRoutingBlock::GeoRoutingEntry::getLength() const
		{
			Length ret = flags.getLength();
			ret += margin.getLength();
			if (getFlag(GeoRoutingEntry::EID_REQUIRED)) {
				ret += BundleString(eid.getString()).getLength();
			}
			if (getFlag(GeoRoutingEntry::GEO_REQUIRED)) {
				ret += geopoint.getLength();
			}
			return ret;
		}

		std::ostream& operator<<(std::ostream &stream, const GeoRoutingBlock::GeoRoutingEntry &entry)
		{
			stream << entry.flags;
			stream << entry.margin;
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::EID_REQUIRED)) {
				dtn::data::BundleString endpoint(entry.eid.getString());
				stream << endpoint;
			}
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::GEO_REQUIRED)) {
				stream << entry.geopoint;
			}
			return stream;
		}

		std::istream& operator>>(std::istream &stream, GeoRoutingBlock::GeoRoutingEntry &entry)
		{
			stream >> entry.flags;
			stream >> entry.margin;
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::EID_REQUIRED)) {
				dtn::data::BundleString ee;
				stream >> ee;
				entry.eid = ee;
			}
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::GEO_REQUIRED)) {
				stream >> entry.geopoint;
			}
			return stream;
		}
	} /* namespace data */
} /* namespace dtn */

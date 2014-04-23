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
			// number of elements
			dtn::data::Number count(_entries.size());
			stream << count;

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				const GeoRoutingEntry &entry = (*iter);
				stream << entry;
			}

			return stream;
		}

		std::istream& GeoRoutingBlock::deserialize(std::istream &stream, const Length&)
		{
			// number of elements
			dtn::data::Number count;

			stream >> count;

			for (Number i = 0; i < count; ++i)
			{
				GeoRoutingEntry entry;
				stream >> entry;
				_entries.push_back(entry);
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

		const GeoRoutingBlock::tracking_list& GeoRoutingBlock::getTrack() const
		{
			return _entries;
		}

		void GeoRoutingBlock::append(const dtn::data::EID &eid)
		{
			GeoRoutingEntry entry(eid);

			// include timestamp
			entry.setFlag(GeoRoutingEntry::TIMESTAMP_PRESENT, true);

			// include geo data???
			entry.setFlag(GeoRoutingEntry::GEODATA_PRESENT, true);
			entry.geopoint.set(0xaa,0xaa);

			// use default timestamp
			//entry.timestamp.set();

			_entries.push_back(entry);
		}

		GeoRoutingBlock::GeoRoutingEntry::GeoRoutingEntry()
		{
		}

		GeoRoutingBlock::GeoRoutingEntry::GeoRoutingEntry(const dtn::data::EID &eid)
		 : endpoint(eid)
		{
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

			if (getFlag(GeoRoutingEntry::TIMESTAMP_PRESENT)) {
				cout << "TIMESTAMP_PRESENT" << endl;
				ret += timestamp.getLength();
			}
			if (getFlag(GeoRoutingEntry::GEODATA_PRESENT)) {
				cout << "GEODATA_PRESENT  length=" << geopoint.getLength() << endl;
				ret += geopoint.getLength();
			}

			ret += BundleString(endpoint.getString()).getLength();

			return ret;
		}

		std::ostream& operator<<(std::ostream &stream, const GeoRoutingBlock::GeoRoutingEntry &entry)
		{
			stream << entry.flags;

			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::TIMESTAMP_PRESENT)) {
				stream << entry.timestamp;
			}
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::GEODATA_PRESENT)) {
				stream << entry.geopoint;
			}

			dtn::data::BundleString endpoint(entry.endpoint.getString());
			stream << endpoint;
			return stream;
		}

		std::istream& operator>>(std::istream &stream, GeoRoutingBlock::GeoRoutingEntry &entry)
		{
			stream >> entry.flags;

			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::TIMESTAMP_PRESENT)) {
				stream >> entry.timestamp;
			}
			if (entry.getFlag(GeoRoutingBlock::GeoRoutingEntry::GEODATA_PRESENT)) {
				stream >> entry.geopoint;
			}

			BundleString endpoint;
			stream >> endpoint;
			entry.endpoint = dtn::data::EID((std::string&)endpoint);
			return stream;
		}
	} /* namespace data */
} /* namespace dtn */

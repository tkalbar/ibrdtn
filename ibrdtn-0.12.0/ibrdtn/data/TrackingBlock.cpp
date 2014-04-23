/*
 * TrackingBlock.cpp
 *
 */

#include "ibrdtn/data/TrackingBlock.h"
#include "ibrdtn/data/BundleString.h"

namespace dtn
{
	namespace data
	{
		const dtn::data::block_t TrackingBlock::BLOCK_TYPE = 193;

		dtn::data::Block* TrackingBlock::Factory::create()
		{
			return new TrackingBlock();
		}

		TrackingBlock::TrackingBlock()
		 : dtn::data::Block(TrackingBlock::BLOCK_TYPE)
		{
			// set the replicate in every fragment bit
			set(REPLICATE_IN_EVERY_FRAGMENT, true);
		}

		TrackingBlock::TrackingBlock(int track_hops, int track_geo, int tr_intvl)
		: dtn::data::Block(TrackingBlock::BLOCK_TYPE)
		{
		}


		TrackingBlock::~TrackingBlock()
		{
		}

		Length TrackingBlock::getLength() const
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
				const TrackingEntry &entry = (*iter);
				ret += entry.getLength();
			}

			return ret;
		}

		std::ostream& TrackingBlock::serialize(std::ostream &stream, Length&) const
		{
			// number of elements
			dtn::data::Number count(_entries.size());
			stream << count;

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				const TrackingEntry &entry = (*iter);
				stream << entry;
			}

			return stream;
		}

		std::istream& TrackingBlock::deserialize(std::istream &stream, const Length&)
		{
			// number of elements
			dtn::data::Number count;

			stream >> count;

			for (Number i = 0; i < count; ++i)
			{
				TrackingEntry entry;
				stream >> entry;
				_entries.push_back(entry);
			}

			return stream;
		}

		Length TrackingBlock::getLength_strict() const
		{
			return getLength();
		}

		std::ostream& TrackingBlock::serialize_strict(std::ostream &stream, Length &length) const
		{
			return serialize(stream, length);
		}

		const TrackingBlock::tracking_list& TrackingBlock::getTrack() const
		{
			return _entries;
		}

		void TrackingBlock::append(const dtn::data::EID &eid)
		{
			TrackingEntry entry(eid);

			// include timestamp
			entry.setFlag(TrackingEntry::TIMESTAMP_PRESENT, true);

			// include geo data???
			entry.setFlag(TrackingEntry::GEODATA_PRESENT, true);
			entry.geopoint.set(0xaa,0xaa);

			// use default timestamp
			//entry.timestamp.set();

			_entries.push_back(entry);
		}

		TrackingBlock::TrackingEntry::TrackingEntry()
		{
		}

		TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)
		 : endpoint(eid)
		{
			setFlag(TrackingEntry::EID_PRESENT, true);
			setFlag(TrackingEntry::GEODATA_PRESENT, false);
		}

		TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)
		 : geopoint(lat,lon)
		{
			setFlag(TrackingEntry::EID_PRESENT, false);
			setFlag(TrackingEntry::GEODATA_PRESENT, true);
		}

		TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid, float lat, float lon)
		 : endpoint(eid) , geopoint(lat,lon)
		{
			setFlag(TrackingEntry::EID_PRESENT, true);
			setFlag(TrackingEntry::GEODATA_PRESENT, true);
		}

		TrackingBlock::TrackingEntry::~TrackingEntry()
		{
		}

		bool TrackingBlock::TrackingEntry::getFlag(TrackingBlock::TrackingEntry::FLAGS f) const
		{
			return flags.getBit(f);
		}

		void TrackingBlock::TrackingEntry::setFlag(TrackingBlock::TrackingEntry::FLAGS f, bool value)
		{
			flags.setBit(f, value);
		}

		Length TrackingBlock::TrackingEntry::getLength() const
		{
			Length ret = flags.getLength();

			if (getFlag(TrackingEntry::TIMESTAMP_PRESENT)) {
				cout << "TIMESTAMP_PRESENT" << endl;
				ret += timestamp.getLength();
			}
			if (getFlag(TrackingEntry::GEODATA_PRESENT)) {
				cout << "GEODATA_PRESENT  length=" << geopoint.getLength() << endl;
				ret += geopoint.getLength();
			}

			ret += BundleString(endpoint.getString()).getLength();

			return ret;
		}

		std::ostream& operator<<(std::ostream &stream, const TrackingBlock::TrackingEntry &entry)
		{
			stream << entry.flags;

			if (entry.getFlag(TrackingBlock::TrackingEntry::TIMESTAMP_PRESENT)) {
				stream << entry.timestamp;
			}
			if (entry.getFlag(TrackingBlock::TrackingEntry::GEODATA_PRESENT)) {
				stream << entry.geopoint;
			}

			dtn::data::BundleString endpoint(entry.endpoint.getString());
			stream << endpoint;
			return stream;
		}

		std::istream& operator>>(std::istream &stream, TrackingBlock::TrackingEntry &entry)
		{
			stream >> entry.flags;

			if (entry.getFlag(TrackingBlock::TrackingEntry::TIMESTAMP_PRESENT)) {
				stream >> entry.timestamp;
			}
			if (entry.getFlag(TrackingBlock::TrackingEntry::GEODATA_PRESENT)) {
				stream >> entry.geopoint;
			}

			BundleString endpoint;
			stream >> endpoint;
			entry.endpoint = dtn::data::EID((std::string&)endpoint);
			return stream;
		}
	} /* namespace data */
} /* namespace dtn */

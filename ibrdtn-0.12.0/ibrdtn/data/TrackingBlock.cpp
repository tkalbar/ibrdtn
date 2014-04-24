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

		bool TrackingBlock::getFlag(TrackingBlock::FLAGS f) const
		{
			return procflags.getBit(f);
		}

		void TrackingBlock::setFlag(TrackingBlock::FLAGS f, bool value)
		{
			procflags.setBit(f, value);
		}

		/**
		 * This function should update the entry list in preparation
		 * for sending the bundle out
		 *
		 * things to do:
		 * - figure out when the bundle was received (how long has this node held it)
		 * - grab the file with the GPS logs for this node
		 * - read through the GPS logs, picking out entries for the right interval, and add TrackingEntry for them
		 */
		void TrackingBlock::finalizeEntryList()
		{

		}


		Length TrackingBlock::getLength() const
		{
			Length ret = 0;

			// the flags indicating what to track
			ret += procflags.getLength();

			// the geo tracking interval (must be present, but ignored if TRACK_GEO is not selected)
			ret += tracking_interval.getLength();

			// number of TrackingElement in list
			dtn::data::Number count(_entries.size());
			ret += count.getLength();

			// NOTE!!!
			// in order to compute this we need to update the tracking_list
			// and then whatever we return has to remain accurate when someone serializes the block
			// tricky tricky tricky

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				cout << "TrackingBlock::getLength() counting a TrackingEntry..." << endl;
				const TrackingEntry &entry = (*iter);
				ret += entry.getLength();
			}

			return ret;
		}

		std::ostream& TrackingBlock::serialize(std::ostream &stream, Length&) const
		{
			// the flags indicating what to track
			stream << procflags;

			// the geo tracking interval (must be present, but ignored if TRACK_GEO is not selected)
			stream << tracking_interval;

			// number of TrackingElement in list
			dtn::data::Number count(_entries.size());
			cout << "TrackingBlock::serialize()  counted entries: " << count.get() << endl;
			stream << count;

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				cout << "TrackingBlock::getLength() serializing a TrackingEntry..." << endl;
				const TrackingEntry &entry = (*iter);
				stream << entry;
			}

			return stream;
		}

		std::istream& TrackingBlock::deserialize(std::istream &stream, const Length&)
		{
			// the flags indicating what to track
			stream >> procflags;

			// the geo tracking interval (must be present, but ignored if TRACK_GEO is not selected)
			stream >> tracking_interval;

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

		// append a HOPDATA tracking entry
		void TrackingBlock::append(const dtn::data::EID &eid)
		{
			TrackingEntry entry(eid);

			if (getFlag(TrackingBlock::TRACK_TIMESTAMP)) {
				// use the current time
				dtn::data::DTNTime dtntime;
				dtntime.set();
				entry.timestamp = dtntime.getTimestamp();
			} else {
				entry.timestamp = 0;
			}

			_entries.push_back(entry);
		}

		// append a GEODATA tracking entry
		void TrackingBlock::append(float lat, float lon)
		{
			TrackingEntry entry(lat,lon);

			if (getFlag(TrackingBlock::TRACK_TIMESTAMP)) {
				// use the current time
				dtn::data::DTNTime dtntime;
				dtntime.set();
				entry.timestamp = dtntime.getTimestamp();
			} else {
				entry.timestamp = 0;
			}

			_entries.push_back(entry);
		}

		TrackingBlock::TrackingEntry::TrackingEntry()
		{
		}

		TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)
		 : endpoint(eid)
		{
			entry_type = TrackingEntry::HOPDATA;
		}

		TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)
		 : geopoint(lat,lon)
		{
			entry_type = TrackingEntry::GEODATA;
		}

		TrackingBlock::TrackingEntry::~TrackingEntry()
		{
		}

		/*
		bool TrackingBlock::TrackingEntry::getFlag(TrackingBlock::TrackingEntry::FLAGS f) const
		{
			return flags.getBit(f);
		}

		void TrackingBlock::TrackingEntry::setFlag(TrackingBlock::TrackingEntry::FLAGS f, bool value)
		{
			flags.setBit(f, value);
		}
		*/

		Length TrackingBlock::TrackingEntry::getLength() const
		{
			Length ret = entry_type.getLength() + timestamp.getLength();

			if (entry_type == TrackingEntry::HOPDATA) {
				ret += BundleString(endpoint.getString()).getLength();
			} else if (entry_type == TrackingEntry::HOPDATA) {
				ret += geopoint.getLength();
			} else {
				cout << "ERROR ERROR: illegal entry_type!!" << endl;
			}

			return ret;
		}

		std::ostream& operator<<(std::ostream &stream, const TrackingBlock::TrackingEntry &entry)
		{
			stream << entry.entry_type;
			stream << entry.timestamp;

			if (entry.entry_type == TrackingBlock::TrackingEntry::HOPDATA) {
				dtn::data::BundleString endpoint(entry.endpoint.getString());
				stream << endpoint;
			} else if (entry.entry_type == TrackingBlock::TrackingEntry::GEODATA) {
				stream << entry.geopoint;
			}

			return stream;
		}

		std::istream& operator>>(std::istream &stream, TrackingBlock::TrackingEntry &entry)
		{
			stream >> entry.entry_type;
			stream >> entry.timestamp;

			if (entry.entry_type == TrackingBlock::TrackingEntry::HOPDATA) {
				dtn::data::BundleString ee;
				stream >> ee;
				entry.endpoint = ee;
			} else if (entry.entry_type == TrackingBlock::TrackingEntry::GEODATA) {
				stream >> entry.geopoint;
			}

			return stream;
		}

	} /* namespace data */
} /* namespace dtn */

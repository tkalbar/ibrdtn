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
		void TrackingBlock::finalizeEntryList() const
		{
			// check if we even want to the gps coords
			if (procflags.getBit(TRACK_GEO) == 0) {
				cout << "finalizeEntryList(): not tracking geo.  Returning!" << endl;
				return;
			}
			cout << "finalizeEntryList(): we ARE tracking geo.  Let's get it on! " << endl;

			Timestamp recTime;

			const int MAX_CHARS_PER_LINE = 2048;
			const int MAX_TOKENS_PER_LINE = 3;
			const char* const DELIMITER = ":";

			cout << "finalizeEntryList(): allocated some local vars... " << endl;

			// the last entry in the tracking entry list should have been
			// created when the bundle was received here
			if (_entries.empty()) {
				cout << "finalizeEntryList(): _entries is empty.  Returning!" << endl;
				return;
			}
			TrackingEntry entry = _entries.back();
			cout << "finalizeEntryList(): grabbed _entries.back()" << endl;

			if (entry.entry_type == TrackingEntry::HOPDATA) {
				recTime = entry.timestamp;
			} else {
				// otherwise give up
				return;
			}

			// worry about time conversion later

			// read the gps log file
			// create a file-reading object
			ifstream fin;
			fin.open("/tmp/gpslog.log"); // open a file
			if (!fin.good()) {
				cout << "finalizeEntryList(): failed to open gpslog.  Returning!" << endl;
				return; // exit if file not found
			}

			// read each line of the file
			while (!fin.eof()) {
				// read an entire line into memory
				char buf[MAX_CHARS_PER_LINE];
				fin.getline(buf, MAX_CHARS_PER_LINE);

				std::vector<std::string> gpsFields;
				std::istringstream lineString(buf);
				std::string tsString;
				std::string latString;
				std::string lonString;
				float lat, lon;
				int ts;

				if (0 < std::getline(  lineString , tsString , ';' )) {
					ts = atoi(tsString.c_str());
				} else { return; }

				if (0 < std::getline(  lineString , latString , ';' )) {
					lat = atof(latString.c_str());
				} else { return; }

				if (0 < std::getline(  lineString , lonString , ';' )) {
					lon = atof(lonString.c_str());
				} else { return; }

				cout << "parsed location data: " << ts << " , " << lat << " , " << lon << endl;

				//TODO: Can't call this because it's not const!!
				// need to institute global finalize() methods for ext blocks.
				//append(ts,lat,lon);
			}
		}



		Length TrackingBlock::getLength() const
		{
			cout << "TrackingBlock::getLength()" << endl;

			finalizeEntryList();

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
			cout << "TrackingBlock::serialize()" << endl;

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
			cout << "TrackingBlock::deserialize()" << endl;

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
			cout << "TrackingBlock::getTrack()" << endl;
			return _entries;
		}

		// append a HOPDATA tracking entry
		void TrackingBlock::append(const dtn::data::EID &eid)
		{
			cout << "TrackingBlock::append(const dtn::data::EID &eid)" << endl;

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
		void TrackingBlock::append(int time, float lat, float lon)
		{
			cout << "TrackingBlock::append(int time, float lat, float lon)" << endl;

			TrackingEntry entry(lat,lon);

			if (getFlag(TrackingBlock::TRACK_TIMESTAMP)) {
				// use the current time
				//dtn::data::DTNTime dtntime;
				//dtntime.set();
				//entry.timestamp = dtntime.getTimestamp();
				entry.timestamp = time;
			} else {
				entry.timestamp = 0;
			}

			_entries.push_back(entry);
		}

		TrackingBlock::TrackingEntry::TrackingEntry()
		{
			cout << "TrackingBlock::TrackingEntry::TrackingEntry()" << endl;
		}

		TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)
		 : endpoint(eid)
		{
			cout << "TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)" << endl;
			entry_type = TrackingEntry::HOPDATA;
		}

		TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)
		 : geopoint(lat,lon)
		{
			cout << "TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)" << endl;
			entry_type = TrackingEntry::GEODATA;
		}

		TrackingBlock::TrackingEntry::~TrackingEntry()
		{
		}

		Length TrackingBlock::TrackingEntry::getLength() const
		{
			cout << "TrackingBlock::TrackingEntry::getLength()" << endl;

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
			cout << "operator<<(std::ostream &stream, const TrackingBlock::TrackingEntry &entry)" << endl;

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
			cout << "operator>>(std::istream &stream, TrackingBlock::TrackingEntry &entry)" << endl;

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

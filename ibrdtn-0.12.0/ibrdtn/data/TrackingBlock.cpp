/*
 * TrackingBlock.cpp
 *
 */

#include "ibrdtn/data/TrackingBlock.h"
#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/utils/Clock.h"

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
		void TrackingBlock::updatedEntryList(tracking_list &l) const
		{
			cout << "TrackingBlock::updatedEntryList()" << endl;
			cout << "\ttracking_interval = " << tracking_interval.get() << endl;

			// check if we even want to track the gps coords
			if (procflags.getBit(TRACK_GEO) == 0) {
				cout << "updatedEntryList(): not tracking geo.  Returning!" << endl;
				return;
			}
			//cout << "updatedEntryList(): we ARE tracking geo.  Let's get it on! " << endl;

			// the time this bundle was received measured locally)
			// we will pull this out of the TrackingEntry that was put on the list when it was received
			// there should be a better way to do this, but the rx time doesn't seem to be recorded anywhere else
			Timestamp rxTime;

			const int MAX_CHARS_PER_LINE = 2048;
			const int MAX_TOKENS_PER_LINE = 3;
			const char* const DELIMITER = ":";

			// the last entry in the tracking entry list should have been
			// created when the bundle was received here
			if (_entries.empty()) {
				cout << "updatedEntryList(): _entries is empty.  Returning!" << endl;
				return;
			}
			TrackingEntry entry = _entries.back();

			if (entry.entry_type == TrackingEntry::HOPDATA) {
				rxTime = entry.timestamp;
			} else {
				// otherwise give up
				return;
			}

			// read the gps log file
			// create a file-reading object
			ifstream fin;
			fin.open("/tmp/gpslog.log"); // open a file
			if (!fin.good()) {
				cout << "updatedEntryList(): failed to open gpslog.  Returning!" << endl;
				return; // exit if file not found
			}

			// it will be useful to know the current time
			dtn::data::DTNTime dtntime;
			dtntime.set();
			Timestamp cur_time = dtntime.getTimestamp();

			// read each line of the file
			// create a TrackingEntry based on the line if it satisfies:
			// - it happens after the time the bundle was received at this node
			// - it it at least the tracking_interval since the last one we added an entry for
			Timestamp last_entry_time = 0;
			int num_entries_added = 0;
			float lat, lon;
			int epoch_timestamp;
			while (!fin.eof()) {
				// read an entire line into memory
				char buf[MAX_CHARS_PER_LINE];
				fin.getline(buf, MAX_CHARS_PER_LINE);

				std::vector<std::string> gpsFields;
				std::istringstream lineString(buf);
				std::string tsString;
				std::string latString;
				std::string lonString;

				if (0 < std::getline(  lineString , tsString , ';' )) {
					epoch_timestamp = atoi(tsString.c_str());
				} else { return; }

				if (0 < std::getline(  lineString , latString , ';' )) {
					lat = atof(latString.c_str());
				} else { return; }

				if (0 < std::getline(  lineString , lonString , ';' )) {
					lon = atof(lonString.c_str());
				} else { return; }

				//cout << "parsed location data: " << ts << " , " << lat << " , " << lon << endl;

				// should re-organize class so that tracking_list is a class and
				// append() is a method attached to it
				//append(ts,lat,lon);
				//cout << "TrackingBlock::append(int time, float lat, float lon)" << endl;

				Timestamp ts = epoch_timestamp;
				ts -= dtn::utils::Clock::TIMEVAL_CONVERSION;

				// bail out if for some reason this file goes into the future
				if (ts > cur_time) {
					break;
				}

				if ((ts >= rxTime) && ((ts-last_entry_time)>=tracking_interval)) {
					last_entry_time = ts;
					TrackingEntry entry(lat,lon);
					if (getFlag(TrackingBlock::TRACK_TIMESTAMP)) {
						entry.timestamp = epoch_timestamp;
						// convert from epoch time to dtn time
						entry.timestamp -= dtn::utils::Clock::TIMEVAL_CONVERSION;
					} else {
						entry.timestamp = 0;
					}
					l.push_back(entry);
					num_entries_added++;
					//cout << "\tUSING file entry: (" << ts.get() << " , " << lat << " , " << lon << ")" << endl;
					//cout << "l.size()=" << l.size() << endl;
				} else {
					//cout << "\tDISCARDING file entry: (" << ts.get() << " , " << lat << " , " << lon << ")" << endl;
				}
			}

			// if we didn't add any entries just add the final entry in the file
			// this is for cases where the bundle is forwarded right away
			if (num_entries_added==0) {
				TrackingEntry entry(lat,lon);
				if (getFlag(TrackingBlock::TRACK_TIMESTAMP)) {
					entry.timestamp = epoch_timestamp;
					// convert from epoch time to dtn time
					entry.timestamp -= dtn::utils::Clock::TIMEVAL_CONVERSION;
				} else {
					entry.timestamp = 0;
				}
				l.push_back(entry);
			}
		}



		Length TrackingBlock::getLength() const
		{
			cout << "TrackingBlock::getLength()" << endl;

			Length ret = 0;

			// the flags indicating what to track
			ret += procflags.getLength();

			// the geo tracking interval (must be present, but ignored if TRACK_GEO is not selected)
			ret += tracking_interval.getLength();

			// we need to call this at the top because it affects the count
			//cout << "TrackingBlock::serialize() about to call updatedEntryList()" << endl;
			tracking_list new_entries;
			updatedEntryList(new_entries);

			// number of TrackingElement in list
			dtn::data::Number count(_entries.size() + new_entries.size());
			ret += count.getLength();

			// NOTE!!!
			// in order to compute this we need to update the tracking_list
			// and then whatever we return has to remain accurate when someone serializes the block
			// tricky tricky tricky

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				//cout << "TrackingBlock::getLength() counting a TrackingEntry..." << endl;
				const TrackingEntry &entry = (*iter);
				ret += entry.getLength();
				//cout << "TrackingBlock::getLength():  ret=" << ret << endl;
			}

			// now count up the new entries that accumulate while the bundle
			// was carried by this node.
			// This is kind-of a screwy way to do it.  We have to compute the new
			// entries once for getLength(), then again for serialize().
			// Also it's a race condition, as the length may increase between those two calls.
			// But because there is no finalize() for blocks and the functions are all const it's
			// the best we can do.

			if (!new_entries.empty()) {
				for (tracking_list::const_iterator iter = new_entries.begin(); iter != new_entries.end(); ++iter)
				{
					//cout << "TrackingBlock::getLength() counting a TrackingEntry..." << endl;
					const TrackingEntry &entry = (*iter);
					ret += entry.getLength();
					//cout << "TrackingBlock::getLength():  ret=" << ret << endl;
				}
			}
			cout << "TrackingBlock::getLength():  returning ret=" << ret << endl;

			return ret;
		}

		std::ostream& TrackingBlock::serialize(std::ostream &stream, Length&) const
		{
			cout << "TrackingBlock::serialize()" << endl;

			// the flags indicating what to track
			stream << procflags;

			// the geo tracking interval (must be present, but ignored if TRACK_GEO is not selected)
			stream << tracking_interval;

			// we need to call this at the top because it affects the count
			tracking_list new_entries;
			updatedEntryList(new_entries);

			// number of TrackingElement in list
			dtn::data::Number count(_entries.size() + new_entries.size());
			//cout << "TrackingBlock::serialize()  counted entries: " << count.get() << endl;
			stream << count;

			for (tracking_list::const_iterator iter = _entries.begin(); iter != _entries.end(); ++iter)
			{
				//cout << "TrackingBlock::getLength() serializing a TrackingEntry..." << endl;
				const TrackingEntry &entry = (*iter);
				stream << entry;
			}

			// now serialize the new entries that accumulate while the bundle
			// was carried by this node.

			if (! new_entries.empty()) {
				for (tracking_list::const_iterator iter = new_entries.begin(); iter != new_entries.end(); ++iter)
				{
					//cout << "TrackingBlock::getLength() serializing a TrackingEntry..." << endl;
					const TrackingEntry &entry = (*iter);
					stream << entry;
				}
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
			//cout << "TrackingBlock::append(const dtn::data::EID &eid)" << endl;

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
			//cout << "TrackingBlock::append(int time, float lat, float lon)" << endl;

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
			//cout << "TrackingBlock::TrackingEntry::TrackingEntry()" << endl;
		}

		TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)
		 : endpoint(eid)
		{
			//cout << "TrackingBlock::TrackingEntry::TrackingEntry(const dtn::data::EID &eid)" << endl;
			entry_type = TrackingEntry::HOPDATA;
		}

		TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)
		 : geopoint(lat,lon)
		{
			//cout << "TrackingBlock::TrackingEntry::TrackingEntry(float lat, float lon)" << endl;
			entry_type = TrackingEntry::GEODATA;
		}

		TrackingBlock::TrackingEntry::~TrackingEntry()
		{
		}

		Length TrackingBlock::TrackingEntry::getLength() const
		{
			//cout << "TrackingBlock::TrackingEntry::getLength()" << endl;

			Length ret = entry_type.getLength() + timestamp.getLength();

			int et = entry_type.get();
			if (et == TrackingEntry::HOPDATA) {
				ret += BundleString(endpoint.getString()).getLength();
			} else if (et == TrackingEntry::GEODATA) {
				ret += geopoint.getLength();
			} else {
				cout << "ERROR ERROR: illegal entry_type: " << et << endl;
			}

			return ret;
		}

		std::ostream& operator<<(std::ostream &stream, const TrackingBlock::TrackingEntry &entry)
		{
			//cout << "operator<<(std::ostream &stream, const TrackingBlock::TrackingEntry &entry)" << endl;

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
			//cout << "TrackingEntry deserializer..." << endl;

			stream >> entry.entry_type;
			stream >> entry.timestamp;

			if (entry.entry_type == TrackingBlock::TrackingEntry::HOPDATA) {
				dtn::data::BundleString ee;
				stream >> ee;
				entry.endpoint = ee;
				cout << "\t(HOPDATA , " << entry.timestamp.get() << " , " << ee.c_str() << ")" << endl;
			} else if (entry.entry_type == TrackingBlock::TrackingEntry::GEODATA) {
				stream >> entry.geopoint;
				cout << "\t(GEODATA , " << entry.timestamp.get() << " , " << entry.geopoint.toString() << ")" << endl;
			}

			return stream;
		}

	} /* namespace data */
} /* namespace dtn */

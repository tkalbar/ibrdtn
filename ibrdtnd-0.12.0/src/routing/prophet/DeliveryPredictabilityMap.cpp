/*
 * DeliveryPredictabilityMap.cpp
 *
 *  Created on: 08.01.2013
 *      Author: morgenro
 */

#include "routing/prophet/DeliveryPredictabilityMap.h"
#include "core/BundleCore.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>
#include <vector>

namespace dtn
{
	namespace routing
	{
		const dtn::data::Number DeliveryPredictabilityMap::identifier = NodeHandshakeItem::DELIVERY_PREDICTABILITY_MAP;

		DeliveryPredictabilityMap::DeliveryPredictabilityMap()
		: NeighborDataSetImpl(DeliveryPredictabilityMap::identifier), _beta(0.0), _gamma(0.0), _lastAgingTime(0), _time_unit(0)
		{
		}

		DeliveryPredictabilityMap::DeliveryPredictabilityMap(const size_t &time_unit, const float &beta, const float &gamma)
		: NeighborDataSetImpl(DeliveryPredictabilityMap::identifier), _beta(beta), _gamma(gamma), _lastAgingTime(0), _time_unit(time_unit)
		{
		}

		DeliveryPredictabilityMap::~DeliveryPredictabilityMap() {
		}

		const dtn::data::Number& DeliveryPredictabilityMap::getIdentifier() const
		{
			return identifier;
		}

		dtn::data::Length DeliveryPredictabilityMap::getLength() const
		{
			dtn::data::Length len = 0;
			for(predictmap::const_iterator it = _predictmap.begin(); it != _predictmap.end(); ++it)
			{
				/* calculate length of the EID */
				const std::string eid = it->first.getString();
				dtn::data::Length eid_len = eid.length();
				len += data::Number(eid_len).getLength() + eid_len;

				/* calculate length of the float in fixed notation */
				const float& f = it->second;
				std::stringstream ss;
				ss << f << std::flush;

				dtn::data::Length float_len = ss.str().length();
				len += data::Number(float_len).getLength() + float_len;
			}
			return data::Number(_predictmap.size()).getLength() + len;
		}

		std::ostream& DeliveryPredictabilityMap::serialize(std::ostream& stream) const
		{
			stream << data::Number(_predictmap.size());
			for(predictmap::const_iterator it = _predictmap.begin(); it != _predictmap.end(); ++it)
			{
				const std::string eid = it->first.getString();
				stream << data::Number(eid.length()) << eid;

				const float& f = it->second;
				/* write f into a stringstream to get final length */
				std::stringstream ss;
				ss << f << std::flush;

				stream << data::Number(ss.str().length());
				stream << ss.str();
			}
			IBRCOMMON_LOGGER_DEBUG_TAG("DeliveryPredictabilityMap", 20) << "Serialized with " << _predictmap.size() << " items." << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG("DeliveryPredictabilityMap", 60) << *this << IBRCOMMON_LOGGER_ENDL;
			return stream;
		}

		std::istream& DeliveryPredictabilityMap::deserialize(std::istream& stream)
		{
			data::Number elements_read(0);
			data::Number map_size;
			stream >> map_size;

			while(elements_read < map_size)
			{
				/* read the EID */
				data::Number eid_len;
				stream >> eid_len;

				// create a buffer for the EID
				std::vector<char> eid_cstr(eid_len.get<size_t>());

				// read the EID string
				stream.read(&eid_cstr[0], eid_cstr.size());

				// convert the string into an EID object
				dtn::data::EID eid(std::string(eid_cstr.begin(), eid_cstr.end()));

				if(eid == data::EID())
					throw dtn::InvalidDataException("EID could not be casted, while parsing a dp_map.");

				/* read the probability (float) */
				float f;
				dtn::data::Number float_len;
				stream >> float_len;

				// create a buffer for the data string
				std::vector<char> f_cstr(float_len.get<size_t>());

				// read the data string
				stream.read(&f_cstr[0], f_cstr.size());

				// convert string data into a stringstream
				std::stringstream ss(std::string(f_cstr.begin(), f_cstr.end()));

				// convert string data into a float
				ss >> f;
				if(ss.fail())
					throw dtn::InvalidDataException("Float could not be casted, while parsing a dp_map.");

				/* check if f is in a proper range */
				if(f < 0 || f > 1)
					continue;

				/* insert the data into the map */
				_predictmap[eid] = f;

				elements_read += 1;
			}

			IBRCOMMON_LOGGER_DEBUG_TAG("DeliveryPredictabilityMap", 20) << "Deserialized with " << _predictmap.size() << " items." << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG_TAG("DeliveryPredictabilityMap", 60) << *this << IBRCOMMON_LOGGER_ENDL;
			return stream;
		}

		float DeliveryPredictabilityMap::get(const dtn::data::EID &neighbor) const throw (ValueNotFoundException)
		{
			predictmap::const_iterator it;
			if ((it = _predictmap.find(neighbor)) != _predictmap.end())
			{
				return it->second;
			}

			throw ValueNotFoundException();
		}

		void DeliveryPredictabilityMap::set(const dtn::data::EID &neighbor, float value)
		{
			_predictmap[neighbor] = value;
		}

		void DeliveryPredictabilityMap::clear()
		{
			_predictmap.clear();
		}

		void DeliveryPredictabilityMap::update(const dtn::data::EID &host_b, const DeliveryPredictabilityMap &dpm, const float &p_encounter_first)
		{
			float p_ab = 0.0f;

			try {
				p_ab = get(host_b);
			} catch (const DeliveryPredictabilityMap::ValueNotFoundException&) {
				p_ab = p_encounter_first;
			}

			/**
			 * Calculate transitive values
			 */
			for (predictmap::const_iterator it = dpm._predictmap.begin(); it != dpm._predictmap.end(); ++it)
			{
				const dtn::data::EID &host_c = it->first;
				const float &p_bc = it->second;

				// do not update values for the origin host
				if (host_b.sameHost(host_c)) continue;

				// do not process values with our own EID
				if (dtn::core::BundleCore::local.sameHost(host_c)) continue;

				predictmap::iterator dp_it;
				if ((dp_it = _predictmap.find(host_c)) != _predictmap.end()) {
					dp_it->second = max(dp_it->second, p_ab * p_bc * _beta);
				} else {
					_predictmap[host_c] = p_ab * p_bc * _beta;
				}
			}
		}

		void DeliveryPredictabilityMap::age(const float &p_first_threshold)
		{
			const dtn::data::Timestamp current_time = dtn::utils::Clock::getMonotonicTimestamp();

			// prevent double aging
			if (current_time <= _lastAgingTime) return;

			const dtn::data::Timestamp k = (current_time - _lastAgingTime) / _time_unit;

			predictmap::iterator it;
			for(it = _predictmap.begin(); it != _predictmap.end();)
			{
				if(it->first == dtn::core::BundleCore::local)
				{
					++it;
					continue;
				}

				it->second *= pow(_gamma, k.get<int>());

				if(it->second < p_first_threshold)
				{
					_predictmap.erase(it++);
				} else {
					++it;
				}
			}

			_lastAgingTime = current_time;
		}

		void DeliveryPredictabilityMap::toString(std::ostream &stream) const
		{
			predictmap::const_iterator it;
			for (it = _predictmap.begin(); it != _predictmap.end(); ++it)
			{
				stream << it->first.getString() << ": " << it->second << std::endl;
			}
		}

		std::ostream& operator<<(std::ostream& stream, const DeliveryPredictabilityMap& map)
		{
			map.toString(stream);
			return stream;
		}
	} /* namespace routing */
} /* namespace dtn */

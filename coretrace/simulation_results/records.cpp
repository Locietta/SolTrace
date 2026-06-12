#include "records.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

namespace SolTrace::Result
{
    using element_id = SolTrace::Data::element_id;

    const std::string &ray_event_string(const RayEvent rev)
    {
        auto item = REV_TO_STR.find(rev);
        if (item != REV_TO_STR.cend())
        {
            return item->second;
        }
        else
        {
            return REV_TO_STR.find(RayEvent::UNKNOWN)->second;
        }
    }

    InteractionRecord::InteractionRecord(
        element_id el,
        RayEvent rev,
        const glm::dvec3 &location,
        const glm::dvec3 &direction)
        // : index(-1),
        : element(el),
          event(rev),
          location(location),
          direction(direction)
    {
    }

    InteractionRecord::InteractionRecord(
        element_id el,
        RayEvent rev,
        double px,
        double py,
        double pz,
        double dx,
        double dy,
        double dz)
        : element(el),
          event(rev),
          location(px, py, pz),
          direction(dx, dy, dz)
    {
    }

    InteractionRecord::~InteractionRecord()
    {
    }

    std::ostream &operator<<(std::ostream &os, const InteractionRecord &rec)
    {
        os << "Element: " << rec.element
           << " Location: (" << rec.location.x << ", " << rec.location.y << ", " << rec.location.z << ")"
           << " Direction: (" << rec.direction.x << ", " << rec.direction.y << ", " << rec.direction.z << ")"
           << " Event: " << ray_event_string(rec.event);
        return os;
    }

    RayRecord::RayRecord(ray_id id) : id(id)
    {
    }

    RayRecord::~RayRecord()
    {
        this->interactions.clear();
        return;
    }

    void RayRecord::add_interaction_record(interaction_ptr ip)
    {
        // ip->index = this->interactions.size();
        this->interactions.push_back(ip);
        return;
    }

    void RayRecord::get_position(const interaction_ptr ip, glm::dvec3 &pos)
    {
        pos = ip->location;
        return;
    }

    void RayRecord::get_position(int_fast64_t idx, glm::dvec3 &pos)
    {
        const interaction_ptr ip0 = this->interactions[idx];
        return this->get_position(ip0, pos);
    }

    void RayRecord::get_direction(const interaction_ptr ip,
        glm::dvec3 &cos)
    {
        cos = ip->direction;
        cos = glm::normalize(cos);
        return;
    }

    void RayRecord::get_direction(int_fast64_t idx,
        glm::dvec3 &cos)
    {
        const interaction_ptr ip0 = this->interactions[idx];
        this->get_direction(ip0, cos);
        return;
    }

    const interaction_ptr &RayRecord::operator[](int_fast64_t idx) const
    {
        if (idx < 0 || idx >= this->interactions.size())
        {
            std::stringstream ss;
            ss << "RayRecord: Index " << idx
               << " is out of bounds [0, " << this->interactions.size()
               << "].";
            throw std::invalid_argument(ss.str());
        }
        return this->interactions[idx];
    }

    std::ostream &operator<<(std::ostream &os, const RayRecord &rec)
    {
        os << "Ray: " << rec.id
           << "\nInteractions:\n";
        for (uint_fast64_t k = 0; k < rec.interactions.size(); ++k)
        {
            os << k << " " << *rec.interactions[k] << "\n";
        }
        // os << "Last Cosines: " << rec.cos_last << "\n";
        return os;
    }

    ElementRecord::ElementRecord(element_id eid)
        : elid(eid)
    {
        return;
    }

    ElementRecord::~ElementRecord()
    {
        this->interactions.clear();
        return;
    }

    void ElementRecord::add_interaction_record(interaction_ptr ip)
    {
        this->interactions.push_back(ip);
        return;
    }

    const interaction_ptr &ElementRecord::operator[](int_fast64_t idx) const
    {
        if (idx < 0 || idx >= this->interactions.size())
        {
            std::stringstream ss;
            ss << "ElementRecord: Index " << idx
               << " is out of bounds [0, " << this->interactions.size()
               << "].";
            throw std::invalid_argument(ss.str());
        }
        return this->interactions[idx];
    }

    std::ostream &operator<<(std::ostream &os, const ElementRecord &rec)
    {
        os << "Element: " << rec.elid
           << "\nInteractions:\n";
        for (uint_fast64_t k = 0; k < rec.interactions.size(); ++k)
        {
            os << k << " " << *rec.interactions[k] << "\n";
        }
        return os;
    }

}

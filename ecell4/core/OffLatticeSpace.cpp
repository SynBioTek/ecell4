#include <algorithm>
#include "Context.hpp"
#include "OffLatticeSpace.hpp"

namespace ecell4 {

OffLatticeSpace::OffLatticeSpace(const Real& voxel_radius)
    : base_type(voxel_radius),
      voxels_(),
      positions_(),
      adjoinings_()
{}

OffLatticeSpace::~OffLatticeSpace() {}

void OffLatticeSpace::reset(
        const position_container& positions,
        const coordinate_pair_list_type& adjoining_pairs)
{
    voxels_.clear();
    positions_.clear();
    adjoinings_.clear();

    const std::size_t size(positions.size());

    voxels_.resize(size, vacant_);
    positions_.resize(size);
    adjoinings_.resize(size);

    std::copy(positions.begin(), positions.end(), positions_.begin());

    for (coordinate_pair_list_type::const_iterator itr(adjoining_pairs.begin());
            itr != adjoining_pairs.end(); ++itr)
    {
        const coordinate_type coord0((*itr).first);
        const coordinate_type coord1((*itr).second);

        if (is_in_range(coord0) && is_in_range(coord1))
        {
            adjoinings_.at(coord0).push_back(coord1);
            adjoinings_.at(coord1).push_back(coord0);
        }
        else
        {
            throw IllegalState("A given pair is invalid.");
        }
    }
}

// Same as LatticeSpaceVectorImpl
std::pair<ParticleID, Voxel>
OffLatticeSpace::get_voxel_at(const coordinate_type& coord) const
{
    const VoxelPool* vp(voxels_.at(coord));
    const std::string loc(vp->is_vacant() || vp->location()->is_vacant() ?
                          "" : vp->location()->species().serial());
    return std::make_pair(vp->get_particle_id(coord),
                          Voxel(vp->species(), coord, vp->radius(), vp->D(), loc));
}

// Same as LatticeSpaceVectorImpl
VoxelPool* OffLatticeSpace::get_voxel_pool_at(const coordinate_type& coord) const
{
    return voxels_.at(coord);
}

// Same as LatticeSpaceVectorImpl
const Particle OffLatticeSpace::particle_at(const coordinate_type& coord) const
{
    const VoxelPool* vp(voxels_.at(coord));
    return Particle(vp->species(),
                    coordinate2position(coord),
                    vp->radius(),
                    vp->D());
}

// Same as LatticeSpaceVectorImpl
bool OffLatticeSpace::update_voxel(const ParticleID& pid, const Voxel& v)
{
    const coordinate_type& to_coord(v.coordinate());
    if (!is_in_range(to_coord))
        throw NotSupported("Out of bounds");

    VoxelPool* new_vp(get_voxel_pool(v)); //XXX: need MoleculeInfo
    VoxelPool* dest_vp(get_voxel_pool_at(to_coord));

    if (dest_vp != new_vp->location())
    {
        throw NotSupported(
            "Mismatch in the location. Failed to place '"
            + new_vp->species().serial() + "' to '"
            + dest_vp->species().serial() + "'.");
    }

    const coordinate_type from_coord(pid != ParticleID() ? get_coord(pid) : -1);
    if (from_coord != -1)
    {
        // move
        VoxelPool* src_vp(voxels_.at(from_coord));
        src_vp->remove_voxel_if_exists(from_coord);

        //XXX: use location?
        dest_vp->replace_voxel(to_coord, from_coord);
        voxel_container::iterator from_itr(voxels_.begin() + from_coord);
        (*from_itr) = dest_vp;

        new_vp->add_voxel(coordinate_id_pair_type(pid, to_coord));
        voxel_container::iterator to_itr(voxels_.begin() + to_coord);
        (*to_itr) = new_vp;
        return false;
    }

    // new
    dest_vp->remove_voxel_if_exists(to_coord);

    new_vp->add_voxel(coordinate_id_pair_type(pid, to_coord));
    voxel_container::iterator to_itr(voxels_.begin() + to_coord);
    (*to_itr) = new_vp;
    return true;
}
// Same as LatticeSpaceVectorImpl
bool OffLatticeSpace::remove_voxel(const ParticleID& pid)
{
    for (molecule_pool_map_type::iterator i(molecule_pools_.begin());
         i != molecule_pools_.end(); ++i)
    {
        const boost::shared_ptr<MoleculePool>& vp((*i).second);
        MoleculePool::const_iterator j(vp->find(pid));
        if (j != vp->end())
        {
            const coordinate_type coord((*j).coordinate);
            if (!vp->remove_voxel_if_exists(coord))
            {
                return false;
            }

            voxel_container::iterator itr(voxels_.begin() + coord);
            (*itr) = vp->location();
            vp->location()->add_voxel(
                coordinate_id_pair_type(ParticleID(), coord));
            return true;
        }
    }
    return false;
}

// Same as LatticeSpaceVectorImpl
bool OffLatticeSpace::remove_voxel(const coordinate_type& coord)
{
    voxel_container::iterator itr(voxels_.begin() + coord);
    VoxelPool* vp(*itr);
    if (vp->is_vacant())
    {
        return false;
    }
    if (vp->remove_voxel_if_exists(coord))
    {
        (*itr) = vp->location();
        vp->location()->add_voxel(
            coordinate_id_pair_type(ParticleID(), coord));
        return true;
    }
    return false;
}

bool OffLatticeSpace::can_move(const coordinate_type& src, const coordinate_type& dest) const
{
    if (src == dest) return false;

    const VoxelPool* src_vp(voxels_.at(src));
    if (src_vp->is_vacant()) return false;

    VoxelPool* dest_vp(voxels_.at(dest));

    return (voxels_.at(dest) == src_vp->location());
}

bool OffLatticeSpace::move(const coordinate_type& src,
                           const coordinate_type& dest,
                           const std::size_t candidate)
{
    if (src == dest) return false;

    VoxelPool* src_vp(voxels_.at(src));
    if (src_vp->is_vacant()) return true;

    VoxelPool* dest_vp(voxels_.at(dest));
    if (dest_vp != src_vp->location()) return false;

    src_vp->replace_voxel(src, dest, candidate);
    voxel_container::iterator src_itr(voxels_.begin() + src);
    (*src_itr) = dest_vp;

    dest_vp->replace_voxel(dest, src);
    voxel_container::iterator dest_itr(voxels_.begin() + dest);
    (*dest_itr) = src_vp;

    return true;
}

std::pair<OffLatticeSpace::coordinate_type, bool>
OffLatticeSpace::move_to_neighbor(VoxelPool* const& src_vp,
                                  VoxelPool* const& loc,
                                  coordinate_id_pair_type& info,
                                  const Integer nrand)
{
    const coordinate_type src(info.coordinate);
    coordinate_type dest(get_neighbor(src, nrand));

    VoxelPool* dest_vp(voxels_.at(dest));

    if (dest_vp != loc)
        return std::make_pair(dest, false);

    voxels_.at(src) = loc; // == dest_vp
    voxels_.at(dest) = src_vp;

    src_vp->replace_voxel(src, dest);
    dest_vp->replace_voxel(dest, src);
    return std::make_pair(dest, true);
}

/*
 * for LatticeSpaceBase
 */

Real3 OffLatticeSpace::coordinate2position(const coordinate_type& coord) const
{
    return positions_.at(coord);
}

OffLatticeSpace::coordinate_type
OffLatticeSpace::position2coordinate(const Real3& pos) const
{
    coordinate_type coordinate(0);
    Real shortest_length = length(positions_.at(0) - pos);

    for (coordinate_type coord(1); coord < size(); ++coord)
    {
        const Real len(length(positions_.at(coord) - pos));
        if (len < shortest_length)
        {
            coordinate = coord;
            shortest_length = len;
        }
    }

    return coordinate;
}

Integer OffLatticeSpace::num_neighbors(const coordinate_type& coord) const
{
    return adjoinings_.at(coord).size();
}

OffLatticeSpace::coordinate_type
OffLatticeSpace::get_neighbor(const coordinate_type& coord, const Integer& nrand) const
{
    return adjoinings_.at(coord).at(nrand);
}

OffLatticeSpace::coordinate_type
OffLatticeSpace::get_neighbor_boundary(const coordinate_type& coord, const Integer& nrand) const
{
    return get_neighbor(coord, nrand);
}

Integer OffLatticeSpace::size() const
{
    return voxels_.size();
}

Integer3 OffLatticeSpace::shape() const
{
    throw NotSupported("OffLatticeSpace::shape() is not supported.");
}

OffLatticeSpace::coordinate_type OffLatticeSpace::inner2coordinate(const coordinate_type inner) const
{
    throw NotSupported("OffLatticeSpace::inner2coordinate() is not supported.");
}

Integer OffLatticeSpace::inner_size() const
{
    return size();
}

/*
 * protected functions
 */

Integer OffLatticeSpace::count_voxels(const boost::shared_ptr<VoxelPool>& vp) const
{
    return static_cast<Integer>(std::count(voxels_.begin(), voxels_.end(), vp.get()));
}


bool OffLatticeSpace::is_in_range(const coordinate_type& coord) const
{
    return 0 <= coord && coord < voxels_.size();
}

OffLatticeSpace::coordinate_type
OffLatticeSpace::get_coord(const ParticleID& pid) const
{
    for (molecule_pool_map_type::const_iterator itr(molecule_pools_.begin());
         itr != molecule_pools_.end(); ++itr)
    {
        const boost::shared_ptr<MoleculePool>& vp((*itr).second);
        for (MoleculePool::const_iterator vitr(vp->begin());
             vitr != vp->end(); ++vitr)
        {
            if ((*vitr).pid == pid)
            {
                return (*vitr).coordinate;
            }
        }
    }
    // throw NotFound("A corresponding particle is not found");
    return -1;
}

} // ecell4

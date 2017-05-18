#ifndef __ECELL4_LATTICE_LATTICE_WORLD_HPP
#define __ECELL4_LATTICE_LATTICE_WORLD_HPP

#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include <ecell4/core/VoxelPool.hpp>
#include <ecell4/core/VoxelSpaceBase.hpp>
#include <ecell4/core/LatticeSpaceVectorImpl.hpp>
#include <ecell4/core/OffLatticeSpace.hpp>
#include <ecell4/core/RandomNumberGenerator.hpp>
#include <ecell4/core/SerialIDGenerator.hpp>
#include <ecell4/core/Model.hpp>
#include <ecell4/core/Shape.hpp>

namespace ecell4
{

namespace spatiocyte
{

struct MoleculeInfo
{
    const Real radius;
    const Real D;
    const std::string loc;
};

class SpatiocyteWorld : public Space
{
public:

    typedef LatticeSpaceVectorImpl default_space_type;

    typedef VoxelSpaceBase::coordinate_id_pair_type coordinate_id_pair_type;
    typedef VoxelSpaceBase::coordinate_type coordinate_type;

    typedef VoxelSpaceBase::identified_voxel identified_voxel;

    struct SpaceItem
    {
        boost::shared_ptr<VoxelSpaceBase> space;
        coordinate_type                   offset;
    };

public:

    SpatiocyteWorld(const Real3& edge_lengths, const Real& voxel_radius,
        const boost::shared_ptr<RandomNumberGenerator>& rng)
        : size_(0), rng_(rng)
    {
        add_space(new default_space_type(edge_lengths, voxel_radius));
    }

    SpatiocyteWorld(const Real3& edge_lengths, const Real& voxel_radius) : size_(0)
    {
        add_space(new default_space_type(edge_lengths, voxel_radius));
        rng_ = boost::shared_ptr<RandomNumberGenerator>(new GSLRandomNumberGenerator());
        (*rng_).seed();
    }

    SpatiocyteWorld(const Real3& edge_lengths = Real3(1, 1, 1)) : size_(0)
    {
        add_space(new default_space_type(edge_lengths, edge_lengths[0] / 100.0));
        rng_ = boost::shared_ptr<RandomNumberGenerator>(new GSLRandomNumberGenerator());
        (*rng_).seed();
    }

    SpatiocyteWorld(const std::string filename) : size_(0)
    {
        add_space(new default_space_type(Real3(1, 1, 1), 0.01));
        rng_ = boost::shared_ptr<RandomNumberGenerator>(new GSLRandomNumberGenerator());
        this->load(filename);
    }

    SpatiocyteWorld(VoxelSpaceBase* space, const boost::shared_ptr<RandomNumberGenerator>& rng)
        : size_(0), rng_(rng)
    {
        add_space(space);
    }

    /*
     * Class functions
     */

    /**
     * draw attributes of species and return it as a molecule info.
     * @param sp a species
     * @return info a molecule info
     */
    MoleculeInfo get_molecule_info(const Species& sp) const;

    std::pair<identified_voxel, bool> new_voxel(const Voxel& v)
    {
        ParticleID pid(sidgen_());
        const bool is_succeeded(update_voxel(pid, v));
        return std::make_pair(std::make_pair(pid, v), is_succeeded);
    }

    std::pair<identified_voxel, bool> new_voxel(const Species& sp, const coordinate_type& coord)
    {
        const MoleculeInfo minfo(get_molecule_info(sp));
        return new_voxel(Voxel(sp, coord, minfo.radius, minfo.D, minfo.loc));
    }

    std::pair<coordinate_type, bool>
    check_neighbor(const coordinate_type coord, const std::string& loc);

    boost::shared_ptr<RandomNumberGenerator> rng()
    {
        return rng_;
    }

    identified_voxel
    make_pid_voxel_pair(const VoxelPool* vpool, const coordinate_id_pair_type& info) const
    {
        const std::string loc(vpool->location()->is_vacant() ?
                "" : vpool->location()->species().serial());
        return identified_voxel(
            ParticleID(info.pid),
            Voxel(vpool->species(), info.coordinate, vpool->radius(), vpool->D(), loc));
    }

    identified_voxel
    make_pid_voxel_pair(const VoxelPool* vpool, const coordinate_type& coord) const
    {
        const ParticleID pid(vpool->get_particle_id(coord));
        const coordinate_id_pair_type info(pid, coord);
        return make_pid_voxel_pair(vpool, info);
    }

    identified_voxel choice(const Species& sp)
    {
        const MoleculePool* mpool(find_molecule_pool(sp));
        const Integer i(rng_->uniform_int(0, mpool->size()-1));
        return make_pid_voxel_pair(mpool, mpool->at(i));
    }

    boost::shared_ptr<Model> lock_model() const
    {
        return model_.lock();
    }

    // XXX: Not implemented
    const Species& draw_species(const Species& pttrn) const;

    // XXX: Never called
    Integer add_neighbors(const Species& sp, const coordinate_type center);


public:

    /*
     * Python API
     */

    void set_value(const Species& sp, const Real value);

    /**
     * create and add a new particle
     * @param p a particle
     * @return a pair of a pair of pid (a particle id) and p (a particle)
     * and bool (if it's succeeded or not)
     */
    std::pair<identified_particle, bool> new_particle(const Particle& p);
    std::pair<identified_particle, bool> new_particle(const Species& sp, const Real3& pos)
    {
        const MoleculeInfo info(get_molecule_info(sp));
        return new_particle(Particle(sp, pos, info.radius, info.D));
    }

    bool update_particle(const ParticleID& pid, const Particle& p);

    std::vector<identified_particle> list_structure_particles() const;
    std::vector<identified_particle> list_non_structure_particles() const;

    std::pair<identified_voxel, bool>
    new_voxel_structure(const Species& sp, const coordinate_type& coord);

    std::pair<identified_voxel, bool>
    new_voxel_interface(const Species& sp, const coordinate_type& coord);

    Integer add_structure(const Species& sp, const boost::shared_ptr<const Shape> shape);
    Integer add_interface(const Species& sp);

    std::vector<Species> list_structure_species() const;
    std::vector<Species> list_non_structure_species() const;

    bool add_molecules(const Species& sp, const Integer& num);
    bool add_molecules(const Species& sp, const Integer& num,
                       const boost::shared_ptr<const Shape> shape);
    void remove_molecules(const Species& sp, const Integer& num);

    void bind_to(boost::shared_ptr<Model> model);


protected:

    void add_space(VoxelSpaceBase *space);
    SpaceItem& get_corresponding_space(const coordinate_type& coordinate);
    const SpaceItem& get_corresponding_space(const coordinate_type& coordinate) const;
    std::pair<identified_voxel, bool> new_voxel_structure(const Voxel& v);
    Integer add_structure3(const Species& sp, const boost::shared_ptr<const Shape> shape);
    Integer add_structure2(const Species& sp, const boost::shared_ptr<const Shape> shape);
    bool is_surface_voxel(const coordinate_type coord,
            const boost::shared_ptr<const Shape> shape) const;

public:

    /*
     * Wrapped functions
     */

#define\
    wrap_getter(rettype, fn)\
    inline rettype fn() const { return spaces_.at(0).space->fn(); }

#define\
    wrap_getter_with_arg(rettype, fn, argtype)\
    inline rettype fn(argtype arg) const { return spaces_.at(0).space->fn(arg); }

#define\
    wrap_setter(fn, argtype)\
    inline void fn(argtype arg) { spaces_.at(0).space->fn(arg); }

#define\
    wrap_mutable(rettype, fn, argtype)\
    inline rettype fn(argtype arg) { return spaces_.at(0).space->fn(arg); }

    /*
     * SpaceTraits
     */
    wrap_getter(const Real, t)
    wrap_setter(set_t, const Real&)

    void save(const std::string& filename) const;
    void load(const std::string& filename);

    /*
     * CompartmentSpaceTraits
     */
    wrap_getter(const Real, volume)

    Integer num_species() const
    {
        Integer num_species = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_species += (*itr).space->num_species();
        }
        return num_species;
    }

    bool has_species(const Species& species) const
    {
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            if ((*itr).space->has_species(species))
                return true;
        }
        return false;
    }

    // bool has_species_exact(const Species &sp) const;

    Integer num_molecules(const Species& species) const
    {
        Integer num_molecules = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_molecules += (*itr).space->num_molecules(species);
        }
        return num_molecules;
    }

    Integer num_molecules_exact(const Species& species) const
    {
        Integer num_molecules = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_molecules += (*itr).space->num_molecules_exact(species);
        }
        return num_molecules;
    }

    wrap_getter_with_arg(Real, get_value,       const Species&)
    wrap_getter_with_arg(Real, get_value_exact, const Species&)

    /*
     * ParticleSpaceTraits
     */
    wrap_getter(const Real3&, edge_lengths)
    wrap_getter(Real3,        actual_lengths)

    Integer num_particles() const
    {
        Integer num_particles(0);
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_particles += (*itr).space->num_particles();
        }
        return num_particles;
    }

    Integer num_particles(const Species& species) const
    {
        Integer num_particles(0);
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_particles += (*itr).space->num_particles(species);
        }
        return num_particles;
    }

    Integer num_particles_exact(const Species& species) const
    {
        Integer num_particles(0);
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_particles += (*itr).space->num_particles_exact(species);
        }
        return num_particles;
    }

    bool has_particle(const ParticleID& pid) const
    {
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            if ((*itr).space->has_particle(pid))
                return true;
        }
        return false;
    }

    wrap_getter_with_arg(identified_particle, get_particle, const ParticleID&)

    wrap_getter         (std::vector<identified_particle>, list_particles)
    wrap_getter_with_arg(std::vector<identified_particle>, list_particles,       const Species&)
    wrap_getter_with_arg(std::vector<identified_particle>, list_particles_exact, const Species&)

    /*
     * LatticeSpaceTraits
     */
    const Real3 coordinate2position(const coordinate_type& coordinate) const
    {
        const SpaceItem& item(get_corresponding_space(coordinate));
        return item.space->coordinate2position(coordinate - item.offset);
    }

    wrap_getter_with_arg(coordinate_type, position2coordinate, const Real3&)

    Integer num_neighbors(const coordinate_type& coordinate) const
    {
        const SpaceItem& item(get_corresponding_space(coordinate));
        return item.space->num_neighbors(coordinate - item.offset);
    }

    coordinate_type get_neighbor(coordinate_type coordinate, Integer nrand) const
    {
        const SpaceItem& item(get_corresponding_space(coordinate));
        return item.space->get_neighbor(coordinate - item.offset, nrand) + item.offset;
    }

    coordinate_type get_neighbor_boundary(coordinate_type coordinate, Integer nrand) const
    {
        const SpaceItem& item(get_corresponding_space(coordinate));
        return item.space->get_neighbor_boundary(coordinate - item.offset, nrand) + item.offset;
    }

    Integer size() const
    {
        return size_;
    }

    wrap_getter(const Integer3, shape)
    wrap_getter(const Integer,  inner_size)
    wrap_getter_with_arg(const coordinate_type, inner2coordinate, const coordinate_type)

    /*
     * VoxelSpaceTraits
     */
    wrap_getter(Real, voxel_radius)
    wrap_getter(Real, unit_voxel_volume)
    wrap_getter(Real, voxel_volume)
    wrap_getter(Real, unit_area)

    wrap_getter(std::vector<Species>, list_species)

    Integer num_voxels() const
    {
        Integer num_voxels = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_voxels += (*itr).space->num_voxels();
        }
        return num_voxels;
    }

    Integer num_voxels(const Species& species) const
    {
        Integer num_voxels = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_voxels += (*itr).space->num_voxels(species);
        }
        return num_voxels;
    }

    Integer num_voxels_exact(const Species& species) const
    {
        Integer num_voxels = 0;
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            num_voxels += (*itr).space->num_voxels_exact(species);
        }
        return num_voxels;
    }

    bool has_voxel(const ParticleID& pid) const
    {
        for (std::vector<SpaceItem>::const_iterator itr(spaces_.begin()); itr != spaces_.end(); ++itr)
        {
            if ((*itr).space->has_voxel(pid))
                return true;
        }
        return false;
    }

    wrap_getter_with_arg(identified_voxel, get_voxel, const ParticleID&)

    wrap_getter         (std::vector<identified_voxel>, list_voxels)
    wrap_getter_with_arg(std::vector<identified_voxel>, list_voxels,       const Species&)
    wrap_getter_with_arg(std::vector<identified_voxel>, list_voxels_exact, const Species&)

    // bool has_voxel_pool(const Species& sp) const;

    wrap_mutable        (      VoxelPool*, find_voxel_pool, const Species&)
    wrap_getter_with_arg(const VoxelPool*, find_voxel_pool, const Species&)

    wrap_getter_with_arg(bool, has_molecule_pool, const Species&)

    wrap_mutable        (      MoleculePool*, find_molecule_pool, const Species&)
    wrap_getter_with_arg(const MoleculePool*, find_molecule_pool, const Species&)

    wrap_mutable(bool, remove_particle, const ParticleID&)
    wrap_mutable(bool, on_structure, const Voxel&)

    /* Not wrapped functions
    bool make_structure_type(const Species& sp,
                             Shape::dimension_kind dimension,
                             const std::string loc);
    bool make_interface_type(const Species& sp,
                             Shape::dimension_kind dimension,
                             const std::string loc);
    bool make_molecular_pool(const Species& sp,
                             Real radius,
                             Real D,
                             const std::string loc);
     */

    wrap_getter_with_arg(identified_voxel, get_voxel_at,      const coordinate_type&)
    wrap_getter_with_arg(const VoxelPool*, get_voxel_pool_at, const coordinate_type&)

    /* Not wrapped function
    virtual const Particle particle_at(const coordinate_type& coord) const;
    */

    bool update_voxel(const ParticleID& pid, const Voxel& v)
    {
        return spaces_.at(0).space->update_voxel(pid, v);
    }

    wrap_mutable(bool, remove_voxel, const ParticleID&)
    wrap_mutable(bool, remove_voxel, const coordinate_type&)

    bool can_move(const coordinate_type& src, const coordinate_type& dest) const
    {
        return (*spaces_.at(0).space).can_move(src, dest);
    }

    bool move(const coordinate_type& src, const coordinate_type& dest,
              const std::size_t candidate=0)
    {
        return spaces_.at(0).space->move(src, dest, candidate);
    }

    std::pair<coordinate_type, bool>
    move_to_neighbor(VoxelPool* const& from_mt, VoxelPool* const& loc,
                     coordinate_id_pair_type& info, const Integer nrand)
    {
        return spaces_.at(0).space->move_to_neighbor(from_mt, loc, info, nrand);
    }

#undef wrap_getter
#undef wrap_getter_with_arg
#undef wrap_setter

protected:

    Integer size_;
    std::vector<SpaceItem> spaces_;

    boost::shared_ptr<RandomNumberGenerator> rng_;
    SerialIDGenerator<ParticleID> sidgen_;

    boost::weak_ptr<Model> model_;
};

SpatiocyteWorld* create_spatiocyte_world_cell_list_impl(
    const Real3& edge_lengths, const Real& voxel_radius,
    const Integer3& matrix_sizes,
    const boost::shared_ptr<RandomNumberGenerator>& rng);
SpatiocyteWorld* create_spatiocyte_world_vector_impl(
    const Real3& edge_lengths, const Real& voxel_radius,
    const boost::shared_ptr<RandomNumberGenerator>& rng);
SpatiocyteWorld* create_spatiocyte_world_offlattice_impl(
    const Real3& edge_lengths, const Real& voxel_radius,
    const boost::shared_ptr<RandomNumberGenerator>& rng);

/**
 * Alias functions for Cython
 */

inline SpatiocyteWorld* create_spatiocyte_world_cell_list_impl_alias(
    const Real3& edge_lengths, const Real& voxel_radius,
    const Integer3& matrix_sizes,
    const boost::shared_ptr<RandomNumberGenerator>& rng)
{
    return create_spatiocyte_world_cell_list_impl(
        edge_lengths, voxel_radius, matrix_sizes, rng);
}

inline SpatiocyteWorld* create_spatiocyte_world_vector_impl_alias(
    const Real3& edge_lengths, const Real& voxel_radius,
    const boost::shared_ptr<RandomNumberGenerator>& rng)
{
    return create_spatiocyte_world_vector_impl(edge_lengths, voxel_radius, rng);
}

inline SpatiocyteWorld* create_spatiocyte_world_offlattice_impl_alias(
    const Real3& edge_lengths, const Real& voxel_radius,
    const boost::shared_ptr<RandomNumberGenerator>& rng)
{
    return create_spatiocyte_world_offlattice_impl(edge_lengths, voxel_radius, rng);
}

} // spatiocyte

} // ecell4

#endif /* __ECELL4_LATTICE_LATTICE_WORLD_HPP */

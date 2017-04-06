#define BOOST_TEST_MODULE "OffLatticeSpace_test"

#ifdef UNITTEST_FRAMEWORK_LIBRARY_EXIST
#   include <boost/test/unit_test.hpp>
#else
#   define BOOST_TEST_NO_LIB
#   include <boost/test/included/unit_test.hpp>
#endif

#include <boost/test/floating_point_comparison.hpp>

#include <ecell4/core/OffLatticeSpace.hpp>
#include <ecell4/core/SerialIDGenerator.hpp>

using namespace ecell4;

struct Fixture
{
    const Real voxel_radius;
    const Species species;
    const Voxel voxel;
    OffLatticeSpace space;
    SerialIDGenerator<ParticleID> sidgen;

    Fixture() :
        voxel_radius(2.5e-9),
        species(/* serial = */ "SpeciesA",
                /* radius = */ "2.5e-9",
                /* D = */      "1e-12"),
        voxel(/* species = */    species,
              /* coordinate = */ 3,
              /* radius = */     2.5e-9,
              /* D = */          1e-12),
        space(voxel_radius)
    {
        OffLatticeSpace::position_container positions;
        const Real unit(voxel_radius / sqrt(3.0));
        for (int i(0); i < 10; ++i)
            positions.push_back(
                    Real3(unit * i, unit * i, unit * i));
        OffLatticeSpace::coordinate_pair_list_type adjoining_pairs;
        for (int i(1); i < 10; ++i )
            adjoining_pairs.push_back(
                    std::make_pair(i-1, i));
        space.reset(positions, adjoining_pairs);
        // space = OffLatticeSpace(voxel_radius, positions, adjoining_pairs);
    }
};

BOOST_FIXTURE_TEST_SUITE(suite, Fixture)

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_constructor) {}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_molecules)
{
    const ParticleID pid(sidgen());
    space.update_voxel(pid, voxel);

    BOOST_CHECK_EQUAL(space.num_molecules(species), 1);
}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_voxelspacebase)
{
    const ParticleID pid(sidgen());
    space.update_voxel(pid, voxel);

    BOOST_CHECK_EQUAL(space.list_species().size(), 1);
    BOOST_CHECK_EQUAL(space.num_voxels_exact(species), 1);
    BOOST_CHECK_EQUAL(space.num_voxels(species), 1);
    BOOST_CHECK_EQUAL(space.num_voxels(), 1);

    BOOST_CHECK(space.has_voxel(pid));
    BOOST_CHECK(!space.has_voxel(sidgen()));

    BOOST_CHECK_EQUAL(space.list_voxels().size(), 1);
    BOOST_CHECK_EQUAL(space.list_voxels(species).size(), 1);
    BOOST_CHECK_EQUAL(space.list_voxels_exact(species).size(), 1);

    BOOST_CHECK_EQUAL(space.list_voxels().at(0).first, pid);
    BOOST_CHECK_EQUAL(space.list_voxels(species).at(0).first, pid);
    BOOST_CHECK_EQUAL(space.list_voxels_exact(species).at(0).first, pid);

    BOOST_CHECK_EQUAL(space.get_voxel(pid).first, pid);

    BOOST_CHECK_NO_THROW(space.find_voxel_pool(species));
    BOOST_CHECK(space.has_molecule_pool(species));
    BOOST_CHECK_NO_THROW(space.find_molecule_pool(species));
}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_voxel)
{
    const ParticleID pid(sidgen());

    BOOST_CHECK(space.update_voxel(pid, voxel));
    BOOST_CHECK(space.remove_voxel(pid));
    BOOST_CHECK(!space.remove_voxel(3));

    BOOST_CHECK(space.update_voxel(pid, voxel));
    BOOST_CHECK(space.remove_voxel(3));
    BOOST_CHECK(!space.remove_voxel(pid));
}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_move)
{
    const ParticleID pid(sidgen());
    BOOST_CHECK(space.update_voxel(pid, voxel));

    BOOST_CHECK(space.can_move(3, 4));
    BOOST_CHECK(space.move(3, 4, 0));
    BOOST_CHECK_EQUAL(space.get_voxel_at(3).first, ParticleID());
    BOOST_CHECK_EQUAL(space.get_voxel_at(4).first, pid);

    BOOST_CHECK(!space.can_move(3, 4));

    BOOST_CHECK(space.can_move(4, 5));
    OffLatticeSpace::coordinate_id_pair_type info(pid, 4);
    std::pair<OffLatticeSpace::coordinate_type, bool> result(
            space.move_to_neighbor(
                /* src_vp = */ space.get_voxel_pool_at(4),
                /* loc = */    space.get_voxel_pool_at(4)->location(),
                /* info = */   info,
                /* nrand = */  1));
    BOOST_CHECK_EQUAL(result.first, 5);
    BOOST_CHECK(result.second);
    BOOST_CHECK_EQUAL(space.get_voxel_at(4).first, ParticleID());
    BOOST_CHECK_EQUAL(space.get_voxel_at(5).first, pid);
}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_at)
{
    BOOST_CHECK_EQUAL(space.size(), 10);

    const ParticleID pid(sidgen());
    BOOST_CHECK(space.update_voxel(pid, voxel));

    BOOST_CHECK_NO_THROW(space.get_voxel_at(3));
    BOOST_CHECK_EQUAL(space.get_voxel_at(3).first, pid);

    BOOST_CHECK_NO_THROW(space.particle_at(3));
    BOOST_CHECK_EQUAL(space.particle_at(3).species(), species);
    BOOST_CHECK_EQUAL(space.particle_at(3).position(), space.coordinate2position(3));
    BOOST_CHECK_EQUAL(space.particle_at(3).radius(), 2.5e-9);
    BOOST_CHECK_EQUAL(space.particle_at(3).D(), 1e-12);
}

BOOST_AUTO_TEST_CASE(OffLatticeSpace_test_neighbor)
{
    BOOST_CHECK_EQUAL(space.num_neighbors(0), 1);
    BOOST_CHECK_EQUAL(space.num_neighbors(1), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(2), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(3), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(4), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(5), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(6), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(7), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(8), 2);
    BOOST_CHECK_EQUAL(space.num_neighbors(9), 1);

    BOOST_CHECK_EQUAL(space.get_neighbor(0, 0), 1);
    BOOST_CHECK_EQUAL(space.get_neighbor(1, 0), 0);
    BOOST_CHECK_EQUAL(space.get_neighbor(1, 1), 2);
    BOOST_CHECK_EQUAL(space.get_neighbor(2, 0), 1);
    BOOST_CHECK_EQUAL(space.get_neighbor(2, 1), 3);
    BOOST_CHECK_EQUAL(space.get_neighbor(3, 0), 2);
    BOOST_CHECK_EQUAL(space.get_neighbor(3, 1), 4);
    BOOST_CHECK_EQUAL(space.get_neighbor(4, 0), 3);
    BOOST_CHECK_EQUAL(space.get_neighbor(4, 1), 5);
    BOOST_CHECK_EQUAL(space.get_neighbor(5, 0), 4);
    BOOST_CHECK_EQUAL(space.get_neighbor(5, 1), 6);
    BOOST_CHECK_EQUAL(space.get_neighbor(6, 0), 5);
    BOOST_CHECK_EQUAL(space.get_neighbor(6, 1), 7);
    BOOST_CHECK_EQUAL(space.get_neighbor(7, 0), 6);
    BOOST_CHECK_EQUAL(space.get_neighbor(7, 1), 8);
    BOOST_CHECK_EQUAL(space.get_neighbor(8, 0), 7);
    BOOST_CHECK_EQUAL(space.get_neighbor(8, 1), 9);
    BOOST_CHECK_EQUAL(space.get_neighbor(9, 0), 8);
}

BOOST_AUTO_TEST_SUITE_END()


struct HCPOffLatticeSpaceTestFixture {
    const Real voxel_radius;
    boost::shared_ptr<OffLatticeSpace> space;
    SerialIDGenerator<ParticleID> sidgen;

    HCPOffLatticeSpaceTestFixture() :
        voxel_radius(2.5e-9),
        space(create_hcp_offlattice_space(voxel_radius, Integer3(10,10,10)))
    {}
};

BOOST_FIXTURE_TEST_SUITE(HCPOffLatticeSpaceTest, HCPOffLatticeSpaceTestFixture)

BOOST_AUTO_TEST_CASE(Size)
{
    BOOST_CHECK_EQUAL(space->size(), 1000);
}

BOOST_AUTO_TEST_CASE(NeighborList)
{
    for (Integer i(0); i < space->size(); ++i)
    {
        BOOST_CHECK_EQUAL(space->num_neighbors(i), 12);

        for (Integer j(1); j < space->num_neighbors(i); ++j)
            for (Integer k(0); k < j; ++k)
                BOOST_CHECK(space->get_neighbor(i, j) != space->get_neighbor(i, k));

    }
}

BOOST_AUTO_TEST_CASE(NeighborDistance)
{
    const Real voxel_x(voxel_radius * 2.0 * sqrt(6.0) / 3.0),
               voxel_y(voxel_radius * sqrt(3.0)),
               voxel_z(voxel_radius * 2.0);

    for (Integer i(0); i < space->size(); ++i)
    {
        const Real3 position(space->coordinate2position(i));
        for (Integer j(0); j < space->num_neighbors(i); ++j)
        {
            const Real3 neighbor(space->coordinate2position(space->get_neighbor(i, j)));
            Real3 direction(neighbor-position);
            if (direction[0] > 5 * voxel_x)
                direction[0] -= 10 * voxel_x;
            else if (direction[0] < -5 * voxel_x)
                direction[0] += 10 * voxel_x;

            if (direction[1] > 5 * voxel_y)
                direction[1] -= 10 * voxel_y;
            else if (direction[1] < -5 * voxel_y)
                direction[1] += 10 * voxel_y;

            if (direction[2] > 5 * voxel_z)
                direction[2] -= 10 * voxel_z;
            else if (direction[2] < -5 * voxel_z)
                direction[2] += 10 * voxel_z;

            BOOST_CHECK(length(direction) < voxel_radius*2.1);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

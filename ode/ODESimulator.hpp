#ifndef __ODESIMULATOR_HPP
#define __ODESIMULATOR_HPP

#include <boost/shared_ptr.hpp>

#include <ecell4/core/exceptions.hpp>
#include <ecell4/core/types.hpp>
#include <ecell4/core/get_mapper_mf.hpp>
#include <ecell4/core/NetworkModel.hpp>
#include <ecell4/core/Simulator.hpp>

#include "ODEWorld.hpp"

namespace ecell4
{

namespace ode
{

class ODESystem
{
public:

    typedef std::vector<double> state_type;

    ODESystem(boost::shared_ptr<NetworkModel> model, Real const& volume)
        : model_(model), volume_(volume)
    {
        initialize();
    }

    void initialize()
    {
        NetworkModel::species_container_type const& species(model_->species());
        state_type::size_type i(0);
        for (NetworkModel::species_container_type::const_iterator
                 it(species.begin()); it != species.end(); ++it)
        {
            index_map_[*it] = i;
            ++i;
        }
    }

    void operator()(state_type const& x, state_type& dxdt, double const& t)
    {
        for (state_type::iterator i(dxdt.begin()); i != dxdt.end(); ++i)
        {
            *i = 0.0;
        }

        NetworkModel::reaction_rule_container_type const&
            reaction_rules(model_->reaction_rules());
        for (NetworkModel::reaction_rule_container_type::const_iterator
                 i(reaction_rules.begin()); i != reaction_rules.end(); ++i)
        {
            double flux((*i).k() * volume_);

            ReactionRule::reactant_container_type const&
                reactants((*i).reactants());
            ReactionRule::product_container_type const&
                products((*i).products());
            for (ReactionRule::reactant_container_type::iterator
                     j(reactants.begin()); j != reactants.end(); ++j)
            {
                flux *= x[index_map_[*j]] / volume_;
            }

            for (ReactionRule::reactant_container_type::iterator
                     j(reactants.begin()); j != reactants.end(); ++j)
            {
                dxdt[index_map_[*j]] -= flux;
            }

            for (ReactionRule::product_container_type::iterator
                     j(products.begin()); j != products.end(); ++j)
            {
                dxdt[index_map_[*j]] += flux;
            }
        }
    }

protected:

    typedef utils::get_mapper_mf<
        Species, state_type::size_type>::type species_map_type;

    boost::shared_ptr<NetworkModel> model_;
    Real volume_;

    species_map_type index_map_;
};

struct StateAndTimeBackInserter
{
    typedef std::vector<ODESystem::state_type> state_container_type;
    typedef std::vector<double> time_container_type;

    state_container_type& m_states;
    time_container_type& m_times;

    StateAndTimeBackInserter(
        state_container_type& states, time_container_type& times)
        : m_states(states), m_times(times)
    {
        ;
    }

    void operator()(ODESystem::state_type const&x, double t)
    {
        m_states.push_back(x);
        m_times.push_back(t);
    }
};

class ODESimulator
    : public Simulator
{
public:
    ODESimulator(
        boost::shared_ptr<NetworkModel> model,
        boost::shared_ptr<ODEWorld> world)
        : model_(model), world_(world), t_(0), num_steps_(0)
    {
        ;
    }

    void step(void)
    {
        throw NotImplemented("a step size must be specified.");
    }

    bool step(Real const& upto);

    Integer num_steps(void) const
    {
        return num_steps_;
    }

    Real t(void) const
    {
        return t_;
    }

protected:

    boost::shared_ptr<NetworkModel> model_;
    boost::shared_ptr<ODEWorld> world_;

    Real t_;
    Integer num_steps_;
};

} // ode

} // ecell4

#endif //__ODESIMULATOR_HPP
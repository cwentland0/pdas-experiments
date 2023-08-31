#ifndef PDAS_EXPERIMENTS_LSPG_HPP_
#define PDAS_EXPERIMENTS_LSPG_HPP_

#include "pressio/ode_advancers.hpp"
#include "pressio/rom_subspaces.hpp"
#include "pressio/rom_lspg_unsteady.hpp"
#include "observer.hpp"

template<class AppType, class ParserType>
void run_mono_lspg(AppType & system, ParserType & parser)
{
    namespace pdas = pdaschwarz;
    namespace plspg = pressio::rom::lspg;
    namespace pnlins = pressio::nonlinearsolvers;

    using app_t = AppType;
    using scalar_type = typename app_t::scalar_type;
    using reduced_state_type = Eigen::Matrix<scalar_type, Eigen::Dynamic, 1>;

    // read and define trial space
    auto trans = pdas::read_vector_from_binary<scalar_type>(parser.romAffineShiftFile());
    auto basis = pdas::read_matrix_from_binary<scalar_type>(
        parser.romFullMeshPodBasisFile(), parser.romModeCount());
    const auto trialSpace = pressio::rom::create_trial_column_subspace<
        reduced_state_type>(move(basis), move(trans), true);

    // project initial condition
    auto state = system.initialCondition();
    auto u = pressio::ops::clone(state);
    pressio::ops::update(
        u, 0.,
        state, 1,
        trialSpace.translationVector(), -1);
    auto reducedState = trialSpace.createReducedState();
    pressio::ops::product(::pressio::transpose(),
        1., trialSpace.basisOfTranslatedSpace(), u,
        0., reducedState);

    // define ROM problem
    const auto odeScheme = parser.odeScheme();
    auto problem = pressio::rom::lspg::create_unsteady_problem(
        odeScheme, trialSpace, system);
    auto stepperObj = problem.lspgStepper();
    using stepperType = decltype(stepperObj);

    // define solver
    using hessian_t       = Eigen::Matrix<scalar_type, -1, -1>;
    using solver_tag      = pressio::linearsolvers::direct::HouseholderQR;
    using linear_solver_t = pressio::linearsolvers::Solver<solver_tag, hessian_t>;
    linear_solver_t linSolverObj;

    auto NonLinSolver = pressio::create_gauss_newton_solver(stepperObj, linSolverObj);
    // TODO: generalize this
    NonLinSolver.setStopCriterion(pnlins::Stop::WhenAbsolutel2NormOfGradientBelowTolerance);
    NonLinSolver.setStopTolerance(1e-5);

    StateObserver Obs(parser.stateSamplingFreq());
    const auto startTime = static_cast<typename app_t::scalar_type>(0.0);
    pressio::ode::advance_n_steps(stepperObj, reducedState, startTime,
        parser.timeStepSize(),
        pressio::ode::StepCount(parser.numSteps()),
        Obs, NonLinSolver);
}

#endif
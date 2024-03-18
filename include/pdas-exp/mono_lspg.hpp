#ifndef PDAS_EXPERIMENTS_LSPG_HPP_
#define PDAS_EXPERIMENTS_LSPG_HPP_

#include "pressio/ode_advancers.hpp"
#include "pressio/rom_subspaces.hpp"
#include "pressio/rom_lspg_unsteady.hpp"
#include "pda-schwarz/rom_utils.hpp"
#include "observer.hpp"
#include <chrono>

template<class AppType, class ParserType>
void run_mono_lspg(AppType & system, ParserType & parser)
{
    pressio::log::initialize(pressio::logto::terminal);
    pressio::log::setVerbosity({parser.loglevel()});

    namespace pda    = pressiodemoapps;
    namespace pdas   = pdaschwarz;
    namespace plspg  = pressio::rom::lspg;
    namespace pnlins = pressio::nonlinearsolvers;

    using app_t              = AppType;
    using scalar_type        = typename app_t::scalar_type;
    using reduced_state_type = Eigen::Matrix<scalar_type, Eigen::Dynamic, 1>;
    using hessian_t          = Eigen::Matrix<scalar_type, -1, -1>;
    using solver_tag         = pressio::linearsolvers::direct::HouseholderQR;
    using linear_solver_t    = pressio::linearsolvers::Solver<solver_tag, hessian_t>;
    linear_solver_t linSolverObj;

    const auto numDofsPerCell = system.numDofPerCell();
    auto state = system.initialCondition();
    StateObserver Obs(parser.stateSamplingFreq());
    RuntimeObserver Obs_run("runtime.bin");
    const auto startTime = static_cast<typename app_t::scalar_type>(0.0);

    // UNSAMPLED ROM
    if (!parser.isHyper()) {

        // read and define full trial space
        auto trans = pdas::read_vector_from_binary<scalar_type>(
            parser.romTransFile());
        auto basis = pdas::read_matrix_from_binary<scalar_type>(
            parser.romBasisFile(), parser.romModeCount());
        const auto trialSpace = pressio::rom::create_trial_column_subspace<
            reduced_state_type>(std::move(basis), std::move(trans), true);

        // project initial condition
        auto u = pressio::ops::clone(state);
        pressio::ops::update(
            u, 0.,
            state, 1,
            trialSpace.translationVector(), -1);
        auto reducedState = trialSpace.createReducedState();
        pressio::ops::product(::pressio::transpose(),
            1., trialSpace.basisOfTranslatedSpace(), u,
            0., reducedState);

        auto problem = pressio::rom::lspg::create_unsteady_problem(
            parser.odeScheme(), trialSpace, system);
        auto stepperObj = problem.lspgStepper();

        auto NonLinSolver = pressio::create_gauss_newton_solver(stepperObj, linSolverObj);
        // TODO: generalize this
        NonLinSolver.setStopCriterion(pnlins::Stop::WhenAbsolutel2NormOfGradientBelowTolerance);
        NonLinSolver.setStopTolerance(1e-5);

        // execute
        auto runtimeStart = std::chrono::high_resolution_clock::now();
        pressio::ode::advance_n_steps(
            stepperObj, reducedState, startTime,
            parser.timeStepSize(),
            pressio::ode::StepCount(parser.numSteps()),
            Obs, NonLinSolver);
        auto runtimeEnd = std::chrono::high_resolution_clock::now();
        auto nsElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(runtimeEnd - runtimeStart).count();
        double secElapsed = static_cast<double>(nsElapsed) * 1e-9;
        Obs_run(secElapsed);

    }
    // HYPER-REDUCED ROM
    else {
        const auto meshObjHyp = pda::load_cellcentered_uniform_mesh_eigen(parser.meshDirHyper());
        auto systemHyp = pda::create_problem_eigen(
            meshObjHyp, parser.probId(), parser.fluxOrder(),
            parser.icFlag(), parser.userParams()
        );

        // read and define sampled trial space
        auto transFull = pdas::read_vector_from_binary<scalar_type>(
            parser.romTransFile());
        auto basisFull = pdas::read_matrix_from_binary<scalar_type>(
            parser.romBasisFile(), parser.romModeCount());
        const auto stencilGids = pdas::create_cell_gids_vector_and_fill_from_ascii(parser.hyperStencilFile());
        auto transHyp = pdas::reduce_vector_on_stencil_mesh(transFull, stencilGids, numDofsPerCell);
        auto basisHyp = pdas::reduce_matrix_on_stencil_mesh(basisFull, stencilGids, numDofsPerCell);
        const auto trialSpaceFull = pressio::rom::create_trial_column_subspace<
            reduced_state_type>(std::move(basisFull), std::move(transFull), true);
        const auto trialSpaceHyp = pressio::rom::create_trial_column_subspace<
            reduced_state_type>(std::move(basisHyp), std::move(transHyp), true);

        // project initial condition
        auto u = pressio::ops::clone(state);
        pressio::ops::update(
            u, 0.,
            state, 1,
            trialSpaceFull.translationVector(), -1);
        auto reducedState = trialSpaceFull.createReducedState();
        pressio::ops::product(::pressio::transpose(),
            1., trialSpaceFull.basisOfTranslatedSpace(), u,
            0., reducedState);

        pdas::HypRedUpdater<scalar_type> hrUpdater(numDofsPerCell, parser.hyperStencilFile(), parser.hyperSampleFile());

        // define ROM problem
        auto problem = plspg::create_unsteady_problem(parser.odeScheme(), trialSpaceHyp, systemHyp, hrUpdater);
        auto stepperObj = problem.lspgStepper();

        // nonlinear solver
        auto weigher = pdaschwarz::Weigher<scalar_type>(
            parser.gpodWeigherType(),
            parser.gpodBasisFile(),
            parser.hyperSampleFile(),
            parser.gpodModeCount(),
            numDofsPerCell
        );
        auto NonLinSolver = pressio::create_gauss_newton_solver(stepperObj, linSolverObj, weigher);
        // TODO: generalize this
        NonLinSolver.setStopCriterion(pnlins::Stop::WhenAbsolutel2NormOfGradientBelowTolerance);
        NonLinSolver.setStopTolerance(1e-5);

        // execute
        auto runtimeStart = std::chrono::high_resolution_clock::now();
        pressio::ode::advance_n_steps(
            stepperObj, reducedState, startTime,
            parser.timeStepSize(),
            pressio::ode::StepCount(parser.numSteps()),
            Obs, NonLinSolver);
        auto runtimeEnd = std::chrono::high_resolution_clock::now();
        auto nsElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(runtimeEnd - runtimeStart).count();
        double secElapsed = static_cast<double>(nsElapsed) * 1e-9;
        Obs_run(secElapsed);

    }

    pressio::log::finalize();

}

#endif
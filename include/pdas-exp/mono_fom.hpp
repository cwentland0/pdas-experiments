#include "pressio/ode_steppers_implicit.hpp"
#include "pressio/ode_advancers.hpp"
#include "observer.hpp"

template<class AppType, class ParserType>
void run_mono_fom(AppType & system, ParserType & parser)
{
    using app_t = AppType;
    using state_t = typename app_t::state_type;
    using jacob_t = typename app_t::jacobian_type;

    state_t state = system.initialCondition();

    const auto odeScheme = parser.odeScheme();
    auto stepperObj = pressio::ode::create_implicit_stepper(odeScheme, system);

    using lin_solver_t = pressio::linearsolvers::Solver<
        pressio::linearsolvers::iterative::Bicgstab, jacob_t>;
    lin_solver_t linSolverObj;
    auto NonLinSolver = pressio::create_newton_solver(stepperObj, linSolverObj);
    NonLinSolver.setStopTolerance(1e-5);

    StateObserver Obs(parser.stateSamplingFreq());
    const auto startTime = static_cast<typename app_t::scalar_type>(0.0);
    pressio::ode::advance_n_steps(
        stepperObj, state, startTime,
        parser.timeStepSize(),
        pressio::ode::StepCount(parser.numSteps()),
        Obs, NonLinSolver);
}
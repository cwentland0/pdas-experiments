
#ifndef PDAS_EXPERIMENTS_DECOMP_HPP_
#define PDAS_EXPERIMENTS_DECOMP_HPP_

#include "pda-schwarz/schwarz.hpp"
#include "observer.hpp"

template<class AppType, class ParserType>
void run_decomp(ParserType & parser)
{

    namespace pda  = pressiodemoapps;
    namespace pdas = pdaschwarz;
    namespace pode = pressio::ode;

    // FIXME: generalize when expanded in Schwarz
    const auto order  = pda::InviscidFluxReconstruction::FirstOrder;

    // tiling, meshes, and decomposition
    auto tiling = std::make_shared<pdas::Tiling>(parser.meshDir());
    auto [meshPaths, meshObjs] = pdas::create_meshes(parser.meshDir(), tiling->count());
    auto subdomains = pdas::create_subdomains<AppType>(
        meshPaths, meshObjs, *tiling,
        parser.probId(),
        parser.odeScheme(),
        order,
        parser.domTypeVec(),
        parser.romAffineShiftRoot(),
        parser.romPodBasisRoot(),
        parser.romModeCountVec(),
        parser.icFlag(),
        parser.userParams());
    auto dtVec = parser.dtVec();
    pdas::SchwarzDecomp decomp(subdomains, tiling, dtVec);

    // observer
    vector<StateObserver> obsVec((*decomp.m_tiling).count());
    for (int domIdx = 0; domIdx < (*decomp.m_tiling).count(); ++domIdx) {
        obsVec[domIdx] = StateObserver("state_snapshots_" + to_string(domIdx) + ".bin", parser.stateSamplingFreq());
        obsVec[domIdx](::pressio::ode::StepCount(0), 0.0, decomp.m_subdomainVec[domIdx]->m_state);
    }
    RuntimeObserver obs_time("runtime.bin", (*tiling).count());

    // solve
    const int numSteps = parser.finalTime() / decomp.m_dtMax;
    double time = 0.0;
    for (int outerStep = 1; outerStep <= numSteps; ++outerStep)
    {
        cout << "Step " << outerStep << endl;

        // compute contoller step until convergence
        auto runtimeIter = decomp.calc_controller_step(
            outerStep,
            time,
            parser.relTol(),
            parser.absTol(),
            parser.convStepMax(),
            parser.isAdditive()
        );

        time += decomp.m_dtMax;

        // output observer
        if ((outerStep % parser.stateSamplingFreq()) == 0) {
            const auto stepWrap = pode::StepCount(outerStep);
            for (int domIdx = 0; domIdx < (*decomp.m_tiling).count(); ++domIdx) {
                obsVec[domIdx](stepWrap, time, decomp.m_subdomainVec[domIdx]->m_state);
            }
        }

        // runtime observer
        obs_time(runtimeIter);

    }
}

#endif
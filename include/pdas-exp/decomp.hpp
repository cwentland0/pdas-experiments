
#ifndef PDAS_EXPERIMENTS_DECOMP_HPP_
#define PDAS_EXPERIMENTS_DECOMP_HPP_

#include <chrono>
#include "pda-schwarz/schwarz.hpp"
#include "observer.hpp"

template<class AppType, class ParserType>
void run_decomp(ParserType & parser)
{

    namespace pda  = pressiodemoapps;
    namespace pdas = pdaschwarz;
    namespace pode = pressio::ode;

    // tiling and meshes
    auto tiling = std::make_shared<pdas::Tiling>(parser.meshDirFull());
    auto [meshObjsFull, meshPathsFull] = pdas::create_meshes(parser.meshDirFull(), tiling->count());

    auto schemeVec = parser.schemeVec();
    auto fluxOrderVec = parser.fluxOrderVec();
    auto subdomains = pdas::create_subdomains<AppType>(
        meshObjsFull, *tiling,
        parser.probId(),
        schemeVec,
        fluxOrderVec,
        parser.domTypeVec(),
        parser.romTransRoot(),
        parser.romBasisRoot(),
        parser.romModeCountVec(),
        parser.icFlag(),
        parser.hyperSampleFiles(),
        parser.userParams()
    );
    auto dtVec = parser.dtVec();
    pdas::SchwarzDecomp decomp(subdomains, tiling, dtVec);

    // observer
    std::vector<StateObserver> obsVec((*decomp.m_tiling).count());
    for (int domIdx = 0; domIdx < (*decomp.m_tiling).count(); ++domIdx) {
        obsVec[domIdx] = StateObserver("state_snapshots_" + std::to_string(domIdx) + ".bin", parser.stateSamplingFreq());
        obsVec[domIdx](::pressio::ode::StepCount(0), 0.0, *decomp.m_subdomainVec[domIdx]->getStateFull());
    }
    RuntimeObserver obs_time("runtime.bin");

    // solve
    const int numSteps     = parser.finalTime() / decomp.m_dtMax;
    const auto relTol      = parser.relTol();
    const auto absTol      = parser.absTol();
    const auto convStepMax = parser.convStepMax();

    int numSubiters;
    std::chrono::time_point<std::chrono::high_resolution_clock> runtimeStart;
    double secsElapsed;

#if defined SCHWARZ_ENABLE_OMP
#pragma omp parallel firstprivate(numSteps, relTol, absTol, convStepMax)
#endif
{

    double time = 0.0;
    for (int outerStep = 1; outerStep <= numSteps; ++outerStep)
    {

#if defined SCHWARZ_ENABLE_OMP
#pragma barrier
#pragma omp master
#endif
        {
            std::cout << "Step " << outerStep << std::endl;
            auto runtimeStart = std::chrono::high_resolution_clock::now();
        }

        // compute contoller step until convergence
        if (parser.schwarzMode() == pdas::SchwarzMode::Multiplicative) {
            numSubiters = decomp.calc_controller_step(
                parser.schwarzMode(),
                outerStep,
                time,
                relTol,
                absTol,
                convStepMax
            );
        }
        else {
            numSubiters = decomp.additive_step(
                outerStep,
                time,
                relTol,
                absTol,
                convStepMax
            );
        }
        time += decomp.m_dtMax;

#if defined SCHWARZ_ENABLE_OMP
#pragma omp barrier
#pragma omp master
#endif
        {
            // calculate step runtime, output
            const auto runtimeEnd = std::chrono::high_resolution_clock::now();
            const auto nsDuration = std::chrono::duration_cast<std::chrono::nanoseconds>(runtimeEnd - runtimeStart);
            secsElapsed = static_cast<double>(nsDuration.count());
            obs_time(secsElapsed, numSubiters);

            // output observer
            if ((outerStep % parser.stateSamplingFreq()) == 0) {
                const auto stepWrap = pode::StepCount(outerStep);
                for (int domIdx = 0; domIdx < (*decomp.m_tiling).count(); ++domIdx) {
                    obsVec[domIdx](stepWrap, time, *decomp.m_subdomainVec[domIdx]->getStateFull());
                }
            }
        }

    }
} // end parallel block
}

#endif
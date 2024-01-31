// This is largely copied from pressio-tutorials, with some simplifications, and additions for Schwarz domain decompositions

#ifndef PDAS_EXPERIMENTS_PARSER_HPP_
#define PDAS_EXPERIMENTS_PARSER_HPP_

#include <string>

#include "pressio/ode_steppers_implicit.hpp"

#include "yaml-cpp/parser.h"
#include "yaml-cpp/yaml.h"

// TODO: validation of inputs

// parsing time stepping scheme
pressio::ode::StepScheme string_to_ode_scheme(const std::string & strIn)
{
    namespace pode = pressio::ode;

    if      (strIn == "BDF1")          { return pode::StepScheme::BDF1; }
    else if (strIn == "CrankNicolson") { return pode::StepScheme::CrankNicolson; }
    else if (strIn == "BDF2")          { return pode::StepScheme::BDF2; }
    else{
        throw std::runtime_error("string_to_ode_scheme: Invalid odeScheme");
    }
}

pressiodemoapps::InviscidFluxReconstruction int_to_flux_order(const int intIn)
{
    if      (intIn == 1) { return pressiodemoapps::InviscidFluxReconstruction::FirstOrder; }
    else if (intIn == 3) { return pressiodemoapps::InviscidFluxReconstruction::Weno3; }
    else if (intIn == 5) { return pressiodemoapps::InviscidFluxReconstruction::Weno5; }
    else{
        throw std::runtime_error("int_to_order: Invalid order");
    }
}

// This covers parameters used by all simulation types
template <typename ScalarType>
class ParserCommon
{

protected:
    std::string meshDirPathFull_    = "";
    std::string meshDirPathHyper_   = "";
    int stateSamplingFreq_          = {};
    ScalarType finalTime_           = {};
    std::string problemName_        = "";
    int icFlag_                     = -1;
    std::unordered_map<std::string, ScalarType> userParams_ = {};

public:
    ParserCommon() = delete;
    ParserCommon(YAML::Node & node){
        this->parseImpl(node);
    }

    auto meshDirFull()          const { return meshDirPathFull_; }
    auto stateSamplingFreq()    const { return stateSamplingFreq_; }
    auto finalTime()            const { return finalTime_; }
    auto problemName()          const { return problemName_; }
    auto icFlag()               const { return icFlag_; }
    auto userParams()           const { return userParams_; }

private:
    void parseImpl(YAML::Node & node)
    {
        std::string entry = "meshDirFull";
        if (node[entry]) meshDirPathFull_ = node[entry].as<std::string>();
        else throw std::runtime_error("Input: missing " + entry);

        entry = "finalTime";
        if (node[entry]) finalTime_ = node[entry].as<ScalarType>();
        else throw std::runtime_error("Input: missing " + entry);

        entry = "stateSamplingFreq";
        if (node[entry]) stateSamplingFreq_ = node[entry].as<int>();
        else throw std::runtime_error("Input: missing " + entry);

        entry = "problemName";
        if (node[entry]) problemName_ = node[entry].as<std::string>();
        else throw std::runtime_error("Input: missing " + entry);

        // NOTE: making this required for now
        entry = "icFlag";
        if (node[entry]) icFlag_ = node[entry].as<int>();
        else throw std::runtime_error("Input: missing " + entry);

    }
};

// For monolithic simulations only
template <typename ScalarType>
class ParserMono
{

protected:

    ScalarType dt_ = -1.0;
    std::string odeSchemeString_ = "";
    pressio::ode::StepScheme odeScheme_;
    int numSteps_;
    pressiodemoapps::InviscidFluxReconstruction fluxOrder_ = {};

public:
    ParserMono() = delete;
    ParserMono(YAML::Node & node) {
        this->parseImpl(node);
    }

    auto timeStepSize() const { return dt_; }
    auto odeScheme()    const { return odeScheme_; }
    auto numSteps()     const { return numSteps_; }
    auto fluxOrder()    const { return fluxOrder_; }

private:
    void parseImpl(YAML::Node & node)
    {
        // Do not throw errors for absent parameters here
        // Will apply globally for decomposition if set here

        std::string entry = "timeStepSize";
        if (node[entry]) dt_ = node[entry].as<ScalarType>();

        // time stepping scheme
        entry = "odeScheme";
        if (node[entry]) {
            odeSchemeString_ = node[entry].as<std::string>();
            odeScheme_ = string_to_ode_scheme(odeSchemeString_);
        }

        entry = "fluxOrder";
        if (node[entry]) {
            int fluxOrderInt = node[entry].as<int>();
            fluxOrder_ = int_to_flux_order(fluxOrderInt);
        }
    }

};

// ONLY monolithic ROMs
template <typename ScalarType>
class ParserRom: public ParserMono<ScalarType>
{

protected:

    bool isRom_   = false;
    std::string romAlgoName_        = "";
    int romSize_                    = {};
    std::string romBasisFileName_   = "";
    std::string romTransFileName_   = "";

    bool isHyper_ = false;
    std::string meshDirPathHyper_     = "";
    std::string hyperStencilFileName_ = "";
    std::string hyperSampleFileName_  = "";

public:
    ParserRom() = delete;
    ParserRom(YAML::Node & node)
    : ParserMono<ScalarType>::ParserMono(node) {
        this->parseImpl(node);
    }

    auto isRom()        const { return isRom_; }
    auto romAlgorithm() const { return romAlgoName_; }
    auto romModeCount() const { return romSize_; }
    auto romBasisFile() const { return romBasisFileName_; }
    auto romTransFile() const { return romTransFileName_; }

    auto isHyper()          const { return isHyper_; }
    auto meshDirHyper()     const { return meshDirPathHyper_; }
    auto hyperSampleFile()  const { return hyperSampleFileName_; }
    auto hyperStencilFile() const { return hyperStencilFileName_; }

private:
    void parseImpl(YAML::Node & parentNode) {
        auto romNode = parentNode["rom"];
        if (romNode) {
            isRom_ = true;

            std::string entry = "algorithm";
            if (romNode[entry]) romAlgoName_ = romNode[entry].as<std::string>();
            else throw std::runtime_error("Input rom: missing " + entry);

            entry = "numModes";
            if (romNode[entry]) romSize_ = romNode[entry].as<int>();
            else throw std::runtime_error("Input rom: missing " + entry);

            entry = "basisFile";
            if (romNode[entry]) romBasisFileName_ = romNode[entry].as<std::string>();
            else throw std::runtime_error("Input rom: missing " + entry);

            entry = "transFile";
            if (romNode[entry]) romTransFileName_ = romNode[entry].as<std::string>();
            else throw std::runtime_error("Input rom: missing " + entry);

        }

        auto hyperNode = parentNode["hyper"];
        if (hyperNode) {
            isHyper_ = true;

            std::string entry = "meshDirHyper";
            if (hyperNode[entry]) meshDirPathHyper_ = hyperNode[entry].as<std::string>();
            else throw std::runtime_error("Input hyper: missing " + entry);

            entry = "sampleFile";
            if (hyperNode[entry]) hyperSampleFileName_ = hyperNode[entry].as<std::string>();
            else throw std::runtime_error("Input hyper: missing " + entry);

            entry = "stencilFile";
            if (hyperNode[entry]) hyperStencilFileName_ = hyperNode[entry].as<std::string>();
            else throw std::runtime_error("Input hyper: missing " + entry);

        }
    }

};

// decomposed solution
// mostly just generalization of above to vector inputs
template <typename ScalarType>
class ParserDecomp
{

protected:

    bool isDecomp_ = false;

    std::vector<std::string> domTypeVec_;
    int ndomains_;
    std::vector<ScalarType> dtVec_;

    std::vector<std::string> odeSchemeStringVec_;
    std::vector<pressio::ode::StepScheme> odeSchemeVec_;

    bool hasRom_ = false;
    std::vector<int> romSizeVec_;
    std::string romBasisRoot_ = "";
    std::string romTransRoot_ = "";

    bool hasHyper_ = false;
    std::vector<std::string> hyperSampleFiles_;

    pdaschwarz::SchwarzMode schwarzMode_ = pdaschwarz::SchwarzMode::Multiplicative;
    ScalarType relTol_ = 1e-11;
    ScalarType absTol_ = 1e-11;
    int convStepMax_ = 10;

public:
    ParserDecomp() = delete;
    ParserDecomp(YAML::Node & node) {
        this->parseImpl(node);
    }

    auto isDecomp()         const { return isDecomp_; }
    auto domTypeVec()       const { return domTypeVec_; }
    auto dtVec()            const { return dtVec_; }

    auto romModeCountVec()  const { return romSizeVec_; }
    auto romBasisRoot()     const { return romBasisRoot_; }
    auto romTransRoot()     const { return romTransRoot_; }

    auto hyperSampleFiles()     const { return hyperSampleFiles_; }

    auto schwarzMode()      const { return schwarzMode_; }
    auto relTol()           const { return relTol_; }
    auto absTol()           const { return absTol_; }
    auto convStepMax()      const { return convStepMax_; }

private:
    void parseImpl(YAML::Node & parentNode) {
        auto decompNode = parentNode["decomp"];
        if (decompNode) {
            isDecomp_ = true;

            std::string entry = "domainTypes";
            if (decompNode[entry]) domTypeVec_ = decompNode[entry].as<std::vector<std::string>>();
            else throw std::runtime_error("Input decomp: missing " + entry);

            ndomains_ = domTypeVec_.size();
            if (ndomains_ < 2) throw std::runtime_error("Input decomp: fewer than 2 subdomains");

            entry = "timeStepSize";
            if (decompNode[entry]) {
                dtVec_ = decompNode[entry].as<std::vector<ScalarType>>();
            }
            else if (parentNode[entry]) {
                ScalarType dt = parentNode[entry].as<ScalarType>();
                dtVec_ = std::vector<ScalarType>(ndomains_, dt);
            }
            else {
                throw std::runtime_error("Input: missing " + entry);
            }

            // Multiplicative Schwarz by default
            entry = "additive";
            if (decompNode[entry]) {
                bool additive = decompNode[entry].as<bool>();
                if (additive) schwarzMode_ = pdaschwarz::SchwarzMode::Additive;
            }

            // Tolerances have default values
            entry = "relTol";
            if (decompNode[entry]) relTol_ = decompNode[entry].as<ScalarType>();
            entry = "absTol";
            if (decompNode[entry]) absTol_ = decompNode[entry].as<ScalarType>();
            entry = "convStepMax";
            if (decompNode[entry]) convStepMax_ = decompNode[entry].as<ScalarType>();

            entry = "odeScheme";
            if (decompNode[entry]) {
                odeSchemeStringVec_ = decompNode[entry].as<std::vector<std::string>>();
                odeSchemeVec_.resize(ndomains_);
                for (int domIdx = 0; domIdx < ndomains_; ++domIdx) {
                    odeSchemeVec_[domIdx] = string_to_ode_scheme(odeSchemeStringVec_[domIdx]);
                }
            }
            else if (parentNode[entry]) {
                std::string odeString = parentNode[entry].as<std::string>();
                odeSchemeStringVec_ = std::vector<std::string>(ndomains_, odeString);
                auto odeScheme = string_to_ode_scheme(odeString);
                odeSchemeVec_ = std::vector<pressio::ode::StepScheme>(ndomains_, odeScheme);
            }
            else {
                throw std::runtime_error("Input: missing " + entry);
            }

            // check if there are any ROM subdomains
            for (int domIdx = 0; domIdx < ndomains_; ++domIdx) {
                if ((domTypeVec_[domIdx] == "Galerkin") ||
                    (domTypeVec_[domIdx] == "LSPG") ||
                    (domTypeVec_[domIdx] == "LSPGHyper"))
                {
                    hasRom_ = true;
                    break;
                }
            }
            
            // check for hyper-reduction
            if ((decompNode[""]))

            if (hasRom_) {
                entry = "numModes";
                if (decompNode[entry]) romSizeVec_ = decompNode[entry].as<std::vector<int>>();
                else throw std::runtime_error("Input decomp: missing " + entry);

                entry = "basisFileRoot";
                if (decompNode[entry]) romBasisRoot_ = decompNode[entry].as<std::string>();
                else throw std::runtime_error("Input decomp: missing " + entry);

                entry = "transFileRoot";
                if (decompNode[entry]) romTransRoot_ = decompNode[entry].as<std::string>();
                else throw std::runtime_error("Input decomp: missing " + entry);

            }
            else {
                romSizeVec_.resize(ndomains_, -1);
            }
        }
        else {
            // check variables that MUST be set for monolithic
            std::string entry = "odeScheme";
            if (!parentNode[entry]) {
                throw std::runtime_error("Input: missing " + entry);
            }
            
            entry = "fluxOrder";
            if (!parentNode[entry]) {
                throw std::runtime_error("Input: missing " + entry);
            }
            
        }
    }

};


/*
    Generic problem interface
*/

template <typename ScalarType>
class ParserProblem
    : public ParserCommon<ScalarType>
    , public ParserRom<ScalarType>
    , public ParserDecomp<ScalarType>
{

public:
    ParserProblem() = delete;
    ParserProblem(YAML::Node & node)
    : ParserCommon<ScalarType>::ParserCommon(node)
    , ParserRom<ScalarType>::ParserRom(node)
    , ParserDecomp<ScalarType>::ParserDecomp(node)
    {
        this->parseImpl();
    }

private:
    void parseImpl() {

        // can't be monolithic ROM and decomposition
        if ((this->isRom_) && (this->isDecomp_)) {
            throw std::runtime_error("Input: cannot set rom and decomp fields in same input file");
        }

        // make sure time step and scheme were set for monolithic simulation
        // doesn't throw error in class construction b/c not needed for decomposed solution
        if (!this->isDecomp_) {

            // catch time step
            if (this->dt_ == -1.0) throw std::runtime_error("Input: missing timeStepSize");
            this->numSteps_ = static_cast<int>(this->finalTime_/this->dt_);

            // catch ODE scheme
            if (this->odeSchemeString_ == "") throw std::runtime_error("Input: missing odeScheme");
        }

    }


};

/*
    Problem-specific parser implementations
    Use this to specify parameters
*/

template <typename ScalarType>
class Parser2DSwe: public ParserProblem<ScalarType>
{

    pressiodemoapps::Swe2d probId_ = {};

public:
    Parser2DSwe() = delete;
    Parser2DSwe(YAML::Node & node)
    : ParserProblem<ScalarType>::ParserProblem(node)
    {
        this->parseImpl(node);
    }

    auto probId()           const{ return probId_; }

private:
    void parseImpl(YAML::Node & node) {

        // SWE only admits two problem IDs, but really should only be one
        if (this->problemName_ != "SlipWall") {
            throw std::runtime_error("Invalid problemName: " + this->problemName_);
        }
        if (this->isDecomp_) {
            probId_ = pressiodemoapps::Swe2d::CustomBCs;
        }
        else {
            probId_ = pressiodemoapps::Swe2d::SlipWall;
        }

        // physical parameters
        const std::array<std::string, 2> names = {"gravity", "coriolis"};
        for (int i = 0; i < 2; ++i) {
            if (node[names[i]]) {
                this->userParams_[names[i]] = node[names[i]].as<ScalarType>();
            }
            else {
                throw std::runtime_error("Input: missing " + names[i]);
            }
        }

        // initial condition parameters
        if (this->icFlag_ == 1) {
            const std::array<std::string, 3> names = {"pulseMagnitude", "pulseX", "pulseY"};
            for (int i = 0; i < 3; ++i) {
                if (node[names[i]]) {
                    this->userParams_[names[i]] = node[names[i]].as<ScalarType>();
                }
                else {
                    throw std::runtime_error("Input: missing " + names[i]);
                }
            }
        }
        else if (this->icFlag_ == 2) {
            const std::array<std::string, 6> names = {
                "pulseMagnitude1", "pulseX1", "pulseY1",
                "pulseMagnitude2", "pulseX2", "pulseY2"};
            for (int i = 0; i < 6; ++i) {
                if (node[names[i]]) {
                    this->userParams_[names[i]] = node[names[i]].as<ScalarType>();
                }
                else {
                    throw std::runtime_error("Input: missing " + names[i]);
                }
            }
        }
        else {
            throw std::runtime_error("Invalid SWE icFlag: " + std::to_string(this->icFlag_));
        }
    }
};


template <typename ScalarType>
class Parser2DEuler: public ParserProblem<ScalarType>
{
    // TODO: make sure icFlag_ is set correctly
    pressiodemoapps::Euler2d probId_ = {};

public:
    Parser2DEuler() = delete;
    Parser2DEuler(YAML::Node & node)
    : ParserProblem<ScalarType>::ParserProblem(node)
    {
        this->parseImpl(node);
    }

    auto probId() const{ return probId_; }

private:
    void parseImpl(YAML::Node & node) {

        std::string entry;

        if (this->problemName_ == "Riemann") {
            probId_ = pressiodemoapps::Euler2d::Riemann;
            if ((this->icFlag_ < 1) || (this->icFlag_ > 2)) {
                throw std::runtime_error("Invalid icFlag for Riemann: " + std::to_string(this->icFlag_));
            }

            // same name regardless of icFlag
            entry = "riemannTopRightPressure";
            if (node[entry]) this->userParams_[entry] = node[entry].as<ScalarType>();
            else throw std::runtime_error("Input: missing " + entry);

            if (this->icFlag_ == 2) {
                const std::array<std::string, 4> names = {
                    "riemannTopRightXVel", "riemannTopRightYVel",
                    "riemannTopRightDensity", "riemannBotLeftPressure"};
                for (int i = 0; i < 4; ++i) {
                    if (node[names[i]]) {
                        this->userParams_[names[i]] = node[names[i]].as<ScalarType>();
                    }
                    else {
                        throw std::runtime_error("Input: missing " + names[i]);
                    }
                }
            }

        }
        else if (this->problemName_ == "NormalShock") {
            probId_ = pressiodemoapps::Euler2d::NormalShock;
            // no icFlag for this case

            entry = "normalShockMach";
            if (node[entry]) this->userParams_[entry] = node[entry].as<ScalarType>();
            else throw std::runtime_error("Input: missing " + entry);

        }
        // TODO: expand
        else {
            throw std::runtime_error("Invalid problemName: " + this->problemName_);
        }

        // only one physical parameter, gamma
        entry = "gamma";
        if (node[entry]) this->userParams_[entry] = node[entry].as<ScalarType>();
        else throw std::runtime_error("Input: missing " + entry);

    }
};

#endif
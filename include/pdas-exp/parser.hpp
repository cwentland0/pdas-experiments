// This is largely copied from pressio-tutorials, with some simplifications, and additions for Schwarz domain decompositions

#ifndef PDAS_EXPERIMENTS_PARSER_HPP_
#define PDAS_EXPERIMENTS_PARSER_HPP_

#include <string>

#include "pressio/ode_steppers_implicit.hpp"

#include "yaml-cpp/parser.h"
#include "yaml-cpp/yaml.h"

// parsing time stepping scheme
pressio::ode::StepScheme string_to_ode_scheme(const std::string & strIn)
{
    namespace pode = pressio::ode;

    if (strIn == "BDF1") { return pode::StepScheme::BDF1; }
    else if (strIn == "CrankNicolson") { return pode::StepScheme::CrankNicolson; }
    else if (strIn == "BDF2") { return pode::StepScheme::BDF2; }
    else{
        throw std::runtime_error("string_to_ode_scheme: Invalid odeScheme");
    }
}

// This covers parameters used by all simulation types
template <typename ScalarType>
class ParserCommon
{

protected:
    std::string meshDirPath_    = "empty";
    int stateSamplingFreq_      = -1;
    ScalarType finalTime_       = {};

public:
    ParserCommon() = delete;
    ParserCommon(YAML::Node & node){
        this->parseImpl(node);
    }

    auto meshDir()              const { return meshDirPath_; }
    auto stateSamplingFreq()    const { return stateSamplingFreq_; }
    auto finalTime()            const { return finalTime_; }

private:
    void parseImpl(YAML::Node & node)
    {
        std::string entry = "meshDir";
        if (node[entry]) meshDirPath_ = node[entry].as<std::string>();
        else throw std::runtime_error("Input: missing " + entry);

        entry = "finalTime";
        if (node[entry]) finalTime_ = node[entry].as<ScalarType>();
        else throw std::runtime_error("Input: missing " + entry);

        entry = "stateSamplingFreq";
        if (node[entry]) stateSamplingFreq_ = node[entry].as<int>();
        else throw std::runtime_error("Input: missing " + entry);

    }
};

// For monolithic simulations only
template <typename ScalarType>
class ParserMono
{

protected:

    ScalarType dt_ = -1.0;
    std::string odeSchemeString_ = "empty";
    pressio::ode::StepScheme odeScheme_;
    int numSteps_;

public:
    ParserMono() = delete;
    ParserMono(YAML::Node & node) {
        this->parseImpl(node);
    }

    auto timeStepSize() const { return dt_; }
    auto odeScheme()    const { return odeScheme_; }
    auto numSteps()     const { return numSteps_; }

private:
    void parseImpl(YAML::Node & node)
    {
        // Do not throw errors for absent parameters here
        // Will apply globally for decomposition if set here

        auto entry = "timeStepSize";
        if (node[entry]) dt_ = node[entry].as<ScalarType>();

        entry = "odeScheme";
        if (node[entry]) {
            odeSchemeString_ = node[entry].as<std::string>();
            odeScheme_ = string_to_ode_scheme(odeSchemeString_);
        }
    }

};

// ONLY monolithic ROMs
template <typename ScalarType>
class ParserRom: public ParserMono<ScalarType>
{

protected:

    bool isRom_ = false;

    std::string romAlgoName_ = "empty";
    int romSize_= {};
    std::string romFullMeshPodBasisFileName_ = "empty";
    std::string romAffineShiftFileName_ = "empty";
    bool isAffine_ = false;

public:
    ParserRom() = delete;
    ParserRom(YAML::Node & node)
    : ParserMono<ScalarType>::ParserMono(node) {
        this->parseImpl(node);
    }

    auto isRom()                    const { return isRom_; }
    auto romAlgorithm()             const { return romAlgoName_; }
    auto romModeCount()             const{ return romSize_; }
    auto romFullMeshPodBasisFile()  const{ return romFullMeshPodBasisFileName_; }
    auto romAffineShiftFile()	    const{ return romAffineShiftFileName_; }
    auto romIsAffine()              const{ return isAffine_; }

private:
    void parseImpl(YAML::Node & parentNode) {
        auto romNode = parentNode["rom"];
        if (romNode) {
            isRom_ = true;

            std::string entry = "algorithm";
            if (romNode[entry]) {
                romAlgoName_ = romNode[entry].as<std::string>();
                std::transform(romAlgoName_.begin(), romAlgoName_.end(), romAlgoName_.begin(),
                    [](int c){ return std::tolower(c); });
            }
            else throw std::runtime_error("Input: rom: missing " + entry);

            entry = "numModes";
            if (romNode[entry]) romSize_ = romNode[entry].as<int>();
            else throw std::runtime_error("Input: rom: missing " + entry);

            entry = "fullMeshPodFile";
            if (romNode[entry]) romFullMeshPodBasisFileName_ = romNode[entry].as<std::string>();
            else throw std::runtime_error("Input: rom: missing " + entry);

            // TODO: for now, strictly required to supply translation vector
            // can turn it off with isAffine, but fails if invalid file passed
            entry = "   ";
            if (romNode[entry]) romAffineShiftFileName_ = romNode[entry].as<std::string>();
            else throw std::runtime_error("Input: rom: missing " + entry);

            // TODO: for now, strictly required to set affine flag
            // opaque to user if shift file is set, no obvious default
            entry = "isAffine";
            if (romNode[entry]) isAffine_ = romNode[entry].as<bool>();
            else throw std::runtime_error("Input: rom: missing " + entry);
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
    std::string romPodBasisRoot_ = "empty";
    std::string romAffineShiftRoot_ = "empty";

public:
    ParserDecomp() = delete;
    ParserDecomp(YAML::Node & node) {
        this->parseImpl(node);
    }

    bool isDecomp() const { return isDecomp_; }

private:
    void parseImpl(YAML::Node & parentNode) {
        auto decompNode = parentNode["decomp"];
        if (decompNode) {
            isDecomp_ = true;

            std::string entry = "domainTypes";
            if (decompNode[entry]) domTypeVec_ = decompNode[entry].as<std::vector<std::string>>();
            else throw std::runtime_error("Input: decomp: missing " + entry);

            ndomains_ = domTypeVec_.size();
            if (ndomains_ < 2) throw runtime_error("Input: decomp has fewer than 2 subdomains");

            // set to lowercase
            for (int domIdx = 0; domIdx < ndomains_; ++domIdx) {
                std::transform(
                    domTypeVec_[domIdx].begin(),
                    domTypeVec_[domIdx].end(),
                    domTypeVec_[domIdx].begin(),
                    [](int c){ return std::tolower(c); });
            }

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
                if ((domTypeVec_[domIdx] == "Galerkin") || (domTypeVec_[domIdx] == "LSPG")) {
                    hasRom_ = true;
                    break;
                }
            }

            if (hasRom_) {
                entry = "numModes";
                if (decompNode[entry]) romSizeVec_ = decompNode[entry].as<std::vector<int>>();
                else throw std::runtime_error("Input: decomp: missing " + entry);
                
                entry = "podFileRoot";
                if (decompNode[entry]) romPodBasisRoot_ = decompNode[entry].as<std::string>();
                else throw std::runtime_error("Input: decomp: missing " + entry);
                
                entry = "affineShiftFileRoot";
                if (decompNode[entry]) romAffineShiftRoot_ = decompNode[entry].as<std::string>();
                else throw std::runtime_error("Input: decomp: missing " + entry);
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
        this->validate();
    }

private:
    void validate() {

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
            if (this->odeSchemeString_ == "empty") throw std::runtime_error("Input: missing odeScheme");
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

    // gravity, coriolis, pulseMagnitude
    std::array<ScalarType, 3> params_ = {9.8, -3.0, 0.125};

public:
    Parser2DSwe() = delete;
    Parser2DSwe(YAML::Node & node)
    : ParserProblem<ScalarType>::ParserProblem(node)
    {
        this->parseImpl(node);
    }
    
    auto gravity()  const{ return params_[0]; }
    auto coriolis() const{ return params_[1]; }
    auto pulseMagnitude() const{ return params_[2]; }

private:
    void parseImpl(YAML::Node & node) {
        const std::array<std::string, 3> names = {"gravity", "coriolis", "pulsemag"};
        for (int i = 0; i < 3; ++i) {
            if (node[names[i]]) {
                params_[i] = node[names[i]].as<ScalarType>();
            }
            else{
                throw std::runtime_error("Cannot find " + names[i] + " in input file");
            }
        }
    }
};

#endif
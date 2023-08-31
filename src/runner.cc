// Largely copied from pressio-tutorials

#include <cassert>

#include "pda-schwarz/schwarz.hpp"
#include "pdas-exp/parser.hpp"
#include "pdas-exp/mono_fom.hpp"
#include "pdas-exp/mono_lspg.hpp"
#include "pdas-exp/decomp.hpp"

template<class AppType, class ParserType>
void dispatch_mono(AppType fomSystem, ParserType & parser)
{
    if (!parser.isRom()) {
        // monolithic FOM
        run_mono_fom(fomSystem, parser);
    }
    else {
        // monolithic ROM
        if (parser.romAlgorithm() == "Galerkin") {
            throw std::runtime_error("Monolithic Galerkin not implemented yet");
        }
        else {
            run_mono_lspg(fomSystem, parser);
        }
    }
}

template<class AppType, class ParserType>
void dispatch_decomp(ParserType & parser)
{
    run_decomp<AppType>(parser);
}

bool file_exists(const std::string & fileIn){
    std::ifstream infile(fileIn);
    return (infile.good() != 0);
}

std::string check_and_get_inputfile(int argc, char *argv[])
{
    if (argc != 2){
        throw std::runtime_error("Call as: ./exe <path-to-inputfile>");
    }
    const std::string inputFile = argv[1];
    std::cout << "Input file: " << inputFile << "\n";
    assert( file_exists(inputFile) );
    return inputFile;
}

int main(int argc, char *argv[])
{

    namespace pda  = pressiodemoapps;
    namespace pdas = pdaschwarz;
    namespace pode = pressio::ode;
    using scalar_t = double;

    const auto inputFile = check_and_get_inputfile(argc, argv);
    auto node = YAML::LoadFile(inputFile);

    // "system" is strictly required
    const auto eqsNode = node["equations"];
    if (!eqsNode){ throw std::runtime_error("Missing 'equations' in yaml input!"); }
    const auto eqsName = eqsNode.as<std::string>();

    // TODO: generalize
    const auto order = pda::InviscidFluxReconstruction::FirstOrder;

    // TODO: need to incorporate physical parameter settings
    if (eqsName == "2d_swe") {
        Parser2DSwe<scalar_t> parser(node);

        if (parser.isDecomp()) {
            using app_t = pdas::swe2d_app_type;
            dispatch_decomp<app_t>(parser);
        }
        else {
            const auto meshObj = pda::load_cellcentered_uniform_mesh_eigen<scalar_t>(parser.meshDir());
            const auto gravity  = parser.gravity();
            const auto coriolis = parser.coriolis();
            const auto pulseMag = parser.pulseMagnitude();
            // TODO: If SWE expanded, stop using this shortcut method
            auto fomSystem = pda::create_slip_wall_swe_2d_problem_eigen(meshObj, order, gravity, coriolis, pulseMag);
            dispatch_mono(fomSystem, parser);
        }

    }
    else if (eqsName == "2d_euler") {
        Parser2DEuler<scalar_t> parser(node);

        if (parser.isDecomp()) {
            using app_t = pdas::euler2d_app_type;
            dispatch_decomp<app_t>(parser);
        }
        else {
            const auto meshObj = pda::load_cellcentered_uniform_mesh_eigen<scalar_t>(parser.meshDir());
            auto fomSystem = pda::create_problem_eigen(meshObj, parser.probId(), order, parser.icFlag());
            dispatch_mono(fomSystem, parser);
        }

    }
    else {
        throw std::runtime_error("Invalid 'equations': " + eqsName);
    }

    return 0;
}
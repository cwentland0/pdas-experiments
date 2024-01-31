// Largely copied from pressio-tutorials

#include <cassert>

#include "pda-schwarz/schwarz.hpp"
#include "pdas-exp/parser.hpp"
#include "pdas-exp/mono_fom.hpp"
#include "pdas-exp/mono_lspg.hpp"
// #include "pdas-exp/decomp.hpp"

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
        else if (parser.romAlgorithm() == "LSPG") {
            run_mono_lspg(fomSystem, parser);
        }
        else {
            throw std::runtime_error("Invalid ROM algorithm");
        }
    }
}

// template<class AppType, class ParserType>
// void dispatch_decomp(ParserType & parser)
// {
//     run_decomp<AppType>(parser);
// }

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

    // "equations" is strictly required
    const auto eqsNode = node["equations"];
    if (!eqsNode){ throw std::runtime_error("Missing 'equations' in yaml input!"); }
    const auto eqsName = eqsNode.as<std::string>();

    // TODO: need to incorporate physical parameter settings
    if (eqsName == "2d_swe") {
        Parser2DSwe<scalar_t> parser(node);

        if (parser.isDecomp()) {
            // using app_t = pdas::swe2d_app_type;
            // dispatch_decomp<app_t>(parser);
        }
        else {
            const auto meshObj = pda::load_cellcentered_uniform_mesh_eigen<scalar_t>(parser.meshDirFull());
            auto fomSystem = pda::create_problem_eigen(
                meshObj, parser.probId(), parser.fluxOrder(),
                parser.icFlag(), parser.userParams()
            );

            dispatch_mono(fomSystem, parser);
        }

    }
    else if (eqsName == "2d_euler") {
        Parser2DEuler<scalar_t> parser(node);

        if (parser.isDecomp()) {
        //     using app_t = pdas::euler2d_app_type;
        //     dispatch_decomp<app_t>(parser);
        }
        else {
            const auto meshObj = pda::load_cellcentered_uniform_mesh_eigen<scalar_t>(parser.meshDirFull());

            // NOTE: this hack is required because there is no generic overload
            //  to supply user parameters, since not all problems accept params like SWE.
            //  Have to force it to use the CustomBC overload, which catches invalid problems.
            // using mesh_t = decltype(meshObj);
            // auto bcFunctor = pdas::BCFunctor<mesh_t>(pdas::BCType::HomogNeumann);
            // auto fomSystem = pda::create_problem_eigen(meshObj, parser.probId(), order,
            //     bcFunctor, bcFunctor, bcFunctor, bcFunctor, parser.icFlag(), parser.userParams());
            auto fomSystem = pda::create_problem_eigen(
                meshObj, parser.probId(), parser.fluxOrder(),
                parser.icFlag(), parser.userParams()
            );

            dispatch_mono(fomSystem, parser);
        }

    }
    else {
        throw std::runtime_error("Invalid 'equations': " + eqsName);
    }

    return 0;
}
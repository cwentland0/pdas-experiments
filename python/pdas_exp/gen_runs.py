
import os
import subprocess
import argparse

import yaml

from defaults import check_params, get_params_combo
from utils import check_meshdir, mkdir, catchlist

ALGOS = ["FOM", "LSPG", "LSPGHyper"]

def gen_runs(
    equations,
    order,
    problem,
    scheme,
    tf,
    dt,
    sampfreq,
    meshroot,
    nx,
    ny,
    runtype,
    icFlag,
    runner,
    loglevel,
    outdir_base,
    phys_params_user=None,
    ic_params_user=None,
    rom_algo=None,
    nmodes=None,
    basis_dir=None,
    basis_file=None,
    shift_file=None,
    ndomX=None,
    ndomY=None,
    overlap=None,
    isadditive=False,
    sampalgo=None,
    sampperc=0.0,
    run=False,
):

    # ----- START CHECKS -----

    assert loglevel in ["debug", "info", "off"]
    assert scheme in ["BDF1", "BDF2"]
    assert tf > 0.0
    assert sampfreq >= 1
    assert runtype in [
        "fom", "rom", "hyper",
        "fom_decomp", "rom_decomp", "hyper_decomp",
    ]
    assert os.path.isdir(outdir_base)
    assert os.path.isdir(meshroot)

    if phys_params_user is None:
        phys_params_user = {}
    if ic_params_user is None:
        ic_params_user = {}
    check_params(phys_params_user, ic_params_user, equations, order, problem, icFlag)

    if "hyper" in runtype:
        assert sampalgo in ["random", "eigenvec"]
        assert sampperc > 0.0

    if order == 1:
        order_dir = "firstorder"
    elif order == 3:
        order_dir = "weno3"
    elif order == 5:
        order_dir = "weno5"
    else:
        raise ValueError(f"Invalid order: {order}")

    if "decomp" not in runtype:
        ndomX = 1
        ndomY = 1
        ndomains = 1
        assert dt > 0.0
    else:
        assert ndomX >= 1
        assert ndomY >= 1
        ndomains = ndomX * ndomY
        assert ndomains > 1, "No point to decomp if 1x1"
        assert overlap is not None
        assert overlap >= 0
        dt = catchlist(dt, float, ndomains)

    # handle nmodes, algorithm
    if runtype != "fom":
        assert nmodes is not None
        assert rom_algo is not None
        assert basis_dir is not None
        assert basis_file is not None
        assert shift_file is not None

        basis_dir = os.path.join(basis_dir, f"{nx}x{ny}", f"{ndomX}x{ndomY}")
        if "decomp" in runtype:
            nmodes = catchlist(nmodes, int, ndomains)
            rom_algo = catchlist(rom_algo, str, ndomains)
            assert all([algo in ALGOS for algo in rom_algo])
            basis_dir = os.path.join(basis_dir, f"overlap{overlap}", order_dir)
            basis_root = os.path.join(basis_dir, basis_file)
            shift_root = os.path.join(basis_dir, shift_file)
            assert all([
                os.path.isfile(f"{basis_root}_{dom_idx}.bin") \
                for dom_idx in range(ndomains)
            ]), f"Basis file not found at {basis_dir}"
            assert all([
                os.path.isfile(f"{shift_root}_{dom_idx}.bin") \
                for dom_idx in range(ndomains)
            ]), f"Affine shift file not found at {basis_dir}"
        else:
            assert isinstance(nmodes, int)
            assert rom_algo in ALGOS
            basis_dir = os.path.join(basis_dir, order_dir)
            basis_root = os.path.join(basis_dir, basis_file)
            shift_root = os.path.join(basis_dir, shift_file)
            assert os.path.isfile(basis_root + ".bin")
            assert os.path.isfile(shift_root + ".bin")

    # check that mesh exists
    meshdir = os.path.join(meshroot, f"{nx}x{ny}", f"{ndomX}x{ndomY}")
    assert os.path.isdir(meshdir)

    if "decomp" not in runtype:
        meshdir_full = os.path.join(meshdir, order_dir, "full")
        check_meshdir(meshdir_full)
    else:
        meshdir_full = os.path.join(meshdir, f"overlap{overlap}", order_dir, "full")
        assert os.path.isfile(os.path.join(meshdir_full, "info_domain.dat"))
        for dom_idx in range(ndomains):
            check_meshdir(os.path.join(meshdir_full, f"domain_{dom_idx}"))

    if "hyper" in runtype:
        if runtype == "hyper":
            finaldir = f"modes_{nmodes}_samp_{sampperc}"
            meshdir_hyper = os.path.join(meshdir, order_dir, sampalgo, finaldir)
        elif runtype == "hyper_decomp":
            raise ValueError("FIX FINALDIR")
            meshdir_hyper = os.path.join(meshdir, f"overlap{overlap}", order_dir, sampalgo, f"samp_{sampperc}")
        assert os.path.isdir(meshdir_hyper), meshdir_hyper

        if runtype == "hyper_decomp":
            sampfiles = [os.path.join(meshdir_hyper, f"domain_{dom_idx}", "sample_mesh_gids.dat") for dom_idx in range(ndomains)]

    # ----- END CHECKS -----

    # ----- START RUN GENERATION -----

    # generate permutations of parameter lists
    params_names_list, params_combo = get_params_combo(phys_params_user, ic_params_user, equations, problem, icFlag)

    for run_idx, run_list in enumerate(params_combo):

        rundir = os.path.join(outdir_base, runtype)
        mkdir(rundir)

        # mesh directory
        rundir = os.path.join(rundir, f"{nx}x{ny}")
        mkdir(rundir)

        # decomp directory
        if "decomp" in runtype:
            rundir = os.path.join(rundir, f"{ndomX}x{ndomY}")
            mkdir(rundir)
            rundir = os.path.join(rundir, f"overlap{overlap}")
            mkdir(rundir)

        rundir = os.path.join(rundir, order_dir)
        mkdir(rundir)

        # parameter directory
        dirname = ""
        for param_idx, param in enumerate(params_names_list):
            dirname += param + str(run_list[param_idx]) + "_"
        dirname = dirname[:-1]
        rundir = os.path.join(rundir, dirname)
        mkdir(rundir)

        # ROM algo and mode count directory
        if runtype in ["rom", "hyper"]:
            rundir = os.path.join(rundir, f"{rom_algo}{nmodes}")
            mkdir(rundir)
            if runtype == "hyper":
                rundir = os.path.join(rundir, sampalgo)
                mkdir(rundir)
                finaldir = f"modes_{nmodes}_samp_{sampperc}"
                rundir = os.path.join(rundir, finaldir)
                mkdir(rundir)
        elif "decomp" in runtype:
            # additive/multiplicative directory
            if isadditive:
                rundir = os.path.join(rundir, "additive")
            else:
                rundir = os.path.join(rundir, "multiplicative")
            mkdir(rundir)

            dirname = ""
            for dom_idx in range(ndomains):
                if rom_algo[dom_idx] == "FOM":
                    dirname += "FOM_"
                else:
                    dirname += f"{rom_algo[dom_idx]}{nmodes[dom_idx]}_"
            dirname = dirname[:-1]
            rundir = os.path.join(rundir, dirname)
            mkdir(rundir)
            if runtype == "hyper_decomp":
                rundir = os.path.join(rundir, sampalgo)
                mkdir(rundir)
                rundir = os.path.join(rundir, f"samp_{sampperc}")
                mkdir(rundir)

        runfile = os.path.join(rundir, "input.yaml")
        with open(runfile, "w") as f:

            f.write(f"loglevel: \"{loglevel}\"\n")
            f.write(f"equations: \"{equations}\"\n")
            f.write(f"fluxOrder: {order}\n")
            f.write(f"problemName: \"{problem}\"\n")
            f.write(f"icFlag: {icFlag}\n")
            for param_idx, param in enumerate(params_names_list):
                f.write(f"{param}: {run_list[param_idx]}\n")

            # these may need to be modified?
            f.write(f"meshDirFull: \"{meshdir_full}\"\n")
            f.write(f"odeScheme: \"{scheme}\"\n")
            f.write(f"finalTime: {tf}\n")
            f.write(f"stateSamplingFreq: {sampfreq}\n")
            if "decomp" not in runtype:
                f.write(f"timeStepSize: {dt}\n")

            if runtype in ["rom", "hyper"]:
                f.write("rom:\n")
                f.write(f"  algorithm: \"{rom_algo}\"\n")
                f.write(f"  numModes: {nmodes}\n")
                f.write(f"  basisFile: \"{basis_root}.bin\"\n")
                f.write(f"  transFile: \"{shift_root}.bin\"\n")

                if runtype == "hyper":
                    f.write("hyper:\n")
                    f.write(f"  meshDirHyper: \"{meshdir_hyper}\"\n")
                    f.write(f"  sampleFile: \"{meshdir_hyper}/sample_mesh_gids.dat\"\n")
                    f.write(f"  stencilFile: \"{meshdir_hyper}/stencil_mesh_gids.dat\"\n")


            if "decomp" in runtype:
                f.write("decomp:\n")
                f.write(f"  domainTypes: {rom_algo}\n")
                f.write(f"  timeStepSize: {dt}\n")
                f.write(f"  additive: {isadditive}\n")

                if runtype in ["rom_decomp", "hyper_decomp"]:
                    f.write(f"  numModes: {nmodes}\n")
                    f.write(f"  basisFileRoot: \"{basis_root}\"\n")
                    f.write(f"  transFileRoot: \"{shift_root}\"\n")
                    if runtype == "hyper_decomp":
                        f.write(f"  sampleFiles: {sampfiles}\n")

        if run:
            print(f"Executing run at {rundir}")
            os.chdir(rundir)
            subprocess.call([runner, runfile])
        else:
            print(f"Input file written to {runfile}")

    if not run:
        print("Pass run=True to execute next time")

    # ----- END RUN GENERATION -----

    print("Finished")

if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("settings_file")
    args = parser.parse_args()
    f = open(args.settings_file, "r")
    inputs = yaml.safe_load(f)

    # handle parameters
    phys_params_user = inputs["phys_params_user"]
    if phys_params_user is None:
        phys_params_user = {}
    elif isinstance(phys_params_user, dict):
        for key, value in phys_params_user.items():
            if not isinstance(value, list):
                phys_params_user[key] = [value]
    else:
        raise ValueError("phys_params_user must be a nested input")

    ic_params_user = inputs["ic_params_user"]
    if ic_params_user is None:
        ic_params_user = {}
    elif isinstance(ic_params_user, dict):
        for key, value in ic_params_user.items():
            if not isinstance(value, list):
                ic_params_user[key] = [value]
    else:
        raise ValueError("ic_params_user must be a nested input")

    # handle ROM inputs
    if inputs["runtype"] != "fom":
        rom_algo = inputs["rom_algo"]
        nmodes = inputs["nmodes"]
        basis_dir = inputs["basis_dir"]
        basis_file = inputs["basis_file"]
        shift_file = inputs["shift_file"]
    else:
        rom_algo = None
        nmodes = None
        basis_dir = None
        basis_file = None
        shift_file = None

    if "hyper" in inputs["runtype"]:
        sampalgo   = inputs["sampalgo"]
        sampperc   = inputs["sampperc"]
    else:
        sampalgo = None
        sampperc = None

    # handle decomp inputs
    if "decomp" in inputs["runtype"]:
        ndomX = inputs["ndomX"]
        ndomY = inputs["ndomY"]
        overlap = inputs["overlap"]
        isadditive = inputs["isadditive"]
    else:
        ndomX = None
        ndomY = None
        overlap = None
        isadditive = None

    gen_runs(
        inputs["equations"],
        inputs["order"],
        inputs["problem"],
        inputs["scheme"],
        inputs["tf"],
        inputs["dt"],
        inputs["sampfreq"],
        inputs["meshroot"],
        inputs["nx"],
        inputs["ny"],
        inputs["runtype"],
        inputs["icFlag"],
        inputs["runner"],
        inputs["loglevel"],
        inputs["outdir_base"],
        phys_params_user=phys_params_user,
        ic_params_user=ic_params_user,
        rom_algo=rom_algo,
        nmodes=nmodes,
        basis_dir=basis_dir,
        basis_file=basis_file,
        shift_file=shift_file,
        ndomX=ndomX,
        ndomY=ndomY,
        overlap=overlap,
        isadditive=isadditive,
        sampalgo=sampalgo,
        sampperc=sampperc,
        run=inputs["run"],
    )


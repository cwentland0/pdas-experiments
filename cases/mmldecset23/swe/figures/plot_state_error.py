import os

import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rc("font", family="sans", size="10")
mpl.rc("axes", labelsize="x-large")
mpl.rc("figure", facecolor="w")
mpl.rc("text", usetex=False)

from pdas.data_utils import load_unified_helper
from pdas.prom_utils import load_reduced_data
from pdas.error_utils import calc_error_norms


# ----- START USER INPUTS -----

nvars = 3

datadir_base = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/runs"
casename = "coriolis{param}_gravity9.8_pulseMagnitude0.125_pulseX1.0_pulseY1.0"
paramlist = [0.0, -0.5, -1.0, -1.5, -2.0, -2.5, -3.0, -3.5, -4.0]
paramplot = -0.5
dataroot = "state_snapshots"
dt = 0.01
samplist = [1, 10]

meshname = "100x100"
ndomX = 2
ndomY = 2

meshdir_mono = f"/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/meshes/{meshname}/1x1"
basisdir_mono = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/pod_bases/coriolis_0p0_to_n4p0/1x1"

meshdir_decomp_base = f"/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/meshes/{meshname}/{ndomX}x{ndomY}"
overlap_list = [4, 10, 20]

nmodes = 40

ylim_spacenorm = [1e-3, 2e-1]
ylim_spacetimenorm = [1e-4, 2e-3]
colorlist = ["b", "r", "g"]
varnames = ["height", "xmom", "ymom"]
varlabels = ["Height", "X-momentum", "Y-momentum"]
outdir_base = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/figures/plots/error_overlap"

# ----- END USER INPUTS -----

nparams = len(paramlist)
noverlap = len(overlap_list)
timeplot_param_idx = paramlist.index(paramplot)

error_rom_spacetime = np.zeros((nvars, nparams), dtype=np.float64)
error_decomp_spacetime = np.zeros((nvars, noverlap, nparams), dtype=np.float64)

for param_idx, param in enumerate(paramlist):
    # load monolithic FOM data
    datadir_fom = os.path.join(datadir_base, "fom", meshname, casename.format(param=param))
    meshlist_mono, datalist_fom = load_unified_helper(
        meshdirs=meshdir_mono,
        datadirs=datadir_fom,
        nvars=nvars,
        dataroot=dataroot,
    )

    # load monolithic ROM data
    datadir_rom = os.path.join(datadir_base, "rom", meshname, casename.format(param=param), f"LSPG{nmodes}")
    data_rom = load_reduced_data(
        datadir_rom,
        dataroot,
        nvars,
        meshdir_mono,
        basisdir_mono,
        "basis",
        "center",
        "norm",
        nmodes,
    )

    # monolithic ROM error, space norm only
    errorlist, _ = calc_error_norms(
        meshlist=meshlist_mono*2,
        datalist=[datalist_fom[0], data_rom],
        nvars=nvars,
        dataroot=dataroot,
        dtlist=dt,
        samplist=samplist,
        timenorm=False,
        spacenorm=True,
        relative=False,
    )
    if param_idx == 0:
        nsamps = errorlist[0].shape[0]
        tf = dt * nsamps * samplist[1]
        t = np.linspace(0.0, tf, nsamps)
        error_rom_space = np.zeros((nsamps, nvars, nparams), dtype=np.float64)
    error_rom_space[:, :, param_idx] = errorlist[0]

    # monolithic ROM error, spacetime norm
    errorlist, _ = calc_error_norms(
        meshlist=meshlist_mono*2,
        datalist=[datalist_fom[0], data_rom],
        nvars=nvars,
        dataroot=dataroot,
        dtlist=dt,
        samplist=samplist,
        timenorm=True,
        spacenorm=True,
        relative=True,
    )
    error_rom_spacetime[:, param_idx] = errorlist[0]


    for overlap_idx, overlap in enumerate(overlap_list):
        # load decomposed ROM data
        datadir_decomp = os.path.join(datadir_base, "decomp", meshname, f"{ndomX}x{ndomY}", f"overlap{overlap}", casename.format(param=param), "multiplicative")
        datadir_decomp = os.path.join(datadir_decomp, (f"LSPG{nmodes}_"*ndomX*ndomY)[:-1])
        meshdir_decomp = os.path.join(meshdir_decomp_base, f"overlap{overlap}")
        _, datalist_decomp = load_unified_helper(
            meshdirs=meshdir_decomp,
            datadirs=datadir_decomp,
            nvars=nvars,
            dataroot=dataroot,
            merge_decomp=True,
        )

        # decomposed ROM error, space only
        errorlist, _ = calc_error_norms(
            meshlist=meshlist_mono*2,
            datalist=[datalist_fom[0], datalist_decomp[0]],
            nvars=nvars,
            dataroot=dataroot,
            dtlist=dt,
            samplist=samplist,
            timenorm=False,
            spacenorm=True,
            relative=False,
        )
        if param_idx == 0:
            nsamps = errorlist[0].shape[0]
            error_decomp_space = np.zeros((nsamps, nvars, noverlap, nparams), dtype=np.float64)
        error_decomp_space[:, :, overlap_idx, param_idx] = errorlist[0]

        # decomposed ROM error, spacetime
        errorlist, _ = calc_error_norms(
            meshlist=meshlist_mono*2,
            datalist=[datalist_fom[0], datalist_decomp[0]],
            nvars=nvars,
            dataroot=dataroot,
            dtlist=dt,
            samplist=samplist,
            timenorm=True,
            spacenorm=True,
            relative=True,
        )

        error_decomp_spacetime[:, overlap_idx, param_idx] = errorlist[0]

legend_labels = ["Monolithic"] + [f"$N_o$ = {overlap}" for overlap in overlap_list]

for var_idx in range(nvars):

    # space only norm
    fig, ax = plt.subplots(1, 1)
    ax.semilogy(t, error_rom_space[:, var_idx, timeplot_param_idx], "k")
    for overlap_idx, overlap in enumerate(overlap_list):
        ax.semilogy(t, error_decomp_space[:, var_idx, overlap_idx, timeplot_param_idx], colorlist[overlap_idx])
    ax.legend(legend_labels, fontsize=12)
    ax.set_title(f"{varlabels[var_idx]}, $\mu$ = {paramlist[timeplot_param_idx]}", fontsize=14)
    ax.set_xlabel("t")
    ax.set_ylabel("Absolute $\ell^2$ error")
    ax.set_ylim(ylim_spacenorm)
    ax.tick_params(axis="both", which="major", labelsize=12)

    plt.tight_layout()
    outfile = os.path.join(outdir_base, f"error_spacenorm_k{nmodes}_{varnames[var_idx]}.png")
    print(f"Saving image to {outfile}")
    plt.savefig(outfile)
    plt.close(fig)

    # spacetime norm
    fig, ax = plt.subplots(1, 1)
    ax.semilogy(paramlist, error_rom_spacetime[var_idx, :], "k")
    for overlap_idx, overlap in enumerate(overlap_list):
        ax.semilogy(paramlist, error_decomp_spacetime[var_idx, overlap_idx, :], colorlist[overlap_idx])
    ax.legend(legend_labels, fontsize=12)
    ax.set_title(f"{varlabels[var_idx]}", fontsize=14)
    ax.set_xlabel("$\mu$")
    ax.set_ylabel("Relative $\ell^2$ error")
    ax.set_ylim(ylim_spacetimenorm)
    ax.tick_params(axis="both", which="major", labelsize=12)

    plt.tight_layout()
    outfile = os.path.join(outdir_base, f"error_spacetimenorm_k{nmodes}_{varnames[var_idx]}.png")
    print(f"Saving image to {outfile}")
    plt.savefig(outfile)
    plt.close(fig)


print("Finished")

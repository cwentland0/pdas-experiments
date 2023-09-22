import os

import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rc("font", family="sans", size="10")
mpl.rc("axes", labelsize="x-large")
mpl.rc("figure", facecolor="w")
mpl.rc("text", usetex=False)

from pdas.data_utils import load_unified_helper
from pdas.prom_utils import load_pod_basis, calc_projection
from pdas.error_utils import calc_error_norms

# ----- START USER INPUTS -----

nvars = 3

datadir_base = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/runs/fom/100x100"
casename = "coriolis{param}_gravity9.8_pulseMagnitude0.125_pulseX1.0_pulseY1.0"
# param_list = [0.0, -0.5, -1.0, -1.5, -2.0, -2.5, -3.0, -3.5, -4.0]
param_list = [0.0, -1.0, -2.0, -3.0, -4.0]
dataroot = "state_snapshots"
dt = 0.01
samprate = 1

meshdir_mono = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/meshes/100x100/1x1"
basisdir_mono = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/pod_bases/coriolis_0p0_to_n4p0/1x1"

overlap = 10
meshdir_decomp = f"/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/meshes/100x100/2x2/overlap{overlap}"
basisdir_decomp = f"/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/pod_bases/coriolis_0p0_to_n4p0/2x2/overlap{overlap}"

modelist = [10, 20, 40]

ylim = [1e-5, 1e-2]
colorlist = ["b", "r", "g"]
varnames = ["height", "xmom", "ymom"]
varlabels = ["Height", "X-momentum", "Y-momentum"]
outdir_base = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/figures/plots/proj_error"

# ----- END USER INPUTS -----

maxmodes = np.amax(modelist)

# load monolithic basis
basis_mono, center_mono, norm_mono = load_pod_basis(
    basisdir_mono,
    return_basis=True,
    return_center=True,
    return_norm=True,
    nmodes=maxmodes,
)

# load decomposed basis
basis_decomp, center_decomp, norm_decomp = load_pod_basis(
    basisdir_decomp,
    return_basis=True,
    return_center=True,
    return_norm=True,
    nmodes=maxmodes,
)

nparams = len(param_list)
nmodes = len(modelist)
error_arr = np.zeros((nvars, 2, nmodes, nparams), dtype=np.float64)

# loop cases
for param_idx, param in enumerate(param_list):

    print(f"Parameter: {param}")

    # load data (monolithic)
    datadir = os.path.join(datadir_base, casename.format(param=param))
    meshlist_mono, datalist_mono = load_unified_helper(
        meshdirs=meshdir_mono,
        datadirs=datadir,
        nvars=nvars,
        dataroot=dataroot,
    )

    # loop modes
    for mode_idx, modes in enumerate(modelist):

        print(f"Modes: {modes}")

        datalist_mono_proj = calc_projection(
            modes,
            meshlist=meshlist_mono,
            datalist=datalist_mono,
            nvars=nvars,
            dataroot=dataroot,
            basis_in=basis_mono,
            center=center_mono,
            norm=norm_mono,
        )

        datalist_decomp_proj = calc_projection(
            modes,
            meshlist=meshlist_mono,
            datalist=datalist_mono,
            nvars=nvars,
            dataroot=dataroot,
            basis_in=basis_decomp,
            center=center_decomp,
            norm=norm_decomp,
            meshdir_decomp=meshdir_decomp,
        )

        errorlist, _ = calc_error_norms(
            meshlist=meshlist_mono * 3,
            datalist=[datalist_mono[0], datalist_mono_proj[0], datalist_decomp_proj[0]],
            nvars=nvars,
            dataroot=dataroot,
            dtlist=dt,
            samplist=samprate,
            timenorm=True,
            spacenorm=True,
            relative=True,
        )

        error_arr[:, 0, mode_idx, param_idx] = errorlist[0]
        error_arr[:, 1, mode_idx, param_idx] = errorlist[1]

legend_labels = [f"K = {modes}" for modes in modelist]

# plot
for var_idx in range(nvars):

    fig, ax = plt.subplots()

    artists = [None for _ in range(nmodes)]
    for mode_idx, modes in enumerate(modelist):
        artists[mode_idx], = ax.semilogy(
            param_list, error_arr[var_idx, 0, mode_idx, :],
            color=colorlist[mode_idx],
            linestyle="-"
        )
        ax.semilogy(
            param_list, error_arr[var_idx, 1, mode_idx, :],
            color=colorlist[mode_idx],
            linestyle="--"
        )

    ax.set_ylim(ylim)
    ax.set_xlabel("$\mu$")
    ax.set_ylabel("Relative projection error")
    ax.set_title(varlabels[var_idx], fontsize=14)
    ax.tick_params(axis="both", which="major", labelsize=12)
    ax.legend(artists, legend_labels, loc="upper left", fontsize=12)

    plt.tight_layout()
    outfile = os.path.join(outdir_base, f"error_{varnames[var_idx]}.png")
    print(f"Saving image to {outfile}")
    plt.savefig(outfile)
    plt.close(fig)

print("Finished")

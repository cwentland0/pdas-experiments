import os

import numpy as np

from pdas.data_utils import load_meshes, decompose_domain_data, write_to_binary

# ----- START USER INPUTS -----

ic_idx = 10

caseroot = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_euler"
ncellsX = 300
ncellsY = 300
order = "firstorder"
nvars = 4

ndomX = 2
ndomY = 2

overlap_list = [0, 10, 20]
param_list = [1.0, 1.125, 1.25, 1.375, 1.5, 1.625, 1.75, 1.875, 2.0]

casename = "gamma1.4_riemannTopRightPressure{param}_riemannTopRightXVel0.0_riemannTopRightYVel0.0_riemannTopRightDensity1.5_riemannBotLeftPressure0.029"

datafile = "state_snapshots.bin"

# ----- END USER INPUTS -----

def mkdir(dirpath):
    if not os.path.isdir(dirpath):
        os.mkdir(dirpath)

datadir_base = os.path.join(caseroot, "runs", "fom", f"{ncellsX}x{ncellsY}", order)

# make directories
outdir_base = os.path.join(caseroot, "runs", "ic_files")
mkdir(outdir_base)
outdir_base = os.path.join(outdir_base, f"{ncellsX}x{ncellsY}")
mkdir(outdir_base)
outdir_base = os.path.join(outdir_base, f"{ndomX}x{ndomY}")
mkdir(outdir_base)

for param_idx, param in enumerate(param_list):
    # load snapshot
    casedir = casename.format(param=param)
    infile = os.path.join(datadir_base, casedir, datafile)
    data = np.fromfile(infile, dtype=np.float64)
    data = np.reshape(data, (nvars, ncellsX, ncellsY, -1), order="F")
    data_snap = data[:, :, :, ic_idx]

    if (ndomX == 1) and (ndomY == 1):
        outdir = os.path.join(outdir_base, order)
        mkdir(outdir)
        outdir = os.path.join(outdir, casedir)
        mkdir(outdir)

        data_snap_out = data_snap.flatten(order="F")
        outfile = os.path.join(outdir, f"ic_file_idx{ic_idx}.bin")
        print(f"Saving IC file to {outfile}")
        write_to_binary(data_snap_out, outfile)

    else:
        for overlap_idx, overlap in enumerate(overlap_list):

            # load mesh
            meshdir = os.path.join(caseroot, "meshes", f"{ncellsX}x{ncellsY}", f"{ndomX}x{ndomY}", f"overlap{overlap}", order, "full")
            _, meshlist_decomp = load_meshes(meshdir, merge_decomp=False)

            # decompose initial conditions
            data_snap_full = np.transpose(data_snap, (1, 2, 0))
            data_decomp = decompose_domain_data(
                data_snap_full,
                meshlist_decomp,
                overlap,
                is_ts=False,
                is_ts_decomp=False,
            )

            outdir = os.path.join(outdir_base, f"overlap{overlap}")
            mkdir(outdir)
            outdir = os.path.join(outdir, order)
            mkdir(outdir)
            outdir = os.path.join(outdir, casedir)
            mkdir(outdir)

            # write to binary
            for j in range(ndomY):
               for i in range(ndomX):
                   dom_idx = i + 2 * j
                   data_out = np.transpose(data_decomp[i][j][0], (2, 0, 1))
                   data_out = data_out.flatten(order="F")
                   outfile = os.path.join(outdir, f"ic_file_idx{ic_idx}_dom{dom_idx}.bin")
                   write_to_binary(data_out, outfile)

print("Finished")



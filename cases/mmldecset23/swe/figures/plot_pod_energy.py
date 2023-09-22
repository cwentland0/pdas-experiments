import os

from pdas.prom_utils import load_pod_basis
from pdas.vis_utils import plot_pod_res_energy

# ----- START USER INPUTS -----

basedir = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/pod_bases/coriolis_0p0_to_n4p0"

ndomX = 2
ndomY = 2

overlap_iso = 10
labels_iso = ['Monolithic', '$\Omega_1$', '$\Omega_2$', '$\Omega_3$', '$\Omega_4$']

dom_idx_multi = 0
overlap_list_multi = [10, 20]
labels_multi = ['Monolithic', '$n_o$ = 10', '$n_o$ = 20']

plotcolors = ["k", "r", "b", "c", "m"]
linestyles = ["-"] * (1 + ndomX * ndomY)
xlim = [0, 60]

outdir = "/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/figures/plots"

# ----- END USER INPUTS -----

ndomains = ndomX * ndomY
assert dom_idx_multi < ndomains

basisdir_mono = os.path.join(basedir, "1x1")
svals_mono, = load_pod_basis(basisdir_mono, return_basis=False, return_svals=True)

# ----- plot single domain vs multiple subdomains, one overlap value -----

basisdir_decomp_iso = os.path.join(basedir, f"{ndomX}x{ndomY}", f"overlap{overlap_iso}")
svals_decomp_iso, = load_pod_basis(basisdir_decomp_iso, return_basis=False, return_svals=True)

svals = [svals_mono] + svals_decomp_iso

plot_pod_res_energy(
    outdir,
    svals=svals,
    xlim=xlim,
    plotcolors=plotcolors,
    linestyles=linestyles,
    legend_labels=labels_iso,
    outsuff="_subdom",
)

# ----- plot single domain vs one subdomain, multiple overlap values

svals = [svals_mono]
for overlap in overlap_list_multi:
    basisdir_decomp_multi = os.path.join(basedir, f"{ndomX}x{ndomY}", f"overlap{overlap}")
    svals_decomp_multi, = load_pod_basis(basisdir_decomp_multi, return_basis=False, return_svals=True)
    svals.append(svals_decomp_multi[dom_idx_multi])

plot_pod_res_energy(
    outdir,
    svals=svals,
    xlim=xlim,
    plotcolors=plotcolors,
    linestyles=linestyles,
    legend_labels=labels_multi,
    outsuff="_overlap",
)

print("Finished")

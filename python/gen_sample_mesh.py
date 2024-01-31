from argparse import ArgumentParser

from pdas.prom_utils import gen_sample_mesh

parser = ArgumentParser()
parser.add_argument("--algo",       dest="algo",       type=str)
parser.add_argument("--fulldir",    dest="fulldir",    type=str)
parser.add_argument("--outdir",     dest="outdir",     type=str)
parser.add_argument("--sampperc",   dest="sampperc",   type=float)
parser.add_argument("--sampbounds", dest="sampbounds", type=bool)
args = parser.parse_args()

gen_sample_mesh(
    args.algo,
    args.fulldir,
    args.outdir,
    percpoints=args.sampperc,
    randseed=2,
    samp_bounds=args.sampbounds,
)

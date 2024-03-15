from argparse import ArgumentParser

from pdas.samp_utils import gen_sample_mesh

parser = ArgumentParser()
parser.add_argument("--algo",           dest="algo",           type=str)
parser.add_argument("--fulldir",        dest="fulldir",        type=str)
parser.add_argument("--outdir",         dest="outdir",         type=str)
parser.add_argument("--basisdir",       dest="basisdir",       type=str)
parser.add_argument("--nmodes",         dest="nmodes",         type=int)
parser.add_argument("--sampperc",       dest="sampperc",       type=float)
parser.add_argument("--seedqdeim",      dest="seedqdeim",      default=False, action='store_true')
parser.add_argument("--seedphysbounds", dest="seedphysbounds", default=False, action='store_true')
parser.add_argument("--seeddombounds",  dest="seeddombounds",  default=False, action='store_true')
parser.add_argument("--seedphysrate",   dest="seedphysrate",   default=1)
parser.add_argument("--seeddomrate",    dest="seeddomrate",    default=1)
parser.add_argument("--sampphysbounds", dest="sampphysbounds", default=False, action='store_true')
parser.add_argument("--sampdombounds",  dest="sampdombounds",  default=False, action='store_true')
args = parser.parse_args()

gen_sample_mesh(
    args.algo,
    args.fulldir,
    args.sampperc,
    args.outdir,
    basis_dir=args.basisdir,
    nmodes=args.nmodes,
    seed_qdeim=bool(args.seedqdeim),
    seed_phys_bounds=bool(args.seedphysbounds),
    seed_phys_rate=int(args.seedphysrate),
    seed_dom_bounds=bool(args.seeddombounds),
    seed_dom_rate=int(args.seeddomrate),
    samp_phys_bounds=bool(args.sampphysbounds),
    samp_dom_bounds=bool(args.sampdombounds),
    randseed=2,
)

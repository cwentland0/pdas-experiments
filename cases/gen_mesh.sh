#!/bin/bash

MESHDRIVER="/home/crwentl/research/code/pressio-proj/pressio-demoapps/meshing_scripts/create_full_mesh.py"
DECOMPDRIVER="/home/crwentl/research/code/pressio-proj/pressio-demoapps-schwarz/meshing_scripts/create_decomp_meshes.py"

decomp=1

nx=100
ny=100

# decomp settings
domx=2
domy=2
overlap=20

# SWE
#OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/swe/meshes"
#xl="-5.0"
#xu="5.0"
#yl="-5.0"
#yu="5.0"

# Riemann
OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/mmldecset23/riemann/meshes"
xl="0.0"
xu="1.0"
yl="0.0"
yu="1.0"

# get and create output directories
OUTDIR="${OUTDIRBASE}/${nx}x${ny}"
mkdir ${OUTDIR}

# execute
if [ ${decomp} -eq 0 ]; then
    OUTDIR="${OUTDIR}/1x1"
    PARAMS="--numCells ${nx} ${ny} --outDir ${OUTDIR} -s 3 --bounds ${xl} ${xu} ${yl} ${yu}"
    echo ${PARAMS}
    python ${MESHDRIVER} ${PARAMS}
else
    OUTDIR="${OUTDIR}/${domx}x${domy}/overlap${overlap}"
    mkdir ${OUTDIR}
    python ${DECOMPDRIVER} --meshScript ${MESHDRIVER} -n ${nx} ${ny} --outDir ${OUTDIR} -s 3 --bounds ${xl} ${xu} ${yl} ${yu} --numDoms ${domx} ${domy} --overlap ${overlap}
fi

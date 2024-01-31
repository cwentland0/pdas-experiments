#!/bin/bash

# ----- START USER INPUTS -----

MESHDRIVER="/home/crwentl/research/code/pressio-proj/pressio-demoapps/meshing_scripts/create_full_mesh.py"
DECOMPDRIVER="/home/crwentl/research/code/pressio-proj/pressio-demoapps-schwarz/meshing_scripts/create_decomp_meshes.py"
SAMPDRIVER="/home/crwentl/research/code/pressio-proj/pdas-experiments/python/gen_sample_mesh.py"
SAMPMESHDRIVER="/home/crwentl/research/code/pressio-proj/pressio-demoapps/meshing_scripts/create_sample_mesh.py"

# 3: first order
# 5: WENO3
# 7: WENO5
stencil=3

decomp=1
hyper=1

nx=50
ny=50

# decomp settings
domx=2
domy=2
overlap=10

# hyper-reduction settings
sampalgo="random"
sampperc=1.0
sampbounds=1

# SWE
OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/test/swe/meshes"
xl="-5.0"
xu="5.0"
yl="-5.0"
yu="5.0"

# Riemann
#OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/test/riemann/meshes"
#xl="0.0"
#xu="1.0"
#yl="0.0"
#yu="1.0"

# ----- END USER INPUTS -----

# get and create output directories
OUTDIR="${OUTDIRBASE}/${nx}x${ny}"
mkdir ${OUTDIR}

if [ ${stencil} -eq 3 ]; then
    stencilDir="firstorder"
elif [ ${stencil} -eq 5 ]; then
    stencilDir="weno3"
elif [ ${stencil} -eq 7 ]; then
    stencilDir="weno5"
else
    echo "Invalid stencil value: ${stencil}"
    exit
fi

# execute
if [ ${decomp} -eq 0 ]; then

    # full mesh
    OUTDIR="${OUTDIR}/1x1"
    mkdir ${OUTDIR}
    OUTDIR="${OUTDIR}/${stencilDir}"
    mkdir ${OUTDIR}
    FULLDIR="${OUTDIR}/full"
    mkdir ${FULLDIR}
    python3 ${MESHDRIVER} --numCells ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu}

    # sample mesh, if requested
    if [ ${hyper} -eq 1 ]; then
        SAMPDIR="${OUTDIR}/${sampalgo}"
        mkdir ${SAMPDIR}
        SAMPDIR="${SAMPDIR}/samp_${sampperc}"
        if [ ${sampbounds} -eq 1 ]; then
            SAMPDIR="${SAMPDIR}_bounds"
        fi
        mkdir ${SAMPDIR}

        python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} --sampbounds ${sampbounds}
        python3 ${SAMPMESHDRIVER} --fullMeshDir ${FULLDIR} --sampleMeshIndices ${SAMPDIR}/sample_mesh_gids.dat --outDir ${SAMPDIR}
    fi

else
    # full mesh
    OUTDIR="${OUTDIR}/${domx}x${domy}"
    mkdir ${OUTDIR}
    OUTDIR="${OUTDIR}/overlap${overlap}"
    mkdir ${OUTDIR}
    OUTDIR="${OUTDIR}/${stencilDir}"
    mkdir ${OUTDIR}
    FULLDIR="${OUTDIR}/full"
    mkdir ${FULLDIR}

    python3 ${DECOMPDRIVER} --meshScript ${MESHDRIVER} -n ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu} --numDoms ${domx} ${domy} --overlap ${overlap}

    # sample mesh, if requested
    if [ ${hyper} -eq 1 ]; then
        SAMPDIR="${OUTDIR}/${sampalgo}"
        mkdir ${SAMPDIR}
        SAMPDIR="${SAMPDIR}/samp_${sampperc}"
        if [ ${sampbounds} -eq 1 ]; then
            SAMPDIR="${SAMPDIR}_bounds"
        fi
        mkdir ${SAMPDIR}

        python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} --sampbounds ${sampbounds}
    fi

fi

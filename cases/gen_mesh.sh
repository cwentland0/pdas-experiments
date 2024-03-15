#!/bin/bash

declare -a sampperc_arr
declare -a nmodes_arr

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

nx=300
ny=300

# decomp settings
domx=2
domy=2
overlap=0

# hyper-reduction settings
sampalgo="random"
# sampalgo="eigenvec"

# sampperc_arr=("0.001" "0.0025" "0.005" "0.01" "0.025" "0.05")
# sampperc_arr=("0.005" "0.01" "0.025" "0.05")
# nmodes_arr=(20 40 60 80 100)
# nmodes_arr=(60 80 100)
nmodes_arr=(300)
sampperc_arr=("0.1")
# nmodes_arr=(100)

seedqdeim=0
seedphys=0
seedphysrate=0
seeddom=1
seeddomrate=1
sampphys=0
sampdom=0


# SWE
# basisroot="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_swe/pod_bases/coriolis_0p0_to_n4p0"
# OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_swe/meshes"
# xl="-5.0"
# xu="5.0"
# yl="-5.0"
# yu="5.0"

# Riemann
basisroot="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_euler/pod_bases/topRightPress_1p0_to_2p0"
OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_euler/meshes"
xl="0.0"
xu="1.0"
yl="0.0"
yu="1.0"

# ----- END USER INPUTS -----

if [ ${hyper} -eq 0 ]; then
    sampperc_arr=("")
    nmodes_arr=("")
fi

if [ ${decomp} -eq 0 ]; then
    domx=1
    domy=1
fi

# get and create output directories
OUTDIRBASE="${OUTDIRBASE}/${nx}x${ny}"
mkdir ${OUTDIRBASE} || (echo "I can't create directory ${OUTDIRBASE}" && exit)

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

# hyper-reduction flags
FLAGSTR=""
if [ ${seedqdeim} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --seedqdeim"
fi
if [ ${seedphys} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --seedphysbounds"
    if [ ${seedphysrate} -lt 1 ]; then
        echo "Invalid seedphysrate: ${seedphysrate}"
        exit
    fi
    FLAGSTR="${FLAGSTR} --seedphysrate ${seedphysrate}"
fi
if [ ${seeddom} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --seeddombounds"
    if [ ${seeddomrate} -lt 1 ]; then
        echo "Invalid seeddomrate: ${seeddomrate}"
        exit
    fi
    FLAGSTR="${FLAGSTR} --seeddomrate ${seeddomrate}"
fi
if [ ${sampphys} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --sampphysbounds"
fi
if [ ${sampdom} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --sampdombounds"
fi

OUTDIRBASE="${OUTDIRBASE}/${domx}x${domy}"
mkdir -p ${OUTDIRBASE}

# execute
for sampperc in "${sampperc_arr[@]}"; do
    for nmodes in "${nmodes_arr[@]}"; do
        if [ ${decomp} -eq 0 ]; then

            # full mesh
            OUTDIR="${OUTDIRBASE}/${stencilDir}"
            mkdir -p ${OUTDIR}
            FULLDIR="${OUTDIR}/full"
            mkdir -p ${FULLDIR}
            python3 ${MESHDRIVER} --numCells ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu}

            # sample mesh, if requested
            if [ ${hyper} -eq 1 ]; then

                BASISDIR="${basisroot}/${nx}x${ny}/1x1/${stencilDir}"
                BASISSTR="--basisdir ${BASISDIR}"
                MODESTR="--nmodes ${nmodes}"
        
                SAMPDIR="${OUTDIR}/${sampalgo}"
                mkdir -p ${SAMPDIR}
                SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                if [ ${seedqdeim} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_qdeim"
                fi
                if [ ${seedphys} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_phys${seedphysrate}"
                fi
                if [ ${seeddom} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_dom${seeddomrate}"
                fi
                mkdir -p ${SAMPDIR}
        
                python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} ${BASISSTR} ${MODESTR} ${FLAGSTR}
                python3 ${SAMPMESHDRIVER} --fullMeshDir ${FULLDIR} --sampleMeshIndices ${SAMPDIR}/sample_mesh_gids.dat --outDir ${SAMPDIR}
            fi
        
        else
            # full mesh
            OUTDIR="${OUTDIRBASE}/overlap${overlap}"
            mkdir -p ${OUTDIR}
            OUTDIR="${OUTDIR}/${stencilDir}"
            mkdir -p ${OUTDIR}
            FULLDIR="${OUTDIR}/full"
            mkdir -p ${FULLDIR}
        
            python3 ${DECOMPDRIVER} --meshScript ${MESHDRIVER} -n ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu} --numDoms ${domx} ${domy} --overlap ${overlap}
        
            # sample mesh, if requested
            if [ ${hyper} -eq 1 ]; then

                BASISDIR="${basisroot}/${nx}x${ny}/${domx}x${domy}/overlap${overlap}/${stencilDir}"
                BASISSTR="--basisdir ${BASISDIR}"
                MODESTR="--nmodes ${nmodes}"

                SAMPDIR="${OUTDIR}/${sampalgo}"
                mkdir -p ${SAMPDIR}
                SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                if [ ${seedqdeim} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_qdeim"
                fi
                if [ ${seedphys} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_phys${seedphysrate}"
                fi
                if [ ${seeddom} -eq 1 ]; then
                    SAMPDIR="${SAMPDIR}_dom${seeddomrate}"
                fi
                mkdir -p ${SAMPDIR}

                python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} ${BASISSTR} ${MODESTR} ${FLAGSTR}
            fi
        
        fi
    done
done

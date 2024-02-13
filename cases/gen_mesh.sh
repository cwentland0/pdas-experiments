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

decomp=0
hyper=0

nx=100
ny=100

# decomp settings
domx=2
domy=2
overlap=20

# hyper-reduction settings
sampalgo="random"
basisroot="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_swe/pod_bases/coriolis_0p0_to_n4p0"

#declare -a sampperc_arr=("0.0025" "0.005" "0.01" "0.025" "0.05")
#declare -a nmodes_arr=(25 50 75 100)
declare -a sampperc_arr=("0.01")
declare -a nmodes_arr=("[75, 75, 75, 75]")

seedqdeim=1
seedphys=0
seeddom=0
sampphys=1
sampdom=1

# SWE
#OUTDIRBASE="/home/crwentl/research/code/pressio-proj/pdas-experiments/cases/siamuq24/2d_swe/meshes"
#xl="-5.0"
#xu="5.0"
#yl="-5.0"
#yu="5.0"

# Riemann
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

if [ ${hyper} -eq 0 ]; then
    domx=1
    domy=1
fi

# get and create output directories
OUTDIRBASE="${OUTDIRBASE}/${nx}x${ny}"
mkdir ${OUTDIRBASE}

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
fi
if [ ${seeddom} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --seeddombounds"
fi
if [ ${sampphys} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --sampphysbounds"
fi
if [ ${sampdom} -eq 1 ]; then
    FLAGSTR="${FLAGSTR} --sampdombounds"
fi

OUTDIRBASE="${OUTDIRBASE}/${domx}x${domy}"
mkdir ${OUTDIRBASE}

# execute
for sampperc in "${sampperc_arr[@]}"; do
    for nmodes in "${nmodes_arr[@]}"; do
        if [ ${decomp} -eq 0 ]; then
        
            # full mesh
            OUTDIR="${OUTDIRBASE}/${stencilDir}"
            mkdir ${OUTDIR}
            FULLDIR="${OUTDIR}/full"
            mkdir ${FULLDIR}
            python3 ${MESHDRIVER} --numCells ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu}
        
            # sample mesh, if requested
            if [ ${hyper} -eq 1 ]; then
        
                BASISDIR="${basisroot}/1x1/${stencilDir}"
                if [ ${sampalgo} = "eigenvec" ]; then
                    BASISSTR="--basisdir ${BASISDIR}"
                    MODESTR="--nmodes ${nmodes}"
                else
                    if [ ${seedqdeim} -eq 1 ]; then
                        BASISSTR="--basisdir ${BASISDIR}"
                        MODESTR="--nmodes ${nmodes}"
                    else
                        BASISSTR=""
                        MODESTR=""
                    fi
                fi
        
                SAMPDIR="${OUTDIR}/${sampalgo}"
                mkdir ${SAMPDIR}
                if [ ${sampalgo} = "random" ]; then
                    if [ ${seedqdeim} -eq 1 ]; then
                        SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                    else
                        SAMPDIR="${SAMPDIR}/samp_${sampperc}"
                    fi
                else
                    SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                fi
                #if [ ${sampbounds} -eq 1 ]; then
                #    SAMPDIR="${SAMPDIR}_bounds"
                #fi
                #mkdir ${SAMPDIR}
        
                python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} ${BASISSTR} ${MODESTR} ${FLAGSTR}
                python3 ${SAMPMESHDRIVER} --fullMeshDir ${FULLDIR} --sampleMeshIndices ${SAMPDIR}/sample_mesh_gids.dat --outDir ${SAMPDIR}
            fi
        
        else
            # full mesh
            OUTDIR="${OUTDIRBASE}/overlap${overlap}"
            mkdir ${OUTDIR}
            OUTDIR="${OUTDIR}/${stencilDir}"
            mkdir ${OUTDIR}
            FULLDIR="${OUTDIR}/full"
            mkdir ${FULLDIR}
        
            python3 ${DECOMPDRIVER} --meshScript ${MESHDRIVER} -n ${nx} ${ny} --outDir ${FULLDIR} -s ${stencil} --bounds ${xl} ${xu} ${yl} ${yu} --numDoms ${domx} ${domy} --overlap ${overlap}
        
            # sample mesh, if requested
            if [ ${hyper} -eq 1 ]; then

                BASISDIR="${basisroot}/${domx}x${domy}/${stencilDir}"
                if [ ${sampalgo} = "eigenvec" ]; then
                    BASISSTR="--basisdir ${BASISDIR}"
                    MODESTR="--nmodes ${nmodes}"
                else
                    if [ ${seedqdeim} -eq 1 ]; then
                        BASISSTR="--basisdir ${BASISDIR}"
                        MODESTR="--nmodes ${nmodes}"
                    else
                        BASISSTR=""
                        MODESTR=""
                    fi
                fi

                SAMPDIR="${OUTDIR}/${sampalgo}"
                mkdir ${SAMPDIR}
                if [ ${sampalgo} = "random" ]; then
                    if [ ${seedqdeim} -eq 1 ]; then
                        SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                    else
                        SAMPDIR="${SAMPDIR}/samp_${sampperc}"
                    fi
                else
                    SAMPDIR="${SAMPDIR}/modes_${nmodes}_samp_${sampperc}"
                fi

                python3 ${SAMPDRIVER} --algo ${sampalgo} --fulldir ${FULLDIR} --outdir ${SAMPDIR} --sampperc ${sampperc} ${BASISSTR} ${MODESTR} ${FLAGSTR}
            fi
        
        fi
    done
done

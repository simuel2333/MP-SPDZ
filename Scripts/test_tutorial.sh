#!/usr/bin/env bash

while getopts XYC opt; do
    case $opt in
	X) compile_opts=-X
	   dabit=1
	   ;;
	Y) dabit=2
	   ;;
    esac
done

shift $[OPTIND-1]

for i in 0 1; do
    seq 0 3 > Player-Data/Input-P$i-0
done

# clean state
rm Player-Data/*Params*
rm Player-Data/*Secrets*

function test_vm
{
    ulimit -c unlimited
    vm=$1
    shift
    if ! Scripts/$vm.sh tutorial $* > /dev/null ||
	    ! grep 'weighted average: 2.333' logs/tutorial-0; then
	for i in $(seq 4 -1 0); do
	    echo == Party $i
	    cat logs/tutorial-$i
	done
	exit 1
    fi
}

# big buckets for smallest batches
run_opts="$run_opts -B 5"

for dabit in ${dabit:-0 1 2}; do
    if [[ $dabit = 1 ]]; then
	compile_opts="$compile_opts -X"
    elif [[ $dabit = 2 ]]; then
	compile_opts="$compile_opts -Y"
    fi

    ./compile.py -R 64 $compile_opts tutorial

    for i in ring rep4-ring semi2k brain mal-rep-ring ps-rep-ring sy-rep-ring \
	     spdz2k; do
	test_vm $i $run_opts
    done

    ./compile.py  $compile_opts tutorial

    for i in rep-field shamir mal-rep-field ps-rep-field sy-rep-field \
		       mal-shamir sy-shamir hemi semi \
		       soho cowgear mascot; do
	test_vm $i $run_opts
    done

    test_vm chaigear $run_opts -l 3 -c 2
done

if test $dabit != 0; then
    ./compile.py -R 64 -Z 3 tutorial
    test_vm ring $run_opts

    ./compile.py -R 64 -Z 4 tutorial
    test_vm rep4-ring $run_opts

    ./compile.py -R 64 -Z ${PLAYERS:-2} tutorial
    test_vm semi2k $run_opts
fi

./compile.py tutorial

test_vm cowgear $run_opts -T
test_vm chaigear $run_opts -T -l 3 -c 2

if test $skip_binary; then
   exit
fi

./compile.py -B 16  $compile_opts tutorial

for i in replicated mal-rep-bin semi-bin ccd mal-ccd; do
    test_vm $i $run_opts
done

test_vm yao

for i in tinier rep-bmr mal-rep-bmr shamir-bmr mal-shamir-bmr; do
    test_vm $i $run_opts
done

# Setting Up
This repository contains instructions on creating 2 OPAM Switches to automate the execution of the algorithm.


## To setup, The below is the command history of me setting up the switches.

mkdir seu_setup
cd seu_setup/
sudo apt-get update
sudo apt-get install -y vim
vim cil-switch.export
vim frama-switch.export
opam switch create frama-switch 4.12.1
opam list
opam switch
eval $(opam env --switch=frama-switch --set-switch)
opam switch import ./frama-switch.export 
sudo apt-get install -y autoconf pkg-config m4 zlib1g-dev libgmp-dev graphviz libgtk-3-dev libcairo2-dev libgtksourceview-2.0-dev libgmp-dev build-essential unzip
apt-get install -y   autoconf pkg-config m4 zlib1g-dev libgmp-dev   graphviz libgtk-3-dev libcairo2-dev   libgtksourceview-3.0-dev build-essential unzip
sudo apt-get install -y   autoconf pkg-config m4 zlib1g-dev libgmp-dev   graphviz libgtk-3-dev libcairo2-dev   libgtksourceview-3.0-dev build-essential unzip
opam switch import ./frama-switch.export 
opam switch
opam list
opam list | grep frama-c
opam switch
opam switch create cil-switch 4.01.0
eval $(opam env --switch=cil-switch --set-switch)
opam switch
opam switch import ./cil-switch.export 
opam repository add archive git+https://github.com/ocaml/opam-repository-archive
opam repository add archive git+https://github.com/ocaml/opam-repository-archive
opam switch
opam update
opam switch
opam switch import ./cil-switch.export 
opam list | grep cil

## To setup CBMC, the following command history.
sudo apt install -y   build-essential   cmake   ninja-build   flex   bison   libboost-all-dev   zlib1g-dev
git clone https://github.com/diffblue/cbmc.git
cd cbmc/
mkdir build
cd build
cmake -G Ninja -DWITH_JBMC=OFF ..
ninja
    //now you can verify that cbmc has been installed
./bin/cbmc --version
echo 'int main(){int x=0; assert(x==0);}' > test.c

    //to download it system wide, inside the 'build' directory itself, run
sudo ninja install
    //now change to any directory you want to and run
cbmc --version

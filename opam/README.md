# CRV (Conditionally Relevant Variables) Implementation
This repository contains all the details from setup to testing the algorithm locally on your laptop. Find the Docker Image at https://hub.docker.com/repository/docker/vrajnandak/crv_implementation/general

## Relevant folders & files
- **opam-repository folder**: Contains the OCaml package download and was downloaded only for usage and is meant to be untouched by an end-user.
- **seu-setup folder**: Contains the 'cil switch' export files for creation of stable independent OPAM switches. This folder is to be untouched if using the docker image since the setup has already been done. Only refer to this if you are setting up locally on your laptop.
- **demo**: Is the most relevant folder towards executing and checking for conditional relevance of variables.

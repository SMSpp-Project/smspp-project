# --------------------------------------------------------------------------- #
#    Dockerfile for CI/CD                                                     #
#                                                                             #
#    This file contains the commands to build a Docker image containing       #
#    all the packages needed to build and test the project.                   #
#    Once built and uploaded in the repository's container registry           #
#    (See: https://gitlab.com/smspp/smspp-project/container_registry),        #
#    the image can be fetched and used by the GitLab Runner.                  #
#                                                                             #
#    Login to GitLab registry with:                                           #
#                                                                             #
#        $ docker login registry.gitlab.com                                   #
#                                                                             #
#    Build this with:                                                         #
#                                                                             #
#        $ docker build -t registry.gitlab.com/smspp/smspp-project .          #
#                                                                             #
#    Upload with:                                                             #
#                                                                             #
#        $ docker push registry.gitlab.com/smspp/smspp-project                #
#                                                                             #
#    Run (locally) with:                                                      #
#                                                                             #
#        $ docker run --rm -it registry.gitlab.com/smspp/smspp-project:latest #
#                                                                             #
#    Note: you need to rebuild and upload the image only when this file       #
#          changes, not when SMS++ changes.                                   #
#                                                                             #
#                              Niccolo' Iardella                              #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# Latest Ubuntu image
FROM ubuntu:latest

# Install required packages, run the INSTALL.sh script, and clean up
RUN apt-get update && apt-get install -y wget sudo && \
    wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | bash && \
    rm -rf /var/lib/apt/lists/*

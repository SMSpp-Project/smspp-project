# --------------------------------------------------------------------------- #
#    Dockerfile for CI/CD                                                     #
#                                                                             #
#    This file contains the commands to build a Docker image with all         #
#    the packages needed to build and test the project. The same image        #
#    serves both CIs: GitLab pulls it from its container registry and         #
#    GitHub Actions from the GitHub one (ghcr.io):                            #
#                                                                             #
#        GitLab : registry.gitlab.com/smspp/smspp-project                     #
#        GitHub : ghcr.io/smspp-project/smspp-project                         #
#                                                                             #
#    On GitHub the image is (re)built and pushed automatically by the         #
#    .github/workflows/docker-image.yml workflow whenever this file or        #
#    INSTALL.sh changes. To build and push it by hand for GitLab, log         #
#    in to the registry with:                                                 #
#                                                                             #
#        $ docker login registry.gitlab.com                                   #
#                                                                             #
#    build this with:                                                         #
#                                                                             #
#        $ docker build --no-cache --pull \                                   #
#              -t registry.gitlab.com/smspp/smspp-project .                   #
#                                                                             #
#    The --no-cache --pull flags force a fresh build: INSTALL.sh is           #
#    fetched at build time, so Docker would otherwise reuse the cached        #
#    RUN layer and silently keep an outdated image.                           #
#                                                                             #
#    upload with:                                                             #
#                                                                             #
#        $ docker push registry.gitlab.com/smspp/smspp-project                #
#                                                                             #
#    run (locally) with:                                                      #
#                                                                             #
#        $ docker run --rm -it registry.gitlab.com/smspp/smspp-project:latest #
#                                                                             #
#    Note: rebuild and upload the image when this file or INSTALL.sh          #
#          changes, not when SMS++ changes.                                   #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# Latest Ubuntu image
FROM ubuntu:latest

# Install required packages, run the INSTALL.sh script, and clean up
RUN apt-get update && apt-get install -y wget sudo && \
    wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | bash -s -- --without-cplex --without-gurobi --without-smspp && \
    rm -rf /var/lib/apt/lists/*

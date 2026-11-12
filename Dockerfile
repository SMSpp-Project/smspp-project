# --------------------------------------------------------------------------- #
#    Dockerfile for CI/CD                                                     #
#                                                                             #
#    This file contains the commands to build a Docker image with all         #
#    the packages needed to build and test the project. The same image        #
#    serves both CIs, GitLab and GitHub Actions, which pull it from the       #
#    GitHub registry:                                                         #
#                                                                             #
#        ghcr.io/smspp-project/smspp-project                                  #
#                                                                             #
#    The image is (re)built and pushed automatically by the                   #
#    .github/workflows/docker-image.yml workflow whenever this file or        #
#    INSTALL.sh changes, and by hand from the Actions tab of GitHub           #
#    (workflow_dispatch), e.g., when a dependency has a new release.          #
#                                                                             #
#    build it locally with:                                                   #
#                                                                             #
#        $ docker build --no-cache --pull -t smspp-ci .                       #
#                                                                             #
#    The --no-cache --pull flags force a fresh build: INSTALL.sh is           #
#    fetched at build time, so Docker would otherwise reuse the cached        #
#    RUN layer and silently keep an outdated image.                           #
#                                                                             #
#    run (locally) with:                                                      #
#                                                                             #
#        $ docker run --rm -it ghcr.io/smspp-project/smspp-project:latest     #
#                                                                             #
#    Note: rebuild the image when this file or INSTALL.sh changes, or a       #
#          dependency has a new release, not when SMS++ changes.              #
#                                                                             #
#    Note: Gurobi is installed but unlicensed: the CI jobs materialize a      #
#          WLS license at run time from masked CI/CD variables (GitLab)       #
#          or secrets (GitHub), so no license ever enters the image.          #
#                                                                             #
#    Note: MOSEK is installed but unlicensed as well: the CI jobs find its    #
#          license in the file CI/CD variable MOSEKLM_LICENSE_FILE            #
#          (GitLab) or write it from the secret MOSEK_LICENSE (GitHub),       #
#          and MOSEK reads its path from the environment.                     #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# Latest Ubuntu image
FROM ubuntu:latest

# Install required packages, run the INSTALL.sh script, and clean up
RUN apt-get update && apt-get install -y wget sudo && \
    wget -qO- https://gitlab.com/smspp/smspp-project/-/raw/develop/INSTALL.sh | \
      bash -s -- --without-cplex --without-smspp && \
    rm -rf /var/lib/apt/lists/*

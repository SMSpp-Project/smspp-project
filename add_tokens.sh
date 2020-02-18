#!/usr/bin/env bash
# --------------------------------------------------------------------------- #
#    This Bash script adds the GitLab deploy tokens to the submodule URLs     #
#    defined in .gitmodules, so that the GitLab CI/CD runner can fetch them.  #
#                                                                             #
#    A token must be defined for each module at:                              #
#        https://gitlab.com/smspp/<module>/-/settings/repository              #
#    and then added as environment variable at:                               #
#        https://gitlab.com/smspp/smspp-project/-/settings/ci_cd              #
#                                                                             #
#    NOTE: This will be required as long as SMS++ repositories are private!   #
# --------------------------------------------------------------------------- #

submodules=(
  "bundlesolver"
  "lukfiblock"
  "mcfblock"
  "milpsolver"
  "sddpblock"
  "smilpblock"
  "smspp"
  "stochasticblock"
  "subgradsolver"
  "ucblock"
  "tests"
  "tools"
)

for i in "${submodules[@]}"; do
  :
  TOKEN_NAME="${i}_TOKEN"
  echo "$TOKEN_NAME"
  echo "${!TOKEN_NAME}"
  sed -i "s/gitlab.com\/smspp\/$i/pages-job:${!TOKEN_NAME}@gitlab.com\/smspp\/$i/" .gitmodules
done

#!/bin/bash

set -e

script_name="HTML Builder"
script_version="1.5.1"

tmpDir="tmp"
webDir="web"

show_help=0
clean_mode=0
clean_before=0
production_mode=0
node_help=0

echo "$script_name version $script_version"

function Usage () {
  echo "Usage $0 [OPTION]"
  echo
  echo "Without options, will build in development mode, this means no minimization and no compression"
  echo
  echo "-h exit after showing this help"
  echo "-c exit after cleaning the temporary and output directories"
  echo "-k clean before build"
  echo "-p build in production mode"
  echo "-n help for Node.js, npm and npm modules"
  echo
  echo "Up to date doc should be here:"
  echo "https://calinradoni.github.io/pages/200913-embedded-website-bash.html"
}

function NodeHelp() {
    echo "The build process needs Node.js and some npm modules."
    echo "Check Node.js's version by running 'node --version && npm --version && npx --version'"
    echo "To install Node.js in Ubuntu 20.04 I have used:"
    echo
    echo "curl -sL https://deb.nodesource.com/setup_14.x | sudo -E bash -"
    echo "sudo apt-get install -y nodejs"
    echo
    echo "More information about installing Node.js can be found on these links:"
    echo "    - https://docs.npmjs.com/downloading-and-installing-node-js-and-npm"
    echo "    - https://github.com/nodesource/distributions/blob/master/README.md"
    echo
    echo "The npm modules can be installed by running 'npm install' in this directory."
    echo "The used modules are:"
    echo "    - clean-css and clean-css-cli"
    echo "    - html-minifier"
    echo "    - inline-source and inline-source-cli"
    echo "    - jshint"
    echo "    - terser"
    echo
    echo "To update 'npm' run 'sudo npm install -g npm'"
    echo "To update the modules run 'npm update' in this directory."
}

function BuildJS () {
  find src -maxdepth 1 -type f -name '*.js' ! -name main.js -exec cat {} + > ./${tmpDir}/script.js
  cat src/main.js >> ./${tmpDir}/script.js

  ./node_modules/.bin/jshint ./${tmpDir}/script.js
}

function MinimizeJS () {
  mv ./${tmpDir}/script.js ./${tmpDir}/script_src.js
  ./node_modules/.bin/terser ./${tmpDir}/script_src.js -o ./${tmpDir}/script.js -c -m
}

function BuildCSS () {
  find src -maxdepth 1 -type f -name '*.css' ! -name main.css -exec cat {} + > ./${tmpDir}/style.css
  cat src/main.css >> ./${tmpDir}/style.css
}

function MinimizeCSS () {
  mv ./${tmpDir}/style.css ./${tmpDir}/style_src.css
  ./node_modules/.bin/cleancss -o ./${tmpDir}/style.css ./${tmpDir}/style_src.css
}

function CopyImages () {
  find src -maxdepth 1 -type f \( -name '*.ico' -o -name '*.png' -o -name '*.jpg' \) -exec cp {} ./${webDir}/ \;
}

function BuildHTML () {
  ./node_modules/.bin/inline-source --root ./${tmpDir} ./src/index.html ./${webDir}/index.html
}

function BuildHTML_Prod () {
  ./node_modules/.bin/inline-source --root ./${tmpDir} ./src/index.html ./${tmpDir}/index.html
  ./node_modules/.bin/html-minifier --collapse-whitespace --remove-comments \
        --remove-empty-attributes --remove-optional-tags --remove-redundant-attributes \
        --remove-script-type-attributes --remove-style-link-type-attributes --remove-tag-whitespace \
        --minify-css true --minify-js true \
        ./${tmpDir}/index.html -o ./${webDir}/index.html
  gzip -k ./${webDir}/index.html
}

while getopts ":chkpn" option
do
  case $option in
    c ) clean_mode=1;;
    k ) clean_before=1;;
    h ) show_help=1;;
    p ) production_mode=1;;
    n ) node_help=1;;
    * ) Usage; exit 1;;
  esac
done

if [[ $show_help -eq 1 ]]; then
  Usage
  exit 0
fi

if [[ $node_help -eq 1 ]]; then
  NodeHelp
  exit 0
fi

if [[ $clean_mode -eq 1 ]]; then
  rm -rf ./${tmpDir}
  rm -rf ./${webDir}
  exit 0
fi

if [[ $clean_before -eq 1 ]]; then
  rm -rf ./${tmpDir}
  rm -rf ./${webDir}
fi

mkdir -p {${tmpDir},${webDir}}

if [[ $production_mode -eq 1 ]]; then
  BuildJS
  MinimizeJS
  BuildCSS
  MinimizeCSS
  CopyImages
  BuildHTML_Prod
else
  BuildJS
  BuildCSS
  CopyImages
  BuildHTML
fi

echo "Build directory:"
ls -l ./${tmpDir}
echo "Output directory:"
ls -l ./${webDir}

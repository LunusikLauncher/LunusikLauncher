#!/bin/bash

BLUE='\033[94m'
GREEN='\033[92m'
RED='\033[91m'
RESET='\033[0m'

cd "$(dirname "$0")"
BASE_DIR=$(pwd)

echo "${BLUE}=== Generating files and Creating translations ===${RESET}"
mkdir -p build-linux > /dev/null 2>&1
cd "build-linux"
cmake ../.. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
bash ../../translations/compile_languages.sh > /dev/null 2>&1


echo "${BLUE}=== Project Compilation ===${RESET}"

cd "${BASE_DIR}"
cd "build-linux"
cmake --build . -j$(nproc) > /dev/null 2>&1


if [ ! -f "LunusikLauncher" ]; then
    echo "${RED}[ERROR] Failed compilation${RESET}"
    exit 1
fi


echo "${BLUE}=== Creating a folder with all files ===${RESET}"

rm -rf build > /dev/null 2>&1

if command -v cqtdeployer &> /dev/null; then
    cqtdeployer -bin LunusikLauncher -qmake $(which qmake6) noTranslations -targetDir build > /dev/null 2>&1
else
    echo "${BLUE}[INFO] linuxdeployqt/cqtdeployer not installed${RESET}"
fi

echo "${BLUE}=== Copying licenses to the build ===${RESET}"

cp ../../LICENSE build/bin > /dev/null 2>&1
cp ../../COPYING.md build/bin > /dev/null 2>&1
cp ../../fonts/OFL.txt build/bin > /dev/null 2>&1



echo "${GREEN}=== Successful compilation ===${RESET}"
#!/bin/bash

RED="\e[31m"
GREEN="\e[32m"
DEFAULT="\e[39m"

error () {
    echo -e "${RED}Error: ${DEFAULT}$1"
    exit 1
}

echo "BUILDING PYTHON STUBS"

# check that conda is installed on the system.
if ! which conda &> /dev/null; then
	error "Cannot generate python stubs. Conda has not been installed."
fi

proto_path="`dirname $0`/server"
env_name="tensor-bus"
# check if the gen_path has been given as an argument
if [ $# -gt 0 ] && [ ! -z "${1}" ]; then
	gen_path="${1}"
else
	gen_path="${proto_path}/../generated"
fi

found_env="n"
for environment in `conda env list | awk '{print $1}'`; do
    if [[ "${env_name}" == "${environment}" ]]; then
        found_env="y"
        break
    fi
done

if [[ "${found_env}" == "n" ]]; then
    pushd ${proto_path}
    conda env update -f environment.yml
    popd
fi

mkdir -p ${gen_path}
conda run -n tensor-bus python -m grpc.tools.protoc -I${proto_path} --python_out=${gen_path} --grpc_python_out=${gen_path} shm_server.proto

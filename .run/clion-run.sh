#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/../webproxy-lab"
exec make "$@"

#!/usr/bin/env sh

# SPDX-FileCopyrightText: © 2022 Melg Eight <public.melg8@gmail.com>
#
# SPDX-License-Identifier: MIT

set -e

DEFAULT_WORKSPACE="$(pwd)"
export DEFAULT_WORKSPACE

python -m megalinter.run

echo "All checks are passed."
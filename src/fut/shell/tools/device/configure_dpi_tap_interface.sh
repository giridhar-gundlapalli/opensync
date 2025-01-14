#!/bin/sh

# Copyright (c) 2015, Plume Design Inc. All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of the Plume Design Inc. nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# FUT environment loading
# shellcheck disable=SC1091
source /tmp/fut-base/shell/config/default_shell.sh
[ -e "/tmp/fut-base/fut_set_env.sh" ] && source /tmp/fut-base/fut_set_env.sh
source "${FUT_TOPDIR}/shell/lib/unit_lib.sh"
[ -e "${PLATFORM_OVERRIDE_FILE}" ] && source "${PLATFORM_OVERRIDE_FILE}" || raise "${PLATFORM_OVERRIDE_FILE}" -ofm
[ -e "${MODEL_OVERRIDE_FILE}" ] && source "${MODEL_OVERRIDE_FILE}" || raise "${MODEL_OVERRIDE_FILE}" -ofm

usage()
{
    cat <<usage_string
tools/device/configure_dpi_tap_interface.sh [-h] arguments
Description:
    - Script configures tap interface required for DPI testcases
Arguments:
    -h  show this help message
    \$1 (lan_bridge_if)    : Interface name used for LAN bridge        : (string)(required)
Script usage example:
   ./tools/device/configure_dpi_tap_interface.sh br-wan
usage_string
}
if [ -n "${1}" ] >/dev/null 2>&1; then
    case "${1}" in
    help | \
        --help | \
        -h)
        usage && exit 1
        ;;
    *) ;;

    esac
fi

NARGS=1
[ $# -lt ${NARGS} ] && raise "Requires at least '${NARGS}' input argument(s)" -arg
bridge=${1}
of_port=20001
tap_ifname=${bridge}.devdpi

check_if_port_in_bridge "${tap_ifname}" "${bridge}"
if [ $? = 0 ]; then
    log -deb "tools/device/configure_dpi_tap_interface.sh: Port '${tap_ifname}' exists in bridge '${bridge}', removing..."
    ovs-vsctl del-port "${bridge}" "${tap_ifname}" &&
        log -deb " tools/device/configure_dpi_tap_interface.sh: ovs-vsctl del-port ${bridge} ${tap_ifname} - Success" ||
        raise "FAIL: Could not remove port '${tap_ifname}' from bridge '${bridge}'" -l  "tools/device/configure_dpi_tap_interface.sh" -ds
fi

wait_for_function_response 0 "ovs-vsctl add-port ${bridge} ${tap_ifname} -- set interface ${tap_ifname} 'type=internal' -- set interface ${tap_ifname} 'ofport_request=${of_port}'" &&
    log "tools/device/configure_dpi_tap_interface.sh: Add port '${tap_ifname}' to bridge ${bridge} - Success" ||
    raise "FAIL: Could not add port '${tap_ifname}' to bridge ${bridge}" -l "tools/device/configure_dpi_tap_interface.sh:" -tc

wait_for_function_response 0 "ip link set ${tap_ifname} up" &&
    log "tools/device/configure_dpi_tap_interface.sh: Bring interface '${tap_ifname}' up - Success" ||
    raise "FAIL: Could not bring interface '${tap_ifname}' up" -l "tools/device/configure_dpi_tap_interface.sh:" -tc

wait_for_function_response 0 "gen_no_flood_cmd ${bridge} ${tap_ifname}" &&
    log "tools/device/configure_dpi_tap_interface.sh: Set interface '${tap_ifname}' no-flood - Success" ||
    raise "FAIL: Could not set interface '${tap_ifname}' no-flood" -l "tools/device/configure_dpi_tap_interface.sh:" -tc

exit 0

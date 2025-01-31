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

manager_setup_file="vpnm/vpnm_setup.sh"
usage()
{
cat << usage_string
vpnm/vpnm_tunnel_interface.sh [-h] arguments
Description:
    - Check if a virtual tunnel interface (VTI) can be successfully created by VPNM.
      This also checks the SDK integration / Linux kernel config enablement of VTI interfaces.
      VTI interfaces are well supported in the Linux kernel but embedded SDKs often do
      not enable them by default.
Arguments:
    -h : show this help message

Testcase procedure:
    - On DEVICE: Run: ./${manager_setup_file} (see ${manager_setup_file} -h)
                 Run: ./vpnm/vpnm_tunnel_interface.sh
Script usage example:
    ./vpnm/vpnm_tunnel_interface.sh
usage_string
}
if [ -n "${1}" ]; then
    case "${1}" in
        help | \
        --help | \
        -h)
            usage && exit 1
            ;;
        *)
            ;;
    esac
fi

NARGS=0
[ $# -ne ${NARGS} ] && usage && raise "Requires exactly '${NARGS}' input argument(s)" -l "vpnm/vpnm_tunnel_interface.sh" -arg

# Execute on EXIT signal.
trap '
fut_info_dump_line
check_restore_ovsdb_server
fut_info_dump_line
' EXIT SIGINT SIGTERM

check_ovsdb_table_exist Tunnel_Interface &&
    log "vpnm/vpnm_tunnel_interface.sh: Tunnel_Interface table exists in ovsdb - Success" ||
    raise "FAIL: Tunnel_Interface table does not exist in ovsdb" -l "vpnm/vpnm_tunnel_interface.sh" -s

empty_ovsdb_table VPN_Tunnel
empty_ovsdb_table IPSec_Config
empty_ovsdb_table IPSec_State
empty_ovsdb_table Tunnel_Interface

ip link del Tunnel0 2>/dev/null

log "vpnm/vpnm_tunnel_interface.sh: Inserting a tunnel interface row"
insert_ovsdb_entry Tunnel_Interface \
    -i if_name Tunnel0 \
    -i if_type vti \
    -i enable true \
    -i local_endpoint_addr 192.168.200.10 \
    -i remote_endpoint_addr 8.8.8.8\
    -i key 10 &&
        log "vpnm/vpnm_tunnel_interface.sh: insert_ovsdb_entry - Tunnel_Interface - Success" ||
        raise "FAIL: insert_ovsdb_entry - Tunnel_Interface" -l "vpnm/vpnm_tunnel_interface.sh" -oe

sleep 1

log "vpnm/vpnm_tunnel_interface.sh: Checking if tunnel interface created"
ip link show Tunnel0 &&
    log "vpnm/vpnm_tunnel_interface.sh: VTI tunnel interface created - Success" ||
    raise "FAIL: VTI tunnel interface NOT created" -l "vpnm/vpnm_tunnel_interface.sh" -oe

pass

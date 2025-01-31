/*
Copyright (c) 2015, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <ev.h>
#include <osw_types.h>
#include <osw_ut.h>
#include <const.h>
#include <util.h>
#include <log.h>

struct osw_op_class_matrix {
    int op_class;
    int start_freq_mhz;
    enum osw_channel_width width;
    int ctrl_chan[64];
    int center_chan_idx[32];
};

#define OSW_OP_CLASS_END -1
#define OSW_CHANNEL_END -2
/*
 * FIXME
 * The OSW_CHANNEL_WILDCARD is a temporary hack because I don't know how verify
 * channel for Operating Classes that don't have Channel Set defined in spec.
 */
#define OSW_CHANNEL_WILDCARD -3

/*
 * Combines "Table E-4—Global operating classes" from:
 * - IEEE Std 802.11-2020
 * - IEEE Std 802.11ax-2021
 */
static const struct osw_op_class_matrix g_op_class_matrix[] = {
    /* 2.4 GHz */
    { 81, 2407, OSW_CHANNEL_20MHZ, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, OSW_CHANNEL_END },
                                   { OSW_CHANNEL_END }, },
    { 82, 2414, OSW_CHANNEL_20MHZ, { 14, OSW_CHANNEL_END },
                                   { OSW_CHANNEL_END }, },
    { 83, 2407, OSW_CHANNEL_40MHZ, { 1, 2, 3, 4, 5, 6, 7, 8, 9, OSW_CHANNEL_END },
                                   { OSW_CHANNEL_END }, },
    { 84, 2407, OSW_CHANNEL_40MHZ, { 5, 6, 7, 8, 9, 10, 11, 12, 13, OSW_CHANNEL_END },
                                   { OSW_CHANNEL_END }, },
    /* 5 GHz */
    { 115, 5000, OSW_CHANNEL_20MHZ, { 36, 40, 44, 48, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 116, 5000, OSW_CHANNEL_40MHZ, { 36, 44, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 117, 5000, OSW_CHANNEL_40MHZ, { 40, 48, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 118, 5000, OSW_CHANNEL_20MHZ, { 52, 56, 60, 64, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 119, 5000, OSW_CHANNEL_40MHZ, { 52, 60, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 120, 5000, OSW_CHANNEL_40MHZ, { 56, 64, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 121, 5000, OSW_CHANNEL_20MHZ, { 100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 122, 5000, OSW_CHANNEL_40MHZ, { 100, 108, 116, 124, 132, 140, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 123, 5000, OSW_CHANNEL_40MHZ, { 104, 112, 120, 128, 136, 144, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 124, 5000, OSW_CHANNEL_20MHZ, { 149, 153, 157, 161, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 125, 5000, OSW_CHANNEL_20MHZ, { 149, 153, 157, 161, 165, 169, 173, 177, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 126, 5000, OSW_CHANNEL_40MHZ, { 149, 157, 165, 173, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 127, 5000, OSW_CHANNEL_40MHZ, { 153, 161, 169, 177, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 128, 5000, OSW_CHANNEL_80MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                    { 42, 58, 106, 122, 138, 155, 171, OSW_CHANNEL_END }, },
    { 129, 5000, OSW_CHANNEL_160MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                     { 50, 114, 163, OSW_CHANNEL_END }, },
    { 130, 5000, OSW_CHANNEL_80P80MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                       { 42, 58, 106, 122, 138, 155, 171, OSW_CHANNEL_END }, },
    /* 6 GHz */
    { 131, 5950, OSW_CHANNEL_20MHZ, { 1, 5, 9, 13, 17, 21, 25, 29, 33, 37, 41, 45,
                                      49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93,
                                      97, 101, 105, 109, 113, 117, 121, 125, 129, 133,
                                      137, 141, 145, 149, 153, 157, 161, 165, 169, 173,
                                      177, 181, 185, 189, 193, 197, 201, 205, 209, 213,
                                      217, 221, 225, 229, 233, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    { 132, 5950, OSW_CHANNEL_40MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                    { 3, 11, 19, 27, 35, 43, 51, 59, 67, 75, 83, 91, 99,
                                      107, 115, 123, 131, 139, 147, 155, 163, 171, 179,
                                      187, 195, 203, 211, 219, 227, OSW_CHANNEL_END }, },
    { 133, 5950, OSW_CHANNEL_80MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                    { 7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 83,
                                      199, 215, OSW_CHANNEL_END }, },
    { 134, 5950, OSW_CHANNEL_160MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                     { 15, 47, 79, 111, 143, 175, 207, OSW_CHANNEL_END }, },
    { 135, 5950, OSW_CHANNEL_80P80MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                       { 7, 23, 39, 55, 71, 87, 103, 119, 135, 151, 167, 83,
                                         199, 215, OSW_CHANNEL_END }, },
    { 136, 5925, OSW_CHANNEL_20MHZ, { OSW_CHANNEL_WILDCARD, OSW_CHANNEL_END },
                                    { OSW_CHANNEL_END }, },
    /* End */
    { OSW_OP_CLASS_END, 0, 0, { OSW_CHANNEL_END, }, { OSW_CHANNEL_END } },
};

static void
strip_trailing_whitespace(char *str)
{
    char *p;
    while ((p = strrchr(str, ' ')) != NULL)
        *p = '\0';
}

int
osw_ifname_cmp(const struct osw_ifname *a,
               const struct osw_ifname *b)
{
    assert(a != NULL);
    assert(b != NULL);

    const size_t max_len = sizeof(a->buf);
    WARN_ON(strnlen(a->buf, max_len) == max_len);
    WARN_ON(strnlen(b->buf, max_len) == max_len);
    return strncmp(a->buf, b->buf, max_len);
}

const char *
osw_pmf_to_str(enum osw_pmf pmf)
{
    switch (pmf) {
        case OSW_PMF_DISABLED: return "disabled";
        case OSW_PMF_OPTIONAL: return "optional";
        case OSW_PMF_REQUIRED: return "required";
    }
    return "";
}
const char *
osw_vif_type_to_str(enum osw_vif_type t)
{
    switch (t) {
        case OSW_VIF_UNDEFINED: return "undefined";
        case OSW_VIF_AP: return "ap";
        case OSW_VIF_AP_VLAN: return "ap_vlan";
        case OSW_VIF_STA: return "sta";
    }
    return "";
}

const char *
osw_acl_policy_to_str(enum osw_acl_policy p)
{
    switch (p) {
        case OSW_ACL_NONE: return "none";
        case OSW_ACL_ALLOW_LIST: return "allow";
        case OSW_ACL_DENY_LIST: return "deny";
    }
    return "";
}

const char *
osw_channel_width_to_str(enum osw_channel_width w)
{
    switch (w) {
        case OSW_CHANNEL_20MHZ: return "20";
        case OSW_CHANNEL_40MHZ: return "40";
        case OSW_CHANNEL_80MHZ: return "80";
        case OSW_CHANNEL_160MHZ: return "160";
        case OSW_CHANNEL_80P80MHZ: return "80p80";
    }
    return "";
}

const char *
osw_channel_dfs_state_to_str(enum osw_channel_state_dfs s)
{
    switch (s) {
        case OSW_CHANNEL_NON_DFS: return "non_dfs";
        case OSW_CHANNEL_DFS_CAC_POSSIBLE: return "cac_possible";
        case OSW_CHANNEL_DFS_CAC_IN_PROGRESS: return "cac_in_progress";
        case OSW_CHANNEL_DFS_CAC_COMPLETED: return "cac_completed";
        case OSW_CHANNEL_DFS_NOL: return "nol";
    }
    return "";
}

const char *
osw_radar_to_str(enum osw_radar_detect r)
{
    switch (r) {
        case OSW_RADAR_UNSUPPORTED: return "unsupported";
        case OSW_RADAR_DETECT_ENABLED: return "enabled";
        case OSW_RADAR_DETECT_DISABLED: return "disabled";
    }
    return "";
}

const char *
osw_band_to_str(enum osw_band b)
{
    switch (b) {
        case OSW_BAND_UNDEFINED: return "undefined";
        case OSW_BAND_2GHZ: return "2ghz";
        case OSW_BAND_5GHZ: return "5ghz";
        case OSW_BAND_6GHZ: return "6ghz";
    }
    return "";
}

void
osw_wpa_to_str(char *out, size_t len, const struct osw_wpa *wpa)
{
    out[0] = 0;
    csnprintf(&out, &len, " wpa=");
    if (wpa->wpa) csnprintf(&out, &len, "wpa ");
    if (wpa->rsn) csnprintf(&out, &len, "rsn ");
    if (wpa->pairwise_tkip) csnprintf(&out, &len, "tkip ");
    if (wpa->pairwise_ccmp) csnprintf(&out, &len, "ccmp ");
    if (wpa->akm_psk) csnprintf(&out, &len, "psk ");
    if (wpa->akm_sae) csnprintf(&out, &len, "sae ");
    if (wpa->akm_ft_psk) csnprintf(&out, &len, "ft_psk ");
    if (wpa->akm_ft_sae) csnprintf(&out, &len, "ft_sae ");
    csnprintf(&out, &len, "pmf=%s ", osw_pmf_to_str(wpa->pmf));
    csnprintf(&out, &len, "mdid=%04x ", wpa->ft_mobility_domain);
    csnprintf(&out, &len, "gtk=%d ", wpa->group_rekey_seconds);
    strip_trailing_whitespace(out);
}

void
osw_ap_mode_to_str(char *out, size_t len, const struct osw_ap_mode *mode)
{
    out[0] = 0;
    if (mode->wnm_bss_trans) csnprintf(&out, &len, "btm ");
    if (mode->rrm_neighbor_report) csnprintf(&out, &len, "rrm ");
    if (mode->wmm_enabled) csnprintf(&out, &len, "wmm ");
    if (mode->wmm_uapsd_enabled) csnprintf(&out, &len, "uapsd ");
    if (mode->ht_enabled) csnprintf(&out, &len, "ht ");
    if (mode->ht_required) csnprintf(&out, &len, "htR ");
    if (mode->vht_enabled) csnprintf(&out, &len, "vht ");
    if (mode->vht_required) csnprintf(&out, &len, "vhtR ");
    if (mode->he_enabled) csnprintf(&out, &len, "he ");
    if (mode->he_required) csnprintf(&out, &len, "heR ");
    if (mode->wps) csnprintf(&out, &len, "wps ");
    if (mode->supported_rates) csnprintf(&out, &len, "supp:" OSW_RATES_FMT " ", OSW_RATES_ARG(mode->supported_rates));
    if (mode->basic_rates) csnprintf(&out, &len, "basic:" OSW_RATES_FMT " ", OSW_RATES_ARG(mode->basic_rates));
    if (mode->beacon_rate.type != OSW_BEACON_RATE_UNSPEC) csnprintf(&out, &len, "bcn:"OSW_BEACON_RATE_FMT " ", OSW_BEACON_RATE_ARG(&mode->beacon_rate));
    strip_trailing_whitespace(out);
}

enum osw_band
osw_freq_to_band(const int freq)
{
    const int b2ch1 = 2412;
    const int b2ch13 = 2472;
    const int b2ch14 = 2484;
    const int b5ch36 = 5180;
    const int b5ch177 = 5885;
    const int b6ch1 = 5955;
    const int b6ch2 = 5935;
    const int b6ch233 = 7115;

    if (freq == b2ch14)
        return OSW_BAND_2GHZ;
    if (freq == b6ch2)
        return OSW_BAND_6GHZ;
    if (freq >= b2ch1 && freq <= b2ch13)
        return OSW_BAND_2GHZ;
    if (freq >= b5ch36 && freq <= b5ch177)
        return OSW_BAND_5GHZ;
    if (freq >= b6ch1 && freq <= b6ch233)
        return OSW_BAND_6GHZ;

    return OSW_BAND_UNDEFINED;
}

enum osw_band
osw_channel_to_band(const struct osw_channel *channel)
{
    assert(channel != NULL);
    return osw_freq_to_band(channel->control_freq_mhz);
}

int
osw_freq_to_chan(const int freq)
{
    const int b2ch1 = 2412;
    const int b2ch13 = 2472;
    const int b2ch14 = 2484;
    const int b5ch36 = 5180;
    const int b5ch177 = 5885;
    const int b6ch1 = 5955;
    const int b6ch2 = 5935;
    const int b6ch233 = 7115;

    if (freq == b2ch14)
        return 14;
    if (freq == b6ch2)
        return 2;
    if (freq >= b2ch1 && freq <= b2ch13)
        return (freq - 2407) / 5;
    if (freq >= b5ch36 && freq <= b5ch177)
        return (freq - 5000) / 5;
    if (freq >= b6ch1 && freq <= b6ch233)
        return (freq - 5950) / 5;

    return 0;
}

int
osw_channel_width_to_mhz(const enum osw_channel_width w)
{
    switch (w) {
        case OSW_CHANNEL_20MHZ: return 20;
        case OSW_CHANNEL_40MHZ: return 40;
        case OSW_CHANNEL_80MHZ: return 80;
        case OSW_CHANNEL_160MHZ: return 160;
        case OSW_CHANNEL_80P80MHZ: return 0; /* N/A */
    }
    return 0;
}

bool
osw_channel_width_down(enum osw_channel_width *w)
{
    switch (*w) {
        case OSW_CHANNEL_20MHZ:
            return false;
        case OSW_CHANNEL_40MHZ:
            *w = OSW_CHANNEL_20MHZ;
            return true;
        case OSW_CHANNEL_80MHZ:
            *w = OSW_CHANNEL_40MHZ;
            return true;
        case OSW_CHANNEL_160MHZ:
            *w = OSW_CHANNEL_80MHZ;
            return true;
        case OSW_CHANNEL_80P80MHZ:
            *w = OSW_CHANNEL_80MHZ;
            return true;
    }
    return false;
}

enum osw_channel_width
osw_channel_width_min(const enum osw_channel_width a,
                      const enum osw_channel_width b)
{
    if (a < b) return a;
    else return b;
}

static const int *
osw_2g_chan_list(int chan, int width, int max_chan)
{
    static const int lists[] = {
        20, 1, 0,
        20, 2, 0,
        20, 3, 0,
        20, 4, 0,
        20, 5, 0,
        20, 6, 0,
        20, 7, 0,
        20, 8, 0,
        20, 9, 0,
        20, 10, 0,
        20, 11, 0,
        20, 12, 0,
        20, 13, 0,
        40, 9, 13, 0,
        40, 8, 12, 0,
        40, 7, 11, 0,
        40, 6, 10, 0,
        40, 5, 9, 0,
        40, 4, 8, 0,
        40, 3, 7, 0,
        40, 2, 6, 0,
        40, 1, 5, 0,
        -1,
    };
    const int *start;
    const int *p;

    for (p = lists; *p != -1; p++) {
        if (*p == width) {
            bool out_of_range = false;
            bool found = false;
            for (start = ++p; *p; p++) {
                if (*p == chan) found = true;
                if (*p > max_chan) out_of_range = true;
            }
            if (found == true && out_of_range == false) {
                return start;
            }
        }
    }

    return NULL;
}

const int *
osw_channel_sidebands(enum osw_band band, int chan, int width, int max_2g_chan)
{
    static int empty[] = { 0 };
    switch (band) {
        case OSW_BAND_UNDEFINED: return empty;
        case OSW_BAND_2GHZ: return osw_2g_chan_list(chan, width, max_2g_chan);
        case OSW_BAND_5GHZ: return unii_5g_chan2list(chan, width);
        case OSW_BAND_6GHZ: return unii_6g_chan2list(chan, width);
    }
    return empty;
}

int
osw_channel_ht40_offset(const struct osw_channel *c, int max_2g_chan)
{
    const int mhz = osw_channel_width_to_mhz(c->width);
    const int freq = c->control_freq_mhz;
    const int chan = osw_freq_to_chan(freq);
    const enum osw_band band = osw_freq_to_band(freq);
    const int *chans = osw_channel_sidebands(band, chan, mhz, max_2g_chan);
    if (chans == NULL) return 0;

    int sum = 0;
    int n = 0;
    while ((*chans) != 0) {
        sum += *chans;
        n++;
        chans++;
    }

    if (n == 0) return 0;
    const int center = sum / n;
    if (center < chan) return -1;
    if (center > chan) return  1;
    return 0;
}

int
osw_chan_to_freq(enum osw_band band, int chan)
{
    switch (band) {
        case OSW_BAND_UNDEFINED: return 0;
        case OSW_BAND_2GHZ: return 2407 + (chan * 5);
        case OSW_BAND_5GHZ: return 5000 + (chan * 5);
        case OSW_BAND_6GHZ: return 5950 + (chan * 5);
    }
    return 0;
}

static int
osw_chan_avg(const int *chans)
{
    int sum = 0;
    int n = 0;
    while (chans != NULL && *chans != 0) {
        sum += *chans;
        n++;
        chans++;
    }
    if (n == 0) return 0;
    else return sum / n;
}

void
osw_channel_compute_center_freq(struct osw_channel *c, int max_2g_chan)
{
    const enum osw_band b = osw_freq_to_band(c->control_freq_mhz);
    const int w = osw_channel_width_to_mhz(c->width);
    const int cn = osw_freq_to_chan(c->control_freq_mhz);
    const int *chans = osw_channel_sidebands(b, cn, w, max_2g_chan);
    const int avg = osw_chan_avg(chans);
    c->center_freq0_mhz = osw_chan_to_freq(b, avg);
}

int
osw_hwaddr_cmp(const struct osw_hwaddr *addr_a,
               const struct osw_hwaddr *addr_b)
{
    return memcmp(addr_a->octet, addr_b->octet, sizeof(addr_a->octet));
}

bool
osw_channel_from_channel_num_width(uint8_t channel_num,
                                   enum osw_channel_width width,
                                   struct osw_channel *channel)
{
    assert(channel != NULL);

    const struct osw_op_class_matrix* entry;
    for (entry = g_op_class_matrix; entry->op_class != OSW_OP_CLASS_END; entry++) {
        if (entry->width != width)
            continue;

        const int *ctrl_chan;
        for (ctrl_chan = entry->ctrl_chan; *ctrl_chan != OSW_CHANNEL_END; ctrl_chan++) {
            if (*ctrl_chan == channel_num || *ctrl_chan == OSW_CHANNEL_WILDCARD) {
                memset(channel, 0, sizeof(*channel));
                channel->width = width;
                channel->control_freq_mhz = entry->start_freq_mhz + (channel_num * 5);
                return true;
            }
        }
    }

    return false;
}

bool
osw_channel_from_op_class(uint8_t op_class,
                          uint8_t channel_num,
                          struct osw_channel *channel)
{
    assert(channel != NULL);

    const struct osw_op_class_matrix* entry;
    for (entry = g_op_class_matrix; entry->op_class != OSW_OP_CLASS_END; entry++)
        if (entry->op_class == op_class)
            break;

    if (entry->op_class == OSW_OP_CLASS_END)
        return false;

    const int *ctrl_chan;
    for (ctrl_chan = entry->ctrl_chan; *ctrl_chan != OSW_CHANNEL_END; ctrl_chan++) {
        if (*ctrl_chan == channel_num)
            break;
        if (*ctrl_chan == OSW_CHANNEL_WILDCARD)
            break;
    }

    if (*ctrl_chan == OSW_CHANNEL_END)
        return false;

    memset(channel, 0, sizeof(*channel));
    channel->width = entry->width;
    channel->control_freq_mhz = entry->start_freq_mhz + (channel_num * 5);

    return true;
}

static const int *
osw_op_class_matrx_chans_scan(const int *chans,
                              const int spacing_mhz,
                              const int base_freq_mhz,
                              const int scan_freq_mhz)
{
    for (; *chans != OSW_CHANNEL_END; chans++) {
        const int cur_freq_mhz = base_freq_mhz + ((*chans) * spacing_mhz);
        if (cur_freq_mhz == scan_freq_mhz)
            return chans;
    }
    return NULL;
}

bool
osw_channel_to_op_class(const struct osw_channel *orig_channel,
                        uint8_t *op_class)
{
    assert(orig_channel != NULL);
    assert(op_class != NULL);

    struct osw_channel copy = *orig_channel;
    struct osw_channel *channel = &copy;
    osw_channel_compute_center_freq(channel, 13);

    const struct osw_op_class_matrix* entry;
    for (entry = g_op_class_matrix; entry->op_class != OSW_OP_CLASS_END; entry++) {
        if (channel->width != entry->width)
            continue;

        const int spacing_mhz = 5;
        const int base_freq_mhz = entry->start_freq_mhz;
        const int *chans = entry->ctrl_chan;
        int scan_freq_mhz = channel->control_freq_mhz;

        if (entry->ctrl_chan[0] == OSW_CHANNEL_WILDCARD) {
            chans = entry->center_chan_idx;
            scan_freq_mhz = channel->center_freq0_mhz;
        }

        const int *found = osw_op_class_matrx_chans_scan(chans,
                                                         spacing_mhz,
                                                         base_freq_mhz,
                                                         scan_freq_mhz);
        if (found == NULL)
            continue;

        assert(entry->op_class > 0 && entry->op_class <= UINT8_MAX);
        *op_class = entry->op_class;
        return true;
    }

    return false;
}

bool
osw_op_class_to_20mhz(uint8_t op_class,
                      uint8_t chan_num,
                      uint8_t *op_class_20mhz)
{
    assert(op_class > 0);
    assert(chan_num > 0);
    assert(op_class_20mhz != NULL);

    const struct osw_op_class_matrix* entry;
    for (entry = g_op_class_matrix; entry->op_class != OSW_OP_CLASS_END; entry++) {
        if (entry->op_class != op_class)
            continue;

        if (entry->width == OSW_CHANNEL_20MHZ) {
            const int *ctrl_chan;
            for (ctrl_chan = entry->ctrl_chan; *ctrl_chan != OSW_CHANNEL_END; ctrl_chan++) {
                if (*ctrl_chan == chan_num) {
                    assert(entry->op_class > 0 && entry->op_class <= UINT8_MAX);
                    *op_class_20mhz = entry->op_class;
                    return true;
                }
            }

            return false;
        }

        /*
         * Move backward and look for the closest 20 MHz op_class and check
         * whether it contains given channel.
         */
        const struct osw_op_class_matrix* prev_entry = entry;
        for (prev_entry = entry; prev_entry >= g_op_class_matrix; prev_entry--) {
            if (prev_entry->width != OSW_CHANNEL_20MHZ)
                continue;

            const int *ctrl_chan;
            for (ctrl_chan = prev_entry->ctrl_chan; *ctrl_chan != OSW_CHANNEL_END; ctrl_chan++) {
                if (*ctrl_chan == chan_num) {
                    assert(prev_entry->op_class > 0 && prev_entry->op_class <= UINT8_MAX);
                    *op_class_20mhz = prev_entry->op_class;
                    return true;
                }
            }
        }

        return false;
    }

    return false;
}

enum osw_band
osw_op_class_to_band(uint8_t op_class)
{
    assert(op_class > 0);

    const struct osw_op_class_matrix *entry;
    for (entry = g_op_class_matrix; entry->op_class != OSW_OP_CLASS_END; entry++) {
        if (entry->op_class != op_class)
            continue;

        /* It's fine to fallback to center freq. The
         * osw_freq_to_band() is a smooth function and can
         * infer bands for, eg. channel 42 on 5GHz, even
         * though that's not a valid primary channel.
         */
        const int first_chan = (entry->ctrl_chan[0] != OSW_CHANNEL_WILDCARD)
                             ? (entry->ctrl_chan[0])
                             : (entry->center_chan_idx[0]);
        const int freq = entry->start_freq_mhz + (5 * first_chan);
        return osw_freq_to_band(freq);
    }
    return OSW_BAND_UNDEFINED;
}

bool
osw_freq_is_dfs(int freq_mhz)
{
    static const int dfs_freqs[] = {
        5260, 5280, 5300, 5320, /* 52-64 */
        5500, 5520, 5540, 5560, /* 100-112 */
        5580, 5600, 5620, 5640, /* 116-128 */
        5660, 5680, 5700, 5720, /* 132-144 */
    };
    size_t i;
    for (i = 0; i < ARRAY_SIZE(dfs_freqs); i++) {
        if (dfs_freqs[i] == freq_mhz)
            return true;
    }
    return false;
}

bool
osw_channel_overlaps_dfs(const struct osw_channel *c)
{
    /* max 2GHz channel doesn't matter for this check so any
     * value, including 11, is perfectly fine.
     */
    const int max_2g_chan = 11;
    const enum osw_band b = osw_freq_to_band(c->control_freq_mhz);
    const int w = osw_channel_width_to_mhz(c->width);
    const int cn = osw_freq_to_chan(c->control_freq_mhz);
    const int *chans = osw_channel_sidebands(b, cn, w, max_2g_chan);
    while (chans != NULL && *chans != 0) {
        const int freq = osw_chan_to_freq(b, *chans);
        if (osw_freq_is_dfs(freq) == true) return true;
        chans++;
    }
    return false;
}

const char *
osw_reg_dfs_to_str(enum osw_reg_dfs dfs)
{
    switch (dfs) {
        case OSW_REG_DFS_UNDEFINED: return "undefined";
        case OSW_REG_DFS_FCC: return "fcc";
        case OSW_REG_DFS_ETSI: return "etsi";
    }
    return "unreachable";
}

void
osw_hwaddr_list_to_str(char *out,
                       size_t len,
                       const struct osw_hwaddr_list *acl)
{
    size_t i;

    out[0] = 0;
    for (i = 0; i < acl->count; i++) {
        const struct osw_hwaddr *addr = &acl->list[i];
        csnprintf(&out, &len, OSW_HWADDR_FMT ",",
                  OSW_HWADDR_ARG(addr));
    }

    if (acl->count > 0 && out[-1] == ',')
        out[-1] = 0;
}

void
osw_ap_psk_list_to_str(char *out,
                       size_t len,
                       const struct osw_ap_psk_list *psk)
{
    size_t i;

    out[0] = 0;
    for (i = 0; i < psk->count; i++) {
        const struct osw_ap_psk *p = &psk->list[i];
        const size_t max = ARRAY_SIZE(p->psk.str);
        csnprintf(&out, &len, "%d:len=%d,",
                  p->key_id,
                  strnlen(p->psk.str, max));
    }

    if (psk->count > 0 && out[-1] == ',')
        out[-1] = 0;
}

void
osw_neigh_list_to_str(char *out,
                      size_t len,
                      const struct osw_neigh_list *neigh)
{
    size_t i;

    out[0] = 0;
    for (i = 0; i < neigh->count; i++) {
        const struct osw_neigh *p = &neigh->list[i];
        csnprintf(&out, &len,
                  " "OSW_HWADDR_FMT"/%08x/%u/%u/%u,",
                  OSW_HWADDR_ARG(&p->bssid),
                  p->bssid_info,
                  p->op_class,
                  p->channel,
                  p->phy_type);
    }

    if (neigh->count > 0 && out[-1] == ',')
        out[-1] = 0;
}

bool
osw_ap_psk_is_same(const struct osw_ap_psk *a,
                   const struct osw_ap_psk *b)
{
    const size_t max = ARRAY_SIZE(a->psk.str);

    if (a->key_id != b->key_id) return false;
    if (strncmp(a->psk.str, b->psk.str, max) != 0) return false;

    return true;
}

int
osw_cs_get_max_2g_chan(const struct osw_channel_state *channel_states,
                       size_t n_channel_states)
{
    int max = 0;
    size_t i;
    for (i = 0; channel_states != NULL && i < n_channel_states; i++) {
        const struct osw_channel_state *cs = &channel_states[i];
        const struct osw_channel *c = &cs->channel;
        const int freq = c->control_freq_mhz;
        const enum osw_band band = osw_freq_to_band(freq);
        if (band != OSW_BAND_2GHZ) continue;
        const int chan = osw_freq_to_chan(freq);
        if (max < chan) {
            max = chan;
        }
    }
    return max;
}

bool
osw_cs_chan_intersects_state(const struct osw_channel_state *channel_states,
                             size_t n_channel_states,
                             const struct osw_channel *c,
                             enum osw_channel_state_dfs state)
{
    const int max_2g_chan = osw_cs_get_max_2g_chan(channel_states, n_channel_states);
    const int freq = c->control_freq_mhz;
    const int chan = osw_freq_to_chan(freq);
    const int width = osw_channel_width_to_mhz(c->width);
    const enum osw_band band = osw_freq_to_band(freq);
    const int *chans = osw_channel_sidebands(band, chan, width, max_2g_chan);

    if (chans == NULL) {
        return false;
    }

    while (*chans != 0) {
        size_t i;
        for (i = 0; i < n_channel_states; i++) {
            const struct osw_channel_state *cs = &channel_states[i];
            const struct osw_channel *oc = &cs->channel;
            const int oc_freq = oc->control_freq_mhz;
            const int oc_chan = osw_freq_to_chan(oc_freq);
            if (oc_chan != *chans) continue;
            if (cs->dfs_state == state) return true;
        }
        chans++;
    }

    return false;
}

bool
osw_cs_chan_is_valid(const struct osw_channel_state *channel_states,
                     size_t n_channel_states,
                     const struct osw_channel *c)
{
    const int max_2g_chan = osw_cs_get_max_2g_chan(channel_states, n_channel_states);
    const int freq = c->control_freq_mhz;
    const int chan = osw_freq_to_chan(freq);
    const int width = osw_channel_width_to_mhz(c->width);
    const enum osw_band band = osw_freq_to_band(freq);
    const int *chans = osw_channel_sidebands(band, chan, width, max_2g_chan);

    if (chans == NULL) {
        return false;
    }

    while (*chans != 0) {
        size_t i;
        for (i = 0; i < n_channel_states; i++) {
            const struct osw_channel_state *cs = &channel_states[i];
            const struct osw_channel *oc = &cs->channel;
            const int oc_freq = oc->control_freq_mhz;
            const int oc_chan = osw_freq_to_chan(oc_freq);
            if (oc_chan == *chans) break;
        }
        const bool not_found = (i == n_channel_states);
        if (not_found) return false;
        chans++;
    }

    return true;
}

bool
osw_cs_chan_is_usable(const struct osw_channel_state *cs,
                      size_t n_cs,
                      const struct osw_channel *c)
{
    const enum osw_channel_state_dfs state = OSW_CHANNEL_DFS_NOL;
    const bool valid = osw_cs_chan_is_valid(cs, n_cs, c);
    const bool contains_nol = osw_cs_chan_intersects_state(cs, n_cs, c, state);
    return valid && !contains_nol;
}

int
osw_rate_legacy_to_halfmbps(enum osw_rate_legacy rate)
{
    switch (rate) {
        case OSW_RATE_CCK_1_MBPS: return 2;
        case OSW_RATE_CCK_2_MBPS: return 4;
        case OSW_RATE_CCK_5_5_MBPS: return 11;
        case OSW_RATE_CCK_11_MBPS: return 22;
        case OSW_RATE_OFDM_6_MBPS: return 12;
        case OSW_RATE_OFDM_9_MBPS: return 18;
        case OSW_RATE_OFDM_12_MBPS: return 24;
        case OSW_RATE_OFDM_18_MBPS: return 36;
        case OSW_RATE_OFDM_24_MBPS: return 48;
        case OSW_RATE_OFDM_36_MBPS: return 72;
        case OSW_RATE_OFDM_48_MBPS: return 96;
        case OSW_RATE_OFDM_54_MBPS: return 108;
        case OSW_RATE_UNSPEC: return 0;
    }
    return 0;
}

enum osw_rate_legacy
osw_rate_legacy_from_halfmbps(int halfmbps)
{
    if (halfmbps == 2) return OSW_RATE_CCK_1_MBPS;
    if (halfmbps == 4) return OSW_RATE_CCK_2_MBPS;
    if (halfmbps == 11) return OSW_RATE_CCK_5_5_MBPS;
    if (halfmbps == 22) return OSW_RATE_CCK_11_MBPS;
    if (halfmbps == 12) return OSW_RATE_OFDM_6_MBPS;
    if (halfmbps == 18) return OSW_RATE_OFDM_9_MBPS;
    if (halfmbps == 24) return OSW_RATE_OFDM_12_MBPS;
    if (halfmbps == 36) return OSW_RATE_OFDM_18_MBPS;
    if (halfmbps == 48) return OSW_RATE_OFDM_24_MBPS;
    if (halfmbps == 72) return OSW_RATE_OFDM_36_MBPS;
    if (halfmbps == 96) return OSW_RATE_OFDM_48_MBPS;
    if (halfmbps == 108) return OSW_RATE_OFDM_54_MBPS;

    return OSW_RATE_UNSPEC;
}

const char *
osw_beacon_rate_type_to_str(enum osw_beacon_rate_type type)
{
    switch (type) {
        case OSW_BEACON_RATE_UNSPEC: return "unspec";
        case OSW_BEACON_RATE_ABG: return "abg";
        case OSW_BEACON_RATE_HT: return "ht";
        case OSW_BEACON_RATE_VHT: return "vhT";
        case OSW_BEACON_RATE_HE: return "he";
    }
    return "";
}

const struct osw_beacon_rate *
osw_beacon_rate_cck(void)
{
    static const struct osw_beacon_rate rate = {
        .type = OSW_BEACON_RATE_ABG,
        .u = {
            .legacy = OSW_RATE_CCK_1_MBPS,
        },
    };
    return &rate;
}

const struct osw_beacon_rate *
osw_beacon_rate_ofdm(void)
{
    static const struct osw_beacon_rate rate = {
        .type = OSW_BEACON_RATE_ABG,
        .u = {
            .legacy = OSW_RATE_OFDM_6_MBPS,
        },
    };
    return &rate;
}

uint32_t
osw_suite_from_dash_str(const char *str)
{
    uint8_t b[4] = {0};
    /* eg. "00-50-f2-5" -> b[] = {0x00, 0x50, 0xf2, 5} */
    sscanf(str, "%02"SCNx8"-%02"SCNx8"-%02"SCNx8"-%"SCNu8,
           &b[0], &b[1], &b[2], &b[3]);
    return ((uint32_t)b[0] << 24)
         | ((uint32_t)b[1] << 16)
         | ((uint32_t)b[2] <<  8)
         | ((uint32_t)b[3] <<  0);
}

void
osw_suite_into_dash_str(char *buf, size_t buf_size, uint32_t suite)
{
    const uint8_t b[] = {
        ((suite >> 24) & 0xff),
        ((suite >> 16) & 0xff),
        ((suite >>  8) & 0xff),
        ((suite >>  0) & 0xff),
    };
    snprintf(buf, buf_size,
             "%02"PRIx8"-%02"SCNx8"-%02"SCNx8"-%"SCNu8,
             b[0], b[1], b[2], b[3]);
}

enum osw_akm
osw_suite_into_akm(const uint32_t s)
{
    switch (s) {
        case OSW_SUITE_AKM_RSN_EAP: return OSW_AKM_RSN_EAP;
        case OSW_SUITE_AKM_RSN_PSK: return OSW_AKM_RSN_PSK;
        case OSW_SUITE_AKM_RSN_FT_EAP: return OSW_AKM_RSN_FT_EAP;
        case OSW_SUITE_AKM_RSN_FT_PSK: return OSW_AKM_RSN_FT_PSK;
        case OSW_SUITE_AKM_RSN_EAP_SHA256: return OSW_AKM_RSN_EAP_SHA256;
        case OSW_SUITE_AKM_RSN_PSK_SHA256: return OSW_AKM_RSN_PSK_SHA256;
        case OSW_SUITE_AKM_RSN_SAE: return OSW_AKM_RSN_SAE;
        case OSW_SUITE_AKM_RSN_FT_SAE: return OSW_AKM_RSN_FT_SAE;
        case OSW_SUITE_AKM_RSN_EAP_SUITE_B_192: return OSW_AKM_RSN_EAP_SUITE_B_192;
        case OSW_SUITE_AKM_RSN_FT_EAP_SHA384: return OSW_AKM_RSN_FT_EAP_SHA384;
        case OSW_SUITE_AKM_RSN_FT_PSK_SHA384: return OSW_AKM_RSN_FT_PSK_SHA384;
        case OSW_SUITE_AKM_RSN_PSK_SHA384: return OSW_AKM_RSN_PSK_SHA384;
        case OSW_SUITE_AKM_WPA_NONE: return OSW_AKM_WPA_NONE;
        case OSW_SUITE_AKM_WPA_8021X: return OSW_AKM_WPA_8021X;
        case OSW_SUITE_AKM_WPA_PSK: return OSW_AKM_WPA_PSK;
        case OSW_SUITE_AKM_WFA_DPP: return OSW_AKM_WFA_DPP;
    }

    return OSW_AKM_UNSPEC;
}

uint32_t
osw_suite_from_akm(const enum osw_akm akm)
{
    switch (akm) {
        case OSW_AKM_UNSPEC: return 0;
        case OSW_AKM_RSN_EAP: return OSW_SUITE_AKM_RSN_EAP;
        case OSW_AKM_RSN_PSK: return OSW_SUITE_AKM_RSN_PSK;
        case OSW_AKM_RSN_FT_EAP: return OSW_SUITE_AKM_RSN_FT_EAP;
        case OSW_AKM_RSN_FT_PSK: return OSW_SUITE_AKM_RSN_FT_PSK;
        case OSW_AKM_RSN_EAP_SHA256: return OSW_SUITE_AKM_RSN_EAP_SHA256;
        case OSW_AKM_RSN_PSK_SHA256: return OSW_SUITE_AKM_RSN_PSK_SHA256;
        case OSW_AKM_RSN_SAE: return OSW_SUITE_AKM_RSN_SAE;
        case OSW_AKM_RSN_FT_SAE: return OSW_SUITE_AKM_RSN_FT_SAE;
        case OSW_AKM_RSN_EAP_SUITE_B_192: return OSW_SUITE_AKM_RSN_EAP_SUITE_B_192;
        case OSW_AKM_RSN_FT_EAP_SHA384: return OSW_SUITE_AKM_RSN_FT_EAP_SHA384;
        case OSW_AKM_RSN_FT_PSK_SHA384: return OSW_SUITE_AKM_RSN_FT_PSK_SHA384;
        case OSW_AKM_RSN_PSK_SHA384: return OSW_SUITE_AKM_RSN_PSK_SHA384;
        case OSW_AKM_WPA_NONE: return OSW_SUITE_AKM_WPA_NONE;
        case OSW_AKM_WPA_8021X: return OSW_SUITE_AKM_WPA_8021X;
        case OSW_AKM_WPA_PSK: return OSW_SUITE_AKM_WPA_PSK;
        case OSW_AKM_WFA_DPP: return OSW_SUITE_AKM_WFA_DPP;
    }
    return 0;
}

enum osw_cipher
osw_suite_into_cipher(const uint32_t s)
{
    switch (s) {
        case OSW_SUITE_CIPHER_RSN_NONE: return OSW_CIPHER_RSN_NONE;
        case OSW_SUITE_CIPHER_RSN_WEP_40: return OSW_CIPHER_RSN_WEP_40;
        case OSW_SUITE_CIPHER_RSN_TKIP: return OSW_CIPHER_RSN_TKIP;
        case OSW_SUITE_CIPHER_RSN_CCMP_128: return OSW_CIPHER_RSN_CCMP_128;
        case OSW_SUITE_CIPHER_RSN_WEP_104: return OSW_CIPHER_RSN_WEP_104;
        case OSW_SUITE_CIPHER_RSN_BIP_CMAC_128: return OSW_CIPHER_RSN_BIP_CMAC_128;
        case OSW_SUITE_CIPHER_RSN_GCMP_128: return OSW_CIPHER_RSN_GCMP_128;
        case OSW_SUITE_CIPHER_RSN_GCMP_256: return OSW_CIPHER_RSN_GCMP_256;
        case OSW_SUITE_CIPHER_RSN_CCMP_256: return OSW_CIPHER_RSN_CCMP_256;
        case OSW_SUITE_CIPHER_RSN_BIP_GMAC_128: return OSW_CIPHER_RSN_BIP_GMAC_128;
        case OSW_SUITE_CIPHER_RSN_BIP_GMAC_256: return OSW_CIPHER_RSN_BIP_GMAC_256;
        case OSW_SUITE_CIPHER_RSN_BIP_CMAC_256: return OSW_CIPHER_RSN_BIP_CMAC_256;
        case OSW_SUITE_CIPHER_WPA_NONE: return OSW_CIPHER_WPA_NONE;
        case OSW_SUITE_CIPHER_WPA_WEP_40: return OSW_CIPHER_WPA_WEP_40;
        case OSW_SUITE_CIPHER_WPA_TKIP: return OSW_CIPHER_WPA_TKIP;
        case OSW_SUITE_CIPHER_WPA_CCMP: return OSW_CIPHER_WPA_CCMP;
        case OSW_SUITE_CIPHER_WPA_WEP_104: return OSW_CIPHER_WPA_WEP_104;
    }
    return OSW_CIPHER_UNSPEC;
}

uint32_t
osw_suite_from_cipher(const enum osw_cipher cipher)
{
    switch (cipher) {
        case OSW_CIPHER_UNSPEC: return 0;
        case OSW_CIPHER_RSN_NONE: return OSW_SUITE_CIPHER_RSN_NONE;
        case OSW_CIPHER_RSN_WEP_40: return OSW_SUITE_CIPHER_RSN_WEP_40;
        case OSW_CIPHER_RSN_TKIP: return OSW_SUITE_CIPHER_RSN_TKIP;
        case OSW_CIPHER_RSN_CCMP_128: return OSW_SUITE_CIPHER_RSN_CCMP_128;
        case OSW_CIPHER_RSN_WEP_104: return OSW_SUITE_CIPHER_RSN_WEP_104;
        case OSW_CIPHER_RSN_BIP_CMAC_128: return OSW_SUITE_CIPHER_RSN_BIP_CMAC_128;
        case OSW_CIPHER_RSN_GCMP_128: return OSW_SUITE_CIPHER_RSN_GCMP_128;
        case OSW_CIPHER_RSN_GCMP_256: return OSW_SUITE_CIPHER_RSN_GCMP_256;
        case OSW_CIPHER_RSN_CCMP_256: return OSW_SUITE_CIPHER_RSN_CCMP_256;
        case OSW_CIPHER_RSN_BIP_GMAC_128: return OSW_SUITE_CIPHER_RSN_BIP_GMAC_128;
        case OSW_CIPHER_RSN_BIP_GMAC_256: return OSW_SUITE_CIPHER_RSN_BIP_GMAC_256;
        case OSW_CIPHER_RSN_BIP_CMAC_256: return OSW_SUITE_CIPHER_RSN_BIP_CMAC_256;
        case OSW_CIPHER_WPA_NONE: return OSW_SUITE_CIPHER_WPA_NONE;
        case OSW_CIPHER_WPA_WEP_40: return OSW_SUITE_CIPHER_WPA_WEP_40;
        case OSW_CIPHER_WPA_TKIP: return OSW_SUITE_CIPHER_WPA_TKIP;
        case OSW_CIPHER_WPA_CCMP: return OSW_SUITE_CIPHER_WPA_CCMP;
        case OSW_CIPHER_WPA_WEP_104: return OSW_SUITE_CIPHER_WPA_WEP_104;
    }
    return 0;
}

const char *
osw_akm_into_cstr(const enum osw_akm akm)
{
    switch (akm) {
        case OSW_AKM_UNSPEC: return "unspec";
        case OSW_AKM_RSN_EAP: return "rsn-eap";
        case OSW_AKM_RSN_PSK: return "rsn-psk";
        case OSW_AKM_RSN_FT_EAP: return "rsn-ft-eap";
        case OSW_AKM_RSN_FT_PSK: return "rsn-ft-psk";
        case OSW_AKM_RSN_EAP_SHA256: return "rsn-eap-sha256";
        case OSW_AKM_RSN_PSK_SHA256: return "rsn-psk-sha256";
        case OSW_AKM_RSN_SAE: return "rsn-sae";
        case OSW_AKM_RSN_FT_SAE: return "rsn-ft-sae";
        case OSW_AKM_RSN_EAP_SUITE_B_192: return "rsn-eap-suite-b-192";
        case OSW_AKM_RSN_FT_EAP_SHA384: return "rsn-ft-eap-sha384";
        case OSW_AKM_RSN_FT_PSK_SHA384: return "rsn-ft-psk-sha384";
        case OSW_AKM_RSN_PSK_SHA384: return "rsn-psk-sha384";
        case OSW_AKM_WPA_NONE: return "wpa-none";
        case OSW_AKM_WPA_8021X: return "wpa-8021x";
        case OSW_AKM_WPA_PSK: return "wpa-psk";
        case OSW_AKM_WFA_DPP: return "wfa-dpp";
    }
    return NULL;
}

const char *
osw_cipher_into_cstr(const enum osw_cipher cipher)
{
    switch (cipher) {
        case OSW_CIPHER_UNSPEC: return "unspec";
        case OSW_CIPHER_RSN_NONE: return "rsn-none";
        case OSW_CIPHER_RSN_WEP_40: return "rsn-wep-40";
        case OSW_CIPHER_RSN_TKIP: return "rsn-tkip";
        case OSW_CIPHER_RSN_CCMP_128: return "rsn-ccmp-128";
        case OSW_CIPHER_RSN_WEP_104: return "rsn-wep-104";
        case OSW_CIPHER_RSN_BIP_CMAC_128: return "rsn-bip-cmac-128";
        case OSW_CIPHER_RSN_GCMP_128: return "rsn-gcmp-128";
        case OSW_CIPHER_RSN_GCMP_256: return "rsn-gcmp-256";
        case OSW_CIPHER_RSN_CCMP_256: return "rsn-ccmp-256";
        case OSW_CIPHER_RSN_BIP_GMAC_128: return "rsn-bip-gmac-128";
        case OSW_CIPHER_RSN_BIP_GMAC_256: return "rsn-bip-gmac-256";
        case OSW_CIPHER_RSN_BIP_CMAC_256: return "rsn-bip-cmac-256";
        case OSW_CIPHER_WPA_NONE: return "wpa-none";
        case OSW_CIPHER_WPA_WEP_40: return "wpa-wep-40";
        case OSW_CIPHER_WPA_TKIP: return "wpa-tkip";
        case OSW_CIPHER_WPA_CCMP: return "wpa-ccmp";
        case OSW_CIPHER_WPA_WEP_104: return "wpa-wep-104";
    }
    return NULL;
}

const char *
osw_band_into_cstr(const enum osw_band band)
{
    switch (band) {
        case OSW_BAND_UNDEFINED:
            return "undefined";
        case OSW_BAND_2GHZ:
            return "2.4ghz";
        case OSW_BAND_5GHZ:
            return "5ghz";
        case OSW_BAND_6GHZ:
            return "6ghz";
    }

    return "invalid";
}

#include "osw_types_ut.c"

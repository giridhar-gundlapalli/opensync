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

/* opensync */
#include <log.h>
#include <const.h>
#include <util.h>
#include <os.h>

/* osw */
#include <osw_module.h>
#include <osw_ut.h>
#include <osw_types.h>
#include <osw_conf.h>
#include <osw_state.h>
#include <osw_time.h>
#include <osw_timer.h>

/*
 * DFS Channel Clipping
 *
 * Purpose:
 *
 * Installs an osw_conf_mutator to apply channel
 * transformations based on channel availability
 * on a given PHY.
 *
 *
 * Description:
 *
 * Sometimes the configuraton data model will end
 * up requesting an invalid channel, most often
 * tied to DFS NOL states.
 *
 * Sometimes the PHY will run away to an undesired
 * channel, whereas it could've kept some
 * resemblence to the original config.
 *
 * As such it is always preferred to maintain the
 * primary channel of the original configuration
 * request, even if it means narrowing down the
 * channel bandwidth. This is done in order to
 * maintain service to possible Leaf Repetater
 * nodes which are bound to primary channels.
 *
 * If that can't be done, because primary channel
 * itself became unavailable, then the next best
 * thing is to keep the current channel the system
 * is operating on.
 *
 * If that can't be done for some reason, which is
 * unlikely, but possible in a case where state
 * report comes late, a first usable 20MHz channel
 * is picked.
 *
 * If all fails, PHY is considered unusable,
 * and is disabled, probably until one of the
 * channels comes out of NOL, at which point
 * osw_confsync and osw_conf will try again,
 * allowing this module to mutate the channel to
 * _some_ usable one.
 */

/* private */
struct ow_dfs_chan_clip {
    struct osw_conf_mutator mut;
    struct osw_timer last_resort_postpone;
    struct osw_timer last_resort_backoff;
};

#define LOG_PREFIX(fmt, ...) "ow: dfs_chan_clip: " fmt, ##__VA_ARGS__
#define LOG_PREFIX_PHY(phy_name, fmt, ...) LOG_PREFIX("%s: " fmt, phy_name, ##__VA_ARGS__)
#define LOG_PREFIX_VIF(phy_name, vif_name, fmt, ...) LOG_PREFIX("%s/%s: " fmt, phy_name, vif_name, ##__VA_ARGS__)
#define mut_to_mod(mut) container_of(mut, struct ow_dfs_chan_clip, mut)
#define OW_DFS_CHAN_CLIP_LAST_RESORT_POSTPONE_SEC OSW_TIME_SEC(3)
#define OW_DFS_CHAN_CLIP_LAST_RESORT_BACKOFF_SEC OSW_TIME_SEC(5)

static void
ow_dfs_chan_clip_postpone_end_cb(struct osw_timer *t)
{
    struct ow_dfs_chan_clip *m = container_of(t,
                                              struct ow_dfs_chan_clip,
                                              last_resort_postpone);
    LOGT(LOG_PREFIX("last resort postpone ended"));
    osw_conf_invalidate(&m->mut);
}

static void
ow_dfs_chan_clip_backoff_end_cb(struct osw_timer *t)
{
    struct ow_dfs_chan_clip *m = container_of(t,
                                              struct ow_dfs_chan_clip,
                                              last_resort_backoff);
    LOGT(LOG_PREFIX("last resort backoff ended"));
    osw_conf_invalidate(&m->mut);
}

static bool
ow_dfs_chan_clip_vif_try_narrow(const struct osw_channel_state *channel_states,
                                size_t n_channel_states,
                                struct osw_channel *c,
                                bool *narrowed)
{
    while (osw_cs_chan_is_usable(channel_states, n_channel_states, c) == false) {
        const bool success = osw_channel_width_down(&c->width);
        if (!success) return false;
        *narrowed = true;
    }
    return true;
}

static bool
ow_dfs_chan_clip_vif_inherit_state(const struct osw_channel_state *channel_states,
                                   size_t n_channel_states,
                                   const struct osw_channel *state_c,
                                   struct osw_channel *c,
                                   bool *narrowed)
{
    if (state_c == NULL) return false;
    *c = *state_c;
    return ow_dfs_chan_clip_vif_try_narrow(channel_states, n_channel_states, c, narrowed);
}

static const int *
ow_dfs_chan_clip_get_chanlist(const struct osw_channel *c,
                              const enum osw_channel_width width,
                              enum osw_band *band)
{
    const int freq = c->control_freq_mhz;
    const int chan = osw_freq_to_chan(freq);
    const int width_mhz = osw_channel_width_to_mhz(width);

    *band = osw_freq_to_band(freq);
    switch (*band) {
        case OSW_BAND_UNDEFINED:
            return NULL;
        case OSW_BAND_2GHZ:
            return NULL;
        case OSW_BAND_5GHZ:
            return unii_5g_chan2list(chan, width_mhz);
        case OSW_BAND_6GHZ:
            return unii_6g_chan2list(chan, width_mhz);
    }

    return NULL;
}

static bool
ow_dfs_chan_clip_chanlist_is_usable(const int *chanlist,
                                    const enum osw_band chanlist_band,
                                    const struct osw_channel_state *channel_states,
                                    size_t n_channel_states)
{
    if (chanlist == NULL) return false;

    while (*chanlist) {
        size_t i;
        for (i = 0; i < n_channel_states; i++) {
            const struct osw_channel_state *cs = &channel_states[i];
            const struct osw_channel *c = &cs->channel;
            const int freq = c->control_freq_mhz;
            const int chan = osw_freq_to_chan(freq);
            const enum osw_band band = osw_freq_to_band(freq);

            if (band != chanlist_band) continue;
            if (chan != *chanlist) continue;
            if (cs->dfs_state == OSW_CHANNEL_DFS_NOL) return false;
            break;
        }
        if (i == n_channel_states) return false;
        chanlist++;
    }

    return true;
}

static bool
ow_dfs_chan_clip_vif_any_widest(const struct osw_channel_state *channel_states,
                                size_t n_channel_states,
                                struct osw_channel *c)
{
    do {
        size_t i;
        for (i = 0; i < n_channel_states; i++) {
            const struct osw_channel_state *cs = &channel_states[i];
            enum osw_band band = OSW_BAND_UNDEFINED;
            const int *chanlist = ow_dfs_chan_clip_get_chanlist(&cs->channel, c->width, &band);
            if (ow_dfs_chan_clip_chanlist_is_usable(chanlist, band, channel_states, n_channel_states)) {
                c->control_freq_mhz = cs->channel.control_freq_mhz;
                return true;
            }
        }
    } while (osw_channel_width_down(&c->width));

    return false;
}

static bool
ow_dfs_chan_clip_vif_any(const struct osw_channel_state *channel_states,
                         size_t n_channel_states,
                         struct osw_channel *c)
{
    size_t i;
    /* FIXME: This could be smarter and look for widest
     * possible channel, but this is intended to be
     * last-resort lookup to provide _any_ level of service.
     * Keep it simple for now. This won't be reached too
     * often anyway.
     */
    for (i = 0; i < n_channel_states; i++) {
        const struct osw_channel_state *cs = &channel_states[i];
        if (cs->dfs_state == OSW_CHANNEL_DFS_NOL) continue;
        const struct osw_channel *oc = &cs->channel;
        *c = *oc;
        return true;
    }
    return false;
}

enum ow_dfs_chan_clip_result {
    OW_DFS_CHAN_ORIGINAL,
    OW_DFS_CHAN_NARROWED,
    OW_DFS_CHAN_INHERITED_STATE,
    OW_DFS_CHAN_INHERITED_STATE_AND_NARROWED,
    OW_DFS_CHAN_LAST_RESORT_WIDEST,
    OW_DFS_CHAN_LAST_RESORT,
    OW_DFS_CHAN_DISABLED,
};

static enum ow_dfs_chan_clip_result
ow_dfs_chan_clip_vif_chan(struct ow_dfs_chan_clip *m,
                          const struct osw_channel_state *channel_states,
                          size_t n_channel_states,
                          const struct osw_channel *state_c,
                          struct osw_channel *c,
                          bool *enabled)
{
    bool narrowed = false;
    bool usable = false;
    struct osw_channel copy = *c;

    usable = ow_dfs_chan_clip_vif_try_narrow(channel_states,
                                             n_channel_states,
                                             c,
                                             &narrowed);
    if (usable) {
        return narrowed
             ? OW_DFS_CHAN_NARROWED
             : OW_DFS_CHAN_ORIGINAL;
    }

    *c = copy;
    narrowed = false;
    usable = ow_dfs_chan_clip_vif_inherit_state(channel_states,
                                                n_channel_states,
                                                state_c,
                                                c,
                                                &narrowed);
    if (usable) {
        return narrowed
             ? OW_DFS_CHAN_INHERITED_STATE_AND_NARROWED
             : OW_DFS_CHAN_INHERITED_STATE;
    }

    /* Radar events take time to propagate. CSA takes
     * non-zero time. Radar events typically move to another
     * valid channel and it makes sense to inherit that. To
     * do that properly however, there needs to be a grace
     * period time before last-resort scenario are tried. As
     * the dust settles down, report ORIGINAL channel.
     */

    if (osw_timer_is_armed(&m->last_resort_postpone)) {
        LOGT(LOG_PREFIX("still postponing last resort"));
        return OW_DFS_CHAN_ORIGINAL;
    }

    if (osw_timer_is_armed(&m->last_resort_backoff) == false) {
        LOGT(LOG_PREFIX("starting to postpone last resort"));
        const uint64_t postpone_until = osw_time_mono_clk()
                                      + OW_DFS_CHAN_CLIP_LAST_RESORT_POSTPONE_SEC;
        const uint64_t backoff_until = osw_time_mono_clk()
                                     + OW_DFS_CHAN_CLIP_LAST_RESORT_BACKOFF_SEC;
        osw_timer_arm_at_nsec(&m->last_resort_postpone, postpone_until);
        osw_timer_arm_at_nsec(&m->last_resort_backoff, backoff_until);
        return OW_DFS_CHAN_ORIGINAL;
    }

    LOGT(LOG_PREFIX("last resort postpone bypassed"));

    *c = copy;
    usable = ow_dfs_chan_clip_vif_any_widest(channel_states,
                                             n_channel_states,
                                             c);
    if (usable) {
        return OW_DFS_CHAN_LAST_RESORT_WIDEST;
    }

    *c = copy;
    usable = ow_dfs_chan_clip_vif_any(channel_states,
                                      n_channel_states,
                                      c);
    if (usable) {
        return OW_DFS_CHAN_LAST_RESORT;
    }

    *enabled = false;
    *c = copy;
    return OW_DFS_CHAN_DISABLED;
}

static void
ow_dfs_chan_clip_vif_log(const char *phy_name,
                         const char *vif_name,
                         const struct osw_channel *orig_c,
                         const struct osw_channel *new_c,
                         const enum ow_dfs_chan_clip_result result)
{
    /* FIXME: Some of these messages perhaps should be
     * considered as DEBUG instead of INFO. Depending on how
     * verbose this will end up being in real-world, this
     * can be kept as is, or needs to be updated.
     */
    switch (result) {
        case OW_DFS_CHAN_ORIGINAL:
            LOGD(LOG_PREFIX_VIF(phy_name, vif_name,
                                "original: " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c)));
            break;
        case OW_DFS_CHAN_NARROWED:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "narrowed: "OSW_CHANNEL_FMT " -> " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c),
                                OSW_CHANNEL_ARG(new_c)));
            break;
        case OW_DFS_CHAN_INHERITED_STATE:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "inheriting state: "OSW_CHANNEL_FMT " -> " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c),
                                OSW_CHANNEL_ARG(new_c)));
            break;
        case OW_DFS_CHAN_INHERITED_STATE_AND_NARROWED:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "inheriting state (narrowed): "OSW_CHANNEL_FMT " -> " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c),
                                OSW_CHANNEL_ARG(new_c)));
            break;
        case OW_DFS_CHAN_LAST_RESORT_WIDEST:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "last resort widest "OSW_CHANNEL_FMT " -> " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c),
                                OSW_CHANNEL_ARG(new_c)));
            break;
        case OW_DFS_CHAN_LAST_RESORT:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "last resort "OSW_CHANNEL_FMT " -> " OSW_CHANNEL_FMT,
                                OSW_CHANNEL_ARG(orig_c),
                                OSW_CHANNEL_ARG(new_c)));
            break;
        case OW_DFS_CHAN_DISABLED:
            LOGI(LOG_PREFIX_VIF(phy_name, vif_name,
                                "disabling because no channels available (wanted "OSW_CHANNEL_FMT ")",
                                OSW_CHANNEL_ARG(orig_c)));
            break;
    }
}

static void
ow_dfs_chan_clip_vif(struct ow_dfs_chan_clip *m,
                     struct osw_conf_phy *phy,
                     struct osw_conf_vif *vif)
{
    if (phy->enabled == false) return;
    if (vif->enabled == false) return;
    if (vif->vif_type != OSW_VIF_AP) return;

    const char *phy_name = phy->phy_name;
    const char *vif_name = vif->vif_name;
    const struct osw_state_vif_info *vif_info = osw_state_vif_lookup(phy_name, vif_name);
    const struct osw_drv_vif_state *vif_state = vif_info ? vif_info->drv_state : NULL;
    const struct osw_state_phy_info *phy_info = osw_state_phy_lookup(phy_name);
    const struct osw_drv_phy_state *phy_state = phy_info ? phy_info->drv_state : NULL;
    const struct osw_channel_state *chan_states = phy_state ? phy_state->channel_states : NULL;
    const size_t n_chan_states = phy_state ? phy_state->n_channel_states : 0;
    const struct osw_channel *state_c = (vif_state &&
                                         vif_state->vif_type == OSW_VIF_AP &&
                                         vif_state->enabled == true)
                                      ? &vif_state->u.ap.channel
                                      : NULL;
    struct osw_channel *c = &vif->u.ap.channel;
    bool *enabled = &vif->enabled;
    const struct osw_channel orig_c = *c;
    const enum ow_dfs_chan_clip_result result = ow_dfs_chan_clip_vif_chan(m,
                                                                          chan_states,
                                                                          n_chan_states,
                                                                          state_c,
                                                                          c,
                                                                          enabled);
    ow_dfs_chan_clip_vif_log(phy_name, vif_name, &orig_c, c, result);
}

static bool
ow_dfs_chan_clip_phy_some_channels_available(const char *phy_name)
{
    const struct osw_state_phy_info *phy_info = osw_state_phy_lookup(phy_name);
    const struct osw_drv_phy_state *phy_state = phy_info ? phy_info->drv_state : NULL;
    const struct osw_channel_state *chan_states = phy_state ? phy_state->channel_states : NULL;
    size_t n_chan_states = phy_state ? phy_state->n_channel_states : 0;

    while (n_chan_states--) {
        if (chan_states->dfs_state != OSW_CHANNEL_DFS_NOL) {
            return true;
        }
        chan_states++;
    }
    return false;
}

static void
ow_dfs_chan_clip_phy(struct osw_conf_phy *phy)
{
    const char *phy_name = phy->phy_name;

    if (phy->enabled == false) return;
    if (ow_dfs_chan_clip_phy_some_channels_available(phy_name)) return;

    LOGI(LOG_PREFIX_PHY(phy_name, "no channels available, disabling"));
    phy->enabled = false;
}

static void
ow_dfs_chan_clip_conf_mutate_cb(struct osw_conf_mutator *mut,
                                struct ds_tree *phy_tree)
{
    LOGT(LOG_PREFIX("mutating"));

    struct ow_dfs_chan_clip *m = container_of(mut, struct ow_dfs_chan_clip, mut);
    struct osw_conf_phy *phy;

    ds_tree_foreach(phy_tree, phy) {
        struct ds_tree *vif_tree = &phy->vif_tree;
        struct osw_conf_vif *vif;

        ow_dfs_chan_clip_phy(phy);

        ds_tree_foreach(vif_tree, vif) {
            ow_dfs_chan_clip_vif(m, phy, vif);
        }
    }
}

static void
ow_dfs_chan_clip_init(struct ow_dfs_chan_clip *m)
{
    LOGT(LOG_PREFIX("initializing"));

    const struct osw_conf_mutator mut = {
        .name = __FILE__,
        .type = OSW_CONF_TAIL,
        .mutate_fn = ow_dfs_chan_clip_conf_mutate_cb,
    };

    m->mut = mut;

    osw_timer_init(&m->last_resort_postpone,
                   ow_dfs_chan_clip_postpone_end_cb);
    osw_timer_init(&m->last_resort_backoff,
                   ow_dfs_chan_clip_backoff_end_cb);
}

static void
ow_dfs_chan_clip_start(struct ow_dfs_chan_clip *m)
{
    LOGT(LOG_PREFIX("starting"));
    osw_conf_register_mutator(&m->mut);
}

static enum ow_dfs_chan_clip_result
ow_dfs_chan_clip_vif_test(struct ow_dfs_chan_clip *m,
                          const struct osw_channel_state *channel_states,
                          size_t n_channel_states,
                          const struct osw_channel *state_c,
                          struct osw_channel *c,
                          bool *enabled)
{
    const struct osw_channel orig_c = *c;
    const enum ow_dfs_chan_clip_result res = ow_dfs_chan_clip_vif_chan(m,
                                                                       channel_states,
                                                                       n_channel_states,
                                                                       state_c,
                                                                       c,
                                                                       enabled);
    ow_dfs_chan_clip_vif_log("", "", &orig_c, c, res);
    return res;
}

OSW_UT(nol)
{
    struct ow_dfs_chan_clip m;
    const struct osw_channel ch36 = { .control_freq_mhz = 5180 };
    const struct osw_channel ch40 = { .control_freq_mhz = 5200 };
    const struct osw_channel ch52 = { .control_freq_mhz = 5260 };
    const struct osw_channel ch56 = { .control_freq_mhz = 5280 };
    const struct osw_channel_state cs5gl[] = {
        { .channel = ch36, .dfs_state = OSW_CHANNEL_NON_DFS },
        { .channel = ch40, .dfs_state = OSW_CHANNEL_NON_DFS },
        { .channel = ch52, .dfs_state = OSW_CHANNEL_DFS_CAC_POSSIBLE },
        { .channel = ch56, .dfs_state = OSW_CHANNEL_DFS_NOL },
    };
    const struct osw_channel_state cs5gl_dfs[] = {
        { .channel = ch52, .dfs_state = OSW_CHANNEL_DFS_CAC_POSSIBLE },
        { .channel = ch56, .dfs_state = OSW_CHANNEL_DFS_NOL },
    };
    const struct osw_channel_state cs5gl_nol[] = {
        { .channel = ch52, .dfs_state = OSW_CHANNEL_DFS_NOL },
        { .channel = ch56, .dfs_state = OSW_CHANNEL_DFS_NOL },
    };
    const struct osw_channel ch36ht20 = { .control_freq_mhz = 5180, .width = OSW_CHANNEL_20MHZ };
    const struct osw_channel ch36ht40 = { .control_freq_mhz = 5180, .width = OSW_CHANNEL_40MHZ };
    const struct osw_channel ch52ht20 = { .control_freq_mhz = 5260, .width = OSW_CHANNEL_20MHZ };
    const struct osw_channel ch52ht40 = { .control_freq_mhz = 5260, .width = OSW_CHANNEL_40MHZ };
    const struct osw_channel ch56ht20 = { .control_freq_mhz = 5280, .width = OSW_CHANNEL_20MHZ };
    const struct osw_channel ch56ht40 = { .control_freq_mhz = 5280, .width = OSW_CHANNEL_40MHZ };
    const struct osw_channel ch56ht80 = { .control_freq_mhz = 5280, .width = OSW_CHANNEL_80MHZ };
    struct osw_channel c;
    bool enabled;

    ow_dfs_chan_clip_init(&m);

    c = ch36ht20;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_ORIGINAL);
    assert(enabled == true);

    c = ch36ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_ORIGINAL);
    assert(enabled == true);

    c = ch52ht20;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_ORIGINAL);
    assert(enabled == true);

    c = ch52ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_NARROWED);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch52ht40.control_freq_mhz);
    assert(c.width == OSW_CHANNEL_20MHZ);

    c = ch56ht20;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), &ch36ht40, &c, &enabled)
        == OW_DFS_CHAN_INHERITED_STATE);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch36ht40.control_freq_mhz);
    assert(c.width == ch36ht40.width);

    c = ch56ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), &ch36ht40, &c, &enabled)
        == OW_DFS_CHAN_INHERITED_STATE);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch36ht40.control_freq_mhz);
    assert(c.width == ch36ht40.width);

    osw_timer_arm_at_nsec(&m.last_resort_backoff, 0);

    c = ch56ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl_dfs, ARRAY_SIZE(cs5gl_dfs), NULL, &c, &enabled)
        == OW_DFS_CHAN_LAST_RESORT_WIDEST);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch52ht40.control_freq_mhz);
    assert(c.width == OSW_CHANNEL_20MHZ);

    c = ch56ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl_nol, ARRAY_SIZE(cs5gl_nol), NULL, &c, &enabled)
        == OW_DFS_CHAN_DISABLED);
    assert(enabled == false);

    c = ch56ht40;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_LAST_RESORT_WIDEST);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch36ht40.control_freq_mhz);
    assert(c.width == OSW_CHANNEL_40MHZ);

    c = ch56ht80;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_LAST_RESORT_WIDEST);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch36ht40.control_freq_mhz);
    assert(c.width == OSW_CHANNEL_40MHZ);

    osw_timer_disarm(&m.last_resort_backoff);
    osw_time_set_mono_clk(0);

    c = ch56ht80;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_ORIGINAL);

    assert(osw_timer_is_armed(&m.last_resort_backoff) == true);
    assert(osw_timer_is_armed(&m.last_resort_postpone) == true);

    osw_timer_core_dispatch((OW_DFS_CHAN_CLIP_LAST_RESORT_POSTPONE_SEC +
                             OW_DFS_CHAN_CLIP_LAST_RESORT_BACKOFF_SEC) / 2);

    assert(osw_timer_is_armed(&m.last_resort_backoff) == true);
    assert(osw_timer_is_armed(&m.last_resort_postpone) == false);

    c = ch56ht80;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_LAST_RESORT_WIDEST);
    assert(enabled == true);
    assert(c.control_freq_mhz == ch36ht40.control_freq_mhz);
    assert(c.width == OSW_CHANNEL_40MHZ);

    osw_timer_core_dispatch(OW_DFS_CHAN_CLIP_LAST_RESORT_BACKOFF_SEC * 2);
    assert(osw_timer_is_armed(&m.last_resort_backoff) == false);
    assert(osw_timer_is_armed(&m.last_resort_postpone) == false);

    osw_time_set_mono_clk(0);

    c = ch56ht80;
    enabled = true;
    assert(ow_dfs_chan_clip_vif_test(&m, cs5gl, ARRAY_SIZE(cs5gl), NULL, &c, &enabled)
        == OW_DFS_CHAN_ORIGINAL);
}

OSW_MODULE(ow_dfs_chan_clip)
{
    static struct ow_dfs_chan_clip m;
    ow_dfs_chan_clip_init(&m);
    ow_dfs_chan_clip_start(&m);
    return &m;
}
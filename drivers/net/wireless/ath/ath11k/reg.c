// SPDX-License-Identifier: BSD-3-Clause-Clear
/*
 * Copyright (c) 2018-2019 The Linux Foundation. All rights reserved.
 */
#include <linux/rtnetlink.h>

#include "core.h"
#include "debug.h"

/* World regdom to be used in case default regd from fw is unavailable */
#define ATH11K_2GHZ_CH01_11      REG_RULE(2412 - 10, 2462 + 10, 40, 0, 20, 0)
#define ATH11K_5GHZ_5150_5350    REG_RULE(5150 - 10, 5350 + 10, 80, 0, 30,\
					  NL80211_RRF_NO_IR)
#define ATH11K_5GHZ_5725_5850    REG_RULE(5725 - 10, 5850 + 10, 80, 0, 30,\
					  NL80211_RRF_NO_IR)

#define ETSI_WEATHER_RADAR_BAND_LOW		5590
#define ETSI_WEATHER_RADAR_BAND_HIGH		5650
#define ETSI_WEATHER_RADAR_BAND_CAC_TIMEOUT	600000

static const struct ieee80211_regdomain ath11k_world_regd = {
	.n_reg_rules = 3,
	.alpha2 =  "00",
	.reg_rules = {
		ATH11K_2GHZ_CH01_11,
		ATH11K_5GHZ_5150_5350,
		ATH11K_5GHZ_5725_5850,
	}
};

static bool ath11k_regdom_changes(struct ath11k *ar, char *alpha2)
{
	const struct ieee80211_regdomain *regd;

	regd = rcu_dereference_rtnl(ar->hw->wiphy->regd);
	/* This can happen during wiphy registration where the previous
	 * user request is received before we update the regd received
	 * from firmware.
	 */
	if (!regd)
		return true;

	return memcmp(regd->alpha2, alpha2, 2) != 0;
}

static void
ath11k_reg_notifier(struct wiphy *wiphy, struct regulatory_request *request)
{
	struct ieee80211_hw *hw = wiphy_to_ieee80211_hw(wiphy);
	struct wmi_init_country_params init_country_param;
	struct wmi_set_current_country_params set_current_param = {};
	struct ath11k *ar = hw->priv;
	int ret;

	ath11k_dbg(ar->ab, ATH11K_DBG_REG,
		   "Regulatory Notification received for %s\n", wiphy_name(wiphy));

	/* Currently supporting only General User Hints. Cell base user
	 * hints to be handled later.
	 * Hints from other sources like Core, Beacons are not expected for
	 * self managed wiphy's
	 */
	if (!(request->initiator == NL80211_REGDOM_SET_BY_USER &&
	      request->user_reg_hint_type == NL80211_USER_REG_HINT_USER)) {
		ath11k_warn(ar->ab, "Unexpected Regulatory event for this wiphy\n");
		return;
	}

	if (!IS_ENABLED(CONFIG_ATH_REG_DYNAMIC_USER_REG_HINTS)) {
		ath11k_dbg(ar->ab, ATH11K_DBG_REG,
			   "Country Setting is not allowed\n");
		return;
	}

	if (!ath11k_regdom_changes(ar, request->alpha2)) {
		ath11k_dbg(ar->ab, ATH11K_DBG_REG, "Country is already set\n");
		return;
	}

	/* Set the country code to the firmware and will receive
	 * the WMI_REG_CHAN_LIST_CC EVENT for updating the
	 * reg info
	 */
	if (ar->ab->hw_params.current_cc_support) {
		memcpy(&set_current_param.alpha2, request->alpha2, 2);
		memcpy(&ar->alpha2, &set_current_param.alpha2, 2);
		ret = ath11k_wmi_send_set_current_country_cmd(ar, &set_current_param);
		if (ret)
			ath11k_warn(ar->ab,
				    "failed set current country code: %d\n", ret);
	} else {
		init_country_param.flags = ALPHA_IS_SET;
		memcpy(&init_country_param.cc_info.alpha2, request->alpha2, 2);
		init_country_param.cc_info.alpha2[2] = 0;

		ret = ath11k_wmi_send_init_country_cmd(ar, init_country_param);
		if (ret)
			ath11k_warn(ar->ab,
				    "INIT Country code set to fw failed : %d\n", ret);
	}

	ath11k_mac_11d_scan_stop(ar);
	ar->regdom_set_by_user = true;
}

int ath11k_reg_update_chan_list(struct ath11k *ar, bool wait)
{
	struct ieee80211_supported_band **bands;
	struct scan_chan_list_params *params;
	struct ieee80211_channel *channel;
	struct ieee80211_hw *hw = ar->hw;
	struct channel_param *ch;
	enum nl80211_band band;
	int num_channels = 0;
	int i, ret, left;

	if (wait && ar->state_11d != ATH11K_11D_IDLE) {
		left = wait_for_completion_timeout(&ar->completed_11d_scan,
						   ATH11K_SCAN_TIMEOUT_HZ);
		if (!left) {
			ath11k_dbg(ar->ab, ATH11K_DBG_REG,
				   "failed to receive 11d scan complete: timed out\n");
			ar->state_11d = ATH11K_11D_IDLE;
		}
		ath11k_dbg(ar->ab, ATH11K_DBG_REG,
			   "reg 11d scan wait left time %d\n", left);
	}

	if (wait &&
	    (ar->scan.state == ATH11K_SCAN_STARTING ||
	    ar->scan.state == ATH11K_SCAN_RUNNING)) {
		left = wait_for_completion_timeout(&ar->scan.completed,
						   ATH11K_SCAN_TIMEOUT_HZ);
		if (!left)
			ath11k_dbg(ar->ab, ATH11K_DBG_REG,
				   "failed to receive hw scan complete: timed out\n");

		ath11k_dbg(ar->ab, ATH11K_DBG_REG,
			   "reg hw scan wait left time %d\n", left);
	}

	if (ar->state == ATH11K_STATE_RESTARTING)
		return 0;

	bands = hw->wiphy->bands;
	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		if (!bands[band])
			continue;

		for (i = 0; i < bands[band]->n_channels; i++) {
			if (bands[band]->channels[i].flags &
			    IEEE80211_CHAN_DISABLED)
				continue;

			num_channels++;
		}
	}

	if (WARN_ON(!num_channels))
		return -EINVAL;

	params = kzalloc(struct_size(params, ch_param, num_channels),
			 GFP_KERNEL);
	if (!params)
		return -ENOMEM;

	params->pdev_id = ar->pdev->pdev_id;
	params->nallchans = num_channels;

	ch = params->ch_param;

	for (band = 0; band < NUM_NL80211_BANDS; band++) {
		if (!bands[band])
			continue;

		for (i = 0; i < bands[band]->n_channels; i++) {
			channel = &bands[band]->channels[i];

			if (channel->flags & IEEE80211_CHAN_DISABLED)
				continue;

			/* TODO: Set to true/false based on some condition? */
			ch->allow_ht = true;
			ch->allow_vht = true;
			ch->allow_he = true;

			ch->dfs_set =
				!!(channel->flags & IEEE80211_CHAN_RADAR);
			ch->is_chan_passive = !!(channel->flags &
						IEEE80211_CHAN_NO_IR);
			ch->is_chan_passive |= ch->dfs_set;
			ch->mhz = channel->center_freq;
			ch->cfreq1 = channel->center_freq;
			ch->minpower = 0;
			ch->maxpower = channel->max_power * 2;
			ch->maxregpower = channel->max_reg_power * 2;
			ch->antennamax = channel->max_antenna_gain * 2;

			/* TODO: Use appropriate phymodes */
			if (channel->band == NL80211_BAND_2GHZ)
				ch->phy_mode = MODE_11G;
			else
				ch->phy_mode = MODE_11A;

			if (channel->band == NL80211_BAND_6GHZ &&
			    cfg80211_channel_is_psc(channel))
				ch->psc_channel = true;

			ath11k_dbg(ar->ab, ATH11K_DBG_WMI,
				   "mac channel [%d/%d] freq %d maxpower %d regpower %d antenna %d mode %d\n",
				   i, params->nallchans,
				   ch->mhz, ch->maxpower, ch->maxregpower,
				   ch->antennamax, ch->phy_mode);

			ch++;
			/* TODO: use quarrter/half rate, cfreq12, dfs_cfreq2
			 * set_agile, reg_class_idx
			 */
		}
	}

	ret = ath11k_wmi_send_scan_chan_list_cmd(ar, params);
	kfree(params);

	return ret;
}

static void ath11k_copy_regd(struct ieee80211_regdomain *regd_orig,
			     struct ieee80211_regdomain *regd_copy)
{
	u8 i;

	/* The caller should have checked error conditions */
	memcpy(regd_copy, regd_orig, sizeof(*regd_orig));

	for (i = 0; i < regd_orig->n_reg_rules; i++)
		memcpy(&regd_copy->reg_rules[i], &regd_orig->reg_rules[i],
		       sizeof(struct ieee80211_reg_rule));
}

int ath11k_regd_update(struct ath11k *ar)
{
	struct ieee80211_regdomain *regd, *regd_copy = NULL;
	int ret, regd_len, pdev_id;
	struct ath11k_base *ab;

	ab = ar->ab;
	pdev_id = ar->pdev_idx;

	spin_lock_bh(&ab->base_lock);

	/* Prefer the latest regd update over default if it's available */
	if (ab->new_regd[pdev_id]) {
		regd = ab->new_regd[pdev_id];
	} else {
		/* Apply the regd received during init through
		 * WMI_REG_CHAN_LIST_CC event. In case of failure to
		 * receive the regd, initialize with a default world
		 * regulatory.
		 */
		if (ab->default_regd[pdev_id]) {
			regd = ab->default_regd[pdev_id];
		} else {
			ath11k_warn(ab,
				    "failed to receive default regd during init\n");
			regd = (struct ieee80211_regdomain *)&ath11k_world_regd;
		}
	}

	if (!regd) {
		ret = -EINVAL;
		spin_unlock_bh(&ab->base_lock);
		goto err;
	}

	regd_len = sizeof(*regd) + (regd->n_reg_rules *
		sizeof(struct ieee80211_reg_rule));

	regd_copy = kzalloc(regd_len, GFP_ATOMIC);
	if (regd_copy)
		ath11k_copy_regd(regd, regd_copy);

	spin_unlock_bh(&ab->base_lock);

	if (!regd_copy) {
		ret = -ENOMEM;
		goto err;
	}

	ret = regulatory_set_wiphy_regd(ar->hw->wiphy, regd_copy);

	kfree(regd_copy);

	if (ret)
		goto err;

	if (ar->state == ATH11K_STATE_ON) {
		ret = ath11k_reg_update_chan_list(ar, true);
		if (ret)
			goto err;
	}

	return 0;
err:
	ath11k_warn(ab, "failed to perform regd update : %d\n", ret);
	return ret;
}

static enum nl80211_dfs_regions
ath11k_map_fw_dfs_region(enum ath11k_dfs_region dfs_region)
{
	switch (dfs_region) {
	case ATH11K_DFS_REG_FCC:
	case ATH11K_DFS_REG_CN:
		return NL80211_DFS_FCC;
	case ATH11K_DFS_REG_ETSI:
	case ATH11K_DFS_REG_KR:
		return NL80211_DFS_ETSI;
	case ATH11K_DFS_REG_MKK:
	case ATH11K_DFS_REG_MKK_N:
		return NL80211_DFS_JP;
	default:
		return NL80211_DFS_UNSET;
	}
}

static u32 ath11k_map_fw_reg_flags(u16 reg_flags)
{
	u32 flags = 0;

	if (reg_flags & REGULATORY_CHAN_NO_IR)
		flags = NL80211_RRF_NO_IR;

	if (reg_flags & REGULATORY_CHAN_RADAR)
		flags |= NL80211_RRF_DFS;

	if (reg_flags & REGULATORY_CHAN_NO_OFDM)
		flags |= NL80211_RRF_NO_OFDM;

	if (reg_flags & REGULATORY_CHAN_INDOOR_ONLY)
		flags |= NL80211_RRF_NO_OUTDOOR;

	if (reg_flags & REGULATORY_CHAN_NO_HT40)
		flags |= NL80211_RRF_NO_HT40;

	if (reg_flags & REGULATORY_CHAN_NO_80MHZ)
		flags |= NL80211_RRF_NO_80MHZ;

	if (reg_flags & REGULATORY_CHAN_NO_160MHZ)
		flags |= NL80211_RRF_NO_160MHZ;

	return flags;
}

static const char *
ath11k_reg_get_regdom_str(enum nl80211_dfs_regions dfs_region)
{
	switch (dfs_region) {
	case NL80211_DFS_FCC:
		return "FCC";
	case NL80211_DFS_ETSI:
		return "ETSI";
	case NL80211_DFS_JP:
		return "JP";
	default:
		return "UNSET";
	}
}

static u16
ath11k_reg_adjust_bw(u16 start_freq, u16 end_freq, u16 max_bw)
{
	u16 bw;

	if (end_freq <= start_freq)
		return 0;

	bw = end_freq - start_freq;
	bw = min_t(u16, bw, max_bw);

	if (bw >= 80 && bw < 160)
		bw = 80;
	else if (bw >= 40 && bw < 80)
		bw = 40;
	else if (bw >= 20 && bw < 40)
		bw = 20;
	else
		bw = 0;

	return bw;
}

static void
ath11k_reg_update_rule(struct ieee80211_reg_rule *reg_rule, u32 start_freq,
		       u32 end_freq, u32 bw, u32 ant_gain, u32 reg_pwr,
		       u32 reg_flags)
{
	reg_rule->freq_range.start_freq_khz = MHZ_TO_KHZ(start_freq);
	reg_rule->freq_range.end_freq_khz = MHZ_TO_KHZ(end_freq);
	reg_rule->freq_range.max_bandwidth_khz = MHZ_TO_KHZ(bw);
	reg_rule->power_rule.max_antenna_gain = DBI_TO_MBI(ant_gain);
	reg_rule->power_rule.max_eirp = DBM_TO_MBM(reg_pwr);
	reg_rule->flags = reg_flags;
}

static void
ath11k_reg_update_weather_radar_band(struct ath11k_base *ab,
				     struct ieee80211_regdomain *regd,
				     struct cur_reg_rule *reg_rule,
				     u8 *rule_idx, u32 flags, u16 max_bw)
{
	u32 start_freq;
	u32 end_freq;
	u16 bw;
	u8 i;

	i = *rule_idx;

	/* there might be situations when even the input rule must be dropped */
	i--;

	/* frequencies below weather radar */
	bw = ath11k_reg_adjust_bw(reg_rule->start_freq,
				  ETSI_WEATHER_RADAR_BAND_LOW, max_bw);
	if (bw > 0) {
		i++;

		ath11k_reg_update_rule(regd->reg_rules + i,
				       reg_rule->start_freq,
				       ETSI_WEATHER_RADAR_BAND_LOW, bw,
				       reg_rule->ant_gain, reg_rule->reg_power,
				       flags);

		ath11k_dbg(ab, ATH11K_DBG_REG,
			   "\t%d. (%d - %d @ %d) (%d, %d) (%d ms) (FLAGS %d)\n",
			   i + 1, reg_rule->start_freq,
			   ETSI_WEATHER_RADAR_BAND_LOW, bw, reg_rule->ant_gain,
			   reg_rule->reg_power, regd->reg_rules[i].dfs_cac_ms,
			   flags);
	}

	/* weather radar frequencies */
	start_freq = max_t(u32, reg_rule->start_freq,
			   ETSI_WEATHER_RADAR_BAND_LOW);
	end_freq = min_t(u32, reg_rule->end_freq, ETSI_WEATHER_RADAR_BAND_HIGH);

	bw = ath11k_reg_adjust_bw(start_freq, end_freq, max_bw);
	if (bw > 0) {
		i++;

		ath11k_reg_update_rule(regd->reg_rules + i, start_freq,
				       end_freq, bw, reg_rule->ant_gain,
				       reg_rule->reg_power, flags);

		regd->reg_rules[i].dfs_cac_ms = ETSI_WEATHER_RADAR_BAND_CAC_TIMEOUT;

		ath11k_dbg(ab, ATH11K_DBG_REG,
			   "\t%d. (%d - %d @ %d) (%d, %d) (%d ms) (FLAGS %d)\n",
			   i + 1, start_freq, end_freq, bw,
			   reg_rule->ant_gain, reg_rule->reg_power,
			   regd->reg_rules[i].dfs_cac_ms, flags);
	}

	/* frequencies above weather radar */
	bw = ath11k_reg_adjust_bw(ETSI_WEATHER_RADAR_BAND_HIGH,
				  reg_rule->end_freq, max_bw);
	if (bw > 0) {
		i++;

		ath11k_reg_update_rule(regd->reg_rules + i,
				       ETSI_WEATHER_RADAR_BAND_HIGH,
				       reg_rule->end_freq, bw,
				       reg_rule->ant_gain, reg_rule->reg_power,
				       flags);

		ath11k_dbg(ab, ATH11K_DBG_REG,
			   "\t%d. (%d - %d @ %d) (%d, %d) (%d ms) (FLAGS %d)\n",
			   i + 1, ETSI_WEATHER_RADAR_BAND_HIGH,
			   reg_rule->end_freq, bw, reg_rule->ant_gain,
			   reg_rule->reg_power, regd->reg_rules[i].dfs_cac_ms,
			   flags);
	}

	*rule_idx = i;
}

struct ieee80211_regdomain *
ath11k_reg_build_regd(struct ath11k_base *ab,
		      struct cur_regulatory_info *reg_info)
{
	struct ieee80211_regdomain *new_regd = NULL;
	struct cur_reg_rule *reg_rule;
	u8 i = 0, j = 0, k = 0;
	u8 num_rules;
	u16 max_bw;
	u32 flags;
	char alpha2[3];

	num_rules = reg_info->num_5ghz_reg_rules + reg_info->num_2ghz_reg_rules;

	/* FIXME: Currently taking reg rules for 6 GHz only from Indoor AP mode list.
	 * This can be updated after complete 6 GHz regulatory support is added.
	 */
	if (reg_info->is_ext_reg_event)
		num_rules += reg_info->num_6ghz_rules_ap[WMI_REG_INDOOR_AP];

	if (!num_rules)
		return new_regd;

	/* Add max additional rules to accommodate weather radar band */
	if (reg_info->dfs_region == ATH11K_DFS_REG_ETSI)
		num_rules += 2;

	new_regd =  kzalloc(sizeof(*new_regd) +
			(num_rules * sizeof(struct ieee80211_reg_rule)),
			GFP_ATOMIC);
	if (!new_regd)
		return new_regd;

	memcpy(new_regd->alpha2, reg_info->alpha2, REG_ALPHA2_LEN + 1);
	memcpy(alpha2, reg_info->alpha2, REG_ALPHA2_LEN + 1);
	alpha2[2] = '\0';
	new_regd->dfs_region = ath11k_map_fw_dfs_region(reg_info->dfs_region);

	ath11k_dbg(ab, ATH11K_DBG_REG,
		   "Country %s, CFG Regdomain %s FW Regdomain %d, num_reg_rules %d\n",
		   alpha2, ath11k_reg_get_regdom_str(new_regd->dfs_region),
		   reg_info->dfs_region, num_rules);
	/* Update reg_rules[] below. Firmware is expected to
	 * send these rules in order(2 GHz rules first and then 5 GHz)
	 */
	for (; i < num_rules; i++) {
		if (reg_info->num_2ghz_reg_rules &&
		    (i < reg_info->num_2ghz_reg_rules)) {
			reg_rule = reg_info->reg_rules_2ghz_ptr + i;
			max_bw = min_t(u16, reg_rule->max_bw,
				       reg_info->max_bw_2ghz);
			flags = 0;
		} else if (reg_info->num_5ghz_reg_rules &&
			   (j < reg_info->num_5ghz_reg_rules)) {
			reg_rule = reg_info->reg_rules_5ghz_ptr + j++;
			max_bw = min_t(u16, reg_rule->max_bw,
				       reg_info->max_bw_5ghz);

			/* FW doesn't pass NL80211_RRF_AUTO_BW flag for
			 * BW Auto correction, we can enable this by default
			 * for all 5G rules here. The regulatory core performs
			 * BW correction if required and applies flags as
			 * per other BW rule flags we pass from here
			 */
			flags = NL80211_RRF_AUTO_BW;
		} else if (reg_info->is_ext_reg_event &&
			   reg_info->num_6ghz_rules_ap[WMI_REG_INDOOR_AP] &&
			   (k < reg_info->num_6ghz_rules_ap[WMI_REG_INDOOR_AP])) {
			reg_rule = reg_info->reg_rules_6ghz_ap_ptr[WMI_REG_INDOOR_AP] +
				   k++;
			max_bw = min_t(u16, reg_rule->max_bw,
				       reg_info->max_bw_6ghz_ap[WMI_REG_INDOOR_AP]);
			flags = NL80211_RRF_AUTO_BW;
		} else {
			break;
		}

		flags |= ath11k_map_fw_reg_flags(reg_rule->flags);

		ath11k_reg_update_rule(new_regd->reg_rules + i,
				       reg_rule->start_freq,
				       reg_rule->end_freq, max_bw,
				       reg_rule->ant_gain, reg_rule->reg_power,
				       flags);

		/* Update dfs cac timeout if the dfs domain is ETSI and the
		 * new rule covers weather radar band.
		 * Default value of '0' corresponds to 60s timeout, so no
		 * need to update that for other rules.
		 */
		if (flags & NL80211_RRF_DFS &&
		    reg_info->dfs_region == ATH11K_DFS_REG_ETSI &&
		    (reg_rule->end_freq > ETSI_WEATHER_RADAR_BAND_LOW &&
		    reg_rule->start_freq < ETSI_WEATHER_RADAR_BAND_HIGH)){
			ath11k_reg_update_weather_radar_band(ab, new_regd,
							     reg_rule, &i,
							     flags, max_bw);
			continue;
		}

		if (reg_info->is_ext_reg_event) {
			ath11k_dbg(ab, ATH11K_DBG_REG,
				   "\t%d. (%d - %d @ %d) (%d, %d) (%d ms) (FLAGS %d) (%d, %d)\n",
				   i + 1, reg_rule->start_freq, reg_rule->end_freq,
				   max_bw, reg_rule->ant_gain, reg_rule->reg_power,
				   new_regd->reg_rules[i].dfs_cac_ms, flags,
				   reg_rule->psd_flag, reg_rule->psd_eirp);
		} else {
			ath11k_dbg(ab, ATH11K_DBG_REG,
				   "\t%d. (%d - %d @ %d) (%d, %d) (%d ms) (FLAGS %d)\n",
				   i + 1, reg_rule->start_freq, reg_rule->end_freq,
				   max_bw, reg_rule->ant_gain, reg_rule->reg_power,
				   new_regd->reg_rules[i].dfs_cac_ms,
				   flags);
		}
	}

	new_regd->n_reg_rules = i;

	return new_regd;
}

void ath11k_regd_update_work(struct work_struct *work)
{
	struct ath11k *ar = container_of(work, struct ath11k,
					 regd_update_work);
	int ret;

	ret = ath11k_regd_update(ar);
	if (ret) {
		/* Firmware has already moved to the new regd. We need
		 * to maintain channel consistency across FW, Host driver
		 * and userspace. Hence as a fallback mechanism we can set
		 * the prev or default country code to the firmware.
		 */
		/* TODO: Implement Fallback Mechanism */
	}
}

void ath11k_reg_init(struct ath11k *ar)
{
	ar->hw->wiphy->regulatory_flags = REGULATORY_WIPHY_SELF_MANAGED;
	ar->hw->wiphy->reg_notifier = ath11k_reg_notifier;
}

void ath11k_reg_free(struct ath11k_base *ab)
{
	int i;

	for (i = 0; i < ab->hw_params.max_radios; i++) {
		kfree(ab->default_regd[i]);
		kfree(ab->new_regd[i]);
	}
}

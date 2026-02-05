// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 *
 * Copyright(c) 2013 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include <drv_types.h>
#include <rtw_btcoex.h>
#include <hal_btcoex.h>

#define BTCOEX_LPS_RF_ON_TIMEOUT	100

/**
 * rtw_btcoex_media_status_notify() - Notify BT coexistence of media status
 * @adpt: driver private adapter
 * @media_status: connection state
 *
 * Context: process context
 * Locking: caller ensures MLME state is stable
 */
void rtw_btcoex_media_status_notify(struct adapter *adpt, u8 media_status)
{
	if (media_status == RT_MEDIA_CONNECT &&
	    check_fwstate(&adpt->mlmepriv, WIFI_AP_STATE))
		rtw_hal_set_hwreg(adpt, HW_VAR_DL_RSVD_PAGE, NULL);

	hal_btcoex_MediaStatusNotify(adpt, media_status);
}

/**
 * rtw_btcoex_halt_notify() - Notify BT coexistence of device halt
 * @adpt: driver private adapter
 *
 * Context: process context
 * Locking: device teardown serialization handled by caller
 */
void rtw_btcoex_halt_notify(struct adapter *adpt)
{
	if (!adpt->bup || adpt->bSurpriseRemoved)
		return;

	hal_btcoex_HaltNotify(adpt);
}

/**
 * rtw_btcoex_reject_ap_aggregated_packet() - Control AP-side AMPDU handling
 * @adpt: driver private adapter
 * @enable: reject aggregation when true
 *
 * Used by BT coexistence logic to reduce interference by disabling
 * AP-side AMPDU negotiation.
 *
 * Context: process context
 * Locking: caller holds appropriate MLME/STA protection
 */
void rtw_btcoex_reject_ap_aggregated_packet(struct adapter *adpt, bool enable)
{
	struct mlme_ext_info *mlmeinfo;
	struct mlme_priv *mlme;
	struct sta_info *sta;

	mlme = &adpt->mlmepriv;
	mlmeinfo = &adpt->mlmeextpriv.mlmext_info;

	if (!enable) {
		mlmeinfo->accept_addba_req = true;
		return;
	}

	mlmeinfo->accept_addba_req = false;

	sta = rtw_get_stainfo(&adpt->stapriv, get_bssid(mlme));
	if (sta)
		send_delba(adpt, 0, sta->hwaddr);
}

/**
 * rtw_btcoex_lps_enter() - Enter low power state for BT coexistence
 * @adpt: driver private adapter
 *
 * Context: process context
 * Locking: power control serialized by caller
 *
 * This function transitions firmware into LPS mode. State flags are
 * updated only after the transition request is issued.
 */
void rtw_btcoex_lps_enter(struct adapter *adpt)
{
	struct pwrctrl_priv *pwrpriv;
	u8 lps_val;

	pwrpriv = adapter_to_pwrctl(adpt);

	lps_val = hal_btcoex_LpsVal(adpt);
	rtw_set_ps_mode(adpt, PS_MODE_MIN, 0, lps_val, "BTCOEX");

	pwrpriv->bpower_saving = true;
}

/**
 * rtw_btcoex_lps_leave() - Leave low power state for BT coexistence
 * @adpt: driver private adapter
 *
 * Context: process context
 * Locking: power control serialized by caller
 *
 * Ensures RF is fully powered before clearing power-saving state.
 */
void rtw_btcoex_lps_leave(struct adapter *adpt)
{
	struct pwrctrl_priv *pwrpriv;

	pwrpriv = adapter_to_pwrctl(adpt);

	if (pwrpriv->pwr_mode != PS_MODE_ACTIVE) {
		rtw_set_ps_mode(adpt, PS_MODE_ACTIVE, 0, 0,
				"BTCOEX");
		LPS_RF_ON_check(adpt, BTCOEX_LPS_RF_ON_TIMEOUT);
	}

	pwrpriv->bpower_saving = false;
}

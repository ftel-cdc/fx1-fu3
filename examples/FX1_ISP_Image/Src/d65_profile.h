/* User-supplied normalized ColorChecker fit, D65.
 * exposure=2.176626515768165; WB=2.3097591400146484,1,1.5355839729309082.
 * Original fit used RAW10 black_level=64. BLC/Gamma/LSC remain app defaults.
 */
#ifndef FX1_IMAGE_D65_PROFILE_H
#define FX1_IMAGE_D65_PROFILE_H
static void apply_d65_profile(isp_config_t *cfg)
{
    static const int16_t ccm_q9[9] = {904, -354, -39, -180, 892, -201, -18, -331, 861};
    cfg->wb.enable = true;
    cfg->wb.gain_r = 591; /* round(2.3097591400146484 * 256) */
    cfg->wb.gain_g = 256;
    cfg->wb.gain_b = 393; /* round(1.5355839729309082 * 256) */
    cfg->dg.enable = true;
    cfg->dg.gain = 557; /* round(exposure_scale * 256); apply exposure once */
    cfg->ccm.enable = true;
    for (unsigned i = 0; i < 9; ++i) cfg->ccm.matrix[i] = ccm_q9[i];
    for (unsigned i = 0; i < 3; ++i) cfg->ccm.offset[i] = 0;
}
#endif

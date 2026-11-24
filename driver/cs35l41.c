#include "cs35l41.h"
#include "cs35l41_dsp.h"
#include <evntrace.h>
#include <TraceLoggingProvider.h>

TRACELOGGING_DEFINE_PROVIDER(
    g_Cs35l41ProtectionProvider,
    "Nabu.Cirrus.Cs35l41.Protection",
    (0xce32fc6c, 0x0698, 0x4dd7, 0xb5, 0xa5, 0xb7, 0xf0, 0xb3, 0x4c, 0xe5, 0x8f));

TRACELOGGING_DEFINE_PROVIDER(
    g_Cs35l41SpeakerProvider,
    "Nabu.Cirrus.Cs35l41.SpeakerProtection",
    (0xf52e70e8, 0xf639, 0x49bd, 0xaf, 0x6a, 0x59, 0xcb, 0x2f, 0x17, 0xf9, 0xa5));

VOID Cs35l41DriverUnload(WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);
    TraceLoggingUnregister(g_Cs35l41SpeakerProvider);
    TraceLoggingUnregister(g_Cs35l41ProtectionProvider);
}

static ULONG Cs35l41DebugLevel = 100;
static ULONG Cs35l41DebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

static const struct cs35l41_otp_packed_element_t otp_map_1[] = {
	/* addr         shift   size */
	{ 0x00002030,	0,	4 }, /*TRIM_OSC_FREQ_TRIM*/
	{ 0x00002030,	7,	1 }, /*TRIM_OSC_TRIM_DONE*/
	{ 0x0000208c,	24,	6 }, /*TST_DIGREG_VREF_TRIM*/
	{ 0x00002090,	14,	4 }, /*TST_REF_TRIM*/
	{ 0x00002090,	10,	4 }, /*TST_REF_TEMPCO_TRIM*/
	{ 0x0000300C,	11,	4 }, /*PLL_LDOA_TST_VREF_TRIM*/
	{ 0x0000394C,	23,	2 }, /*BST_ATEST_CM_VOFF*/
	{ 0x00003950,	0,	7 }, /*BST_ATRIM_IADC_OFFSET*/
	{ 0x00003950,	8,	7 }, /*BST_ATRIM_IADC_GAIN1*/
	{ 0x00003950,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET1*/
	{ 0x00003950,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN1*/
	{ 0x00003954,	0,	7 }, /*BST_ATRIM_IADC_OFFSET2*/
	{ 0x00003954,	8,	7 }, /*BST_ATRIM_IADC_GAIN2*/
	{ 0x00003954,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET2*/
	{ 0x00003954,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN2*/
	{ 0x00003958,	0,	7 }, /*BST_ATRIM_IADC_OFFSET3*/
	{ 0x00003958,	8,	7 }, /*BST_ATRIM_IADC_GAIN3*/
	{ 0x00003958,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET3*/
	{ 0x00003958,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN3*/
	{ 0x0000395C,	0,	7 }, /*BST_ATRIM_IADC_OFFSET4*/
	{ 0x0000395C,	8,	7 }, /*BST_ATRIM_IADC_GAIN4*/
	{ 0x0000395C,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET4*/
	{ 0x0000395C,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN4*/
	{ 0x0000416C,	0,	8 }, /*VMON_GAIN_OTP_VAL*/
	{ 0x00004160,	0,	7 }, /*VMON_OFFSET_OTP_VAL*/
	{ 0x0000416C,	8,	8 }, /*IMON_GAIN_OTP_VAL*/
	{ 0x00004160,	16,	10 }, /*IMON_OFFSET_OTP_VAL*/
	{ 0x0000416C,	16,	12 }, /*VMON_CM_GAIN_OTP_VAL*/
	{ 0x0000416C,	28,	1 }, /*VMON_CM_GAIN_SIGN_OTP_VAL*/
	{ 0x00004170,	0,	6 }, /*IMON_CAL_TEMPCO_OTP_VAL*/
	{ 0x00004170,	6,	1 }, /*IMON_CAL_TEMPCO_SIGN_OTP*/
	{ 0x00004170,	8,	6 }, /*IMON_CAL_TEMPCO2_OTP_VAL*/
	{ 0x00004170,	14,	1 }, /*IMON_CAL_TEMPCO2_DN_UPB_OTP_VAL*/
	{ 0x00004170,	16,	9 }, /*IMON_CAL_TEMPCO_TBASE_OTP_VAL*/
	{ 0x00004360,	0,	5 }, /*TEMP_GAIN_OTP_VAL*/
	{ 0x00004360,	6,	9 }, /*TEMP_OFFSET_OTP_VAL*/
	{ 0x00004448,	0,	8 }, /*VP_SARADC_OFFSET*/
	{ 0x00004448,	8,	8 }, /*VP_GAIN_INDEX*/
	{ 0x00004448,	16,	8 }, /*VBST_SARADC_OFFSET*/
	{ 0x00004448,	24,	8 }, /*VBST_GAIN_INDEX*/
	{ 0x0000444C,	0,	3 }, /*ANA_SELINVREF*/
	{ 0x00006E30,	0,	5 }, /*GAIN_ERR_COEFF_0*/
	{ 0x00006E30,	8,	5 }, /*GAIN_ERR_COEFF_1*/
	{ 0x00006E30,	16,	5 }, /*GAIN_ERR_COEFF_2*/
	{ 0x00006E30,	24,	5 }, /*GAIN_ERR_COEFF_3*/
	{ 0x00006E34,	0,	5 }, /*GAIN_ERR_COEFF_4*/
	{ 0x00006E34,	8,	5 }, /*GAIN_ERR_COEFF_5*/
	{ 0x00006E34,	16,	5 }, /*GAIN_ERR_COEFF_6*/
	{ 0x00006E34,	24,	5 }, /*GAIN_ERR_COEFF_7*/
	{ 0x00006E38,	0,	5 }, /*GAIN_ERR_COEFF_8*/
	{ 0x00006E38,	8,	5 }, /*GAIN_ERR_COEFF_9*/
	{ 0x00006E38,	16,	5 }, /*GAIN_ERR_COEFF_10*/
	{ 0x00006E38,	24,	5 }, /*GAIN_ERR_COEFF_11*/
	{ 0x00006E3C,	0,	5 }, /*GAIN_ERR_COEFF_12*/
	{ 0x00006E3C,	8,	5 }, /*GAIN_ERR_COEFF_13*/
	{ 0x00006E3C,	16,	5 }, /*GAIN_ERR_COEFF_14*/
	{ 0x00006E3C,	24,	5 }, /*GAIN_ERR_COEFF_15*/
	{ 0x00006E40,	0,	5 }, /*GAIN_ERR_COEFF_16*/
	{ 0x00006E40,	8,	5 }, /*GAIN_ERR_COEFF_17*/
	{ 0x00006E40,	16,	5 }, /*GAIN_ERR_COEFF_18*/
	{ 0x00006E40,	24,	5 }, /*GAIN_ERR_COEFF_19*/
	{ 0x00006E44,	0,	5 }, /*GAIN_ERR_COEFF_20*/
	{ 0x00006E48,	0,	10 }, /*VOFF_GAIN_0*/
	{ 0x00006E48,	10,	10 }, /*VOFF_GAIN_1*/
	{ 0x00006E48,	20,	10 }, /*VOFF_GAIN_2*/
	{ 0x00006E4C,	0,	10 }, /*VOFF_GAIN_3*/
	{ 0x00006E4C,	10,	10 }, /*VOFF_GAIN_4*/
	{ 0x00006E4C,	20,	10 }, /*VOFF_GAIN_5*/
	{ 0x00006E50,	0,	10 }, /*VOFF_GAIN_6*/
	{ 0x00006E50,	10,	10 }, /*VOFF_GAIN_7*/
	{ 0x00006E50,	20,	10 }, /*VOFF_GAIN_8*/
	{ 0x00006E54,	0,	10 }, /*VOFF_GAIN_9*/
	{ 0x00006E54,	10,	10 }, /*VOFF_GAIN_10*/
	{ 0x00006E54,	20,	10 }, /*VOFF_GAIN_11*/
	{ 0x00006E58,	0,	10 }, /*VOFF_GAIN_12*/
	{ 0x00006E58,	10,	10 }, /*VOFF_GAIN_13*/
	{ 0x00006E58,	20,	10 }, /*VOFF_GAIN_14*/
	{ 0x00006E5C,	0,	10 }, /*VOFF_GAIN_15*/
	{ 0x00006E5C,	10,	10 }, /*VOFF_GAIN_16*/
	{ 0x00006E5C,	20,	10 }, /*VOFF_GAIN_17*/
	{ 0x00006E60,	0,	10 }, /*VOFF_GAIN_18*/
	{ 0x00006E60,	10,	10 }, /*VOFF_GAIN_19*/
	{ 0x00006E60,	20,	10 }, /*VOFF_GAIN_20*/
	{ 0x00006E64,	0,	10 }, /*VOFF_INT1*/
	{ 0x00007418,	7,	5 }, /*DS_SPK_INT1_CAP_TRIM*/
	{ 0x0000741C,	0,	5 }, /*DS_SPK_INT2_CAP_TRIM*/
	{ 0x0000741C,	11,	4 }, /*DS_SPK_LPF_CAP_TRIM*/
	{ 0x0000741C,	19,	4 }, /*DS_SPK_QUAN_CAP_TRIM*/
	{ 0x00007434,	17,	1 }, /*FORCE_CAL*/
	{ 0x00007434,	18,	7 }, /*CAL_OVERRIDE*/
	{ 0x00007068,	0,	9 }, /*MODIX*/
	{ 0x0000410C,	7,	1 }, /*VIMON_DLY_NOT_COMB*/
	{ 0x0000400C,	0,	7 }, /*VIMON_DLY*/
	{ 0x00000000,	0,	1 }, /*extra bit*/
	{ 0x00017040,	0,	8 }, /*X_COORDINATE*/
	{ 0x00017040,	8,	8 }, /*Y_COORDINATE*/
	{ 0x00017040,	16,	8 }, /*WAFER_ID*/
	{ 0x00017040,	24,	8 }, /*DVS*/
	{ 0x00017044,	0,	24 }, /*LOT_NUMBER*/
};

static const struct cs35l41_otp_packed_element_t otp_map_2[] = {
	/* addr         shift   size */
	{ 0x00002030,	0,	4 }, /*TRIM_OSC_FREQ_TRIM*/
	{ 0x00002030,	7,	1 }, /*TRIM_OSC_TRIM_DONE*/
	{ 0x0000208c,	24,	6 }, /*TST_DIGREG_VREF_TRIM*/
	{ 0x00002090,	14,	4 }, /*TST_REF_TRIM*/
	{ 0x00002090,	10,	4 }, /*TST_REF_TEMPCO_TRIM*/
	{ 0x0000300C,	11,	4 }, /*PLL_LDOA_TST_VREF_TRIM*/
	{ 0x0000394C,	23,	2 }, /*BST_ATEST_CM_VOFF*/
	{ 0x00003950,	0,	7 }, /*BST_ATRIM_IADC_OFFSET*/
	{ 0x00003950,	8,	7 }, /*BST_ATRIM_IADC_GAIN1*/
	{ 0x00003950,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET1*/
	{ 0x00003950,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN1*/
	{ 0x00003954,	0,	7 }, /*BST_ATRIM_IADC_OFFSET2*/
	{ 0x00003954,	8,	7 }, /*BST_ATRIM_IADC_GAIN2*/
	{ 0x00003954,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET2*/
	{ 0x00003954,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN2*/
	{ 0x00003958,	0,	7 }, /*BST_ATRIM_IADC_OFFSET3*/
	{ 0x00003958,	8,	7 }, /*BST_ATRIM_IADC_GAIN3*/
	{ 0x00003958,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET3*/
	{ 0x00003958,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN3*/
	{ 0x0000395C,	0,	7 }, /*BST_ATRIM_IADC_OFFSET4*/
	{ 0x0000395C,	8,	7 }, /*BST_ATRIM_IADC_GAIN4*/
	{ 0x0000395C,	16,	8 }, /*BST_ATRIM_IPKCOMP_OFFSET4*/
	{ 0x0000395C,	24,	8 }, /*BST_ATRIM_IPKCOMP_GAIN4*/
	{ 0x0000416C,	0,	8 }, /*VMON_GAIN_OTP_VAL*/
	{ 0x00004160,	0,	7 }, /*VMON_OFFSET_OTP_VAL*/
	{ 0x0000416C,	8,	8 }, /*IMON_GAIN_OTP_VAL*/
	{ 0x00004160,	16,	10 }, /*IMON_OFFSET_OTP_VAL*/
	{ 0x0000416C,	16,	12 }, /*VMON_CM_GAIN_OTP_VAL*/
	{ 0x0000416C,	28,	1 }, /*VMON_CM_GAIN_SIGN_OTP_VAL*/
	{ 0x00004170,	0,	6 }, /*IMON_CAL_TEMPCO_OTP_VAL*/
	{ 0x00004170,	6,	1 }, /*IMON_CAL_TEMPCO_SIGN_OTP*/
	{ 0x00004170,	8,	6 }, /*IMON_CAL_TEMPCO2_OTP_VAL*/
	{ 0x00004170,	14,	1 }, /*IMON_CAL_TEMPCO2_DN_UPB_OTP_VAL*/
	{ 0x00004170,	16,	9 }, /*IMON_CAL_TEMPCO_TBASE_OTP_VAL*/
	{ 0x00004360,	0,	5 }, /*TEMP_GAIN_OTP_VAL*/
	{ 0x00004360,	6,	9 }, /*TEMP_OFFSET_OTP_VAL*/
	{ 0x00004448,	0,	8 }, /*VP_SARADC_OFFSET*/
	{ 0x00004448,	8,	8 }, /*VP_GAIN_INDEX*/
	{ 0x00004448,	16,	8 }, /*VBST_SARADC_OFFSET*/
	{ 0x00004448,	24,	8 }, /*VBST_GAIN_INDEX*/
	{ 0x0000444C,	0,	3 }, /*ANA_SELINVREF*/
	{ 0x00006E30,	0,	5 }, /*GAIN_ERR_COEFF_0*/
	{ 0x00006E30,	8,	5 }, /*GAIN_ERR_COEFF_1*/
	{ 0x00006E30,	16,	5 }, /*GAIN_ERR_COEFF_2*/
	{ 0x00006E30,	24,	5 }, /*GAIN_ERR_COEFF_3*/
	{ 0x00006E34,	0,	5 }, /*GAIN_ERR_COEFF_4*/
	{ 0x00006E34,	8,	5 }, /*GAIN_ERR_COEFF_5*/
	{ 0x00006E34,	16,	5 }, /*GAIN_ERR_COEFF_6*/
	{ 0x00006E34,	24,	5 }, /*GAIN_ERR_COEFF_7*/
	{ 0x00006E38,	0,	5 }, /*GAIN_ERR_COEFF_8*/
	{ 0x00006E38,	8,	5 }, /*GAIN_ERR_COEFF_9*/
	{ 0x00006E38,	16,	5 }, /*GAIN_ERR_COEFF_10*/
	{ 0x00006E38,	24,	5 }, /*GAIN_ERR_COEFF_11*/
	{ 0x00006E3C,	0,	5 }, /*GAIN_ERR_COEFF_12*/
	{ 0x00006E3C,	8,	5 }, /*GAIN_ERR_COEFF_13*/
	{ 0x00006E3C,	16,	5 }, /*GAIN_ERR_COEFF_14*/
	{ 0x00006E3C,	24,	5 }, /*GAIN_ERR_COEFF_15*/
	{ 0x00006E40,	0,	5 }, /*GAIN_ERR_COEFF_16*/
	{ 0x00006E40,	8,	5 }, /*GAIN_ERR_COEFF_17*/
	{ 0x00006E40,	16,	5 }, /*GAIN_ERR_COEFF_18*/
	{ 0x00006E40,	24,	5 }, /*GAIN_ERR_COEFF_19*/
	{ 0x00006E44,	0,	5 }, /*GAIN_ERR_COEFF_20*/
	{ 0x00006E48,	0,	10 }, /*VOFF_GAIN_0*/
	{ 0x00006E48,	10,	10 }, /*VOFF_GAIN_1*/
	{ 0x00006E48,	20,	10 }, /*VOFF_GAIN_2*/
	{ 0x00006E4C,	0,	10 }, /*VOFF_GAIN_3*/
	{ 0x00006E4C,	10,	10 }, /*VOFF_GAIN_4*/
	{ 0x00006E4C,	20,	10 }, /*VOFF_GAIN_5*/
	{ 0x00006E50,	0,	10 }, /*VOFF_GAIN_6*/
	{ 0x00006E50,	10,	10 }, /*VOFF_GAIN_7*/
	{ 0x00006E50,	20,	10 }, /*VOFF_GAIN_8*/
	{ 0x00006E54,	0,	10 }, /*VOFF_GAIN_9*/
	{ 0x00006E54,	10,	10 }, /*VOFF_GAIN_10*/
	{ 0x00006E54,	20,	10 }, /*VOFF_GAIN_11*/
	{ 0x00006E58,	0,	10 }, /*VOFF_GAIN_12*/
	{ 0x00006E58,	10,	10 }, /*VOFF_GAIN_13*/
	{ 0x00006E58,	20,	10 }, /*VOFF_GAIN_14*/
	{ 0x00006E5C,	0,	10 }, /*VOFF_GAIN_15*/
	{ 0x00006E5C,	10,	10 }, /*VOFF_GAIN_16*/
	{ 0x00006E5C,	20,	10 }, /*VOFF_GAIN_17*/
	{ 0x00006E60,	0,	10 }, /*VOFF_GAIN_18*/
	{ 0x00006E60,	10,	10 }, /*VOFF_GAIN_19*/
	{ 0x00006E60,	20,	10 }, /*VOFF_GAIN_20*/
	{ 0x00006E64,	0,	10 }, /*VOFF_INT1*/
	{ 0x00007418,	7,	5 }, /*DS_SPK_INT1_CAP_TRIM*/
	{ 0x0000741C,	0,	5 }, /*DS_SPK_INT2_CAP_TRIM*/
	{ 0x0000741C,	11,	4 }, /*DS_SPK_LPF_CAP_TRIM*/
	{ 0x0000741C,	19,	4 }, /*DS_SPK_QUAN_CAP_TRIM*/
	{ 0x00007434,	17,	1 }, /*FORCE_CAL*/
	{ 0x00007434,	18,	7 }, /*CAL_OVERRIDE*/
	{ 0x00007068,	0,	9 }, /*MODIX*/
	{ 0x0000410C,	7,	1 }, /*VIMON_DLY_NOT_COMB*/
	{ 0x0000400C,	0,	7 }, /*VIMON_DLY*/
	{ 0x00004000,	11,	1 }, /*VMON_POL*/
	{ 0x00017040,	0,	8 }, /*X_COORDINATE*/
	{ 0x00017040,	8,	8 }, /*Y_COORDINATE*/
	{ 0x00017040,	16,	8 }, /*WAFER_ID*/
	{ 0x00017040,	24,	8 }, /*DVS*/
	{ 0x00017044,	0,	24 }, /*LOT_NUMBER*/
};

static const struct cs35l41_otp_map_element_t cs35l41_otp_map_map[] = {
	{
		.id = 0x01,
		.map = otp_map_1,
		.num_elements = ARRAYSIZE(otp_map_1),
		.bit_offset = 16,
		.word_offset = 2,
	},
	{
		.id = 0x02,
		.map = otp_map_2,
		.num_elements = ARRAYSIZE(otp_map_2),
		.bit_offset = 16,
		.word_offset = 2,
	},
	{
		.id = 0x03,
		.map = otp_map_2,
		.num_elements = ARRAYSIZE(otp_map_2),
		.bit_offset = 16,
		.word_offset = 2,
	},
	{
		.id = 0x06,
		.map = otp_map_2,
		.num_elements = ARRAYSIZE(otp_map_2),
		.bit_offset = 16,
		.word_offset = 2,
	},
	{
		.id = 0x08,
		.map = otp_map_1,
		.num_elements = ARRAYSIZE(otp_map_1),
		.bit_offset = 16,
		.word_offset = 2,
	},
};

static const UINT8 cs35l41_bst_k1_table[4][5] = {
	{ 0x24, 0x32, 0x32, 0x4F, 0x57 },
	{ 0x24, 0x32, 0x32, 0x4F, 0x57 },
	{ 0x40, 0x32, 0x32, 0x4F, 0x57 },
	{ 0x40, 0x32, 0x32, 0x4F, 0x57 }
};

static const UINT8 cs35l41_bst_k2_table[4][5] = {
	{ 0x24, 0x49, 0x66, 0xA3, 0xEA },
	{ 0x24, 0x49, 0x66, 0xA3, 0xEA },
	{ 0x48, 0x49, 0x66, 0xA3, 0xEA },
	{ 0x48, 0x49, 0x66, 0xA3, 0xEA }
};

static const UINT8 cs35l41_bst_slope_table[4] = {
	0x75, 0x6B, 0x3B, 0x28
};

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT  DriverObject,
	__in PUNICODE_STRING RegistryPath
)
{
	NTSTATUS               status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
		"Driver Entry\n");

	WDF_DRIVER_CONFIG_INIT(&config, Cs35l41EvtDeviceAdd);
	config.EvtDriverUnload = Cs35l41DriverUnload;
	(void)TraceLoggingRegister(g_Cs35l41ProtectionProvider);
	(void)TraceLoggingRegister(g_Cs35l41SpeakerProvider);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	//
	// Create a framework driver object to represent our driver.
	//

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		WDF_NO_HANDLE
	);

	if (!NT_SUCCESS(status))
	{
		TraceLoggingUnregister(g_Cs35l41SpeakerProvider);
		TraceLoggingUnregister(g_Cs35l41ProtectionProvider);
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"WdfDriverCreate failed with status 0x%x\n", status);
	}

	return status;
}

NTSTATUS cs35l41_get_clk_config(INT32 freq)
{
	INT32 i;

	for (i = 0; i < ARRAY_SIZE(cs35l41_pll_sysclk); i++) {
		if (cs35l41_pll_sysclk[i].freq == freq)
			return cs35l41_pll_sysclk[i].clk_cfg;
	}

	return STATUS_INVALID_PARAMETER;
}

NTSTATUS cs35l41_reg_write(PCS35L41_CONTEXT pDevice, UINT32 reg, UINT32 data)
{
	UINT32 buf[2];

	buf[0] = _byteswap_ulong(reg);
	buf[1] = _byteswap_ulong(data);

	return SpbWriteDataSynchronously(&pDevice->I2CContext, buf, sizeof(buf));
}

NTSTATUS cs35l41_multi_reg_write(PCS35L41_CONTEXT pDevice, const struct reg_sequence *regs, UINT32 num_regs)
{
	NTSTATUS status = STATUS_SUCCESS;

	for (UINT32 i = 0; i < num_regs; i++) {
		status = cs35l41_reg_write(pDevice, regs[i].reg, regs[i].def);
		if (!NT_SUCCESS(status))
			return status;
	}
	return status;
}

NTSTATUS cs35l41_reg_bulk_write(PCS35L41_CONTEXT pDevice, UINT32 reg, const UINT8* data, UINT32 length)
{
	UINT32 regBE = _byteswap_ulong(reg);

	return SpbWriteDataSynchronouslyEx(&pDevice->I2CContext, &regBE, sizeof(UINT32), data, length);
}

NTSTATUS cs35l41_reg_read(
	_In_ PCS35L41_CONTEXT pDevice,
	UINT32 reg,
	UINT32* data
) {
	NTSTATUS status;
	UINT32 regBE = _byteswap_ulong(reg);

	UINT32 raw_data = 0;
	status = SpbWriteRead(&pDevice->I2CContext, &regBE, sizeof(UINT32), &raw_data,
		sizeof(UINT32), 0, CS35L41_SPB_READ_TIMEOUT_MS);
	*data = _byteswap_ulong(raw_data);

	return status;
}

NTSTATUS cs35l41_reg_bulk_read(
	_In_ PCS35L41_CONTEXT pDevice,
	UINT32 reg,
	UINT32* data,
	UINT32 length
) {
	NTSTATUS status = STATUS_IO_DEVICE_ERROR;
	UINT32 regBE = _byteswap_ulong(reg);

	status = SpbWriteRead(&pDevice->I2CContext, &regBE, sizeof(UINT32), data,
		(UINT16)length, 0, CS35L41_SPB_READ_TIMEOUT_MS);
	
	return status;
}

NTSTATUS cs35l41_reg_update_bits(PCS35L41_CONTEXT pDevice, UINT32 reg, UINT32 mask, UINT32 val)
{
	NTSTATUS status;
	UINT32 temp_val, data;

	status = cs35l41_reg_read(pDevice, reg, &data);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	temp_val = data & ~mask;
	temp_val |= val & mask;

	if (data == temp_val)
	{
		status = STATUS_SUCCESS;
		return status;
	}
	status = cs35l41_reg_write(pDevice, reg, temp_val);

	return status;
}

static const struct cs35l41_otp_map_element_t* cs35l41_find_otp_map(UINT32 otp_id)
{
	INT32 i;

	for (i = 0; i < ARRAYSIZE(cs35l41_otp_map_map); i++) {
		if (cs35l41_otp_map_map[i].id == otp_id)
			return &cs35l41_otp_map_map[i];
	}

	return NULL;
}

NTSTATUS cs35l41_otp_unpack(PCS35L41_CONTEXT pDevice)
{
	const struct cs35l41_otp_map_element_t* otp_map_match;
	const struct cs35l41_otp_packed_element_t* otp_map;
	INT32 bit_offset, word_offset, ret;
	UINT32 bit_sum = 8;
	UINT32 otp_val, otp_id_reg;
	UINT32 otp_mem[CS35L41_OTP_SIZE_WORDS];

	ret = cs35l41_reg_read(pDevice, CS35L41_OTPID, &otp_id_reg);
	if (ret) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Read OTP ID failed: %d\n", ret);
		return ret;
	}

	otp_map_match = cs35l41_find_otp_map(otp_id_reg);

	if (!otp_map_match) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"OTP Map matching ID %d not found\n", otp_id_reg);
		ret = STATUS_INVALID_PARAMETER;
		return ret;
	}

	// The OTP map indexes 32-bit registers. SPB returns raw big-endian bytes.
	ret = cs35l41_reg_bulk_read(pDevice, CS35L41_OTP_MEM0, otp_mem,
		sizeof(otp_mem));
	if (ret) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Read OTP Mem failed: %d\n", ret);
		return ret;
	}

	for (UINT32 i = 0; i < CS35L41_OTP_SIZE_WORDS; i++)
		otp_mem[i] = _byteswap_ulong(otp_mem[i]);

	otp_map = otp_map_match->map;

	bit_offset = otp_map_match->bit_offset;
	word_offset = otp_map_match->word_offset;

	for (UINT32 i = 0; i < otp_map_match->num_elements; i++) {
		Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
			"bitoffset= %d, word_offset=%d, bit_sum mod 32=%d, otp_map[i].size = %u\n",
			bit_offset, word_offset, bit_sum % 32, otp_map[i].size);
		if (bit_offset + otp_map[i].size - 1 >= 32) {
			otp_val = (otp_mem[word_offset] &
				GENMASK(31, bit_offset)) >> bit_offset;
			otp_val |= (otp_mem[++word_offset] &
				GENMASK(bit_offset + otp_map[i].size - 33, 0)) <<
				(32 - bit_offset);
			bit_offset += otp_map[i].size - 32;
		}
		else if (bit_offset + otp_map[i].size - 1 >= 0) {
			otp_val = (otp_mem[word_offset] &
				GENMASK(bit_offset + otp_map[i].size - 1, bit_offset)
				) >> bit_offset;
			bit_offset += otp_map[i].size;
		}
		else /* both bit_offset and otp_map[i].size are 0 */
			otp_val = 0;

		bit_sum += otp_map[i].size;

		if (bit_offset == 32) {
			bit_offset = 0;
			word_offset++;
		}

		if (otp_map[i].reg != 0) {
			ret = cs35l41_reg_update_bits(pDevice, otp_map[i].reg,
				GENMASK(otp_map[i].shift + otp_map[i].size - 1,
					otp_map[i].shift),
				otp_val << otp_map[i].shift);
			if (ret < 0) {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"Write OTP val failed: %d\n", ret);
				return ret;
			}
		}
	}

	return STATUS_SUCCESS;
}

static NTSTATUS cs35l41_boost_config(PCS35L41_CONTEXT pDevice, INT32 boost_ind,
	INT32 boost_cap, INT32 boost_ipk)
{
	UINT8 bst_lbst_val, bst_cbst_range, bst_ipk_scaled;
	INT32 ret;

	switch (boost_ind) {
	case 1000:	/* 1.0 uH */
		bst_lbst_val = 0;
		break;
	case 1200:	/* 1.2 uH */
		bst_lbst_val = 1;
		break;
	case 1500:	/* 1.5 uH */
		bst_lbst_val = 2;
		break;
	case 2200:	/* 2.2 uH */
		bst_lbst_val = 3;
		break;
	default:
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Invalid boost inductor value: %d nH\n", boost_ind);
		return STATUS_INVALID_PARAMETER;
	}

	if (boost_cap >= 0 && boost_cap <= 19) {
		bst_cbst_range = 0;
	}
	else if (boost_cap >= 20 && boost_cap <= 50) {
		bst_cbst_range = 1;
	}
	else if (boost_cap >= 51 && boost_cap <= 100) {
		bst_cbst_range = 2;
	}
	else if (boost_cap >= 101 && boost_cap <= 200) {
		bst_cbst_range = 3;
	}
	else {
		if (boost_cap < 0) {
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
				"Invalid boost capacitor value: %d nH\n", boost_cap);
			return STATUS_INVALID_PARAMETER;
		}
		/* 201 uF and greater */
		bst_cbst_range = 4;
	}

	if (boost_ipk < 1600 || boost_ipk > 4500) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Invalid boost inductor peak current: %d mA\n", boost_ipk);
		return STATUS_INVALID_PARAMETER;
	}

	ret = cs35l41_reg_update_bits(pDevice, CS35L41_BSTCVRT_COEFF,
		CS35L41_BST_K1_MASK | CS35L41_BST_K2_MASK,
		cs35l41_bst_k1_table[bst_lbst_val][bst_cbst_range]
		<< CS35L41_BST_K1_SHIFT |
		cs35l41_bst_k2_table[bst_lbst_val][bst_cbst_range]
		<< CS35L41_BST_K2_SHIFT);
	if (ret) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to write boost coefficients: %d\n", ret);
		return ret;
	}

	ret = cs35l41_reg_update_bits(pDevice, CS35L41_BSTCVRT_SLOPE_LBST,
		CS35L41_BST_SLOPE_MASK | CS35L41_BST_LBST_VAL_MASK,
		cs35l41_bst_slope_table[bst_lbst_val]
		<< CS35L41_BST_SLOPE_SHIFT |
		bst_lbst_val << CS35L41_BST_LBST_VAL_SHIFT);
	if (ret) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to write boost slope/inductor value: %d\n", ret);
		return ret;
	}

	bst_ipk_scaled = (UINT8)((boost_ipk - 1600) / 50) + 0x10;

	ret = cs35l41_reg_update_bits(pDevice, CS35L41_BSTCVRT_PEAK_CUR, CS35L41_BST_IPK_MASK,
		bst_ipk_scaled << CS35L41_BST_IPK_SHIFT);
	if (ret) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to write boost inductor peak current: %d\n", ret);
		return ret;
	}

	return cs35l41_reg_update_bits(pDevice, CS35L41_PWR_CTRL2, CS35L41_BST_EN_MASK,
		CS35L41_BST_EN_DEFAULT << CS35L41_BST_EN_SHIFT);
}

void udelay(ULONG usec) {
	LARGE_INTEGER Interval;
	Interval.QuadPart = -10 * (LONGLONG)usec;
	KeDelayExecutionThread(KernelMode, FALSE, &Interval);
}

void msleep(ULONG msec) {
	udelay(msec * 1000);
}

static NTSTATUS cs35l41_set_test_key(PCS35L41_CONTEXT pDevice, BOOLEAN unlock)
{
	NTSTATUS first = cs35l41_reg_write(pDevice, CS35L41_TEST_KEY_CTL, unlock ? 0x55 : 0xCC);
	NTSTATUS second = cs35l41_reg_write(pDevice, CS35L41_TEST_KEY_CTL, unlock ? 0xAA : 0x33);
	return NT_SUCCESS(first) ? second : first;
}

static NTSTATUS cs35l41_write_protected(PCS35L41_CONTEXT pDevice, UINT32 reg, UINT32 value)
{
	NTSTATUS status = cs35l41_set_test_key(pDevice, TRUE);
	if (NT_SUCCESS(status))
		status = cs35l41_reg_write(pDevice, reg, value);
	NTSTATUS lockStatus = cs35l41_set_test_key(pDevice, FALSE);
	return NT_SUCCESS(status) ? lockStatus : status;
}

static NTSTATUS cs35l41_amp_enable(PCS35L41_CONTEXT pDevice)
{
	NTSTATUS status = cs35l41_write_protected(pDevice, 0x00002084, 0x002F1AA0);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_PWR_CTRL1, CS35L41_GLOBAL_EN_MASK,
		1 << CS35L41_GLOBAL_EN_SHIFT);
	if (NT_SUCCESS(status))
		msleep(1);

	return status;
}

static NTSTATUS cs35l41_amp_disable(PCS35L41_CONTEXT pDevice)
{
	NTSTATUS status = cs35l41_reg_update_bits(pDevice, CS35L41_PWR_CTRL1,
		CS35L41_GLOBAL_EN_MASK, 0);
	if (!NT_SUCCESS(status))
		return status;

	BOOLEAN pdn = FALSE;
	for (UINT32 i = 0; i < 100; i++) {
		UINT32 irqStatus;
		status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS1, &irqStatus);
		if (!NT_SUCCESS(status))
			break;
		if (irqStatus & CS35L41_PDN_DONE_MASK) {
			pdn = TRUE;
			break;
		}
		msleep(1);
	}

	if (!pdn)
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "PDN failed: %!STATUS!\n", status);

	NTSTATUS ackStatus = cs35l41_reg_write(pDevice, CS35L41_IRQ1_STATUS1, CS35L41_PDN_DONE_MASK);
	NTSTATUS patchStatus = cs35l41_write_protected(pDevice, 0x00002084, 0x002F1AA3);
	if (!NT_SUCCESS(status))
		return status;
	return NT_SUCCESS(ackStatus) ? patchStatus : ackStatus;
}

static NTSTATUS cs35l41_component_set_sysclk(PCS35L41_CONTEXT pDevice,
	INT32 source,
	UINT32 freq)
{
	UINT32 fs1_val = 0, fs2_val = 0, val;
	INT32 extclk_cfg;
	if (freq == 0)
		return STATUS_INVALID_PARAMETER;
	extclk_cfg = cs35l41_get_clk_config(freq);
	if (!NT_SUCCESS(extclk_cfg))
		return extclk_cfg;

	if (freq > 6000000) {
		fs1_val = 3 * 4 + 4;
		fs2_val = 8 * 4 + 4;
	}

	if (freq <= 6000000) {
		fs1_val = 3 *
			((24000000 + freq - 1) / freq) + 4;
		fs2_val = 5 *
			((24000000 + freq - 1) / freq) + 4;
	}

	val = fs1_val;
	val |= (fs2_val << CS35L41_FS2_WINDOW_SHIFT) & CS35L41_FS2_WINDOW_MASK;
	NTSTATUS status = cs35l41_write_protected(pDevice, CS35L41_TST_FS_MON0, val);
	if (!NT_SUCCESS(status))
		return status;
	status = cs35l41_reg_update_bits(pDevice,
		CS35L41_SP_RATE_CTRL, 0x3F,
		extclk_cfg);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_PLL_OPENLOOP_MASK,
		1 << CS35L41_PLL_OPENLOOP_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_REFCLK_FREQ_MASK,
		extclk_cfg << CS35L41_REFCLK_FREQ_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_PLL_CLK_EN_MASK,
		0 << CS35L41_PLL_CLK_EN_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_PLL_CLK_SEL_MASK,
		source);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_PLL_OPENLOOP_MASK,
		0 << CS35L41_PLL_OPENLOOP_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PLL_CLK_CTRL,
		CS35L41_PLL_CLK_EN_MASK,
		1 << CS35L41_PLL_CLK_EN_SHIFT);
	return status;
}

static NTSTATUS cs35l41_pcm_hw_params(PCS35L41_CONTEXT pDevice)
{
	INT32 asp_width = 32;
	INT32 asp_wl = 16;
	NabuSpeakerRoute route;
	if (NabuResolveSpeakerRoute((UINT32)pDevice->UID, &route) != 0)
		return STATUS_INVALID_DEVICE_STATE;

	NTSTATUS status = cs35l41_reg_update_bits(pDevice, CS35L41_GLOBAL_CLK_CTRL,
		CS35L41_GLOBAL_FS_MASK,
		0x3 << CS35L41_GLOBAL_FS_SHIFT);
	if (!NT_SUCCESS(status)) return status;

	status = cs35l41_component_set_sysclk(pDevice, CS35L41_PLLSRC_SCLK, Q6AFE_LPASS_IBIT_CLK_1_P536_MHZ);
	if (!NT_SUCCESS(status)) return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_ASP_WIDTH_RX_MASK,
		asp_width << CS35L41_ASP_WIDTH_RX_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_RX_WL,
		CS35L41_ASP_RX_WL_MASK,
		asp_wl << CS35L41_ASP_RX_WL_SHIFT);
	if (!NT_SUCCESS(status)) return status;

    // QCAUD uses the Windows serial slots 0/1. Feed both slots to every amplifier.
    // The stock Android DSP then selects the physical left/right pair via CH_BAL.
    status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FRAME_RX_SLOT,
        CS35L41_ASP_RX1_SLOT_MASK | CS35L41_ASP_RX2_SLOT_MASK,
        (route.Rx1Slot << CS35L41_ASP_RX1_SLOT_SHIFT) | (route.Rx2Slot << CS35L41_ASP_RX2_SLOT_SHIFT));
    if (!NT_SUCCESS(status))
        return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_ASP_WIDTH_TX_MASK,
		asp_width << CS35L41_ASP_WIDTH_TX_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_TX_WL,
		CS35L41_ASP_TX_WL_MASK,
		asp_wl << CS35L41_ASP_TX_WL_SHIFT);
	return status;
}

static NTSTATUS cs35l41_set_dai_fmt(PCS35L41_CONTEXT pDevice)
{
	INT32 sclk_fmt = 0; //SND_SOC_DAIFMT_NB_NF
	INT32 lrclk_fmt = 0; //SND_SOC_DAIFMT_NB_NF
	INT32 slave_mode = 0; //SND_SOC_DAIFMT_CBS_CFS
	INT32 asp_fmt = 2; //I2S

	NTSTATUS status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_ASP_FMT_MASK,
		asp_fmt << CS35L41_ASP_FMT_SHIFT);
	if (!NT_SUCCESS(status)) return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_SCLK_MSTR_MASK,
		slave_mode << CS35L41_SCLK_MSTR_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_LRCLK_MSTR_MASK,
		slave_mode << CS35L41_LRCLK_MSTR_SHIFT);
	if (!NT_SUCCESS(status)) return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_AMP_DIG_VOL_CTRL,
		CS35L41_AMP_PCM_VOL_MASK,
		0 << CS35L41_AMP_PCM_VOL_SHIFT);
	if (!NT_SUCCESS(status)) return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_LRCLK_INV_MASK,
		lrclk_fmt << CS35L41_LRCLK_INV_SHIFT);
	if (!NT_SUCCESS(status)) return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_FORMAT,
		CS35L41_SCLK_INV_MASK,
		sclk_fmt << CS35L41_SCLK_INV_SHIFT);
	return status;
}

static BOOLEAN cs35l41_check_cspl_mbox_sts(enum cs35l41_cspl_mbox_cmd cmd,
	enum cs35l41_cspl_mbox_status sts)
{
	switch (cmd) {
	case CSPL_MBOX_CMD_NONE:
	case CSPL_MBOX_CMD_UNKNOWN_CMD:
		return TRUE;
	case CSPL_MBOX_CMD_PAUSE:
	case CSPL_MBOX_CMD_OUT_OF_HIBERNATE:
		return (sts == CSPL_MBOX_STS_PAUSED);
	case CSPL_MBOX_CMD_RESUME:
		return (sts == CSPL_MBOX_STS_RUNNING);
	case CSPL_MBOX_CMD_REINIT:
		return (sts == CSPL_MBOX_STS_RUNNING);
	case CSPL_MBOX_CMD_STOP_PRE_REINIT:
		return (sts == CSPL_MBOX_STS_RDY_FOR_REINIT);
	default:
		return FALSE;
	}
}

NTSTATUS cs35l41_set_cspl_mbox_cmd(PCS35L41_CONTEXT pDevice, enum cs35l41_cspl_mbox_cmd cmd)
{
	UINT32 sts = 0, i;
	NTSTATUS status;

	// Set mailbox cmd
	status = cs35l41_reg_write(pDevice, CS35L41_DSP_VIRT1_MBOX_1, cmd);
	if (!NT_SUCCESS(status)) {
		if (cmd != CSPL_MBOX_CMD_OUT_OF_HIBERNATE) {
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to write MBOX: %d\n", status);
		}
		return status;
	}

	// Read mailbox status and verify it is appropriate for the given cmd
	for (i = 0; i < 5; i++) {
		udelay(1000);

		status = cs35l41_reg_read(pDevice, CS35L41_DSP_MBOX_2, &sts);
		if (!NT_SUCCESS(status)) {
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to read MBOX STS: %d\n", status);
			continue;
		}

		if (!cs35l41_check_cspl_mbox_sts(cmd, sts)) {
			Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT, "[%u] cmd %u returned invalid sts %u\n", i, cmd, sts);
		}
		else
			return STATUS_SUCCESS;
	}

	if (cmd != CSPL_MBOX_CMD_OUT_OF_HIBERNATE)
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "Failed to set mailbox cmd %u (status %u)\n", cmd, sts);

	return STATUS_UNSUCCESSFUL;
}

static NTSTATUS cs35l41_dsp_mbox_start(PCS35L41_CONTEXT pDevice)
{
	UINT32 fw_status;

	NTSTATUS status = cs35l41_reg_read(pDevice, CS35L41_DSP_MBOX_2, &fw_status);
	if (!NT_SUCCESS(status))
		return status;

	switch (fw_status) {
	case CSPL_MBOX_STS_RUNNING:
	case CSPL_MBOX_STS_PAUSED:
		break;
	default:
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "Firmware status is invalid: %u\n",
			fw_status);
		return STATUS_INVALID_PARAMETER;
	}

	return cs35l41_set_cspl_mbox_cmd(pDevice, CSPL_MBOX_CMD_RESUME);
}

NTSTATUS
GetDeviceUID(
	_In_ WDFDEVICE FxDevice,
	_In_ PINT32 PUID
)
{
	NTSTATUS status = STATUS_ACPI_NOT_INITIALIZED;
	ACPI_EVAL_INPUT_BUFFER_EX inputBuffer;
	RtlZeroMemory(&inputBuffer, sizeof(inputBuffer));

	inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE_EX;
	status = RtlStringCchPrintfA(
		inputBuffer.MethodName,
		sizeof(inputBuffer.MethodName),
		"_UID"
	);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	WDFMEMORY outputMemory;
	PACPI_EVAL_OUTPUT_BUFFER outputBuffer;
	size_t outputArgumentBufferSize = 32;
	size_t outputBufferSize = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument) + outputArgumentBufferSize;

	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = FxDevice;

	status = WdfMemoryCreate(&attributes,
		NonPagedPoolNx,
		0,
		outputBufferSize,
		&outputMemory,
		(PVOID*)&outputBuffer);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	RtlZeroMemory(outputBuffer, outputBufferSize);

	WDF_MEMORY_DESCRIPTOR inputMemDesc;
	WDF_MEMORY_DESCRIPTOR outputMemDesc;
	WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&inputMemDesc, &inputBuffer, (ULONG)sizeof(inputBuffer));
	WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&outputMemDesc, outputMemory, NULL);

	status = WdfIoTargetSendInternalIoctlSynchronously(
		WdfDeviceGetIoTarget(FxDevice),
		NULL,
		IOCTL_ACPI_EVAL_METHOD_EX,
		&inputMemDesc,
		&outputMemDesc,
		NULL,
		NULL
	);
	if (!NT_SUCCESS(status)) {
		goto Exit;
	}

	if (outputBuffer->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) {
		goto Exit;
	}

	if (outputBuffer->Count < 1) {
		goto Exit;
	}

	UINT32 uid;
	if (outputBuffer->Argument[0].DataLength >= 4) {
		uid = *(UINT32*)outputBuffer->Argument->Data;
	}
	else if (outputBuffer->Argument[0].DataLength >= 2) {
		uid = *(UINT16*)outputBuffer->Argument->Data;
	}
	else {
		uid = *(UINT8*)outputBuffer->Argument->Data;
	}
	if (PUID) {
		*PUID = uid;
	}
	else {
		status = STATUS_ACPI_INVALID_ARGUMENT;
	}
Exit:
	if (outputMemory != WDF_NO_HANDLE) {
		WdfObjectDelete(outputMemory);
	}
	return status;
}

// The IRQ status is latched. Keep the converter off across the whole release sequence.
#define CS35L41_BOOST_FAULTS \
    (CS35L41_BST_OVP_ERR | CS35L41_BST_DCM_UVP_ERR | CS35L41_BST_SHORT_ERR)
#define CS35L41_PROTECTION_FAULTS \
    (CS35L41_BOOST_FAULTS | CS35L41_AMP_SHORT_ERR | CS35L41_TEMP_ERR | CS35L41_TEMP_WARN)

static NTSTATUS cs35l41_recover_protection(PCS35L41_CONTEXT pDevice, UINT32 faults)
{
    static const struct {
        UINT32 Interrupt;
        UINT32 Release;
    } protection[] = {
        { CS35L41_AMP_SHORT_ERR, CS35L41_AMP_SHORT_ERR_RLS },
        { CS35L41_TEMP_WARN, CS35L41_TEMP_WARN_ERR_RLS },
        { CS35L41_TEMP_ERR, CS35L41_TEMP_ERR_RLS },
        { CS35L41_BST_OVP_ERR, CS35L41_BST_OVP_ERR_RLS },
        { CS35L41_BST_DCM_UVP_ERR, CS35L41_BST_UVP_ERR_RLS },
        { CS35L41_BST_SHORT_ERR, CS35L41_BST_SHORT_ERR_RLS },
    };

    UINT32 boostMode = 0;
    BOOLEAN allowRestart = FALSE;
    NTSTATUS status;
    if (faults & CS35L41_BOOST_FAULTS) {
        // Permit one automatic restart per D0, only for a voltage fault.
        allowRestart = !pDevice->BoostRecoveryUsed && !(faults & CS35L41_BST_SHORT_ERR);
        pDevice->BoostRecoveryUsed = TRUE;
        UINT32 power;
        status = cs35l41_reg_read(pDevice, CS35L41_PWR_CTRL2, &power);
        if (!NT_SUCCESS(status))
            return status;
        boostMode = power & CS35L41_BST_EN_MASK;
        if (boostMode != 0) {
            status = cs35l41_reg_write(pDevice, CS35L41_PWR_CTRL2, power & ~CS35L41_BST_EN_MASK);
            if (!NT_SUCCESS(status))
                return status;
        }
    }

    for (UINT32 i = 0; i < ARRAY_SIZE(protection); ++i) {
        if (!(faults & protection[i].Interrupt))
            continue;
        status = cs35l41_reg_write(pDevice, CS35L41_IRQ1_STATUS1, protection[i].Interrupt);
        if (!NT_SUCCESS(status))
            return status;
        status = cs35l41_reg_write(pDevice, CS35L41_PROTECT_REL_ERR_IGN, 0);
        if (!NT_SUCCESS(status))
            return status;
        status = cs35l41_reg_update_bits(pDevice, CS35L41_PROTECT_REL_ERR_IGN,
            protection[i].Release, protection[i].Release);
        if (!NT_SUCCESS(status))
            return status;
        status = cs35l41_reg_update_bits(pDevice, CS35L41_PROTECT_REL_ERR_IGN,
            protection[i].Release, 0);
        if (!NT_SUCCESS(status))
            return status;
    }

    // Snapshot the other power fields before the last status check.
    BOOLEAN restoreBoost = allowRestart &&
        boostMode == (CS35L41_BST_EN_DEFAULT << CS35L41_BST_EN_SHIFT);
    UINT32 power = 0;
    if (restoreBoost) {
        status = cs35l41_reg_read(pDevice, CS35L41_PWR_CTRL2, &power);
        if (!NT_SUCCESS(status))
            return status;
        if (power & CS35L41_BST_EN_MASK)
            return STATUS_DEVICE_BUSY;
    }

    // Keep this read adjacent to the restore write.
    UINT32 remaining;
    status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS1, &remaining);
    if (!NT_SUCCESS(status))
        return status;
    if (remaining & CS35L41_PROTECTION_FAULTS)
        return STATUS_DEVICE_BUSY;
    if (restoreBoost)
        return cs35l41_reg_write(pDevice, CS35L41_PWR_CTRL2, power | boostMode);

    return STATUS_SUCCESS;
}

BOOLEAN
OnInterruptIsr(
    IN WDFINTERRUPT Interrupt,
    IN ULONG MessageID
)
{
    UNREFERENCED_PARAMETER(MessageID);
    PCS35L41_CONTEXT pDevice = GetDeviceContext(WdfInterruptGetDevice(Interrupt));
    UINT32 status1, mask1;
    NTSTATUS status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS1, &status1);
    if (!NT_SUCCESS(status))
        return FALSE;
    status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_MASK1, &mask1);
    if (!NT_SUCCESS(status))
        return FALSE;

    UINT32 faults = status1 & ~mask1 & CS35L41_PROTECTION_FAULTS;
    if (faults) {
        // Record the latched status before the first acknowledgement or power write.
        TraceLoggingWrite(g_Cs35l41ProtectionProvider, "ProtectionInterrupt",
            TraceLoggingLevel(TRACE_LEVEL_WARNING),
            TraceLoggingUInt32((UINT32)pDevice->UID, "AmplifierUid"),
            TraceLoggingHexUInt32(status1, "InterruptStatus"),
            TraceLoggingHexUInt32(mask1, "InterruptMask"),
            TraceLoggingHexUInt32(faults, "ActiveFaults"));
        status = cs35l41_recover_protection(pDevice, faults);
        TraceLoggingWrite(g_Cs35l41ProtectionProvider, "ProtectionRecovery",
            TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
            TraceLoggingUInt32((UINT32)pDevice->UID, "AmplifierUid"),
            TraceLoggingHexUInt32(faults, "ActiveFaults"),
            TraceLoggingNTStatus(status, "Result"));
        if (!NT_SUCCESS(status)) {
            Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "Codec %d protection fault 0x%X, recovery stopped: %!STATUS!\n",
                pDevice->UID, faults, status);
            return FALSE;
        }
    }

    UINT32 status4;
    status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS4, &status4);
    if (NT_SUCCESS(status) && (status4 & CS35L41_OTP_BOOT_DONE)) {
        UINT32 mask4;
        status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_MASK4, &mask4);
        if (NT_SUCCESS(status) && !(mask4 & CS35L41_OTP_BOOT_DONE)) {
            status = cs35l41_reg_write(pDevice, CS35L41_IRQ1_MASK4,
                mask4 | CS35L41_OTP_BOOT_DONE);
            return NT_SUCCESS(status) || faults != 0;
        }
    }
    return faults != 0;
}

static NTSTATUS cs35l41_verify_playback_power(PCS35L41_CONTEXT pDevice)
{
    // HyperOS mixer_paths_overlay_static.xml sets VCTRL1 target 0 and Class-H tracking.
    // Keep the analog gain at the original map220v value for the first DSP qualification.
    const UINT32 boostTargetMask = 0xFF;
    NTSTATUS status = cs35l41_reg_update_bits(pDevice, CS35L41_BSTCVRT_VCTRL1, boostTargetMask, 0);
    if (!NT_SUCCESS(status))
        return status;
    status = cs35l41_reg_update_bits(pDevice, CS35L41_BSTCVRT_VCTRL2,
        CS35L41_BST_CTL_SEL_MASK, CS35L41_BST_CTL_SEL_CLASSH);
    if (!NT_SUCCESS(status))
        return status;

    static const struct {
        UINT32 Reg;
        UINT32 Mask;
        UINT32 Value;
    } expected[] = {
        { CS35L41_BSTCVRT_VCTRL1, 0xFF, 0 },
        { CS35L41_BSTCVRT_VCTRL2, CS35L41_BST_CTL_SEL_MASK, CS35L41_BST_CTL_SEL_CLASSH },
        { CS35L41_AMP_GAIN_CTRL, 0x3E0, 10U << 5 },
        { CS35L41_VPBR_CFG, 0x7FFFFFF, 33575688 },
    };
    for (UINT32 index = 0; index < ARRAY_SIZE(expected); index++) {
        UINT32 actual;
        status = cs35l41_reg_read(pDevice, expected[index].Reg, &actual);
        if (!NT_SUCCESS(status))
            return status;
        if ((actual & expected[index].Mask) != expected[index].Value) {
            Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
                "UID %d: playback register 0x%x read back as 0x%x\n",
                pDevice->UID, expected[index].Reg, actual);
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }
    }
    return STATUS_SUCCESS;
}

static VOID cs35l41_trace_speaker_protection(PCS35L41_CONTEXT pDevice, BOOLEAN onStart)
{
    if (!pDevice->dsp.running ||
        !TraceLoggingProviderEnabled(g_Cs35l41SpeakerProvider, TRACE_LEVEL_INFORMATION, 0))
        return;

    UINT32 counters[CS35L41_BDLOG_WORD_COUNT] = { 0 };
    NTSTATUS status = cs35l41_dsp_get_speaker_protection(pDevice, counters);
    TraceLoggingWrite(g_Cs35l41SpeakerProvider, "SpeakerProtectionSnapshot",
        TraceLoggingLevel(TRACE_LEVEL_INFORMATION),
        TraceLoggingUInt32((UINT32)pDevice->UID, "AmplifierUid"),
        TraceLoggingBool(onStart, "OnStart"),
        TraceLoggingHexUInt32((UINT32)status, "ReadStatus"),
        TraceLoggingUInt32(counters[0], "MaxTempRaw"),
        TraceLoggingUInt32(counters[1], "MaxExcursionRaw"),
        TraceLoggingUInt32(counters[2], "OverTempCount"),
        TraceLoggingUInt32(counters[3], "OverExcursionCount"),
        TraceLoggingUInt32(counters[4], "AbnormalMute"),
        TraceLoggingUInt32(counters[5], "VmonLowFlag"));
}

static NTSTATUS cs35l41_configure_irq_gpio(PCS35L41_CONTEXT pDevice)
{
    // Preserve the original input GPIO1, active-low GPIO2 interrupt function
    // and protection mask. A failed transaction must stop startup before DSP
    // or amplifier enablement, instead of silently losing fault reporting.
    const UINT32 gpioMask = CS35L41_GPIO_POL_MASK | CS35L41_GPIO_DIR_MASK;
    NTSTATUS status = cs35l41_reg_update_bits(pDevice, CS35L41_GPIO1_CTRL1,
        gpioMask, 1U << CS35L41_GPIO_DIR_SHIFT);
    if (!NT_SUCCESS(status))
        return status;
    status = cs35l41_reg_update_bits(pDevice, CS35L41_GPIO2_CTRL1, gpioMask, 0);
    if (!NT_SUCCESS(status))
        return status;
    status = cs35l41_reg_update_bits(pDevice, CS35L41_GPIO_PAD_CONTROL,
        CS35L41_GPIO2_CTRL_MASK, 4U << CS35L41_GPIO2_CTRL_SHIFT);
    if (!NT_SUCCESS(status))
        return status;
    status = cs35l41_reg_write(pDevice, CS35L41_IRQ1_MASK1, CS35L41_INT1_MASK_DEFAULT);
    if (!NT_SUCCESS(status))
        return status;

    static const struct { UINT32 Reg; UINT32 Mask; UINT32 Expected; } checks[] = {
        { CS35L41_GPIO1_CTRL1, CS35L41_GPIO_POL_MASK | CS35L41_GPIO_DIR_MASK,
            1U << CS35L41_GPIO_DIR_SHIFT },
        { CS35L41_GPIO2_CTRL1, CS35L41_GPIO_POL_MASK | CS35L41_GPIO_DIR_MASK, 0 },
        { CS35L41_GPIO_PAD_CONTROL, CS35L41_GPIO2_CTRL_MASK,
            4U << CS35L41_GPIO2_CTRL_SHIFT },
        { CS35L41_IRQ1_MASK1, 0xffffffffU, CS35L41_INT1_MASK_DEFAULT }
    };
    for (UINT32 index = 0; index < sizeof(checks) / sizeof(checks[0]); index++) {
        UINT32 actual;
        status = cs35l41_reg_read(pDevice, checks[index].Reg, &actual);
        if (!NT_SUCCESS(status))
            return status;
        if ((actual & checks[index].Mask) != checks[index].Expected)
            return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS
StartCodec(
	PCS35L41_CONTEXT pDevice
) {
	NTSTATUS status = STATUS_SUCCESS;
	if (!pDevice->SetUID) {
		status = STATUS_ACPI_INVALID_DATA;
		return status;
	}

	UINT32 int_status;
	INT32 timeout = 100;
	do {
		if (timeout == 0) {
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_PNP,
				"Timeout waiting for OTP_BOOT_DONE\n");
			status = STATUS_DEVICE_BUSY;
			return status;
		}
		udelay(1000);
		status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS4, &int_status);
		if (!NT_SUCCESS(status))
			return status;
		timeout--;
	} while (!(int_status & CS35L41_OTP_BOOT_DONE));

	status = cs35l41_reg_read(pDevice, CS35L41_IRQ1_STATUS3, &int_status);
	if (!NT_SUCCESS(status))
		return status;
	if (int_status & CS35L41_OTP_BOOT_ERR) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_PNP,
			"OTP Boot error\n");
		status = STATUS_INVALID_PARAMETER;
		return status;
	}

	UINT32 deviceId;
	status = cs35l41_reg_read(pDevice, CS35L41_DEVID, &deviceId);
	if (!NT_SUCCESS(status))
		return status;

	if (deviceId != CS35L41_CHIP_ID) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Invalid device id (0x%x), expected 0x%x\n", deviceId, CS35L41_CHIP_ID);
		status = STATUS_IO_DEVICE_ERROR;
		return status;
	}

	UINT32 reg_revid = 0;
	status = cs35l41_reg_read(pDevice, CS35L41_REVID, &reg_revid);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_PNP,
		"CS35L41 revision 0x%02x\n", reg_revid);

	// Keep the amplifier off until the protected DSP path has been configured.
	status = cs35l41_reg_update_bits(pDevice, CS35L41_PWR_CTRL1, CS35L41_GLOBAL_EN_MASK, 0);
	if (!NT_SUCCESS(status))
		return status;

	static const struct reg_sequence b2Patch[] = {
		{ CS35L41_VIMON_SPKMON_RESYNC, 0x00000000 },
		{ 0x00004310, 0x00000000 },
		{ CS35L41_VPVBST_FS_SEL, 0x00000000 },
		{ CS35L41_BSTCVRT_DCM_CTRL, 0x00000051 },
		{ CS35L41_DSP1_YM_ACCEL_PL0_PRI, 0x00000000 },
		{ CS35L41_DSP1_XM_ACCEL_PL0_PRI, 0x00000000 },
		{ CS35L41_PWR_CTRL2, 0x00000000 },
		{ CS35L41_AMP_GAIN_CTRL, 0x00000000 },
		{ CS35L41_ASP_TX3_SRC, 0x00000000 },
		{ CS35L41_ASP_TX4_SRC, 0x00000000 },
	};
	status = cs35l41_set_test_key(pDevice, TRUE);
	if (NT_SUCCESS(status) && reg_revid == CS35L41_REVID_B2)
		status = cs35l41_multi_reg_write(pDevice, b2Patch, ARRAY_SIZE(b2Patch));
	if (NT_SUCCESS(status))
		status = cs35l41_otp_unpack(pDevice);

	NTSTATUS lockStatus = cs35l41_set_test_key(pDevice, FALSE);
	if (!NT_SUCCESS(status))
		return status;
	if (!NT_SUCCESS(lockStatus))
		return lockStatus;

	status = cs35l41_reg_write(pDevice, CS35L41_DSP1_CCM_CORE_CTRL, 0);
	if (!NT_SUCCESS(status))
		return status;

    status = cs35l41_configure_irq_gpio(pDevice);
    TraceLoggingWrite(g_Cs35l41SpeakerProvider, "NabuInterruptConfigurationCheck",
        TraceLoggingUInt32((UINT32)pDevice->UID, "AmplifierUid"),
        TraceLoggingNTStatus(status, "Result"));
    if (!NT_SUCCESS(status))
        return status;

	//Load DSP firmware
	status = cs35l41_dsp_init(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	//Setup algorithm coefficients for nabu
	status = cs35l41_dsp_configure(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_dsp_run(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	//DSP1 on
	status = cs35l41_dsp_mbox_start(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	//cirrus,boost-peak-milliamp
	INT32 bst_ipk = 4000;
	//cirrus,boost-ind-nanohenry
	INT32 bst_ind = 1000;
	//cirrus,boost-cap-microfarad
	INT32 bst_cap = 15;

	/* Required */
	status = cs35l41_boost_config(pDevice, bst_ind, bst_cap, bst_ipk);
	if (!NT_SUCCESS(status))
		return status;

	// cirrus,asp-sdout-hiz
	status = cs35l41_reg_update_bits(pDevice, CS35L41_SP_HIZ_CTRL, CS35L41_ASP_DOUT_HIZ_MASK, 3);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_set_dai_fmt(pDevice);
	if (!NT_SUCCESS(status))
		return status;
    status = cs35l41_pcm_hw_params(pDevice);
    if (!NT_SUCCESS(status))
        return status;

    status = cs35l41_dsp_apply_stock_music(pDevice);
    if (!NT_SUCCESS(status))
        return status;

    status = cs35l41_dsp_apply_stock_channel(pDevice);
    TraceLoggingWrite(g_Cs35l41SpeakerProvider, "NabuSpeakerRoutingCheck",
        TraceLoggingUInt32((UINT32)pDevice->UID, "AmplifierUid"),
        TraceLoggingNTStatus(status, "Result"));
    if (!NT_SUCCESS(status))
        return status;

	static const struct reg_sequence playbackRegisters[] = {
		{ CS35L41_AMP_DIG_VOL_CTRL, 0x00008004 },
		{ CS35L41_SP_ENABLES, 0x00030001 },
		{ CS35L41_DAC_PCM1_SRC, CS35L41_INPUT_DSP_TX1 },
		{ CS35L41_ASP_TX1_SRC, CS35L41_INPUT_DSP_TX1 },
		{ CS35L41_ASP_TX2_SRC, 0 },
		{ CS35L41_ASP_TX3_SRC, 0 },
		{ CS35L41_ASP_TX4_SRC, 0 },
		{ CS35L41_AMP_GAIN_CTRL, 0x00000153 },
		{ CS35L41_PWR_CTRL2, 0x00003721 },
		{ CS35L41_PWR_CTRL3, 0x01100010 },
	};
	status = cs35l41_multi_reg_write(pDevice, playbackRegisters, ARRAY_SIZE(playbackRegisters));
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_VPBR_CFG, 0x7FFFFFF, 33575688);
	if (!NT_SUCCESS(status))
		return status;
	status = cs35l41_reg_update_bits(pDevice, CS35L41_NG_CFG, 0x3FFF, 16245);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_verify_playback_power(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	status = cs35l41_amp_enable(pDevice);
	if (!NT_SUCCESS(status)) {
		cs35l41_amp_disable(pDevice);
		return status;
	}

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
		"Started! REV:%X UID:%d\n", reg_revid, pDevice->UID);

	pDevice->BoostRecoveryUsed = FALSE;
	pDevice->DevicePoweredOn = TRUE;
	cs35l41_trace_speaker_protection(pDevice, TRUE);
	return status;
}

NTSTATUS
StopCodec(
	PCS35L41_CONTEXT pDevice
) {
	NTSTATUS pauseStatus = STATUS_SUCCESS;
	cs35l41_trace_speaker_protection(pDevice, FALSE);
	if (pDevice->dsp.running)
		pauseStatus = cs35l41_set_cspl_mbox_cmd(pDevice, CSPL_MBOX_CMD_PAUSE);

	NTSTATUS status = cs35l41_amp_disable(pDevice);
	cs35l41_dsp_reset(pDevice);

	pDevice->DevicePoweredOn = FALSE;
	return NT_SUCCESS(status) ? pauseStatus : status;
}

NTSTATUS
OnPrepareHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesRaw,
	_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

This routine caches the SPB resource connection ID.

Arguments:

FxDevice - a handle to the framework device object
FxResourcesRaw - list of translated hardware resources that
the PnP manager has assigned to the device
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	PCS35L41_CONTEXT pDevice = GetDeviceContext(FxDevice);
	BOOLEAN fSpbResourceFound = FALSE;
	NTSTATUS status = STATUS_INSUFFICIENT_RESOURCES;

	UNREFERENCED_PARAMETER(FxResourcesRaw);

	//
	// Parse the peripheral's resources.
	//

	ULONG resourceCount = WdfCmResourceListGetCount(FxResourcesTranslated);

	for (ULONG i = 0; i < resourceCount; i++)
	{
		PCM_PARTIAL_RESOURCE_DESCRIPTOR pDescriptor;
		UCHAR Class;
		UCHAR Type;

		pDescriptor = WdfCmResourceListGetDescriptor(
			FxResourcesTranslated, i);

		switch (pDescriptor->Type)
		{
		case CmResourceTypeConnection:
			//
			// Look for I2C or SPI resource and save connection ID.
			//
			Class = pDescriptor->u.Connection.Class;
			Type = pDescriptor->u.Connection.Type;
			if (Class == CM_RESOURCE_CONNECTION_CLASS_SERIAL &&
				Type == CM_RESOURCE_CONNECTION_TYPE_SERIAL_I2C)
			{
				if (fSpbResourceFound == FALSE)
				{
					status = STATUS_SUCCESS;
					pDevice->I2CContext.I2cResHubId.LowPart = pDescriptor->u.Connection.IdLowPart;
					pDevice->I2CContext.I2cResHubId.HighPart = pDescriptor->u.Connection.IdHighPart;
					fSpbResourceFound = TRUE;
				}
				else
				{
				}
			}
			break;
		default:
			//
			// Ignoring all other resource types.
			//
			break;
		}
	}

	//
	// An SPB resource is required.
	//

	if (fSpbResourceFound == FALSE)
	{
		status = STATUS_NOT_FOUND;
	}

	status = SpbTargetInitialize(FxDevice, &pDevice->I2CContext);

	if (!NT_SUCCESS(status))
	{
		return status;
	}

	status = GetDeviceUID(FxDevice, &pDevice->UID);
	if (!NT_SUCCESS(status)) {
		return status;
	}
	pDevice->SetUID = TRUE;

	return status;
}

NTSTATUS
OnReleaseHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesTranslated
)
/*++

Routine Description:

Arguments:

FxDevice - a handle to the framework device object
FxResourcesTranslated - list of raw hardware resources that
the PnP manager has assigned to the device

Return Value:

Status

--*/
{
	PCS35L41_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	SpbTargetDeinitialize(FxDevice, &pDevice->I2CContext);

	return status;
}

NTSTATUS
OnD0Entry(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
)
/*++

Routine Description:

This routine allocates objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxPreviousState - previous power state

Return Value:

Status

--*/
{
	UNREFERENCED_PARAMETER(FxPreviousState);

	PCS35L41_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	status = StartCodec(pDevice);
	if (!NT_SUCCESS(status))
		cs35l41_dsp_reset(pDevice);
	return status;
}

NTSTATUS
OnD0Exit(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
)
/*++

Routine Description:

This routine destroys objects needed by the driver.

Arguments:

FxDevice - a handle to the framework device object
FxPreviousState - previous power state

Return Value:

Status

--*/
{
	UNREFERENCED_PARAMETER(FxPreviousState);

	PCS35L41_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	status = StopCodec(pDevice);
	if (!NT_SUCCESS(status))
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_PNP, "StopCodec failed: %!STATUS!\n", status);

	return STATUS_SUCCESS;
}

NTSTATUS
Cs35l41EvtDeviceAdd(
	IN WDFDRIVER       Driver,
	IN PWDFDEVICE_INIT DeviceInit
)
{
	NTSTATUS                      status = STATUS_SUCCESS;
	WDF_INTERRUPT_CONFIG          interruptConfig;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDFDEVICE                     device;
	PCS35L41_CONTEXT              devContext;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_PNP,
		"Cs35l41EvtDeviceAdd called\n");

	{
		WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
		WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

		pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
		pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
		pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
		pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;

		WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
	}

	//
	// Setup the device context
	//

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CS35L41_CONTEXT);

	//
	// Create a framework device object.This call will in turn create
	// a WDM device object, attach to the lower stack, and set the
	// appropriate flags and attributes.
	//

	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

	if (!NT_SUCCESS(status))
	{
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceCreate failed with status code 0x%x\n", status);

		return status;
	}

	devContext = GetDeviceContext(device);
	devContext->FxDevice = device;

	//
	// Create an interrupt object for hardware notifications
	//
	WDF_INTERRUPT_CONFIG_INIT(
		&interruptConfig,
		OnInterruptIsr,
		NULL);
	interruptConfig.PassiveHandling = TRUE;

	status = WdfInterruptCreate(
		device,
		&interruptConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->InterruptObject);

	return status;
}

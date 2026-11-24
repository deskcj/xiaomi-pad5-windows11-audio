#include "cs35l41.h"
#include "cs35l41_dsp.h"

#define WMFW_PROT_BUFFER_SIZE 0x85BC
#define COEF_PROT_BUFFER_SIZE 0x14DC

static ULONG Cs35l41DebugLevel = 100;
static ULONG Cs35l41DebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

static const struct cs_dsp_region cs35l41_dsp1_regions[] = {
	{ .type = WMFW_HALO_PM_PACKED,	.base = CS35L41_DSP1_PMEM_0 },
	{ .type = WMFW_HALO_XM_PACKED,	.base = CS35L41_DSP1_XMEM_PACK_0 },
	{ .type = WMFW_HALO_YM_PACKED,	.base = CS35L41_DSP1_YMEM_PACK_0 },
	{. type = WMFW_ADSP2_XM,	.base = CS35L41_DSP1_XMEM_UNPACK24_0},
	{. type = WMFW_ADSP2_YM,	.base = CS35L41_DSP1_YMEM_UNPACK24_0},
};

static const struct reg_sequence cs35l41_fs_errata_patch[] = {
	{ CS35L41_DSP1_RX1_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX2_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX3_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX4_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX5_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX6_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX7_RATE,	0x00000001 },
	{ CS35L41_DSP1_RX8_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX1_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX2_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX3_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX4_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX5_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX6_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX7_RATE,	0x00000001 },
	{ CS35L41_DSP1_TX8_RATE,	0x00000001 },
};

NTSTATUS cs35l41_write_fs_errata(PCS35L41_CONTEXT pDevice)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = cs35l41_multi_reg_write(pDevice, cs35l41_fs_errata_patch,
				     ARRAY_SIZE(cs35l41_fs_errata_patch));
	if (!NT_SUCCESS(status))
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to write fs errata: 0x%x\n", status);

	return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS ReadRegistryString(PCS35L41_CONTEXT pDevice, PWSTR Name, UNICODE_STRING *Value)
{
    WDFKEY key = NULL;
    NTSTATUS status = WdfDeviceOpenRegistryKey(pDevice->FxDevice,
        PLUGPLAY_REGKEY_DRIVER, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &key);
    if (!NT_SUCCESS(status))
        return status;

    WDFSTRING value;
    status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &value);
    if (NT_SUCCESS(status)) {
        UNICODE_STRING name;
        RtlInitUnicodeString(&name, Name);
        status = WdfRegistryQueryString(key, &name, value);
        if (NT_SUCCESS(status)) {
            UNICODE_STRING result;
            WdfStringGetUnicodeString(value, &result);
            if (result.Length == 0)
                status = STATUS_INVALID_PARAMETER;
            else
                status = RtlUnicodeStringCopy(Value, &result);
        }
        WdfObjectDelete(value);
    }
    WdfRegistryClose(key);
    return status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS CopyFileToBuffer(UNICODE_STRING FileDOSPath, UINT8 *Buffer, ULONG Length)
{
    HANDLE handle;
    OBJECT_ATTRIBUTES attributes;
    IO_STATUS_BLOCK io;
    InitializeObjectAttributes(&attributes, &FileDOSPath,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    NTSTATUS status = ZwOpenFile(&handle, GENERIC_READ | SYNCHRONIZE,
        &attributes, &io, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status))
        return status;

    LARGE_INTEGER offset = { 0 };
    status = ZwReadFile(handle, NULL, NULL, NULL, &io, Buffer, Length, &offset, NULL);
    if (NT_SUCCESS(status) && io.Information != Length)
        status = STATUS_END_OF_FILE;
    ZwClose(handle);
    return status;
}

UINT32 parse_sizes(UINT32 pos, UINT8 *wmfw_firmware)
{
	const struct wmfw_adsp2_sizes *adsp2_sizes;

	adsp2_sizes = (void *)&wmfw_firmware[pos];

	Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
		"wmfw: %d XM, %d YM %d PM, %d ZM\n",
		adsp2_sizes->xm, adsp2_sizes->ym,
		adsp2_sizes->pm, adsp2_sizes->zm);

	return pos + sizeof(*adsp2_sizes);
}

static const struct cs_dsp_region *cs_dsp_find_region(INT32 type)
{
	INT32 i;

	for (i = 0; i < ARRAY_SIZE(cs35l41_dsp1_regions); i++)
		if (cs35l41_dsp1_regions[i].type == type)
			return &cs35l41_dsp1_regions[i];

	return NULL;
}

static const char *cs_dsp_mem_region_name(UINT32 type)
{
	switch (type) {
	case WMFW_ADSP1_PM:
		return "PM";
	case WMFW_HALO_PM_PACKED:
		return "PM_PACKED";
	case WMFW_ADSP1_DM:
		return "DM";
	case WMFW_ADSP2_XM:
		return "XM";
	case WMFW_HALO_XM_PACKED:
		return "XM_PACKED";
	case WMFW_ADSP2_YM:
		return "YM";
	case WMFW_HALO_YM_PACKED:
		return "YM_PACKED";
	case WMFW_ADSP1_ZM:
		return "ZM";
	default:
		return NULL;
	}
}

static UINT32 region_to_reg(struct cs_dsp_region const *mem, UINT32 offset)
{
	switch (mem->type) {
	case WMFW_ADSP2_XM:
	case WMFW_ADSP2_YM:
		return mem->base + (offset * 4);
	case WMFW_HALO_XM_PACKED:
	case WMFW_HALO_YM_PACKED:
		return (mem->base + (offset * 3)) & ~0x3;
	case WMFW_HALO_PM_PACKED:
		return mem->base + (offset * 5);
	default:
		Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
			"Unknown memory region type");
		return offset;
	}
}

static INT32 cs_dsp_coeff_parse_int(INT32 bytes, const UINT8 **pos)
{
	INT32 val = 0;

	switch (bytes) {
	case 2:
		val = *((UINT16 *)*pos);
		break;
	case 4:
		val = *((UINT32 *)*pos);
		break;
	default:
		break;
	}

	*pos += bytes;

	return val;
}

static INT32 cs_dsp_coeff_parse_string(INT32 bytes, const UINT8 **pos, const UINT8 **str)
{
	INT32 length;

	switch (bytes) {
	case 1:
		length = **pos;
		break;
	case 2:
		length = *((UINT16 *)*pos);
		break;
	default:
		return 0;
	}

	if (str)
		*str = *pos + bytes;

	*pos += ((length + bytes) + 3) & ~0x03;

	return length;
}

static inline void cs_dsp_coeff_parse_alg(const UINT8 **data,
					  struct cs_dsp_coeff_parsed_alg *blk)
{
	const struct wmfw_adsp_alg_data *raw;
	
	blk->id = cs_dsp_coeff_parse_int(sizeof(raw->id), data);
	blk->name_len = cs_dsp_coeff_parse_string(sizeof(UINT8), data,
							&blk->name);
	cs_dsp_coeff_parse_string(sizeof(UINT16), data, NULL);
	blk->ncoeff = cs_dsp_coeff_parse_int(sizeof(raw->ncoeff), data);

	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"Algorithm ID: %#x\n", blk->id);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"Algorithm name: %.*s\n", blk->name_len, blk->name);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"# of coefficient descriptors: %#x\n", blk->ncoeff);
}

static inline void cs_dsp_coeff_parse_coeff(const UINT8 **data,
					    struct cs_dsp_coeff_parsed_coeff *blk)
{
	const struct wmfw_adsp_coeff_data *raw;
	const UINT8 *tmp;
	INT32 length;

	tmp = *data;
	blk->offset = cs_dsp_coeff_parse_int(sizeof(raw->hdr.offset), &tmp);
	blk->mem_type = cs_dsp_coeff_parse_int(sizeof(raw->hdr.type), &tmp);
	length = cs_dsp_coeff_parse_int(sizeof(raw->hdr.size), &tmp);
	blk->name_len = cs_dsp_coeff_parse_string(sizeof(UINT8), &tmp,
							&blk->name);
	cs_dsp_coeff_parse_string(sizeof(UINT8), &tmp, NULL);
	cs_dsp_coeff_parse_string(sizeof(UINT16), &tmp, NULL);
	blk->ctl_type = cs_dsp_coeff_parse_int(sizeof(raw->ctl_type), &tmp);
	blk->flags = cs_dsp_coeff_parse_int(sizeof(raw->flags), &tmp);
	blk->len = cs_dsp_coeff_parse_int(sizeof(raw->len), &tmp);

	*data = *data + sizeof(raw->hdr) + length;

	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tCoefficient type: %#x\n", blk->mem_type);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tCoefficient offset: %#x\n", blk->offset);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tCoefficient name: %.*s\n", blk->name_len, blk->name);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tCoefficient flags: %#x\n", blk->flags);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tALSA control type: %#x\n", blk->ctl_type);
	Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
		"\tALSA control len: %#x\n", blk->len);
}

static NTSTATUS cs_dsp_parse_coeff(const struct wmfw_region *region)
{
	NTSTATUS status = STATUS_SUCCESS;

	struct cs_dsp_alg_region alg_region;
	struct cs_dsp_coeff_parsed_alg alg_blk;
	struct cs_dsp_coeff_parsed_coeff coeff_blk;
	const UINT8 *data = region->data;
	INT32 i;

	cs_dsp_coeff_parse_alg(&data, &alg_blk);
	for (i = 0; i < alg_blk.ncoeff; i++) {
		cs_dsp_coeff_parse_coeff(&data, &coeff_blk);

		switch (coeff_blk.ctl_type) {
		case WMFW_CTL_TYPE_BYTES:
			break;
		case WMFW_CTL_TYPE_ACKED:
			if (coeff_blk.flags & WMFW_CTL_FLAG_SYS)
				continue;	/* ignore */

			break;
		case WMFW_CTL_TYPE_HOSTEVENT:
		case WMFW_CTL_TYPE_FWEVENT:
			break;
		case WMFW_CTL_TYPE_HOST_BUFFER:
			break;
		default:
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
				"Unknown control type: %d\n",
				coeff_blk.ctl_type);
			return STATUS_UNSUCCESSFUL;
		}

		alg_region.type = coeff_blk.mem_type;
		alg_region.alg = alg_blk.id;
	}

	return status;
}

// The stock HALO loader advances byte addresses and rounds each 4 KiB
// burst down to the packing unit of its memory window.
static NTSTATUS cs_dsp_write_blocks(PCS35L41_CONTEXT pDevice, UINT32 reg,
	const UINT8 *data, UINT32 len, UINT32 type)
{
	UINT32 burstMultiple = sizeof(UINT32);
	switch (type) {
	case WMFW_HALO_PM_PACKED:
		burstMultiple = 20;
		break;
	case WMFW_HALO_XM_PACKED:
	case WMFW_HALO_YM_PACKED:
		burstMultiple = 12;
		break;
	}
	const UINT32 maxBurst = 4096 - (4096 % burstMultiple);
	if ((reg % sizeof(UINT32)) != 0 || (len % sizeof(UINT32)) != 0 ||
		len > MAXUINT32 - reg)
		return STATUS_INVALID_IMAGE_FORMAT;

	while (len != 0) {
		UINT32 chunk = min(len, maxBurst);
		NTSTATUS status = cs35l41_reg_bulk_write(pDevice, reg, data, chunk);
		if (!NT_SUCCESS(status))
			return status;

		data += chunk;
		reg += chunk;
		len -= chunk;
	}

	return STATUS_SUCCESS;
}

NTSTATUS cs_dsp_load(PCS35L41_CONTEXT pDevice, UINT8 *wmfw_firmware)
{
	NTSTATUS status = STATUS_SUCCESS;

	UINT32 pos = 0;
	const struct wmfw_header *header;
	const struct wmfw_footer *footer;
	const struct wmfw_region *region;
	const struct cs_dsp_region *mem;
	const char *region_name;
	UINT32 reg;
	INT32 regions = 0;
	INT32 offset, type;

	header = (VOID *)&wmfw_firmware[0];

	if (memcmp(&header->magic[0], "WMFW", 4) != 0) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"wmfw: invalid magic\n");
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	if (header->ver != 3) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"wmfw: unknown file format %d\n", header->ver);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	//WMFW_HALO
	if (header->core != WMFW_HALO) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"wmfw: invalid core %d != 4(WMFW_HALO)\n", header->core);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	pos = sizeof(*header);
	pos = parse_sizes(pos, wmfw_firmware);

	footer = (VOID *)&wmfw_firmware[pos];
	pos += sizeof(*footer);

	if (header->len != pos) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"wmfw: unexpected header length %d\n", header->len);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
		"wmfw: timestamp %llu\n", footer->timestamp);

	while (pos < WMFW_PROT_BUFFER_SIZE) {
		UINT32 remaining = WMFW_PROT_BUFFER_SIZE - pos;
		if (remaining < sizeof(*region))
			return STATUS_INVALID_IMAGE_FORMAT;

		region = (void *)&(wmfw_firmware[pos]);
		if (region->len > remaining - sizeof(*region))
			return STATUS_INVALID_IMAGE_FORMAT;
		region_name = "Unknown";
		reg = 0;
		offset = region->offset & 0xffffff;
		type = _byteswap_ulong(region->type) & 0xff;

		switch (type) {
		case WMFW_NAME_TEXT:
			region_name = "Firmware name";
			break;
		case WMFW_ALGORITHM_DATA:
			region_name = "Algorithm";
			status = cs_dsp_parse_coeff(region);
			if (!NT_SUCCESS(status))
				goto end;
			break;
		case WMFW_INFO_TEXT:
			region_name = "Information";
			break;
		case WMFW_ABSOLUTE:
			region_name = "Absolute";
			reg = offset;
			break;
		case WMFW_ADSP1_PM:
		case WMFW_ADSP1_DM:
		case WMFW_ADSP2_XM:
		case WMFW_ADSP2_YM:
		case WMFW_ADSP1_ZM:
		case WMFW_HALO_PM_PACKED:
		case WMFW_HALO_XM_PACKED:
		case WMFW_HALO_YM_PACKED:
			mem = cs_dsp_find_region(type);
			if (!mem) {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"No region of type: %x\n", type);
				status = STATUS_UNSUCCESSFUL;
				goto end;
			}

			region_name = cs_dsp_mem_region_name(type);
			reg = region_to_reg(mem, offset);
			break;
		default:
			Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
				    "wmfw.%d: Unknown region type %x at %d(%x)\n",
				    regions, type, pos, pos);
			break;
		}

		Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
			"wmfw.%d: %d bytes at %d in %s\n",
			regions, region->len, offset,
			region_name);

		if (reg) {
			status = cs_dsp_write_blocks(pDevice, reg, region->data, region->len, type);
			if (!NT_SUCCESS(status)) {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"wmfw.%d: Failed to write %d bytes at %d in %s: %d\n",
					regions, region->len, offset, region_name, status);
				goto end;
			}
		}

		pos += region->len + sizeof(*region);
		regions++;
	}

end:
	return status;
}

static void cs_dsp_parse_wmfw_v3_id_header(struct cs_dsp *dsp,
					   struct wmfw_v3_id_hdr *fw, size_t nalgs)
{
	UNREFERENCED_PARAMETER(nalgs);
	dsp->fw_id = _byteswap_ulong(fw->id);
	dsp->fw_id_version = _byteswap_ulong(fw->ver);
	dsp->fw_vendor_id = _byteswap_ulong(fw->vendor_id);

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
		"Firmware: %x vendor: 0x%x v%d.%d.%d, %d algorithms\n",
		dsp->fw_id, dsp->fw_vendor_id,
		(dsp->fw_id_version & 0xff0000) >> 16,
		(dsp->fw_id_version & 0xff00) >> 8, dsp->fw_id_version & 0xff,
		nalgs);
}

static NTSTATUS cs_dsp_create_region(struct cs_dsp *dsp,
						      INT32 type, UINT32 id,
						      UINT32 ver, UINT32 base)
{
	struct cs_dsp_alg_region alg_region;

	alg_region.type = type;
	alg_region.alg = _byteswap_ulong(id);
	alg_region.ver = _byteswap_ulong(ver);
	alg_region.base = _byteswap_ulong(base);

	if (dsp->alg_regions == NULL && dsp->alg_regions_size == 0) {
		dsp->alg_regions = ExAllocatePool2(
			POOL_FLAG_NON_PAGED,
			sizeof(struct cs_dsp_alg_region),
			CS35L41_POOL_TAG
		);
		if (dsp->alg_regions == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
		RtlCopyMemory(dsp->alg_regions, &alg_region, sizeof(struct cs_dsp_alg_region));
		dsp->alg_regions_size = sizeof(alg_region);
	}
	else {
		struct cs_dsp_alg_region *new_alg_regions = ExAllocatePool2(
			POOL_FLAG_NON_PAGED,
			dsp->alg_regions_size + sizeof(alg_region),
			CS35L41_POOL_TAG
		);
		if (new_alg_regions == NULL)
			return STATUS_INSUFFICIENT_RESOURCES;
		RtlCopyMemory(new_alg_regions, dsp->alg_regions, dsp->alg_regions_size);
		ExFreePoolWithTag(dsp->alg_regions, CS35L41_POOL_TAG);
		RtlCopyMemory(new_alg_regions + (dsp->alg_regions_size / sizeof(struct cs_dsp_alg_region)), &alg_region, sizeof(struct cs_dsp_alg_region));
		dsp->alg_regions = new_alg_regions;
		dsp->alg_regions_size = dsp->alg_regions_size + sizeof(struct cs_dsp_alg_region);
	}

	return STATUS_SUCCESS;
}

static NTSTATUS cs_dsp_create_regions(struct cs_dsp *dsp, UINT32 id, UINT32 ver,
				 INT32 nregions, const INT32 *type, UINT32 *base)
{
	NTSTATUS status = STATUS_SUCCESS;
	INT32 i;

	for (i = 0; i < nregions; i++) {
		status = cs_dsp_create_region(dsp, type[i], id, ver, base[i]);
		if (!NT_SUCCESS(status))
			return status;
	}

	return status;
}

static NTSTATUS cs_dsp_halo_create_regions(struct cs_dsp *dsp, UINT32 id, UINT32 ver,
				      UINT32 xm_base, UINT32 ym_base)
{
	static const INT32 types[] = {
		WMFW_ADSP2_XM, WMFW_HALO_XM_PACKED,
		WMFW_ADSP2_YM, WMFW_HALO_YM_PACKED
	};
	UINT32 bases[] = { xm_base, xm_base, ym_base, ym_base };

	return cs_dsp_create_regions(dsp, id, ver, ARRAY_SIZE(types), types, bases);
}

static VOID *cs_dsp_read_algs(PCS35L41_CONTEXT pDevice, size_t n_algs,
			      const struct cs_dsp_region *mem,
			      UINT32 pos, UINT32 len)
{
	NTSTATUS status = STATUS_SUCCESS;

	VOID *alg;
	UINT32 reg;
	UINT32 val; //big endian

	if (n_algs == 0) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "No algorithms\n");
		return NULL;
	}

	if (n_algs > 1024) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "Algorithm count %zx excessive\n", n_algs);
		return NULL;
	}

	/* Read the terminator first to validate the length */
	reg = region_to_reg(mem, pos + len);

	status = cs35l41_reg_bulk_read(pDevice, reg, &val, sizeof(val));
	if (!NT_SUCCESS(status)) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to read algorithm list end: %d\n", status);
		return NULL;
	}

	if (_byteswap_ulong(val) != 0xbedead) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Algorithm list end %x 0x%x != 0xbedead\n",
			reg, _byteswap_ulong(val));
		return NULL;
	}

	/* Convert length from DSP words to bytes */
	len *= sizeof(UINT32);

	alg = ExAllocatePool2(
		POOL_FLAG_NON_PAGED,
		len,
		CS35L41_POOL_TAG
	);
	if (alg == NULL)
		return NULL;

	reg = region_to_reg(mem, pos);

	status = cs35l41_reg_bulk_read(pDevice, reg, alg, len);
	if (!NT_SUCCESS(status)) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to read algorithm list: %d\n", status);
		ExFreePoolWithTag(alg, CS35L41_POOL_TAG);
		return NULL;
	}

	return alg;
}

#pragma warning(disable:4133)
static NTSTATUS cs_setup_algs(PCS35L41_CONTEXT pDevice, struct cs_dsp *dsp)
{
	NTSTATUS status = STATUS_SUCCESS;

	struct wmfw_halo_id_hdr halo_id;
	struct wmfw_halo_alg_hdr *halo_alg;
	const struct cs_dsp_region *mem;
	UINT32 pos, len;
	size_t n_algs;
	INT32 i;

	mem = cs_dsp_find_region(WMFW_ADSP2_XM);
	if (!mem)
		return STATUS_UNSUCCESSFUL;

	status = cs35l41_reg_bulk_read(pDevice, mem->base, &halo_id,
			      sizeof(halo_id));
	if (!NT_SUCCESS(status)) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"Failed to read algorithm info: %d\n", status);
		return status;
	}

	n_algs = _byteswap_ulong(halo_id.n_algs);

	cs_dsp_parse_wmfw_v3_id_header(dsp, &halo_id.fw, n_algs);

	status = cs_dsp_halo_create_regions(dsp, halo_id.fw.id, halo_id.fw.ver,
					 halo_id.xm_base, halo_id.ym_base);
	if (!NT_SUCCESS(status))
		return status;

	/* Calculate offset and length in DSP words */
	pos = sizeof(halo_id) / sizeof(UINT32);
	len = (UINT32)((sizeof(*halo_alg) * n_algs) / sizeof(UINT32));

	halo_alg = cs_dsp_read_algs(pDevice, n_algs, mem, pos, len);
	if (halo_alg == NULL)
		return STATUS_UNSUCCESSFUL;

	for (i = 0; i < n_algs; i++) {
		Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
			"%d: ID %x v%d.%d.%d XM@%x YM@%x\n",
			i, _byteswap_ulong(halo_alg[i].alg.id),
			(_byteswap_ulong(halo_alg[i].alg.ver) & 0xff0000) >> 16,
			(_byteswap_ulong(halo_alg[i].alg.ver) & 0xff00) >> 8,
			_byteswap_ulong(halo_alg[i].alg.ver) & 0xff,
			_byteswap_ulong(halo_alg[i].xm_base),
			_byteswap_ulong(halo_alg[i].ym_base));

		status = cs_dsp_halo_create_regions(dsp, halo_alg[i].alg.id,
						 halo_alg[i].alg.ver,
						 halo_alg[i].xm_base,
						 halo_alg[i].ym_base);
		if (!NT_SUCCESS(status))
			goto out;
	}

out:
	ExFreePoolWithTag(halo_alg, CS35L41_POOL_TAG);
	return status;
}

struct cs_dsp_alg_region *cs_dsp_find_alg_region(PCS35L41_CONTEXT pDevice, INT32 type, UINT32 id)
{
	struct cs_dsp_alg_region *alg_region;

	for (INT32 i = 0; i < (pDevice->dsp.alg_regions_size / sizeof(struct cs_dsp_alg_region)); i++)
		if (id == pDevice->dsp.alg_regions[i].alg &&
			type == pDevice->dsp.alg_regions[i].type)
		{
			alg_region = &pDevice->dsp.alg_regions[i];
			return alg_region;
		}

	return NULL;
}

NTSTATUS cs_dsp_load_coeff(PCS35L41_CONTEXT pDevice, UINT8 *coeff_firmware)
{
	struct wmfw_coeff_hdr *hdr;
	struct wmfw_coeff_item *blk;
	const struct cs_dsp_region *mem;
	struct cs_dsp_alg_region *alg_region;
	const char *region_name;
	UINT32 pos, blocks, type, offset, reg, version;

	hdr = (void *)&coeff_firmware[0];
	if (memcmp(hdr->magic, "WMDR", 4) != 0) {
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"coef: invalid coefficient magic\n");
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	switch (_byteswap_ulong(hdr->rev) & 0xff) {
	case 1:
	case 2:
		break;
	default:
		Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
			"coef: Unsupported coefficient file format %d\n",
			_byteswap_ulong(hdr->rev) & 0xff);
		return STATUS_INVALID_IMAGE_FORMAT;
	}

	Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
		"coef: v%d.%d.%d\n",
		(hdr->ver >> 16) & 0xff,
		(hdr->ver >>  8) & 0xff,
		hdr->ver & 0xff);

	pos = hdr->len;
	if (pos < sizeof(*hdr) || pos >= COEF_PROT_BUFFER_SIZE || (pos & 3))
		return STATUS_INVALID_IMAGE_FORMAT;

	blocks = 0;

	while (pos < COEF_PROT_BUFFER_SIZE) {
		UINT32 remaining = COEF_PROT_BUFFER_SIZE - pos;
		if (remaining < sizeof(*blk))
			return STATUS_INVALID_IMAGE_FORMAT;

		blk = (void *)(&coeff_firmware[pos]);
		if (blk->len > remaining - sizeof(*blk))
			return STATUS_INVALID_IMAGE_FORMAT;

		UINT32 blockSize = (blk->len + sizeof(*blk) + 3) & ~0x03;
		if (blockSize > remaining)
			return STATUS_INVALID_IMAGE_FORMAT;

		type = blk->type;
		offset = blk->offset;
		version = blk->ver >> 8;

		Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
			"coef.%d: %x v%d.%d.%d\n",
			blocks, blk->id,
			(blk->ver >> 16) & 0xff,
			(blk->ver >>  8) & 0xff,
			blk->ver & 0xff);
		Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
			"coef.%d: %d bytes at 0x%x in %x\n",
			blocks, blk->len, offset, type);

		reg = 0;
		region_name = "Unknown";
		switch (type) {
		case (WMFW_NAME_TEXT << 8):
			break;
		case (WMFW_INFO_TEXT << 8):
		case (WMFW_METADATA << 8):
			break;
		case (WMFW_ABSOLUTE << 8):
			if (blk->id == pDevice->dsp.fw_id &&
			    offset == 0) {
				region_name = "global coefficients";
				mem = cs_dsp_find_region(type);
				if (!mem) {
					Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "No ZM\n");
					return STATUS_INVALID_IMAGE_FORMAT;
				}
				reg = region_to_reg(mem, 0);

			} else {
				region_name = "register";
				reg = offset;
			}
			break;

		case WMFW_ADSP1_DM:
		case WMFW_ADSP1_ZM:
		case WMFW_ADSP2_XM:
		case WMFW_ADSP2_YM:
		case WMFW_HALO_XM_PACKED:
		case WMFW_HALO_YM_PACKED:
		case WMFW_HALO_PM_PACKED:
			Cs35l41Print(DEBUG_LEVEL_VERBOSE, DBG_INIT,
				"coef.%d: %d bytes in %x for %x\n",
				blocks, blk->len,
				type, blk->id);

			region_name = cs_dsp_mem_region_name(type);
			mem = cs_dsp_find_region(type);
			if (!mem) {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"No base for region %x\n", type);
				return STATUS_INVALID_IMAGE_FORMAT;
			}

			alg_region = cs_dsp_find_alg_region(pDevice, type, blk->id);
			if (alg_region) {
				if (version != alg_region->ver)
					Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
						"Algorithm coefficient version %d.%d.%d but expected %d.%d.%d\n",
						(version >> 16) & 0xFF,
						(version >> 8) & 0xFF,
						version & 0xFF,
						(alg_region->ver >> 16) & 0xFF,
						(alg_region->ver >> 8) & 0xFF,
						alg_region->ver & 0xFF);

				reg = alg_region->base;
				reg = region_to_reg(mem, reg);
				reg += offset;
			} else {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"No %s for algorithm %x\n",
					region_name, blk->id);
				return STATUS_INVALID_IMAGE_FORMAT;
			}
			break;

		default:
			Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
				"coef.%d: Unknown region type %x at %d\n",
				blocks, type, pos);
			break;
		}

		if (reg) {
			Cs35l41Print(DEBUG_LEVEL_INFO, DBG_INIT,
				"coef.%d: Writing %d bytes at %x\n",
				blocks, blk->len, reg);
			NTSTATUS status = cs_dsp_write_blocks(pDevice, reg, blk->data, blk->len, type);
			if (!NT_SUCCESS(status)) {
				Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
					"coef.%d: Failed to write to %x in %s: %d\n",
					blocks, reg, region_name, status);
				return status;
			}
		}

		pos += blockSize;
		blocks++;
	}

	return STATUS_SUCCESS;
}

NTSTATUS cs_dsp_power_up(PCS35L41_CONTEXT pDevice, UINT8 *wmfw_firmware, UINT8 *coeff_firmware)
{
	NTSTATUS status = STATUS_SUCCESS;

	struct cs_dsp *dsp;
	dsp = &pDevice->dsp;

	status = cs_dsp_load(pDevice, wmfw_firmware);
	if (!NT_SUCCESS(status))
		return status;

	status = cs_setup_algs(pDevice, dsp);
	if (!NT_SUCCESS(status))
		return status;

	status = cs_dsp_load_coeff(pDevice, coeff_firmware);
	if (!NT_SUCCESS(status))
		return status;

	dsp->booted = TRUE;

	return status;
}

NTSTATUS wm_adsp_power_up(PCS35L41_CONTEXT pDevice)
{
    static PWSTR coefficientKeys[] = { L"COEFPath0", L"COEFPath1", L"COEFPath2", L"COEFPath3" };
    UINT32 uid = (UINT32)pDevice->UID;
    if (uid >= ARRAY_SIZE(coefficientKeys))
        return STATUS_INVALID_PARAMETER;

    DECLARE_UNICODE_STRING_SIZE(wmfwPath, 260);
    DECLARE_UNICODE_STRING_SIZE(wmfwFullPath, 272);
    DECLARE_UNICODE_STRING_SIZE(coefPath, 260);
    DECLARE_UNICODE_STRING_SIZE(coefFullPath, 272);

    NTSTATUS status = ReadRegistryString(pDevice, L"WMFWPath", &wmfwPath);
    if (!NT_SUCCESS(status))
        return status;
    status = ReadRegistryString(pDevice, coefficientKeys[uid], &coefPath);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlAppendUnicodeToString(&wmfwFullPath, L"\\DosDevices\\");
    if (NT_SUCCESS(status))
        status = RtlUnicodeStringCat(&wmfwFullPath, &wmfwPath);
    if (!NT_SUCCESS(status))
        return status;

    status = RtlAppendUnicodeToString(&coefFullPath, L"\\DosDevices\\");
    if (NT_SUCCESS(status))
        status = RtlUnicodeStringCat(&coefFullPath, &coefPath);
    if (!NT_SUCCESS(status))
        return status;

    UINT8 *firmware = ExAllocatePool2(POOL_FLAG_NON_PAGED, WMFW_PROT_BUFFER_SIZE, CS35L41_POOL_TAG);
    if (firmware == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    UINT8 *coefficients = ExAllocatePool2(POOL_FLAG_NON_PAGED, COEF_PROT_BUFFER_SIZE, CS35L41_POOL_TAG);
    if (coefficients == NULL) {
        ExFreePoolWithTag(firmware, CS35L41_POOL_TAG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = CopyFileToBuffer(wmfwFullPath, firmware, WMFW_PROT_BUFFER_SIZE);
    if (NT_SUCCESS(status))
        status = CopyFileToBuffer(coefFullPath, coefficients, COEF_PROT_BUFFER_SIZE);
    if (NT_SUCCESS(status))
        status = cs_dsp_power_up(pDevice, firmware, coefficients);
    if (!NT_SUCCESS(status))
        Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT, "UID %d: protection firmware load failed: 0x%x\n", uid, status);

    ExFreePoolWithTag(coefficients, CS35L41_POOL_TAG);
    ExFreePoolWithTag(firmware, CS35L41_POOL_TAG);
    return status;
}

static NTSTATUS cs_dsp_halo_configure_mpu(PCS35L41_CONTEXT pDevice, UINT32 lock_regions)
{
	struct reg_sequence config[] = {
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_LOCK_CONFIG,     0x5555 },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_LOCK_CONFIG,     0xAAAA },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XMEM_ACCESS_0,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YMEM_ACCESS_0,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_WINDOW_ACCESS_0, lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XREG_ACCESS_0,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YREG_ACCESS_0,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XMEM_ACCESS_1,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YMEM_ACCESS_1,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_WINDOW_ACCESS_1, lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XREG_ACCESS_1,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YREG_ACCESS_1,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XMEM_ACCESS_2,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YMEM_ACCESS_2,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_WINDOW_ACCESS_2, lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XREG_ACCESS_2,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YREG_ACCESS_2,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XMEM_ACCESS_3,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YMEM_ACCESS_3,   0xFFFFFFFF },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_WINDOW_ACCESS_3, lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_XREG_ACCESS_3,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_YREG_ACCESS_3,   lock_regions },
		{ CS35L41_DSP1_CTRL_BASE + HALO_MPU_LOCK_CONFIG,     0 },
	};

	return cs35l41_multi_reg_write(pDevice, config, ARRAY_SIZE(config));
}

static NTSTATUS cs_dsp_halo_start_core(PCS35L41_CONTEXT pDevice)
{
	NTSTATUS status = STATUS_SUCCESS;

	status = cs35l41_reg_update_bits(pDevice, CS35L41_DSP1_CTRL_BASE + HALO_CCM_CORE_CONTROL,
				 HALO_CORE_RESET | HALO_CORE_EN,
				 HALO_CORE_RESET | HALO_CORE_EN);
	if (!NT_SUCCESS(status))
		return status;

	return cs35l41_reg_update_bits(pDevice, CS35L41_DSP1_CTRL_BASE + HALO_CCM_CORE_CONTROL,
				  HALO_CORE_RESET, 0);
}

static VOID cs_dsp_halo_stop_core(PCS35L41_CONTEXT pDevice)
{
	cs35l41_reg_update_bits(pDevice, CS35L41_DSP1_CTRL_BASE + HALO_CCM_CORE_CONTROL,
			   HALO_CORE_EN, 0);

	/* reset halo core with CORE_SOFT_RESET */
	cs35l41_reg_update_bits(pDevice, CS35L41_DSP1_CTRL_BASE + HALO_CORE_SOFT_RESET,
			   HALO_CORE_SOFT_RESET_MASK, 1);
}

VOID cs35l41_dsp_reset(PCS35L41_CONTEXT pDevice)
{
    if (pDevice->dsp.running)
        cs_dsp_halo_stop_core(pDevice);
    if (pDevice->dsp.alg_regions != NULL)
        ExFreePoolWithTag(pDevice->dsp.alg_regions, CS35L41_POOL_TAG);
    RtlZeroMemory(&pDevice->dsp, sizeof(pDevice->dsp));
}

#pragma warning(disable:4047)
#pragma warning(disable:4024)
/*
 * Music coefficients from the four stock HyperOS vendor firmware music files.
 */
static const INT32 nabuBRMusic[] = {
    40, 4, 768, 1236, 0, 0, 0, 9,
    8, 0, 37, 0, 51, 4984939, 0, 55,
    134974, 56, 189791, 122, -8303914, 8220424, 8334092, -8334110,
    8334080, 123, -8363346, 8338690, 8334092, -8344396, 8354905, 124,
    -8382595, 8377037, 8334092, -8323616, 8313344, 136, 189751, 1635995354,
};
static const INT32 nabuTRMusic[] = {
    31, 4, 768, 1236, 0, 0, 0, 5,
    8, 0, 37, 0, 122, -8303914, 8220424, 8334092,
    -8334110, 8334080, 123, -8363346, 8338690, 8334092, -8344396, 8354905,
    124, -8382595, 8377037, 8334092, -8323616, 8313344, 493339961,
};
static const INT32 nabuBLMusic[] = {
    40, 4, 768, 1236, 0, 0, 0, 9,
    8, 0, 37, 0, 51, 4984939, 0, 55,
    141033, 56, 194648, 122, -8303914, 8220424, 8334092, -8334110,
    8334080, 123, -8363346, 8338690, 8334092, -8344396, 8354905, 124,
    -8382595, 8377037, 8334092, -8323616, 8313344, 136, 194596, -26013209,
};
static const INT32 nabuTLMusic[] = {
    31, 4, 768, 1236, 0, 0, 0, 5,
    8, 0, 37, 0, 122, -8303914, 8220424, 8334092,
    -8334110, 8334080, 123, -8363346, 8338690, 8334092, -8344396, 8354905,
    124, -8382595, 8377037, 8334092, -8323616, 8313344, 493339961,
};

static const struct {
    const INT32 *music;
    UINT32 musicWordCount;
} nabuSpeaker[] = {
    { nabuBRMusic, ARRAY_SIZE(nabuBRMusic) },
    { nabuTRMusic, ARRAY_SIZE(nabuTRMusic) },
    { nabuBLMusic, ARRAY_SIZE(nabuBLMusic) },
    { nabuTLMusic, ARRAY_SIZE(nabuTLMusic) },
};

typedef struct {
    UINT32 resistance;
    UINT32 calibrationStatus;
    UINT32 calibrationChecksum;
} NABU_SPEAKER_CALIBRATION;

typedef struct {
    UINT32 version;
    NABU_SPEAKER_CALIBRATION speaker[ARRAY_SIZE(nabuSpeaker)];
} NABU_FACTORY_CALIBRATION;

C_ASSERT(sizeof(NABU_FACTORY_CALIBRATION) == 52);

static NTSTATUS cs35l41_read_factory_calibration(PCS35L41_CONTEXT pDevice,
    _Out_ NABU_FACTORY_CALIBRATION *calibration)
{
    WDFKEY key;
    NTSTATUS status = WdfDriverOpenParametersRegistryKey(WdfDeviceGetDriver(pDevice->FxDevice),
        KEY_QUERY_VALUE, WDF_NO_OBJECT_ATTRIBUTES, &key);
    if (!NT_SUCCESS(status))
        return status;

    DECLARE_CONST_UNICODE_STRING(name, L"NabuFactoryCalibration");
    ULONG length = 0;
    ULONG type = 0;
    status = WdfRegistryQueryValue(key, &name, sizeof(*calibration), calibration, &length, &type);
    WdfRegistryClose(key);
    if (!NT_SUCCESS(status))
        return status;
    if (type != REG_BINARY || length != sizeof(*calibration) || calibration->version != 1)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    for (UINT32 index = 0; index < ARRAY_SIZE(calibration->speaker); ++index) {
        const NABU_SPEAKER_CALIBRATION *entry = &calibration->speaker[index];
        if (entry->calibrationStatus != 1 || entry->resistance == 0 || entry->resistance >= 0xffffff ||
            entry->calibrationChecksum != entry->resistance + entry->calibrationStatus)
            return STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    return STATUS_SUCCESS;
}

NTSTATUS cs35l41_dsp_get_speaker_protection(PCS35L41_CONTEXT pDevice,
    _Out_writes_(CS35L41_BDLOG_WORD_COUNT) UINT32 *counters)
{
    // HyperOS SPK_PROT 0.33.0 maps BDLOG_MAX_TEMP...BDLOG_VMON_LOW_FLAG
    // to six consecutive unpacked words of CSPL 29.53.0 X memory.
    if (!pDevice->dsp.running || pDevice->dsp.fw_id != 0x400a4 ||
        pDevice->dsp.fw_id_version != 0x002100)
        return STATUS_INVALID_DEVICE_STATE;
    const struct cs_dsp_region *memory = cs_dsp_find_region(WMFW_ADSP2_XM);
    const struct cs_dsp_alg_region *cspl = cs_dsp_find_alg_region(pDevice, WMFW_ADSP2_XM, 0xcd);
    if (memory == NULL || cspl == NULL || cspl->ver != 0x1d3500)
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    UINT32 address = region_to_reg(memory, cspl->base + 66);
    if (address != 0x02800358)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    UINT32 regBE = _byteswap_ulong(address);
    UINT32 raw[CS35L41_BDLOG_WORD_COUNT];
    NTSTATUS status = SpbWriteRead(&pDevice->I2CContext, &regBE, sizeof(regBE), raw, sizeof(raw), 0, 100);
    if (!NT_SUCCESS(status))
        return status;
    for (UINT32 index = 0; index < ARRAY_SIZE(raw); ++index)
        counters[index] = _byteswap_ulong(raw[index]) & 0xffffff;
    return STATUS_SUCCESS;
}

NTSTATUS cs35l41_dsp_configure(PCS35L41_CONTEXT pDevice)
{
    UINT32 uid = (UINT32)pDevice->UID;
    NABU_FACTORY_CALIBRATION calibration;
    if (uid >= ARRAY_SIZE(nabuSpeaker))
        return STATUS_INVALID_PARAMETER;

    NTSTATUS status = cs35l41_read_factory_calibration(pDevice, &calibration);
    if (!NT_SUCCESS(status))
        return status;
    const NABU_SPEAKER_CALIBRATION *entry = &calibration.speaker[uid];

    // Match wm_halo_apply_calibration() for WM_ADSP_FW_SPK_PROT.
    status = cs35l41_reg_write(pDevice, CAL_R, entry->resistance);
    if (NT_SUCCESS(status))
        status = cs35l41_reg_write(pDevice, CAL_STATUS, entry->calibrationStatus);
    if (NT_SUCCESS(status))
        status = cs35l41_reg_write(pDevice, CAL_CHECKSUM, entry->calibrationChecksum);
    return status;
}

NTSTATUS cs35l41_dsp_apply_stock_music(PCS35L41_CONTEXT pDevice)
{
    // The stock WMFW declares CSPL_UPDATE_PARAMS_CONFIG as word 0 in CSPL Y memory.
    const UINT32 csplId = 0xcd;
    UINT32 uid = (UINT32)pDevice->UID;
    const struct cs_dsp_region *memory = cs_dsp_find_region(WMFW_ADSP2_YM);
    const struct cs_dsp_alg_region *algorithm = cs_dsp_find_alg_region(pDevice, WMFW_ADSP2_YM, csplId);
    UINT32 words[ARRAY_SIZE(nabuBRMusic)];

    if (uid >= ARRAY_SIZE(nabuSpeaker) || memory == NULL || algorithm == NULL || !pDevice->dsp.running)
        return STATUS_INVALID_DEVICE_STATE;

    UINT32 count = nabuSpeaker[uid].musicWordCount;
    if (count > ARRAY_SIZE(words))
        return STATUS_INVALID_BUFFER_SIZE;

    for (UINT32 i = 0; i < count; ++i)
        words[i] = _byteswap_ulong((UINT32)nabuSpeaker[uid].music[i]);

    // Bulk SPB writes preserve the byte order of firmware blobs.
    // Xiaomi's wm_adsp_write_ctl() applies music words in big-endian order.
    UINT32 address = region_to_reg(memory, algorithm->base);
    NTSTATUS status = cs35l41_reg_bulk_write(pDevice, address, (UINT8 *)words, count * sizeof(words[0]));
    if (!NT_SUCCESS(status))
        return status;

    UINT32 readback[ARRAY_SIZE(nabuBRMusic)] = { 0 };
    status = cs35l41_reg_bulk_read(pDevice, address, readback, count * sizeof(readback[0]));
    if (!NT_SUCCESS(status))
        return status;
    // The unpacked DSP window exposes 24-bit words in 32-bit slots.
    for (UINT32 i = 0; i < count; ++i) {
        if ((_byteswap_ulong(readback[i]) & 0xffffff) != ((UINT32)nabuSpeaker[uid].music[i] & 0xffffff))
            return STATUS_DEVICE_DATA_ERROR;
    }

    status = cs35l41_reg_write(pDevice, CSPL_COMMAND, CSPL_CMD_UPDATE_PARAM);
    if (!NT_SUCCESS(status))
        return status;

    for (UINT32 i = 0; i < 5; ++i) {
        UINT32 state;
        status = cs35l41_reg_read(pDevice, CSPL_STATE, &state);
        if (!NT_SUCCESS(status))
            return status;
        if (state == CSPL_ST_RUNNING)
            return STATUS_SUCCESS;
        udelay(100);
    }
    return STATUS_DEVICE_NOT_READY;
}

NTSTATUS cs35l41_dsp_apply_stock_channel(PCS35L41_CONTEXT pDevice)
{
    // The stock protection WMFW describes CH_BAL at CSPL (0xcd) XM word 73.
    // MiCode's Channel Swap control writes 0x400000 for BR/TR, zero for BL/TL.
    const UINT32 csplId = 0xcd;
    const UINT32 channelBalanceOffset = 73;
    const struct cs_dsp_region *memory = cs_dsp_find_region(WMFW_ADSP2_XM);
    const struct cs_dsp_alg_region *cspl = cs_dsp_find_alg_region(pDevice, WMFW_ADSP2_XM, csplId);
    NabuSpeakerRoute route;
    if (NabuResolveSpeakerRoute((UINT32)pDevice->UID, &route) != 0 || memory == NULL || cspl == NULL)
        return STATUS_INVALID_DEVICE_STATE;

    UINT32 address = region_to_reg(memory, cspl->base + channelBalanceOffset);
    if (address != CH_BAL)
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    NTSTATUS status = cs35l41_reg_write(pDevice, CS35L41_DSP1_RX1_SRC, CS35L41_INPUT_SRC_ASPRX1);
    if (!NT_SUCCESS(status))
        return status;
    status = cs35l41_reg_write(pDevice, CS35L41_DSP1_RX2_SRC, CS35L41_INPUT_SRC_ASPRX2);
    if (!NT_SUCCESS(status))
        return status;

    UINT32 expected = route.ChannelBalance;
    status = cs35l41_reg_write(pDevice, address, expected);
    if (!NT_SUCCESS(status))
        return status;

    UINT32 readback = 0;
    status = cs35l41_reg_read(pDevice, address, &readback);
    if (!NT_SUCCESS(status))
        return status;
    if ((readback & 0xffffff) != expected)
        return STATUS_DEVICE_DATA_ERROR;

    // A successful CH_BAL write alone cannot verify either serial channel.
    // Check the actual Windows slot pair and both DSP input muxes before enabling
    // the amplifier. Do not compensate for a failed route with gain changes.
    status = cs35l41_reg_read(pDevice, CS35L41_SP_FRAME_RX_SLOT, &readback);
    if (!NT_SUCCESS(status))
        return status;
    if (((readback & CS35L41_ASP_RX1_SLOT_MASK) >> CS35L41_ASP_RX1_SLOT_SHIFT) != route.Rx1Slot ||
        ((readback & CS35L41_ASP_RX2_SLOT_MASK) >> CS35L41_ASP_RX2_SLOT_SHIFT) != route.Rx2Slot)
        return STATUS_DEVICE_DATA_ERROR;
    status = cs35l41_reg_read(pDevice, CS35L41_DSP1_RX1_SRC, &readback);
    if (!NT_SUCCESS(status))
        return status;
    if (readback != CS35L41_INPUT_SRC_ASPRX1)
        return STATUS_DEVICE_DATA_ERROR;
    status = cs35l41_reg_read(pDevice, CS35L41_DSP1_RX2_SRC, &readback);
    if (!NT_SUCCESS(status))
        return status;
    return readback == CS35L41_INPUT_SRC_ASPRX2 ? STATUS_SUCCESS : STATUS_DEVICE_DATA_ERROR;
}

NTSTATUS cs35l41_dsp_init(PCS35L41_CONTEXT pDevice) {
	NTSTATUS status = STATUS_SUCCESS;

	status = cs35l41_write_fs_errata(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	status = wm_adsp_power_up(pDevice);
	if (!NT_SUCCESS(status))
		return status;

	static const struct reg_sequence dspInputs[] = {
		{ CS35L41_DSP1_RX5_SRC, CS35L41_INPUT_SRC_VPMON },
		{ CS35L41_DSP1_RX6_SRC, CS35L41_INPUT_SRC_CLASSH },
		{ CS35L41_DSP1_RX7_SRC, CS35L41_INPUT_SRC_TEMPMON },
		{ CS35L41_DSP1_RX8_SRC, CS35L41_INPUT_SRC_RSVD },
	};
	return cs35l41_multi_reg_write(pDevice, dspInputs, ARRAY_SIZE(dspInputs));
}

NTSTATUS cs35l41_dsp_run(PCS35L41_CONTEXT pDevice) {
    NTSTATUS status = STATUS_SUCCESS;
    
	if (!pDevice->dsp.booted) {
		status = STATUS_UNSUCCESSFUL;
		goto err;
	}

    status = cs_dsp_halo_configure_mpu(pDevice, 0xFFFFFFFF);
    if (!NT_SUCCESS(status)) {
        Cs35l41Print(DEBUG_LEVEL_ERROR, DBG_INIT,
            "Error configuring MPU: %d\n", status);
        goto err;
    }

    status = cs_dsp_halo_start_core(pDevice);
    if (!NT_SUCCESS(status))
        goto err;

    pDevice->dsp.running = TRUE;

	return STATUS_SUCCESS;

err:
    cs_dsp_halo_stop_core(pDevice);

    return status;
}

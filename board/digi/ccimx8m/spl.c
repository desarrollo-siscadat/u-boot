/*
 * Copyright 2019 Digi International Inc.
 * Copyright 2018-2019 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <hang.h>
#include <spl.h>
#include <asm/io.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/arch/imx8mn_pins.h>
#include <asm/arch/sys_proto.h>
#include <asm/mach-imx/boot_mode.h>
#include <power/pmic.h>
#include <power/bd71837.h>
#include <power/pca9450.h>
#include <asm/arch/clock.h>
#include <asm/mach-imx/gpio.h>
#include <asm/gpio.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <fsl_esdhc_imx.h>
#include <mmc.h>
#include <asm/arch/ddr.h>

#include "../common/hwid.h"

DECLARE_GLOBAL_DATA_PTR;

extern struct dram_timing_info dram_timing_1G;
extern struct dram_timing_info dram_timing_512M;

int spl_board_boot_device(enum boot_device boot_dev_spl)
{
#ifdef CONFIG_SPL_BOOTROM_SUPPORT
	return BOOT_DEVICE_BOOTROM;
#else
	switch (boot_dev_spl) {
	case SD1_BOOT:
	case MMC1_BOOT:
	case SD2_BOOT:
	case MMC2_BOOT:
		return BOOT_DEVICE_MMC1;
	case SD3_BOOT:
	case MMC3_BOOT:
		return BOOT_DEVICE_MMC2;
	case QSPI_BOOT:
		return BOOT_DEVICE_NOR;
	case NAND_BOOT:
		return BOOT_DEVICE_NAND;
	case USB_BOOT:
		return BOOT_DEVICE_BOARD;
	default:
		return BOOT_DEVICE_NONE;
	}
#endif
}

void spl_dram_init(void)
{
	/* Default to RAM size of DVK variant 0x01 (1 GiB) */
	u32 ram = SZ_1G;
	struct digi_hwid my_hwid;

	if (board_read_hwid(&my_hwid)) {
		debug("Cannot read HWID. Using default DDR configuration.\n");
		my_hwid.ram = 0;
	}

	if (my_hwid.ram)
		ram = hwid_get_ramsize(&my_hwid);

	switch (ram) {
	case SZ_512M:
		ddr_init(&dram_timing_512M);
		break;
	case SZ_1G:
	default:
		ddr_init(&dram_timing_1G);
		break;
	}
}

#define I2C_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE | PAD_CTL_PE)
#define PC MUX_PAD_CTRL(I2C_PAD_CTRL)
struct i2c_pads_info i2c_pad_info1 = {
	.scl = {
		.i2c_mode = IMX8MN_PAD_I2C1_SCL__I2C1_SCL | PC,
		.gpio_mode = IMX8MN_PAD_I2C1_SCL__GPIO5_IO14 | PC,
		.gp = IMX_GPIO_NR(5, 14),
	},
	.sda = {
		.i2c_mode = IMX8MN_PAD_I2C1_SDA__I2C1_SDA | PC,
		.gpio_mode = IMX8MN_PAD_I2C1_SDA__GPIO5_IO15 | PC,
		.gp = IMX_GPIO_NR(5, 15),
	},
};

#define USDHC2_CD_GPIO	IMX_GPIO_NR(2, 12)

#define USDHC_PAD_CTRL	(PAD_CTL_DSE6 | PAD_CTL_HYS | PAD_CTL_PUE |PAD_CTL_PE | \
			 PAD_CTL_FSEL2)
#define USDHC_GPIO_PAD_CTRL (PAD_CTL_HYS | PAD_CTL_DSE1)
#define USDHC_CD_PAD_CTRL (PAD_CTL_PE |PAD_CTL_PUE |PAD_CTL_HYS | PAD_CTL_DSE4)

static iomux_v3_cfg_t const usdhc3_pads[] = {
	IMX8MN_PAD_NAND_WE_B__USDHC3_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_WP_B__USDHC3_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA04__USDHC3_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA05__USDHC3_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA06__USDHC3_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_DATA07__USDHC3_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_RE_B__USDHC3_DATA4 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CE2_B__USDHC3_DATA5 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CE3_B__USDHC3_DATA6 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_CLE__USDHC3_DATA7 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_NAND_READY_B__USDHC3_RESET_B | MUX_PAD_CTRL(USDHC_PAD_CTRL),
};

static iomux_v3_cfg_t const usdhc2_pads[] = {
	IMX8MN_PAD_SD2_CLK__USDHC2_CLK | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_CMD__USDHC2_CMD | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_DATA0__USDHC2_DATA0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_DATA1__USDHC2_DATA1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_DATA2__USDHC2_DATA2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_DATA3__USDHC2_DATA3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	IMX8MN_PAD_SD2_CD_B__GPIO2_IO12 | MUX_PAD_CTRL(USDHC_CD_PAD_CTRL),
};

static struct fsl_esdhc_cfg usdhc_cfg[2] = {
	{USDHC2_BASE_ADDR, 0, 4},
	{USDHC3_BASE_ADDR, 0, 8},
};

int board_mmc_init(bd_t *bis)
{
	int i, ret;
	/*
	 * According to the device tree aliases the following map is done:
	 * (U-Boot device node)    (Physical Port)
	 * mmc0                    USDHC3 (eMMC)
	 * mmc1                    USDHC2 (microSD)
	 */
	for (i = 0; i < CONFIG_SYS_FSL_USDHC_NUM; i++) {
		switch (i) {
		case 0:
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC3_CLK);
			imx_iomux_v3_setup_multiple_pads(
				usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
			break;
		case 1:
			usdhc_cfg[i].sdhc_clk = mxc_get_clock(MXC_ESDHC2_CLK);
			imx_iomux_v3_setup_multiple_pads(
				usdhc2_pads, ARRAY_SIZE(usdhc2_pads));
			gpio_request(USDHC2_CD_GPIO, "usdhc2 cd");
			gpio_direction_input(USDHC2_CD_GPIO);
			break;
		default:
			printf("Warning: you configured more USDHC controllers"
				"(%d) than supported by the board\n", i + 1);
			return -EINVAL;
		}

		ret = fsl_esdhc_initialize(bis, &usdhc_cfg[i]);
		if (ret)
			return ret;
	}

	return 0;
}

int board_mmc_getcd(struct mmc *mmc)
{
	struct fsl_esdhc_cfg *cfg = (struct fsl_esdhc_cfg *)mmc->priv;
	int ret = 0;

	switch (cfg->esdhc_base) {
	case USDHC3_BASE_ADDR:
		ret = 1;
		break;
	case USDHC2_BASE_ADDR:
		ret = gpio_get_value(USDHC2_CD_GPIO);
		return ret;
	}

	return 1;
}

#ifdef CONFIG_POWER
#define I2C_PMIC	0
int power_init_board(void)
{
	struct digi_hwid my_hwid;
	struct pmic *p;
	int ret;

	if (board_read_hwid(&my_hwid)) {
		printf("Cannot read HWID\n");
		my_hwid.hv = 0;
	}

	/*
	 * Revision 1 of the ccimx8mn uses the bd71837 PMIC.
	 * Revisions 2 and higher use the pca9450 PMIC
	 */
	if (my_hwid.hv == 1) {
		ret = power_bd71837_init(I2C_PMIC);
		if (ret)
			printf("power init failed");

		p = pmic_get("BD71837");
		pmic_probe(p);

		/* decrease RESET key long push time from the default 10s to 10ms */
		pmic_reg_write(p, BD71837_PWRONCONFIG1, 0x0);

		/* unlock the PMIC regs */
		pmic_reg_write(p, BD71837_REGLOCK, 0x1);

		/*
		* increase VDD_SOC/VDD_DRAM to typical value 0.85v for 1.2Ghz
		* DDR clock
		*/
		pmic_reg_write(p, BD71837_BUCK1_VOLT_RUN, 0x0F);

		/* increase VDD_ARM to typical value 0.95v for Quad-A53, 1.4 GHz */
		pmic_reg_write(p, BD71837_BUCK2_VOLT_RUN, 0x19);

		/* Set VDD_SOC 0.85v for suspend */
		pmic_reg_write(p, BD71837_BUCK1_VOLT_SUSP, 0xf);

		/* lock the PMIC regs */
		pmic_reg_write(p, BD71837_REGLOCK, 0x11);
	} else {
		ret = power_pca9450b_init(I2C_PMIC);
		if (ret)
			printf("power init failed");

		p = pmic_get("PCA9450");
		pmic_probe(p);

		/* BUCKxOUT_DVS0/1 control BUCK123 output */
		pmic_reg_write(p, PCA9450_BUCK123_DVS, 0x29);

		/* increase VDD_SOC/VDD_DRAM to typical value 0.95V before first DRAM access */
		/* Set DVS1 to 0.85v for suspend */
		/* Enable DVS control through PMIC_STBY_REQ and set B1_ENMODE=1 (ON by PMIC_ON_REQ=H) */
		pmic_reg_write(p, PCA9450_BUCK1OUT_DVS0, 0x1C);
		pmic_reg_write(p, PCA9450_BUCK1OUT_DVS1, 0x14);
		pmic_reg_write(p, PCA9450_BUCK1CTRL, 0x59);

		/* set VDD_SNVS_0V8 from default 0.85V */
		pmic_reg_write(p, PCA9450_LDO2CTRL, 0xC0);

		/* set WDOG_B_CFG to cold reset */
		pmic_reg_write(p, PCA9450_RESET_CTRL, 0xA1);
	}

	return 0;
}
#endif

void spl_board_init(void)
{
	puts("Normal Boot\n");
}

#ifdef CONFIG_SPL_LOAD_FIT
int board_fit_config_name_match(const char *name)
{
	/* Just empty function now - can't decide what to choose */
	debug("%s: %s\n", __func__, name);

	return 0;
}
#endif

void board_init_f(ulong dummy)
{
	int ret;

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	arch_cpu_init();

	board_early_init_f();

	timer_init();

	preloader_console_init();

	ret = spl_init();
	if (ret) {
		debug("spl_init() failed: %d\n", ret);
		hang();
	}

	enable_tzc380();

	/* Adjust pmic voltage to 1.0V for 800M */
	setup_i2c(0, CONFIG_SYS_I2C_SPEED, 0x7f, &i2c_pad_info1);

	power_init_board();

	/* DDR initialization */
	spl_dram_init();

	board_init_r(NULL, 0);
}

#ifdef CONFIG_SPL_MMC_SUPPORT

#define UBOOT_RAW_SECTOR_OFFSET 0x40
unsigned long spl_mmc_get_uboot_raw_sector(struct mmc *mmc)
{
	u32 boot_dev = spl_boot_device();
	int part;

	switch (boot_dev) {
		case BOOT_DEVICE_MMC1:
			/* microSD card */
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
		case BOOT_DEVICE_MMC2:
			/* eMMC */
			part = (mmc->part_config >> 3) & PART_ACCESS_MASK;
			/*
			 * On the BOOT partitions, the bootloader is stored
			 * at offset 0.
			 */
			if (part == 1 || part == 2)
				return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR -
				       UBOOT_RAW_SECTOR_OFFSET;
			/*
			 * On the User Data area the boot loader is expected
			 * at UBOOT_RAW_SECTOR_OFFSET offset.
			 */
			return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
	}
	return CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
}
#endif

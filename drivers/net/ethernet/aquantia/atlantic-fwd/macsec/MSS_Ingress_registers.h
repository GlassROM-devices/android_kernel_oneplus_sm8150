#ifndef MSS_INGRESS_REGS_HEADER
#define MSS_INGRESS_REGS_HEADER

#define mssIngressControlRegister_ADDR 0x0000800E
#define mssIngressLutAddressControlRegister_ADDR 0x00008080
#define mssIngressLutControlRegister_ADDR 0x00008081
#define mssIngressLutDataControlRegister_ADDR 0x000080A0

struct mssIngressControlRegister_t {
	union {
		struct {
			unsigned int mssIngressSoftReset : 1;
			unsigned int mssIngressOperationPointToPoint : 1;
			unsigned int mssIngressCreateSci : 1;
			/* Unused  */
			unsigned int mssIngressMaskShortLengthError : 1;
			unsigned int mssIngressDropKayPacket : 1;
			unsigned int mssIngressDropIgprcMiss : 1;
			/* Unused  */
			unsigned int mssIngressCheckIcv : 1;
			unsigned int mssIngressClearGlobalTime : 1;
			unsigned int mssIngressClearCount : 1;
			unsigned int mssIngressHighPriority : 1;
			unsigned int mssIngressRemoveSectag : 1;
			unsigned int mssIngressGlobalValidateFrames : 2;
			unsigned int mssIngressIcvLsb_8BytesEnable : 1;
			unsigned int reserved0 : 2;
		} bits_0;
		unsigned short word_0;
	};
	union {
		struct {
			unsigned int reserved0 : 16;
		} bits_1;
		unsigned short word_1;
	};
};

struct mssIngressLutAddressControlRegister_t {
	union {
		struct {
			unsigned int mssIngressLutAddress : 9;
			unsigned int reserved0 : 3;
			/* 0x0 : Ingress Pre-Security MAC Control FIlter
			 *       (IGPRCTLF) LUT
			 * 0x1 : Ingress Pre-Security Classification LUT (IGPRC)
			 * 0x2 : Ingress Packet Format (IGPFMT) SAKey LUT
			 * 0x3 : Ingress Packet Format (IGPFMT) SC/SA LUT
			 * 0x4 : Ingress Post-Security Classification LUT
			 *       (IGPOC)
			 * 0x5 : Ingress Post-Security MAC Control Filter
			 *       (IGPOCTLF) LUT
			 * 0x6 : Ingress MIB (IGMIB)
			 */
			unsigned int mssIngressLutSelect : 4;
		} bits_0;
		unsigned short word_0;
	};
};

struct mssIngressLutControlRegister_t {
	union {
		struct {
			unsigned int reserved0 : 14;
			unsigned int mssIngressLutRead : 1;
			unsigned int mssIngressLutWrite : 1;
		} bits_0;
		unsigned short word_0;
	};
};

#endif /* MSS_INGRESS_REGS_HEADER */

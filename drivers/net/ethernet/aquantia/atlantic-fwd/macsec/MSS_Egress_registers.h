#ifndef MSS_EGRESS_REGS_HEADER
#define MSS_EGRESS_REGS_HEADER

#define mssEgressControlRegister_ADDR 0x00005002
#define mssEgressSaExpiredStatusRegister_ADDR 0x00005060
#define mssEgressSaThresholdExpiredStatusRegister_ADDR 0x00005062
#define mssEgressLutAddressControlRegister_ADDR 0x00005080
#define mssEgressLutControlRegister_ADDR 0x00005081
#define mssEgressLutDataControlRegister_ADDR 0x000050A0

struct mssEgressControlRegister_t {
	union {
		struct {
			unsigned int mssEgressSoftReset : 1;
			unsigned int mssEgressDropKayPacket : 1;
			unsigned int mssEgressDropEgprcLutMiss : 1;
			unsigned int mssEgressGcmStart : 1;
			unsigned int mssEgresssGcmTestMode : 1;
			unsigned int mssEgressUnmatchedUseSc_0 : 1;
			unsigned int mssEgressDropInvalidSa_scPackets : 1;
			unsigned int reserved0 : 1;
			/* Should always be set to 0. */
			unsigned int mssEgressExternalClassificationEnable : 1;
			unsigned int mssEgressIcvLsb_8BytesEnable : 1;
			unsigned int mssEgressHighPriority : 1;
			unsigned int mssEgressClearCounter : 1;
			unsigned int mssEgressClearGlobalTime : 1;
			unsigned int mssEgressEthertypeExplicitSectagLsb : 3;
		} bits_0;
		unsigned short word_0;
	};
	union {
		struct {
			unsigned int mssEgressEthertypeExplicitSectagMsb : 13;
			unsigned int reserved0 : 3;
		} bits_1;
		unsigned short word_1;
	};
};

struct mssEgressLutAddressControlRegister_t {
	union {
		struct {
			unsigned int mssEgressLutAddress : 9;
			unsigned int reserved0 : 3;
			/* 0x0 : Egress MAC Control FIlter (CTLF) LUT
			 * 0x1 : Egress Classification LUT
			 * 0x2 : Egress SC/SA LUT
			 * 0x3 : Egress SMIB
			 */
			unsigned int mssEgressLutSelect : 4;
		} bits_0;
		unsigned short word_0;
	};
};

struct mssEgressLutControlRegister_t {
	union {
		struct {
			unsigned int reserved0 : 14;
			unsigned int mssEgressLutRead : 1;
			unsigned int mssEgressLutWrite : 1;
		} bits_0;
		unsigned short word_0;
	};
};

#endif /* MSS_EGRESS_REGS_HEADER */

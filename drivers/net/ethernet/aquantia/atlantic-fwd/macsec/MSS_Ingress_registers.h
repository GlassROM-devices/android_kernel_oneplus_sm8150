#ifndef MSS_INGRESS_REGS_HEADER
#define MSS_INGRESS_REGS_HEADER

#define mssIngressControlRegister_ADDR 0x0000800E
#define mssIngressLutAddressControlRegister_ADDR 0x00008080
#define mssIngressLutControlRegister_ADDR 0x00008081
#define mssIngressLutDataControlRegister_ADDR 0x000080A0

//---------------------------------------------------------------------------------
//                  MSS Ingress Control Register: 1E.800E 
//---------------------------------------------------------------------------------
struct mssIngressControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.800E.0 R/W MSS Ingress Soft Reset
                        mssIngressControlRegister_t.bits_0.mssIngressSoftReset
                        Default = 0x0
                        1 = Soft reset
                 <B>Notes:</B>
                        S/W reset  */
      unsigned int   mssIngressSoftReset : 1;    // 1E.800E.0  R/W      Default = 0x0 
                     /* 1 = Soft reset */
                    /*! \brief 1E.800E.1 R/W MSS Ingress Operation Point To Point
                        mssIngressControlRegister_t.bits_0.mssIngressOperationPointToPoint
                        Default = 0x0
                        1 = Enable the SCI for authorization default
                 <B>Notes:</B>
                        The default SCI for authorization is configured in  See MSS Ingress SCI Default [F:0]   See MSS Ingress SCI Default [1F:10] , See MSS Ingress SCI Default [2F:20] , and  See MSS Ingress SCI Default [3F:30] .  */
      unsigned int   mssIngressOperationPointToPoint : 1;    // 1E.800E.1  R/W      Default = 0x0 
                     /* 1 = Enable the SCI for authorization default  */
                    /*! \brief 1E.800E.2 R/W MSS Ingress Create SCI
                        mssIngressControlRegister_t.bits_0.mssIngressCreateSci
                        Default = 0x0
                        0 = SCI from IGPRC LUT
                 <B>Notes:</B>
                        If the SCI is not in the packet and this bit is set to 0, the SCI will be taken from the IGPRC LUT.  */
      unsigned int   mssIngressCreateSci : 1;    // 1E.800E.2  R/W      Default = 0x0 
                     /* 0 = SCI from IGPRC LUT
                          */
                    /*! \brief 1E.800E.3 R/W MSS Ingress Mask Short Length Error
                        mssIngressControlRegister_t.bits_0.mssIngressMaskShortLengthError
                        Default = 0x0
                        Unused
                 <B>Notes:</B>
                        Unused  */
      unsigned int   mssIngressMaskShortLengthError : 1;    // 1E.800E.3  R/W      Default = 0x0 
                     /* Unused */
                    /*! \brief 1E.800E.4 R/W MSS Ingress Drop Kay Packet
                        mssIngressControlRegister_t.bits_0.mssIngressDropKayPacket
                        Default = 0x0
                        1 = Drop KaY packets
                 <B>Notes:</B>
                        Decides whether KaY packets have to be dropped  */
      unsigned int   mssIngressDropKayPacket : 1;    // 1E.800E.4  R/W      Default = 0x0 
                     /* 1 = Drop KaY packets   */
                    /*! \brief 1E.800E.5 R/W MSS Ingress Drop IGPRC Miss
                        mssIngressControlRegister_t.bits_0.mssIngressDropIgprcMiss
                        Default = 0x0
                        1 = Drop IGPRC miss packets
                 <B>Notes:</B>
                        Decides whether Ingress Pre-Security Classification (IGPRC) LUT miss packets are to be dropped  */
      unsigned int   mssIngressDropIgprcMiss : 1;    // 1E.800E.5  R/W      Default = 0x0 
                     /* 1 = Drop IGPRC miss packets */
                    /*! \brief 1E.800E.6 R/W MSS Ingress Check ICV
                        mssIngressControlRegister_t.bits_0.mssIngressCheckIcv
                        Default = 0x0
                        Unused
                 <B>Notes:</B>
                        Unused  */
      unsigned int   mssIngressCheckIcv : 1;    // 1E.800E.6  R/W      Default = 0x0 
                     /* Unused */
                    /*! \brief 1E.800E.7 R/W MSS Ingress Clear Global Time
                        mssIngressControlRegister_t.bits_0.mssIngressClearGlobalTime
                        Default = 0x0
                        1 = Clear global time
                 <B>Notes:</B>
                        Clear global time  */
      unsigned int   mssIngressClearGlobalTime : 1;    // 1E.800E.7  R/W      Default = 0x0 
                     /* 1 = Clear global time */
                    /*! \brief 1E.800E.8 R/W MSS Ingress Clear Count
                        mssIngressControlRegister_t.bits_0.mssIngressClearCount
                        Default = 0x0
                        1 = Clear all MIB counters
                 <B>Notes:</B>
                        If this bit is set to 1, all MIB counters will be cleared.  */
      unsigned int   mssIngressClearCount : 1;    // 1E.800E.8  R/W      Default = 0x0 
                     /* 1 = Clear all MIB counters                        */
                    /*! \brief 1E.800E.9 R/W MSS Ingress High Priority
                        mssIngressControlRegister_t.bits_0.mssIngressHighPriority
                        Default = 0x0
                        1 = MIB counter clear on read enable
                 <B>Notes:</B>
                        If this bit is set to 1, read is given high priority and the MIB count value becomes 0 after read.  */
      unsigned int   mssIngressHighPriority : 1;    // 1E.800E.9  R/W      Default = 0x0 
                     /* 1 = MIB counter clear on read enable */
                    /*! \brief 1E.800E.A R/W MSS Ingress Remove SECTag
                        mssIngressControlRegister_t.bits_0.mssIngressRemoveSectag
                        Default = 0x0
                        1 = Enable removal of SECTag
                 <B>Notes:</B>
                        If this bit is set and either of the following two conditions occurs, the SECTag will be removed.
                        Controlled packet and either the SA or SC is invalid.
                        IGPRC miss.  */
      unsigned int   mssIngressRemoveSectag : 1;    // 1E.800E.A  R/W      Default = 0x0 
                     /* 1 = Enable removal of SECTag */
                    /*! \brief 1E.800E.C:B R/W MSS Ingress Global Validate Frames [1:0]
                        mssIngressControlRegister_t.bits_0.mssIngressGlobalValidateFrames
                        Default = 0x0
                        Default validate frames configuration
                 <B>Notes:</B>
                        If the SC is invalid or if an IGPRC miss packet condition occurs, this default will be used for the validate frames configuration instead of the validate frame entry in the Ingress SC Table (IGSCT).  */
      unsigned int   mssIngressGlobalValidateFrames : 2;    // 1E.800E.C:B  R/W      Default = 0x0 
                     /* Default validate frames configuration
                          */
                    /*! \brief 1E.800E.D R/W MSS Ingress ICV LSB 8 Bytes Enable
                        mssIngressControlRegister_t.bits_0.mssIngressIcvLsb_8BytesEnable
                        Default = 0x0
                        1 = Use LSB
                        0 = Use MSB
                 <B>Notes:</B>
                        This bit selects MSB or LSB 8 bytes selection in the case where the ICV is 8 bytes.
                        0 = MSB is used.  */
      unsigned int   mssIngressIcvLsb_8BytesEnable : 1;    // 1E.800E.D  R/W      Default = 0x0 
                     /* 1 = Use LSB
                        0 = Use MSB */
      unsigned int   reserved0 : 2;
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
      unsigned int   reserved0 : 16;
    } bits_1;
    unsigned short word_1;
  };
};

//---------------------------------------------------------------------------------
//                  MSS Ingress LUT Address Control Register: 1E.8080 
//---------------------------------------------------------------------------------
struct mssIngressLutAddressControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.8080.8:0 R/W MSS Ingress LUT Address [8:0]
                        mssIngressLutAddressControlRegister_t.bits_0.mssIngressLutAddress
                        Default = 0x000
                        LUT address  */
      unsigned int   mssIngressLutAddress : 9;    // 1E.8080.8:0  R/W      Default = 0x000 
                     /* LUT address
                          */
      unsigned int   reserved0 : 3;
                    /*! \brief 1E.8080.F:C R/W MSS Ingress LUT Select [3:0]
                        mssIngressLutAddressControlRegister_t.bits_0.mssIngressLutSelect
                        Default = 0x0
                        LUT select
                 <B>Notes:</B>
                        0x0 : Ingress Pre-Security MAC Control FIlter (IGPRCTLF) LUT
                        0x1 : Ingress Pre-Security Classification LUT (IGPRC)
                        0x2 : Ingress Packet Format (IGPFMT) SAKey LUT
                        0x3 : Ingress Packet Format (IGPFMT) SC/SA LUT
                        0x4 : Ingress Post-Security Classification LUT (IGPOC)
                        0x5 : Ingress Post-Security MAC Control Filter (IGPOCTLF) LUT
                        0x6 : Ingress MIB (IGMIB)  */
      unsigned int   mssIngressLutSelect : 4;    // 1E.8080.F:C  R/W      Default = 0x0 
                     /* LUT select
                          */
    } bits_0;
    unsigned short word_0;
  };
};


//---------------------------------------------------------------------------------
//                  MSS Ingress LUT Control Register: 1E.8081 
//---------------------------------------------------------------------------------
struct mssIngressLutControlRegister_t
{
  union
  {
    struct
    {
      unsigned int   reserved0 : 14;
                    /*! \brief 1E.8081.E R/W MSS Ingress LUT Read
                        mssIngressLutControlRegister_t.bits_0.mssIngressLutRead
                        Default = 0x0
                        1 = LUT read
                 <B>Notes:</B>
                        Setting this bit to 1, will read the LUT. This bit will automatically clear to 0.  */
      unsigned int   mssIngressLutRead : 1;    // 1E.8081.E  R/W      Default = 0x0 
                     /* 1 = LUT read
                          */
                    /*! \brief 1E.8081.F R/W MSS Ingress LUT Write
                        mssIngressLutControlRegister_t.bits_0.mssIngressLutWrite
                        Default = 0x0
                        1 = LUT write
                 <B>Notes:</B>
                        Setting this bit to 1, will write the LUT. This bit will automatically clear to 0.  */
      unsigned int   mssIngressLutWrite : 1;    // 1E.8081.F  R/W      Default = 0x0 
                     /* 1 = LUT write
                          */
    } bits_0;
    unsigned short word_0;
  };
};

//---------------------------------------------------------------------------------
//                  MSS Ingress LUT Data Control Register: 1E.80A0 
//---------------------------------------------------------------------------------
struct mssIngressLutDataControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.80A0.F:0 R/W MSS Ingress LUT Data 0 [F:0]
                        mssIngressLutDataControlRegister_t.bits_0.mssIngressLutData_0
                        Default = 0x0000
                        LUT data bits 15:0
  */
      unsigned int   mssIngressLutData_0 : 16;    // 1E.80A0.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 15:0
                          */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A1.F:0 R/W MSS Ingress LUT Data 1 [1F:10]
                        mssIngressLutDataControlRegister_t.bits_1.mssIngressLutData_1
                        Default = 0x0000
                        LUT data bits 31:16
  */
      unsigned int   mssIngressLutData_1 : 16;    // 1E.80A1.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 31:16
                          */
    } bits_1;
    unsigned short word_1;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A2.F:0 R/W MSS Ingress LUT Data 2 [2F:20]
                        mssIngressLutDataControlRegister_t.bits_2.mssIngressLutData_2
                        Default = 0x0000
                        LUT data bits 47:32
  */
      unsigned int   mssIngressLutData_2 : 16;    // 1E.80A2.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 47:32
                          */
    } bits_2;
    unsigned int word_2;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A3.F:0 R/W MSS Ingress LUT Data 3 [3F:30]
                        mssIngressLutDataControlRegister_t.bits_3.mssIngressLutData_3
                        Default = 0x0000
                        LUT data bits 63:48
  */
      unsigned int   mssIngressLutData_3 : 16;    // 1E.80A3.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 63:48
                          */
    } bits_3;
    unsigned int word_3;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A4.F:0 R/W MSS Ingress LUT Data 4 [4F:40]
                        mssIngressLutDataControlRegister_t.bits_4.mssIngressLutData_4
                        Default = 0x0000
                        LUT data bits 79:64
  */
      unsigned int   mssIngressLutData_4 : 16;    // 1E.80A4.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 79:64
                          */
    } bits_4;
    unsigned int word_4;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A5.F:0 R/W MSS Ingress LUT Data 5 [5F:50]
                        mssIngressLutDataControlRegister_t.bits_5.mssIngressLutData_5
                        Default = 0x0000
                        LUT data bits 95:80
  */
      unsigned int   mssIngressLutData_5 : 16;    // 1E.80A5.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 95:80
                          */
    } bits_5;
    unsigned int word_5;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A6.F:0 R/W MSS Ingress LUT Data 6 [6F:60]
                        mssIngressLutDataControlRegister_t.bits_6.mssIngressLutData_6
                        Default = 0x0000
                        LUT data bits 111:96
  */
      unsigned int   mssIngressLutData_6 : 16;    // 1E.80A6.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 111:96
                          */
    } bits_6;
    unsigned int word_6;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A7.F:0 R/W MSS Ingress LUT Data 7 [7F:70]
                        mssIngressLutDataControlRegister_t.bits_7.mssIngressLutData_7
                        Default = 0x0000
                        LUT data bits 127:112
  */
      unsigned int   mssIngressLutData_7 : 16;    // 1E.80A7.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 127:112
                          */
    } bits_7;
    unsigned int word_7;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A8.F:0 R/W MSS Ingress LUT Data 8 [8F:80]
                        mssIngressLutDataControlRegister_t.bits_8.mssIngressLutData_8
                        Default = 0x0000
                        LUT data bits 143:128
  */
      unsigned int   mssIngressLutData_8 : 16;    // 1E.80A8.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 143:128 */
    } bits_8;
    unsigned int word_8;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80A9.F:0 R/W MSS Ingress LUT Data 9 [9F:90]
                        mssIngressLutDataControlRegister_t.bits_9.mssIngressLutData_9
                        Default = 0x0000
                        LUT data bits 159:144 */
      unsigned int   mssIngressLutData_9 : 16;    // 1E.80A9.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 159:144 */
    } bits_9;
    unsigned int word_9;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AA.F:0 R/W MSS Ingress LUT Data 10 [AF:A0]
                        mssIngressLutDataControlRegister_t.bits_10.mssIngressLutData_10
                        Default = 0x0000
                        LUT data bits 175:160
  */
      unsigned int   mssIngressLutData_10 : 16;    // 1E.80AA.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 175:160 */
    } bits_10;
    unsigned short word_10;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AB.F:0 R/W MSS Ingress LUT Data 11 [BF:B0]
                        mssIngressLutDataControlRegister_t.bits_11.mssIngressLutData_11
                        Default = 0x0000
                        LUT data bits 191:176  */
      unsigned int   mssIngressLutData_11 : 16;    // 1E.80AB.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 191:176 */
    } bits_11;
    unsigned short word_11;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AC.F:0 R/W MSS Ingress LUT Data 12 [CF:C0]
                        mssIngressLutDataControlRegister_t.bits_12.mssIngressLutData_12
                        Default = 0x0000
                        LUT data bits 207:192  */
      unsigned int   mssIngressLutData_12 : 16;    // 1E.80AC.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 207:192 */
    } bits_12;
    unsigned short word_12;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AD.F:0 R/W MSS Ingress LUT Data 13 [DF:D0]
                        mssIngressLutDataControlRegister_t.bits_13.mssIngressLutData_13
                        Default = 0x0000
                        LUT data bits 223:208  */
      unsigned int   mssIngressLutData_13 : 16;    // 1E.80AD.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 223:208 */
    } bits_13;
    unsigned short word_13;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AE.F:0 R/W MSS Ingress LUT Data 14 [EF:E0]
                        mssIngressLutDataControlRegister_t.bits_14.mssIngressLutData_14
                        Default = 0x0000
                        LUT data bits 239:224  */
      unsigned int   mssIngressLutData_14 : 16;    // 1E.80AE.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 239:224 */
    } bits_14;
    unsigned short word_14;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80AF.F:0 R/W MSS Ingress LUT Data 15 [FF:F0]
                        mssIngressLutDataControlRegister_t.bits_15.mssIngressLutData_15
                        Default = 0x0000
                        LUT data bits 255:240  */
      unsigned int   mssIngressLutData_15 : 16;    // 1E.80AF.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 255:240 */
    } bits_15;
    unsigned short word_15;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B0.F:0 R/W MSS Ingress LUT Data 16 [10F:100]
                        mssIngressLutDataControlRegister_t.bits_16.mssIngressLutData_16
                        Default = 0x0000
                        LUT data bits 271:256  */
      unsigned int   mssIngressLutData_16 : 16;    // 1E.80B0.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 271:256 */
    } bits_16;
    unsigned short word_16;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B1.F:0 R/W MSS Ingress LUT Data 17 [11F:110]
                        mssIngressLutDataControlRegister_t.bits_17.mssIngressLutData_17
                        Default = 0x0000
                        LUT data bits 287:272  */
      unsigned int   mssIngressLutData_17 : 16;    // 1E.80B1.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 287:272 */
    } bits_17;
    unsigned short word_17;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B2.F:0 R/W MSS Ingress LUT Data 18 [12F:120]
                        mssIngressLutDataControlRegister_t.bits_18.mssIngressLutData_18
                        Default = 0x0000
                        LUT data bits 303:288  */
      unsigned int   mssIngressLutData_18 : 16;    // 1E.80B2.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 303:288 */
    } bits_18;
    unsigned short word_18;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B3.F:0 R/W MSS Ingress LUT Data 19 [13F:130]
                        mssIngressLutDataControlRegister_t.bits_19.mssIngressLutData_19
                        Default = 0x0000
                        LUT data bits 319:304  */
      unsigned int   mssIngressLutData_19 : 16;    // 1E.80B3.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 319:304 */
    } bits_19;
    unsigned short word_19;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B4.F:0 R/W MSS Ingress LUT Data 20 [14F:140]
                        mssIngressLutDataControlRegister_t.bits_20.mssIngressLutData_20
                        Default = 0x0000
                        LUT data bits 335:320  */
      unsigned int   mssIngressLutData_20 : 16;    // 1E.80B4.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 335:320 */
    } bits_20;
    unsigned int word_20;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B5.F:0 R/W MSS Ingress LUT Data 21 [15F:150]
                        mssIngressLutDataControlRegister_t.bits_21.mssIngressLutData_21
                        Default = 0x0000
                        LUT data bits 351:336  */
      unsigned int   mssIngressLutData_21 : 16;    // 1E.80B5.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 351:336 */
    } bits_21;
    unsigned int word_21;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B6.F:0 R/W MSS Ingress LUT Data 22 [16F:160]
                        mssIngressLutDataControlRegister_t.bits_22.mssIngressLutData_22
                        Default = 0x0000
                        LUT data bits 367:352 */
      unsigned int   mssIngressLutData_22 : 16;    // 1E.80B6.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 367:352 */
    } bits_22;
    unsigned int word_22;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.80B7.F:0 R/W MSS Ingress LUT Data 23 [17F:170]
                        mssIngressLutDataControlRegister_t.bits_23.mssIngressLutData_23
                        Default = 0x0000
                        LUT data bits 383:368  */
      unsigned int   mssIngressLutData_23 : 16;    // 1E.80B7.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 383:368 */
    } bits_23;
    unsigned int word_23;
  };
};

#endif /* MSS_INGRESS_REGS_HEADER */

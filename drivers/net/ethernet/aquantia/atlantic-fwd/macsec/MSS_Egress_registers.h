#ifndef MSS_EGRESS_REGS_HEADER
#define MSS_EGRESS_REGS_HEADER


#define mssEgressControlRegister_ADDR 0x00005002
#define mssEgressSaExpiredStatusRegister_ADDR 0x00005060
#define mssEgressSaThresholdExpiredStatusRegister_ADDR 0x00005062
#define mssEgressLutAddressControlRegister_ADDR 0x00005080
#define mssEgressLutControlRegister_ADDR 0x00005081
#define mssEgressLutDataControlRegister_ADDR 0x000050A0

//---------------------------------------------------------------------------------
//                  MSS Egress Control Register: 1E.5002 
//---------------------------------------------------------------------------------
struct mssEgressControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.5002.0 R/W MSS Egress Soft Reset
                        mssEgressControlRegister_t.bits_0.mssEgressSoftReset
                        Default = 0x0
                        1 = Soft reset
                 <B>Notes:</B>
                        S/W reset  */
      unsigned int   mssEgressSoftReset : 1;    // 1E.5002.0  R/W      Default = 0x0 
                     /* 1 = Soft reset
                          */
                    /*! \brief 1E.5002.1 R/W MSS Egress Drop KAY Packet
                        mssEgressControlRegister_t.bits_0.mssEgressDropKayPacket
                        Default = 0x0
                        1 = Drop KAY packet
                 <B>Notes:</B>
                        Decides whether KAY packets have to be dropped  */
      unsigned int   mssEgressDropKayPacket : 1;    // 1E.5002.1  R/W      Default = 0x0 
                     /* 1 = Drop KAY packet
                          */
                    /*! \brief 1E.5002.2 R/W MSS Egress Drop EGPRC LUT Miss
                        mssEgressControlRegister_t.bits_0.mssEgressDropEgprcLutMiss
                        Default = 0x0
                        1 = Drop Egress Classification LUT miss packets
                 <B>Notes:</B>
                        Decides whether Egress Pre-Security Classification (EGPRC) LUT miss packets are to be dropped  */
      unsigned int   mssEgressDropEgprcLutMiss : 1;    // 1E.5002.2  R/W      Default = 0x0 
                     /* 1 = Drop Egress Classification LUT miss packets */
                    /*! \brief 1E.5002.3 R/W MSS Egress GCM Start
                        mssEgressControlRegister_t.bits_0.mssEgressGcmStart
                        Default = 0x0
                        1 = Start GCM
                 <B>Notes:</B>
                        Indicates GCM to start  */
      unsigned int   mssEgressGcmStart : 1;    // 1E.5002.3  R/W      Default = 0x0 
                     /* 1 = Start GCM */
                    /*! \brief 1E.5002.4 R/W MSS Egresss GCM Test Mode
                        mssEgressControlRegister_t.bits_0.mssEgresssGcmTestMode
                        Default = 0x0
                        1 = Enable GCM test mode
                 <B>Notes:</B>
                        Enables GCM test mode  */
      unsigned int   mssEgresssGcmTestMode : 1;    // 1E.5002.4  R/W      Default = 0x0 
                     /* 1 = Enable GCM test mode */
                    /*! \brief 1E.5002.5 R/W MSS Egress Unmatched Use SC 0
                        mssEgressControlRegister_t.bits_0.mssEgressUnmatchedUseSc_0
                        Default = 0x0
                        1 = Use SC 0 for unmatched packets
                        0 = Unmatched packets are uncontrolled packets
                 <B>Notes:</B>
                        Use SC-Index 0 as default SC for unmatched packets. Otherwise the packets are treated as uncontrolled packets.  */
      unsigned int   mssEgressUnmatchedUseSc_0 : 1;    // 1E.5002.5  R/W      Default = 0x0 
                     /* 1 = Use SC 0 for unmatched packets
                        0 = Unmatched packets are uncontrolled packets */
                    /*! \brief 1E.5002.6 R/W MSS Egress Drop Invalid SA/SC Packets
                        mssEgressControlRegister_t.bits_0.mssEgressDropInvalidSa_scPackets
                        Default = 0x0
                        1 = Drop invalid SA/SC packets
                 <B>Notes:</B>
                        Enables dropping of invalid SA/SC packets.  */
      unsigned int   mssEgressDropInvalidSa_scPackets : 1;    // 1E.5002.6  R/W      Default = 0x0 
                     /* 1 = Drop invalid SA/SC packets */
                    /*! \brief 1E.5002.7 R/W MSS Egress Explicit SECTag Report Short Length
                        mssEgressControlRegister_t.bits_0.mssEgressExplicitSectagReportShortLength
                        Default = 0x0
                        Reserved
                 <B>Notes:</B>
                        Unused.  */
      unsigned int   mssEgressExplicitSectagReportShortLength : 1;    // 1E.5002.7  R/W      Default = 0x0 
                     /* Reserved */
                    /*! \brief 1E.5002.8 R/W MSS Egress External Classification Enable
                        mssEgressControlRegister_t.bits_0.mssEgressExternalClassificationEnable
                        Default = 0x0
                        1 = Drop EGPRC miss packets
                 <B>Notes:</B>
                        If set, internal classification is bypassed. Should always be set to 0.  */
      unsigned int   mssEgressExternalClassificationEnable : 1;    // 1E.5002.8  R/W      Default = 0x0 
                     /* 1 = Drop EGPRC miss packets */
                    /*! \brief 1E.5002.9 R/W MSS Egress ICV LSB 8 Bytes Enable
                        mssEgressControlRegister_t.bits_0.mssEgressIcvLsb_8BytesEnable
                        Default = 0x0
                        1 = Use LSB
                        0 = Use MSB
                 <B>Notes:</B>
                        This bit selects MSB or LSB 8 bytes selection in the case where the ICV is 8 bytes.
                        0 = MSB is used.  */
      unsigned int   mssEgressIcvLsb_8BytesEnable : 1;    // 1E.5002.9  R/W      Default = 0x0 
                     /* 1 = Use LSB
                        0 = Use MSB  */
                    /*! \brief 1E.5002.A R/W MSS Egress High Priority
                        mssEgressControlRegister_t.bits_0.mssEgressHighPriority
                        Default = 0x0
                        1 = MIB counter clear on read enable
                 <B>Notes:</B>
                        If this bit is set to 1, read is given high priority and the MIB count value becomes 0 after read.  */
      unsigned int   mssEgressHighPriority : 1;    // 1E.5002.A  R/W      Default = 0x0 
                     /* 1 = MIB counter clear on read enable */
                    /*! \brief 1E.5002.B R/W MSS Egress Clear Counter
                        mssEgressControlRegister_t.bits_0.mssEgressClearCounter
                        Default = 0x0
                        1 = Clear all MIB counters
                 <B>Notes:</B>
                        If this bit is set to 1, all MIB counters will be cleared.  */
      unsigned int   mssEgressClearCounter : 1;    // 1E.5002.B  R/W      Default = 0x0 
                     /* 1 = Clear all MIB counters */
                    /*! \brief 1E.5002.C R/W MSS Egress Clear Global Time
                        mssEgressControlRegister_t.bits_0.mssEgressClearGlobalTime
                        Default = 0x0
                        1 = Clear global time
                 <B>Notes:</B>
                        Clear global time.  */
      unsigned int   mssEgressClearGlobalTime : 1;    // 1E.5002.C  R/W      Default = 0x0 
                     /* 1 = Clear global time */
                    /*! \brief 1E.5002.F:D R/W MSS Egress Ethertype Explicit SECTag LSB [2:0]
                        mssEgressControlRegister_t.bits_0.mssEgressEthertypeExplicitSectagLsb
                        Default = 0x0
                        Ethertype for explicit SECTag bits 2:0.
                 <B>Notes:</B>
                        Ethertype for explicity SECTag.  */
      unsigned int   mssEgressEthertypeExplicitSectagLsb : 3;    // 1E.5002.F:D  R/W      Default = 0x0 
                     /* Ethertype for explicit SECTag bits 2:0. */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.5003.C:0 R/W MSS Egress Ethertype Explicit SECTag MSB  [F:3]
                        mssEgressControlRegister_t.bits_1.mssEgressEthertypeExplicitSectagMsb
                        Default = 0x0000
                        Ethertype for explicit SECTag bits 15:3.
                 <B>Notes:</B>
                        Ethertype for explicity SECTag.  */
      unsigned int   mssEgressEthertypeExplicitSectagMsb : 13;    // 1E.5003.C:0  R/W      Default = 0x0000 
                     /* Ethertype for explicit SECTag bits 15:3. */
      unsigned int   reserved0 : 3;
    } bits_1;
    unsigned short word_1;
  };
};

//---------------------------------------------------------------------------------
//! \brief                MSS Egress SA Expired Status Register: 1E.5060 
//                  MSS Egress SA Expired Status Register: 1E.5060 
//---------------------------------------------------------------------------------
struct mssEgressSaExpiredStatusRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.5060.F:0 COW MSS Egress SA Expired LSW [F:0]
                        mssEgressSaExpiredStatusRegister_t.bits_0.mssEgressSaExpiredLSW

                        Default = 0x0000

                        SA expired bits 15:0
                        

                 <B>Notes:</B>
                        Write these bits to 1 to clear.
                        When set, these bits identify the SA that has expired when the SA PN reaches all-ones saturation.  */
      unsigned int   mssEgressSaExpiredLSW : 16;    // 1E.5060.F:0  COW      Default = 0x0000 
                     /* SA expired bits 15:0
                          */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.5061.F:0 COW MSS Egress SA Expired MSW [1F:10]
                        mssEgressSaExpiredStatusRegister_t.bits_1.mssEgressSaExpiredMSW

                        Default = 0x0000

                        SA expired bits 31:16
                        

                 <B>Notes:</B>
                        Write these bits to 1 to clear.
                        When set, these bits identify the SA that has expired when the SA PN reaches all-ones saturation.  */
      unsigned int   mssEgressSaExpiredMSW : 16;    // 1E.5061.F:0  COW      Default = 0x0000 
                     /* SA expired bits 31:16
                          */
    } bits_1;
    unsigned short word_1;
  };
};


//---------------------------------------------------------------------------------
//! \brief                MSS Egress SA Threshold Expired Status Register: 1E.5062 
//                  MSS Egress SA Threshold Expired Status Register: 1E.5062 
//---------------------------------------------------------------------------------
struct mssEgressSaThresholdExpiredStatusRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.5062.F:0 COW MSS Egress SA Threshold Expired LSW [F:0]
                        mssEgressSaThresholdExpiredStatusRegister_t.bits_0.mssEgressSaThresholdExpiredLSW

                        Default = 0x0000

                        SA threshold expired bits 15:0
                        

                 <B>Notes:</B>
                        Write these bits to 1 to clear.
                        When set, these bits identify the SA that has expired when the SA PN has reached the configured threshold  See SEC Egress PN Threshold [F:0] and  See SEC Egress PN Threshold [1F:10] .  */
      unsigned int   mssEgressSaThresholdExpiredLSW : 16;    // 1E.5062.F:0  COW      Default = 0x0000 
                     /* SA threshold expired bits 15:0
                          */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.5063.F:0 COW MSS Egress SA Threshold Expired MSW [1F:10]
                        mssEgressSaThresholdExpiredStatusRegister_t.bits_1.mssEgressSaThresholdExpiredMSW

                        Default = 0x0000

                        SA threshold expired bits 31:16
                        

                 <B>Notes:</B>
                        Write these bits to 1 to clear.
                        When set, these bits identify the SA that has expired when the SA PN has reached the configured threshold  See SEC Egress PN Threshold [F:0] and  See SEC Egress PN Threshold [1F:10] .   */
      unsigned int   mssEgressSaThresholdExpiredMSW : 16;    // 1E.5063.F:0  COW      Default = 0x0000 
                     /* SA threshold expired bits 31:16
                          */
    } bits_1;
    unsigned short word_1;
  };
};


//---------------------------------------------------------------------------------
//! \brief                MSS Egress Debug Control Register: 1E.5072 
//                  MSS Egress Debug Control Register: 1E.5072 
//---------------------------------------------------------------------------------
struct mssEgressDebugControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.5072.1:0 R/W MSS Egress Debug Bus Select [1:0]
                        mssEgressDebugControlRegister_t.bits_0.mssEgressDebugBusSelect

                        Default = 0x0

                        1 = Enable ECC
                        

                 <B>Notes:</B>
                        Unused.  */
      unsigned int   mssEgressDebugBusSelect : 2;    // 1E.5072.1:0  R/W      Default = 0x0 
                     /* 1 = Enable ECC
                          */
      unsigned int   reserved0 : 14;
    } bits_0;
    unsigned short word_0;
  };
};


//---------------------------------------------------------------------------------
//! \brief                MSS Egress LUT Address Control Register: 1E.5080 
//                  MSS Egress LUT Address Control Register: 1E.5080 
//---------------------------------------------------------------------------------
struct mssEgressLutAddressControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.5080.8:0 R/W MSS Egress LUT Address [8:0]
                        mssEgressLutAddressControlRegister_t.bits_0.mssEgressLutAddress

                        Default = 0x000

                        LUT address
                        
  */
      unsigned int   mssEgressLutAddress : 9;    // 1E.5080.8:0  R/W      Default = 0x000 
                     /* LUT address
                          */
      unsigned int   reserved0 : 3;
                    /*! \brief 1E.5080.F:C R/W MSS Egress LUT Select [3:0]
                        mssEgressLutAddressControlRegister_t.bits_0.mssEgressLutSelect

                        Default = 0x0

                        LUT select
                        

                 <B>Notes:</B>
                        0x0 : Egress MAC Control FIlter (CTLF) LUT
                        0x1 : Egress Classification LUT
                        0x2 : Egress SC/SA LUT
                        0x3 : Egress SMIB  */
      unsigned int   mssEgressLutSelect : 4;    // 1E.5080.F:C  R/W      Default = 0x0 
                     /* LUT select
                          */
    } bits_0;
    unsigned short word_0;
  };
};

//---------------------------------------------------------------------------------
//                  MSS Egress LUT Control Register: 1E.5081 
//---------------------------------------------------------------------------------
struct mssEgressLutControlRegister_t
{
  union
  {
    struct
    {
      unsigned int   reserved0 : 14;
                    /*! \brief 1E.5081.E R/W MSS Egress LUT Read
                        mssEgressLutControlRegister_t.bits_0.mssEgressLutRead
                        Default = 0x0
                        1 = LUT read
                 <B>Notes:</B>
                        Setting this bit to 1, will read the LUT. This bit will automatically clear to 0.  */
      unsigned int   mssEgressLutRead : 1;    // 1E.5081.E  R/W      Default = 0x0 
                     /* 1 = LUT read */
                    /*! \brief 1E.5081.F R/W MSS Egress LUT Write
                        mssEgressLutControlRegister_t.bits_0.mssEgressLutWrite
                        Default = 0x0
                        1 = LUT write
                 <B>Notes:</B>
                        Setting this bit to 1, will write the LUT. This bit will automatically clear to 0.  */
      unsigned int   mssEgressLutWrite : 1;    // 1E.5081.F  R/W      Default = 0x0 
                     /* 1 = LUT write */
    } bits_0;
    unsigned short word_0;
  };
};

//---------------------------------------------------------------------------------
//                  MSS Egress LUT Data Control Register: 1E.50A0 
//---------------------------------------------------------------------------------
struct mssEgressLutDataControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.50A0.F:0 R/W MSS Egress LUT Data 0 [F:0]
                        mssEgressLutDataControlRegister_t.bits_0.mssEgressLutData_0
                        Default = 0x0000
                        LUT data bits 15:0 */
      unsigned int   mssEgressLutData_0 : 16;    // 1E.50A0.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 15:0
                          */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A1.F:0 R/W MSS Egress LUT Data 1 [1F:10]
                        mssEgressLutDataControlRegister_t.bits_1.mssEgressLutData_1
                        Default = 0x0000
                        LUT data bits 31:16 */
      unsigned int   mssEgressLutData_1 : 16;    // 1E.50A1.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 31:16
                          */
    } bits_1;
    unsigned short word_1;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A2.F:0 R/W MSS Egress LUT Data 2 [2F:20]
                        mssEgressLutDataControlRegister_t.bits_2.mssEgressLutData_2
                        Default = 0x0000
                        LUT data bits 47:32 */
      unsigned int   mssEgressLutData_2 : 16;    // 1E.50A2.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 47:32
                          */
    } bits_2;
    unsigned int word_2;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A3.F:0 R/W MSS Egress LUT Data 3 [3F:30]
                        mssEgressLutDataControlRegister_t.bits_3.mssEgressLutData_3
                        Default = 0x0000
                        LUT data bits 63:48 */
      unsigned int   mssEgressLutData_3 : 16;    // 1E.50A3.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 63:48
                          */
    } bits_3;
    unsigned int word_3;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A4.F:0 R/W MSS Egress LUT Data 4 [4F:40]
                        mssEgressLutDataControlRegister_t.bits_4.mssEgressLutData_4
                        Default = 0x0000
                        LUT data bits 79:64 */
      unsigned int   mssEgressLutData_4 : 16;    // 1E.50A4.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 79:64
                          */
    } bits_4;
    unsigned int word_4;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A5.F:0 R/W MSS Egress LUT Data 5 [5F:50]
                        mssEgressLutDataControlRegister_t.bits_5.mssEgressLutData_5

                        Default = 0x0000

                        LUT data bits 95:80
                        
  */
      unsigned int   mssEgressLutData_5 : 16;    // 1E.50A5.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 95:80
                          */
    } bits_5;
    unsigned int word_5;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A6.F:0 R/W MSS Egress LUT Data 6 [6F:60]
                        mssEgressLutDataControlRegister_t.bits_6.mssEgressLutData_6

                        Default = 0x0000

                        LUT data bits 111:96
                        
  */
      unsigned int   mssEgressLutData_6 : 16;    // 1E.50A6.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 111:96
                          */
    } bits_6;
    unsigned int word_6;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A7.F:0 R/W MSS Egress LUT Data 7 [7F:70]
                        mssEgressLutDataControlRegister_t.bits_7.mssEgressLutData_7

                        Default = 0x0000

                        LUT data bits 127:112
                        
  */
      unsigned int   mssEgressLutData_7 : 16;    // 1E.50A7.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 127:112
                          */
    } bits_7;
    unsigned int word_7;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A8.F:0 R/W MSS Egress LUT Data 8 [8F:80]
                        mssEgressLutDataControlRegister_t.bits_8.mssEgressLutData_8

                        Default = 0x0000

                        LUT data bits 143:128
                        
  */
      unsigned int   mssEgressLutData_8 : 16;    // 1E.50A8.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 143:128
                          */
    } bits_8;
    unsigned int word_8;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50A9.F:0 R/W MSS Egress LUT Data 9 [9F:90]
                        mssEgressLutDataControlRegister_t.bits_9.mssEgressLutData_9

                        Default = 0x0000

                        LUT data bits 159:144
                        
  */
      unsigned int   mssEgressLutData_9 : 16;    // 1E.50A9.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 159:144
                          */
    } bits_9;
    unsigned int word_9;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AA.F:0 R/W MSS Egress LUT Data 10 [AF:A0]
                        mssEgressLutDataControlRegister_t.bits_10.mssEgressLutData_10

                        Default = 0x0000

                        LUT data bits 175:160
                        
  */
      unsigned int   mssEgressLutData_10 : 16;    // 1E.50AA.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 175:160
                          */
    } bits_10;
    unsigned short word_10;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AB.F:0 R/W MSS Egress LUT Data 11 [BF:B0]
                        mssEgressLutDataControlRegister_t.bits_11.mssEgressLutData_11

                        Default = 0x0000

                        LUT data bits 191:176
                        
  */
      unsigned int   mssEgressLutData_11 : 16;    // 1E.50AB.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 191:176
                          */
    } bits_11;
    unsigned short word_11;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AC.F:0 R/W MSS Egress LUT Data 12 [CF:C0]
                        mssEgressLutDataControlRegister_t.bits_12.mssEgressLutData_12
                        Default = 0x0000
                        LUT data bits 207:192 */
      unsigned int   mssEgressLutData_12 : 16;    // 1E.50AC.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 207:192 */
    } bits_12;
    unsigned short word_12;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AD.F:0 R/W MSS Egress LUT Data 13 [DF:D0]
                        mssEgressLutDataControlRegister_t.bits_13.mssEgressLutData_13
                        Default = 0x0000
                        LUT data bits 223:208 */
      unsigned int   mssEgressLutData_13 : 16;    // 1E.50AD.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 223:208 */
    } bits_13;
    unsigned short word_13;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AE.F:0 R/W MSS Egress LUT Data 14 [EF:E0]
                        mssEgressLutDataControlRegister_t.bits_14.mssEgressLutData_14
                        Default = 0x0000
                        LUT data bits 239:224 */
      unsigned int   mssEgressLutData_14 : 16;    // 1E.50AE.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 239:224 */
    } bits_14;
    unsigned short word_14;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50AF.F:0 R/W MSS Egress LUT Data 15 [FF:F0]
                        mssEgressLutDataControlRegister_t.bits_15.mssEgressLutData_15
                        Default = 0x0000
                        LUT data bits 255:240 */
      unsigned int   mssEgressLutData_15 : 16;    // 1E.50AF.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 255:240 */
    } bits_15;
    unsigned short word_15;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B0.F:0 R/W MSS Egress LUT Data 16 [10F:100]
                        mssEgressLutDataControlRegister_t.bits_16.mssEgressLutData_16
                        Default = 0x0000
                        LUT data bits 271:256 */
      unsigned int   mssEgressLutData_16 : 16;    // 1E.50B0.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 271:256 */
    } bits_16;
    unsigned short word_16;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B1.F:0 R/W MSS Egress LUT Data 17 [11F:110]
                        mssEgressLutDataControlRegister_t.bits_17.mssEgressLutData_17
                        Default = 0x0000
                        LUT data bits 287:272 */
      unsigned int   mssEgressLutData_17 : 16;    // 1E.50B1.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 287:272 */
    } bits_17;
    unsigned short word_17;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B2.F:0 R/W MSS Egress LUT Data 18 [12F:120]
                        mssEgressLutDataControlRegister_t.bits_18.mssEgressLutData_18
                        Default = 0x0000
                        LUT data bits 303:288 */
      unsigned int   mssEgressLutData_18 : 16;    // 1E.50B2.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 303:288 */
    } bits_18;
    unsigned short word_18;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B3.F:0 R/W MSS Egress LUT Data 19 [13F:130]
                        mssEgressLutDataControlRegister_t.bits_19.mssEgressLutData_19
                        Default = 0x0000
                        LUT data bits 319:304 */
      unsigned int   mssEgressLutData_19 : 16;    // 1E.50B3.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 319:304 */
    } bits_19;
    unsigned short word_19;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B4.F:0 R/W MSS Egress LUT Data 20 [14F:140]
                        mssEgressLutDataControlRegister_t.bits_20.mssEgressLutData_20
                        Default = 0x0000
                        LUT data bits 335:320 */
      unsigned int   mssEgressLutData_20 : 16;    // 1E.50B4.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 335:320 */
    } bits_20;
    unsigned int word_20;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B5.F:0 R/W MSS Egress LUT Data 21 [15F:150]
                        mssEgressLutDataControlRegister_t.bits_21.mssEgressLutData_21
                        Default = 0x0000
                        LUT data bits 351:336 */
      unsigned int   mssEgressLutData_21 : 16;    // 1E.50B5.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 351:336 */
    } bits_21;
    unsigned int word_21;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B6.F:0 R/W MSS Egress LUT Data 22 [16F:160]
                        mssEgressLutDataControlRegister_t.bits_22.mssEgressLutData_22
                        Default = 0x0000
                        LUT data bits 367:352 */
      unsigned int   mssEgressLutData_22 : 16;    // 1E.50B6.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 367:352 */
    } bits_22;
    unsigned int word_22;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B7.F:0 R/W MSS Egress LUT Data 23 [17F:170]
                        mssEgressLutDataControlRegister_t.bits_23.mssEgressLutData_23
                        Default = 0x0000
                        LUT data bits 383:368 */
      unsigned int   mssEgressLutData_23 : 16;    // 1E.50B7.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 383:368 */
    } bits_23;
    unsigned int word_23;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B8.F:0 R/W MSS Egress LUT Data 24 [18F:180]
                        mssEgressLutDataControlRegister_t.bits_24.mssEgressLutData_24
                        Default = 0x0000
                        LUT data bits 399:384 */
      unsigned int   mssEgressLutData_24 : 16;    // 1E.50B8.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 399:384 */
    } bits_24;
    unsigned int word_24;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50B9.F:0 R/W MSS Egress LUT Data 25 [19F:190]
                        mssEgressLutDataControlRegister_t.bits_25.mssEgressLutData_25
                        Default = 0x0000
                        LUT data bits 415:400 */
      unsigned int   mssEgressLutData_25 : 16;    // 1E.50B9.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 415:400 */
    } bits_25;
    unsigned int word_25;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50BA.F:0 R/W MSS Egress LUT Data 26 [1AF:1A0]
                        mssEgressLutDataControlRegister_t.bits_26.mssEgressLutData_26
                        Default = 0x0000
                        LUT data bits 431:416 */
      unsigned int   mssEgressLutData_26 : 16;    // 1E.50BA.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 431:416 */
    } bits_26;
    unsigned int word_26;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.50BB.F:0 R/W MSS Egress LUT Data 27 [1BF:1B0]
                        mssEgressLutDataControlRegister_t.bits_27.mssEgressLutData_27
                        Default = 0x0000
                        LUT data bits 447:432 */
      unsigned int   mssEgressLutData_27 : 16;    // 1E.50BB.F:0  R/W      Default = 0x0000 
                     /* LUT data bits 447:432 */
    } bits_27;
    unsigned int word_27;
  };
};

#endif /* MSS_EGRESS_REGS_HEADER */

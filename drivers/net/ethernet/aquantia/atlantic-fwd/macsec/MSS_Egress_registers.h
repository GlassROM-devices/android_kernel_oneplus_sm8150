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

#endif /* MSS_EGRESS_REGS_HEADER */

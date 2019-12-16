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

#endif /* MSS_INGRESS_REGS_HEADER */

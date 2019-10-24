#ifndef SEC_INGRESS_REGS_HEADER
#define SEC_INGRESS_REGS_HEADER

#define secIngressPacketEditControlRegister_ADDR 0x00007035

//---------------------------------------------------------------------------------
//                  SEC Ingress Packet Edit Control Register: 1E.7035 
//---------------------------------------------------------------------------------
struct secIngressPacketEditControlRegister_t
{
  union
  {
    struct
    {
                    /*! \brief 1E.7035.1:0 R/W SEC Ingress Version Check Fail Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressVersionCheckFailAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        Invalid MSStag TCI version 1. 1st priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressVersionCheckFailAction : 2;    // 1E.7035.1:0  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.3:2 R/W SEC Ingress TCI Check Fail Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressTciCheckFailAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        Invalid MSStag illegal. 2nd priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressTciCheckFailAction : 2;    // 1E.7035.3:2  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.5:4 R/W SEC Ingress Illigal SL Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressIlligalSlAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        Invalid MSStag short length. 3rd priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressIlligalSlAction : 2;    // 1E.7035.5:4  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.7:6 R/W SEC Ingress Binding Fail Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressBindingFailAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        IGPRC miss, SA invalid, or SC invalid. 4th priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressBindingFailAction : 2;    // 1E.7035.7:6  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.9:8 R/W SEC Ingress Replay Check Fail Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressReplayCheckFailAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        Replay error. 5th priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressReplayCheckFailAction : 2;    // 1E.7035.9:8  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.B:A R/W SEC Ingress ICV Check Fail Action [1:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressIcvCheckFailAction
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        ICV error. 6th priority.
                        0x0 : Drop
                        0x1 : Bypass
                        0x2 : Re-direct  */
      unsigned int   secIngressIcvCheckFailAction : 2;    // 1E.7035.B:A  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.E:C R/W SEC Ingress Reserved Action Code [2:0]
                        secIngressPacketEditControlRegister_t.bits_0.secIngressReservedActionCode
                        Default = 0x0
                        Action code
                 <B>Notes:</B>
                        Unused.  */
      unsigned int   secIngressReservedActionCode : 3;    // 1E.7035.E:C  R/W      Default = 0x0 
                     /* Action code  */
                    /*! \brief 1E.7035.F R/W SEC Ingress Special Ethertype Enable
                        secIngressPacketEditControlRegister_t.bits_0.secIngressSpecialEthertypeEnable
                        Default = 0x0
                        1 = Enable replacement of ethertype with special ethertype  */
      unsigned int   secIngressSpecialEthertypeEnable : 1;    // 1E.7035.F  R/W      Default = 0x0 
                     /* 1 = Enable replacement of ethertype with special ethertype  */
    } bits_0;
    unsigned short word_0;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.7036.F:0 R/W SEC Ingress Packet Edit Special Ethertype [F:0]
                        secIngressPacketEditControlRegister_t.bits_1.secIngressPacketEditSpecialEthertype
                        Default = 0x0000
                        Special ethertype
                 <B>Notes:</B>
                        Replace the ethertype with the configured special ethertype if the " See MACSEC Ingress Special Ethertype Enable " bit is set.  */
      unsigned int   secIngressPacketEditSpecialEthertype : 16;    // 1E.7036.F:0  R/W      Default = 0x0000 
                     /* Special ethertype  */
    } bits_1;
    unsigned short word_1;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.7037.F:0 R/W SEC Ingress Packet Edit Ethertype [F:0]
                        secIngressPacketEditControlRegister_t.bits_2.secIngressPacketEditEthertype
                        Default = 0x0000
                        Ethertype
                 <B>Notes:</B>
                        Ethertype replacement for redirected packets.  */
      unsigned int   secIngressPacketEditEthertype : 16;    // 1E.7037.F:0  R/W      Default = 0x0000 
                     /* Ethertype  */
    } bits_2;
    unsigned int word_2;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.7038.F:0 R/W SEC Ingress Packet Edit SA0 [F:0]
                        secIngressPacketEditControlRegister_t.bits_3.secIngressPacketEditSa0
                        Default = 0x0000
                        Source Address 0
                 <B>Notes:</B>
                        Source address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditSa0 : 16;    // 1E.7038.F:0  R/W      Default = 0x0000 
                     /* Source Address 0  */
    } bits_3;
    unsigned int word_3;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.7039.F:0 R/W SEC Ingress Packet Edit DA0 [F:0]
                        secIngressPacketEditControlRegister_t.bits_4.secIngressPacketEditDa0

                        Default = 0x0000

                        Destination Address 0
                        

                 <B>Notes:</B>
                        Source address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditDa0 : 16;    // 1E.7039.F:0  R/W      Default = 0x0000 
                     /* Destination Address 0
                          */
    } bits_4;
    unsigned int word_4;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.703A.F:0 R/W SEC Ingress Packet Edit SA1 [F:0]
                        secIngressPacketEditControlRegister_t.bits_5.secIngressPacketEditSa1

                        Default = 0x0000

                        Source Address 1
                        

                 <B>Notes:</B>
                        Source address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditSa1 : 16;    // 1E.703A.F:0  R/W      Default = 0x0000 
                     /* Source Address 1
                          */
    } bits_5;
    unsigned int word_5;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.703B.F:0 R/W SEC Ingress Packet Edit DA1 [F:0]
                        secIngressPacketEditControlRegister_t.bits_6.secIngressPacketEditDa1

                        Default = 0x0000

                        Destination Address 1
                        

                 <B>Notes:</B>
                        Destination address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditDa1 : 16;    // 1E.703B.F:0  R/W      Default = 0x0000 
                     /* Destination Address 1
                          */
    } bits_6;
    unsigned int word_6;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.703C.F:0 R/W SEC Ingress Packet Edit SA2 [F:0]
                        secIngressPacketEditControlRegister_t.bits_7.secIngressPacketEditSa2

                        Default = 0x0000

                        Source Address 2
                        

                 <B>Notes:</B>
                        Destination address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditSa2 : 16;    // 1E.703C.F:0  R/W      Default = 0x0000 
                     /* Source Address 2
                          */
    } bits_7;
    unsigned int word_7;
  };
  union
  {
    struct
    {
                    /*! \brief 1E.703D.F:0 R/W SEC Ingress Packet Edit DA2 [F:0]
                        secIngressPacketEditControlRegister_t.bits_8.secIngressPacketEditDa2

                        Default = 0x0000

                        Destination Address 2
                        

                 <B>Notes:</B>
                        Destination address replacement for redirected packets.  */
      unsigned int   secIngressPacketEditDa2 : 16;    // 1E.703D.F:0  R/W      Default = 0x0000 
                     /* Destination Address 2
                          */
    } bits_8;
    unsigned int word_8;
  };
};

#endif /* SEC_INGRESS_REGS_HEADER */
